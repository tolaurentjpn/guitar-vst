#pragma once

#include <array>
#include <cmath>

/** Tiny MLP that scores YIN period candidates and voicing. Weights are baked. */
namespace OctaveNet
{
    static constexpr int kNumFeatures = 12;
    static constexpr int kMaxCandidates = 6;
    static constexpr int kHidden1 = 32;
    static constexpr int kHidden2 = 16;
    static constexpr int kNumOutputs = 2; // score logit, voiced logit

    struct Result
    {
        int bestCandidateIndex = -1;
        float bestScore = -1.0e9f;
        float voicedProbability = 0.0f;
        bool voiced = false;
    };

    void evaluate (const float features[kNumFeatures], float& scoreLogit, float& voicedLogit) noexcept;

    /** Pick the highest-scoring candidate; voiced if sigmoid(voiced) >= threshold. */
    Result selectBest (const float features[kMaxCandidates][kNumFeatures],
                       int numCandidates,
                       float voicedThreshold) noexcept;

    inline float sigmoid (float x) noexcept
    {
        if (x > 20.0f)  return 1.0f;
        if (x < -20.0f) return 0.0f;
        return 1.0f / (1.0f + std::exp (-x));
    }
}
