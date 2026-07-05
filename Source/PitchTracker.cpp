#include "PitchTracker.h"
#include <cmath>
#include <algorithm>

void PitchTracker::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    reset();
}

void PitchTracker::reset()
{
    ringBuffer.assign (static_cast<size_t> (windowSize), 0.0f);
    yinBuffer.assign (static_cast<size_t> (windowSize / 2), 0.0f);
    analysisBuffer.assign (static_cast<size_t> (windowSize), 0.0f);
    writeIndex = 0;
    samplesSinceHop = 0;
    smoothedFrequency = 0.0f;
    lastConfidence = 0.0f;
    voiced = false;
}

float PitchTracker::getMinConfidenceThreshold() const noexcept
{
    return juce::jmap (confidenceThreshold, 0.1f, 0.99f, 0.10f, 0.35f);
}

void PitchTracker::clearVoicing() noexcept
{
    voiced = false;
    smoothedFrequency = 0.0f;
    lastConfidence = 0.0f;
    hangoffHopsRemaining = 0;
}

void PitchTracker::flush() noexcept
{
    std::fill (ringBuffer.begin(), ringBuffer.end(), 0.0f);
    writeIndex = 0;
    samplesSinceHop = 0;
    clearVoicing();
}

void PitchTracker::setWindowSize (int newWindowSize)
{
    windowSize = juce::jmax (256, newWindowSize);
    yinBuffer.assign (static_cast<size_t> (windowSize / 2), 0.0f);
    ringBuffer.assign (static_cast<size_t> (windowSize), 0.0f);
    analysisBuffer.assign (static_cast<size_t> (windowSize), 0.0f);
    writeIndex = 0;
    samplesSinceHop = 0;
}

void PitchTracker::setHopSize (int newHopSize)
{
    hopSize = juce::jmax (32, newHopSize);
}

void PitchTracker::setMinFrequency (float hz)
{
    minFrequency = juce::jlimit (40.0f, 500.0f, hz);
}

void PitchTracker::setMaxFrequency (float hz)
{
    maxFrequency = juce::jlimit (200.0f, 4000.0f, hz);
}

void PitchTracker::setConfidenceThreshold (float threshold)
{
    confidenceThreshold = juce::jlimit (0.1f, 0.99f, threshold);
}

void PitchTracker::setSmoothing (float smoothingAmount)
{
    smoothing = juce::jlimit (0.0f, 0.99f, smoothingAmount);
}

int PitchTracker::getLatencySamples() const noexcept
{
    return windowSize / 2 + hopSize / 2;
}

void PitchTracker::pushSample (float sample)
{
    ringBuffer[static_cast<size_t> (writeIndex)] = sample;
    writeIndex = (writeIndex + 1) % windowSize;

    if (++samplesSinceHop >= hopSize)
    {
        samplesSinceHop = 0;
        runAnalysis();
    }
}

int PitchTracker::findLocalMinimumTau (int centreTau, int minTau, int maxTau) const
{
    centreTau = juce::jlimit (minTau, maxTau, centreTau);
    int localMinTau = centreTau;

    for (int tau = juce::jmax (minTau, centreTau - 3); tau <= juce::jmin (maxTau, centreTau + 3); ++tau)
    {
        if (tau > 0 && tau < static_cast<int> (yinBuffer.size()) - 1
            && yinBuffer[static_cast<size_t> (tau)] <= yinBuffer[static_cast<size_t> (localMinTau)]
            && yinBuffer[static_cast<size_t> (tau)] <= yinBuffer[static_cast<size_t> (tau - 1)]
            && yinBuffer[static_cast<size_t> (tau)] <= yinBuffer[static_cast<size_t> (tau + 1)])
        {
            localMinTau = tau;
        }
    }

    return localMinTau;
}

float PitchTracker::scoreHarmonicClarity (int tau, int minTau, int maxTau) const
{
    if (tau < minTau || tau > maxTau)
        return 0.0f;

    const int fundTau = findLocalMinimumTau (tau, minTau, maxTau);
    const float fundYin = yinBuffer[static_cast<size_t> (fundTau)];

    // A true fundamental should not have a much clearer dip at half its period.
    if (fundTau / 2 >= minTau)
    {
        const int halfTau = findLocalMinimumTau (fundTau / 2, minTau, maxTau);
        const float halfYin = yinBuffer[static_cast<size_t> (halfTau)];

        if (halfYin < fundYin * 0.88f)
            return 0.0f;
    }

    // Likewise reject if double-period is a much clearer minimum (2nd-harmonic lock).
    if (fundTau * 2 <= maxTau)
    {
        const int doubleTau = findLocalMinimumTau (fundTau * 2, minTau, maxTau);
        const float doubleYin = yinBuffer[static_cast<size_t> (doubleTau)];

        if (doubleYin < fundYin * 0.88f)
            return 0.0f;
    }

    float score = 1.0f - juce::jlimit (0.0f, 0.99f, fundYin);

    for (int harmonic = 2; harmonic <= 5; ++harmonic)
    {
        const int harmonicTau = fundTau / harmonic;
        if (harmonicTau < minTau)
            break;

        const int localTau = findLocalMinimumTau (harmonicTau, minTau, maxTau);
        const float yin = yinBuffer[static_cast<size_t> (localTau)];
        score *= (1.0f - juce::jlimit (0.0f, 0.99f, yin));
    }

    return score;
}

int PitchTracker::correctOctaveTau (int tau, float yinThreshold, int minTau, int maxTau) const
{
    const int initialTau = findLocalMinimumTau (tau, minTau, maxTau);

    int seeds[2] { initialTau, initialTau };
    int numSeeds = 1;

    float globalMinYin = 1.0f;
    int globalMinTau = initialTau;
    for (int t = minTau; t <= maxTau; ++t)
    {
        if (yinBuffer[static_cast<size_t> (t)] < yinThreshold
            && yinBuffer[static_cast<size_t> (t)] <= yinBuffer[static_cast<size_t> (t - 1)]
            && yinBuffer[static_cast<size_t> (t)] <= yinBuffer[static_cast<size_t> (t + 1)]
            && yinBuffer[static_cast<size_t> (t)] < globalMinYin)
        {
            globalMinYin = yinBuffer[static_cast<size_t> (t)];
            globalMinTau = t;
        }
    }

    if (globalMinTau != initialTau)
        seeds[numSeeds++] = globalMinTau;

    int candidates[16] {};
    int numCandidates = 0;

    for (int s = 0; s < numSeeds; ++s)
    {
        const int seedTau = findLocalMinimumTau (seeds[s], minTau, maxTau);
        candidates[numCandidates++] = seedTau;

        for (int factor = 2; factor <= 8; factor *= 2)
        {
            if (seedTau / factor >= minTau)
                candidates[numCandidates++] = findLocalMinimumTau (seedTau / factor, minTau, maxTau);

            if (seedTau * factor <= maxTau)
                candidates[numCandidates++] = findLocalMinimumTau (seedTau * factor, minTau, maxTau);
        }
    }

    float bestScore = -1.0f;
    int resolvedTau = initialTau;

    for (int i = 0; i < numCandidates; ++i)
    {
        const int candidateTau = candidates[i];
        const float candidateYin = yinBuffer[static_cast<size_t> (candidateTau)];

        if (candidateYin >= yinThreshold)
            continue;

        const float score = scoreHarmonicClarity (candidateTau, minTau, maxTau);

        if (score > bestScore * 1.04f)
        {
            bestScore = score;
            resolvedTau = candidateTau;
        }
        else if (score >= bestScore * 0.94f && candidateTau < resolvedTau)
        {
            bestScore = juce::jmax (bestScore, score);
            resolvedTau = candidateTau;
        }
    }

    return resolvedTau;
}

float PitchTracker::computeYinPitch (const float* data, int numSamples, float& outConfidence)
{
    const int halfSize = numSamples / 2;
    if (halfSize < 2)
    {
        outConfidence = 0.0f;
        return 0.0f;
    }

    const int maxTau = juce::jmin (halfSize - 1,
                                   static_cast<int> (sampleRate / static_cast<double> (minFrequency)));
    const int minTau = juce::jmax (2,
                                   static_cast<int> (sampleRate / static_cast<double> (maxFrequency)));

    if (maxTau <= minTau)
    {
        outConfidence = 0.0f;
        return 0.0f;
    }

    yinBuffer[0] = 1.0f;

    float runningSum = 0.0f;
    for (int tau = 1; tau <= maxTau; ++tau)
    {
        float sum = 0.0f;
        for (int i = 0; i < halfSize; ++i)
        {
            const float delta = data[i] - data[i + tau];
            sum += delta * delta;
        }

        runningSum += sum;
        yinBuffer[static_cast<size_t> (tau)] = runningSum > 0.0f
            ? sum * static_cast<float> (tau) / runningSum
            : 1.0f;
    }

    const float yinThreshold = juce::jmap (confidenceThreshold, 0.1f, 0.99f, 0.22f, 0.08f);
    int bestTau = minTau;

    for (int tau = minTau; tau <= maxTau; ++tau)
    {
        if (yinBuffer[static_cast<size_t> (tau)] < yinThreshold)
        {
            while (tau + 1 <= maxTau
                   && yinBuffer[static_cast<size_t> (tau + 1)] < yinBuffer[static_cast<size_t> (tau)])
                ++tau;

            bestTau = tau;
            break;
        }
    }

    if (bestTau == minTau && yinBuffer[static_cast<size_t> (bestTau)] >= yinThreshold)
    {
        outConfidence = 0.0f;
        return 0.0f;
    }

    bestTau = correctOctaveTau (bestTau, yinThreshold, minTau, maxTau);

    float betterTau = static_cast<float> (bestTau);
    if (bestTau > 0 && bestTau < maxTau)
    {
        const float s0 = yinBuffer[static_cast<size_t> (bestTau - 1)];
        const float s1 = yinBuffer[static_cast<size_t> (bestTau)];
        const float s2 = yinBuffer[static_cast<size_t> (bestTau + 1)];
        const float denom = s0 - 2.0f * s1 + s2;
        if (std::abs (denom) > 1.0e-6f)
            betterTau += (s0 - s2) / (2.0f * denom);
    }

    const float yinValue = yinBuffer[static_cast<size_t> (bestTau)];
    outConfidence = juce::jlimit (0.0f, 1.0f, 1.0f - yinValue);

    if (betterTau <= 0.0f)
        return 0.0f;

    return static_cast<float> (sampleRate / static_cast<double> (betterTau));
}

void PitchTracker::runAnalysis()
{
    for (int i = 0; i < windowSize; ++i)
    {
        const int index = (writeIndex + i) % windowSize;
        analysisBuffer[static_cast<size_t> (i)] = ringBuffer[static_cast<size_t> (index)];
    }

    float confidence = 0.0f;
    float detected = computeYinPitch (analysisBuffer.data(), windowSize, confidence);
    const float previousConfidence = lastConfidence;
    lastConfidence = confidence;

    if (detected > 0.0f)
    {
        while (detected < minFrequency && detected > 0.0f)
            detected *= 2.0f;

        while (detected > maxFrequency)
            detected *= 0.5f;

        if (smoothedFrequency > minFrequency * 0.5f)
        {
            const float ratio = detected / smoothedFrequency;

            // Correct octave-low relative to recent pitch (detected is half the target).
            if (ratio > 0.42f && ratio < 0.58f)
                detected *= 2.0f;
        }
    }

    const float minConfidence = getMinConfidenceThreshold();
    bool detectedVoiced = detected >= minFrequency
                       && detected <= maxFrequency
                       && confidence >= minConfidence;

    if (detectedVoiced && smoothedFrequency > minFrequency * 0.5f)
    {
        const float ratio = detected / smoothedFrequency;
        const float semitones = std::abs (12.0f * std::log2 (juce::jmax (ratio, 1.0e-6f)));

        // Reject low-confidence pitch jumps (common when lifting a finger or when
        // sympathetic open strings bleed into the signal). Always allow octave
        // corrections upward (e.g. A2 bleed -> A3 played).
        const bool isOctaveUpCorrection = ratio > 1.75f && ratio < 2.25f;
        if (semitones > 2.0f && confidence < 0.55f && ! isOctaveUpCorrection)
        {
            detected = smoothedFrequency;
            confidence = juce::jmax (previousConfidence * 0.92f, minConfidence);
            lastConfidence = confidence;
        }
    }

    detectedVoiced = detected >= minFrequency
                  && detected <= maxFrequency
                  && confidence >= minConfidence;

    if (detectedVoiced)
    {
        voiced = true;
        hangoffHopsRemaining = maxHangoffHops;

        if (smoothedFrequency <= 0.0f)
            smoothedFrequency = detected;
        else
        {
            const float ratio = detected / smoothedFrequency;
            const float semitones = std::abs (12.0f * std::log2 (juce::jmax (ratio, 1.0e-6f)));
            const bool isOctaveUpCorrection = ratio > 1.75f && ratio < 2.25f;

            if (isOctaveUpCorrection)
            {
                // Snap quickly when correcting a prior octave-low lock.
                smoothedFrequency = smoothing * 0.15f * smoothedFrequency
                                  + (1.0f - smoothing * 0.15f) * detected;
            }
            else
            {
                const float adaptiveSmoothing = semitones > 1.0f ? smoothing * 0.25f
                                                 : semitones > 0.25f ? smoothing * 0.55f
                                                 : smoothing;
                smoothedFrequency = adaptiveSmoothing * smoothedFrequency
                                  + (1.0f - adaptiveSmoothing) * detected;
            }
        }
    }
    else if (voiced && hangoffHopsRemaining > 0)
    {
        --hangoffHopsRemaining;
        lastConfidence *= 0.88f;
        voiced = lastConfidence >= minConfidence * 0.55f;

        if (! voiced)
            smoothedFrequency = 0.0f;
    }
    else
    {
        voiced = false;
        smoothedFrequency = 0.0f;
        hangoffHopsRemaining = 0;
    }
}
