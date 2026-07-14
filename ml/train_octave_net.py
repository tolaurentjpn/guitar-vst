#!/usr/bin/env python3
"""Train a tiny octave/voicing MLP on synthetic guitar-like tones and export C++ weights.

Usage:
    python3 ml/train_octave_net.py
    python3 ml/train_octave_net.py --out Source/OctaveNetWeights.h

Requires: numpy (pip install numpy)
"""

from __future__ import annotations

import argparse
import math
import os
import random
import struct
from pathlib import Path

try:
    import numpy as np
except ImportError as exc:
    raise SystemExit("numpy is required: pip install numpy") from exc

NUM_FEATURES = 12
MAX_CANDIDATES = 6
HIDDEN1 = 32
HIDDEN2 = 16
NUM_OUTPUTS = 2

SR = 48000.0
WINDOW = 2048
MIN_HZ = 70.0
MAX_HZ = 1200.0


def harmonic_rich(frequency: float, n: int, sr: float = SR) -> float:
    t = n / sr
    phase = 2.0 * math.pi * frequency * t
    return (
        0.55 * math.sin(phase)
        + 0.35 * math.sin(2.0 * phase)
        + 0.10 * math.sin(3.0 * phase)
    )


def missing_fundamental(frequency: float, n: int, sr: float = SR) -> float:
    t = n / sr
    phase = 2.0 * math.pi * frequency * t
    return (
        0.60 * math.sin(2.0 * phase)
        + 0.30 * math.sin(3.0 * phase)
        + 0.10 * math.sin(4.0 * phase)
    )


def strong_second(frequency: float, n: int, sr: float = SR) -> float:
    """Strong 2nd harmonic — classic octave-low trap."""
    t = n / sr
    phase = 2.0 * math.pi * frequency * t
    return (
        0.25 * math.sin(phase)
        + 0.65 * math.sin(2.0 * phase)
        + 0.10 * math.sin(3.0 * phase)
    )


def make_window(gen, frequency: float, start: int = 0) -> np.ndarray:
    return np.array([gen(frequency, start + i) for i in range(WINDOW)], dtype=np.float64)


def compute_yin(data: np.ndarray, min_hz: float = MIN_HZ, max_hz: float = MAX_HZ):
    half = len(data) // 2
    max_tau = min(half - 1, int(SR / min_hz))
    min_tau = max(2, int(SR / max_hz))
    yin = np.ones(half, dtype=np.float64)
    running = 0.0
    for tau in range(1, max_tau + 1):
        delta = data[:half] - data[tau : tau + half]
        s = float(np.dot(delta, delta))
        running += s
        yin[tau] = (s * tau / running) if running > 0 else 1.0
    return yin, min_tau, max_tau


def soft_harmonic_clarity(yin: np.ndarray, tau: int, min_tau: int, max_tau: int) -> float:
    if tau < min_tau or tau > max_tau:
        return 0.0
    fund = float(yin[tau])
    score = 1.0 - min(0.99, max(0.0, fund))
    for h in range(2, 6):
        ht = tau // h
        if ht < min_tau:
            break
        score *= 1.0 - min(0.99, max(0.0, float(yin[ht])))
    return score


def frame_stats(data: np.ndarray) -> tuple[float, float]:
    rms = float(np.sqrt(np.mean(data * data)) + 1e-8)
    # crude spectral contrast via parity of samples as band proxy + abs diff energy
    low = float(np.mean(np.abs(data[::2])) + 1e-8)
    high = float(np.mean(np.abs(np.diff(data))) + 1e-8)
    contrast = math.log((high + 1e-8) / (low + 1e-8))
    contrast = max(-3.0, min(3.0, contrast))
    return rms, contrast


def yin_at(yin: np.ndarray, tau: int, min_tau: int, max_tau: int) -> float:
    if tau < min_tau or tau > max_tau or tau >= len(yin):
        return 1.0
    return float(yin[tau])


def build_features(
    yin: np.ndarray,
    tau: int,
    min_tau: int,
    max_tau: int,
    prev_hz: float,
    rms: float,
    contrast: float,
) -> np.ndarray:
    cand_hz = SR / max(tau, 1)
    half = tau // 2
    double = tau * 2
    yin_tau = yin_at(yin, tau, min_tau, max_tau)
    yin_half = yin_at(yin, half, min_tau, max_tau) if half >= min_tau else 1.0
    yin_dbl = yin_at(yin, double, min_tau, max_tau) if double <= max_tau else 1.0
    has_prev = 1.0 if prev_hz > MIN_HZ * 0.5 else 0.0
    if has_prev:
        ratio = math.log2(max(cand_hz, 1e-6) / max(prev_hz, 1e-6))
        ratio = max(-2.0, min(2.0, ratio))
    else:
        ratio = 0.0
    tau_norm = (tau - min_tau) / max(max_tau - min_tau, 1)
    feats = np.array(
        [
            yin_tau,
            yin_half,
            yin_dbl,
            soft_harmonic_clarity(yin, tau, min_tau, max_tau),
            ratio,
            rms,
            contrast,
            tau_norm,
            1.0 - min(1.0, max(0.0, yin_tau)),
            yin_tau - yin_half,
            yin_tau - yin_dbl,
            has_prev,
        ],
        dtype=np.float64,
    )
    return feats


def collect_candidates(yin: np.ndarray, min_tau: int, max_tau: int, threshold: float = 0.15):
    # Absolute-threshold first dip (classic YIN)
    seed = min_tau
    found = False
    for tau in range(min_tau, max_tau + 1):
        if yin[tau] < threshold:
            while tau + 1 <= max_tau and yin[tau + 1] < yin[tau]:
                tau += 1
            seed = tau
            found = True
            break
    if not found:
        seed = int(np.argmin(yin[min_tau : max_tau + 1]) + min_tau)

    global_min = seed
    best_v = yin[seed]
    for t in range(min_tau, max_tau + 1):
        if yin[t] < threshold and yin[t] < best_v:
            if t > 0 and t + 1 < len(yin) and yin[t] <= yin[t - 1] and yin[t] <= yin[t + 1]:
                best_v = yin[t]
                global_min = t

    cands = []
    for seed_tau in (seed, global_min):
        for factor in (1, 2, 3):
            for tau in (seed_tau, seed_tau * factor, seed_tau // factor if factor > 1 else seed_tau):
                if min_tau <= tau <= max_tau and tau not in cands:
                    cands.append(tau)
                if len(cands) >= MAX_CANDIDATES:
                    return cands
    return cands[:MAX_CANDIDATES]


def label_candidates(cands: list[int], true_hz: float) -> tuple[int, int]:
    """Return (best_index, voiced). voiced=0 for noise (true_hz<=0)."""
    if true_hz <= 0:
        return 0, 0
    true_tau = SR / true_hz
    best_i = 0
    best_err = 1e9
    for i, tau in enumerate(cands):
        err = abs(math.log2(max(tau, 1) / true_tau))
        if err < best_err:
            best_err = err
            best_i = i
    # If no candidate within ~0.35 octaves, still pick closest but mark weakly
    return best_i, 1


def generate_dataset(n_per_family: int = 80, seed: int = 42):
    rng = random.Random(seed)
    np.random.seed(seed)
    frequencies = [
        82.41, 110.0, 146.83, 164.81, 196.0, 220.0, 246.94, 293.66, 329.63, 392.0, 440.0
    ]
    gens = [harmonic_rich, missing_fundamental, strong_second]
    X = []  # (num_cands, features)
    y_idx = []
    y_voice = []

    for _ in range(n_per_family):
        for gen in gens:
            f0 = rng.choice(frequencies) * (0.98 + 0.04 * rng.random())
            start = rng.randint(0, 4000)
            data = make_window(gen, f0, start)
            # mild noise
            data = data + rng.uniform(0.0, 0.02) * np.random.randn(WINDOW)
            yin, min_tau, max_tau = compute_yin(data)
            cands = collect_candidates(yin, min_tau, max_tau)
            if not cands:
                continue
            rms, contrast = frame_stats(data)
            # prev pitch: correct, octave-wrong, or none
            mode = rng.choice(["none", "correct", "octave_low", "octave_high", "nearby"])
            if mode == "none":
                prev = 0.0
            elif mode == "correct":
                prev = f0
            elif mode == "octave_low":
                prev = f0 * 0.5
            elif mode == "octave_high":
                prev = f0 * 2.0
            else:
                prev = f0 * (0.94 + 0.12 * rng.random())

            feats = np.stack(
                [build_features(yin, tau, min_tau, max_tau, prev, rms, contrast) for tau in cands]
            )
            # pad to MAX_CANDIDATES
            padded = np.zeros((MAX_CANDIDATES, NUM_FEATURES), dtype=np.float64)
            padded[: len(cands)] = feats
            best_i, voiced = label_candidates(cands, f0)
            X.append(padded)
            y_idx.append(best_i)
            y_voice.append(voiced)

    # Noise / unvoiced frames
    for _ in range(n_per_family):
        data = 0.05 * np.random.randn(WINDOW)
        yin, min_tau, max_tau = compute_yin(data)
        cands = collect_candidates(yin, min_tau, max_tau, threshold=0.3)
        if not cands:
            cands = [min_tau]
        rms, contrast = frame_stats(data)
        feats = np.stack(
            [build_features(yin, tau, min_tau, max_tau, 0.0, rms, contrast) for tau in cands]
        )
        padded = np.zeros((MAX_CANDIDATES, NUM_FEATURES), dtype=np.float64)
        padded[: len(cands)] = feats
        X.append(padded)
        y_idx.append(0)
        y_voice.append(0)

    return np.array(X), np.array(y_idx), np.array(y_voice)


def init_params(rng: np.random.Generator):
    def xavier(fan_in, fan_out):
        scale = math.sqrt(2.0 / (fan_in + fan_out))
        return rng.normal(0.0, scale, size=(fan_out, fan_in))

    return {
        "w1": xavier(NUM_FEATURES, HIDDEN1),
        "b1": np.zeros(HIDDEN1),
        "w2": xavier(HIDDEN1, HIDDEN2),
        "b2": np.zeros(HIDDEN2),
        "w3": xavier(HIDDEN2, NUM_OUTPUTS),
        "b3": np.zeros(NUM_OUTPUTS),
    }


def forward_one(feats: np.ndarray, params: dict):
    h1 = np.maximum(0.0, params["w1"] @ feats + params["b1"])
    h2 = np.maximum(0.0, params["w2"] @ h1 + params["b2"])
    out = params["w3"] @ h2 + params["b3"]
    return out, h1, h2


def train(X, y_idx, y_voice, epochs: int = 40, lr: float = 0.02):
    rng = np.random.default_rng(0)
    params = init_params(rng)
    n = len(X)
    for epoch in range(epochs):
        order = rng.permutation(n)
        total_loss = 0.0
        correct = 0
        voice_correct = 0
        for i in order:
            cands = X[i]
            # how many non-zero rows
            num_c = int(np.sum(np.any(cands != 0, axis=1)))
            num_c = max(1, min(MAX_CANDIDATES, num_c))
            scores = []
            caches = []
            for c in range(num_c):
                out, h1, h2 = forward_one(cands[c], params)
                scores.append(out)
                caches.append((cands[c], h1, h2, out))

            logits = np.array([s[0] for s in scores])
            # softmax cross-entropy for candidate pick
            logits = logits - logits.max()
            exp = np.exp(logits)
            probs = exp / exp.sum()
            target = int(y_idx[i])
            if target >= num_c:
                target = 0
            loss_pick = -math.log(max(probs[target], 1e-8))

            # voiced from best-of (use target candidate's voiced logit)
            v_logit = scores[target][1]
            v_prob = 1.0 / (1.0 + math.exp(-max(-20, min(20, v_logit))))
            yv = float(y_voice[i])
            loss_voice = -(yv * math.log(max(v_prob, 1e-8)) + (1 - yv) * math.log(max(1 - v_prob, 1e-8)))
            loss = loss_pick + 0.5 * loss_voice
            total_loss += loss

            pred = int(np.argmax(probs))
            if pred == target:
                correct += 1
            if (v_prob >= 0.5) == (yv >= 0.5):
                voice_correct += 1

            # gradients
            dlogits = probs.copy()
            dlogits[target] -= 1.0

            grads = {k: np.zeros_like(v) for k, v in params.items()}

            for c in range(num_c):
                feats, h1, h2, out = caches[c]
                # dL/dout
                dout = np.zeros(NUM_OUTPUTS)
                dout[0] = dlogits[c]
                if c == target:
                    dout[1] = 0.5 * (v_prob - yv)

                # backprop dense layers
                grads["w3"] += np.outer(dout, h2)
                grads["b3"] += dout
                dh2 = params["w3"].T @ dout
                dh2 *= (h2 > 0)
                grads["w2"] += np.outer(dh2, h1)
                grads["b2"] += dh2
                dh1 = params["w2"].T @ dh2
                dh1 *= (h1 > 0)
                grads["w1"] += np.outer(dh1, feats)
                grads["b1"] += dh1

            for k in params:
                params[k] -= lr * grads[k]

        if (epoch + 1) % 10 == 0 or epoch == 0:
            print(
                f"epoch {epoch+1:3d}  loss={total_loss/n:.4f}  "
                f"pick_acc={correct/n:.3f}  voice_acc={voice_correct/n:.3f}"
            )
        # mild LR decay
        if epoch == 25:
            lr *= 0.5
    return params


def export_header(params: dict, out_path: Path):
    def fmt_array(name: str, arr: np.ndarray) -> str:
        flat = arr.astype(np.float32).ravel()
        body = ",\n    ".join(
            ", ".join(f"{v:.8f}f" for v in flat[i : i + 8]) for i in range(0, len(flat), 8)
        )
        return f"inline constexpr float {name}[{len(flat)}] {{\n    {body}\n}};\n"

    text = f"""#pragma once

/** Auto-generated by ml/train_octave_net.py — do not edit by hand. */
namespace OctaveNetWeights
{{
{fmt_array("w1", params["w1"])}
{fmt_array("b1", params["b1"])}
{fmt_array("w2", params["w2"])}
{fmt_array("b2", params["b2"])}
{fmt_array("w3", params["w3"])}
{fmt_array("b3", params["b3"])}
}}
"""
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(text)
    print(f"Wrote {out_path}")


def main():
    parser = argparse.ArgumentParser()
    root = Path(__file__).resolve().parents[1]
    parser.add_argument(
        "--out",
        type=Path,
        default=root / "Source" / "OctaveNetWeights.h",
    )
    parser.add_argument("--epochs", type=int, default=40)
    args = parser.parse_args()

    print("Generating synthetic dataset...")
    X, y_idx, y_voice = generate_dataset()
    print(f"Samples: {len(X)}")
    params = train(X, y_idx, y_voice, epochs=args.epochs)
    export_header(params, args.out)


if __name__ == "__main__":
    main()
