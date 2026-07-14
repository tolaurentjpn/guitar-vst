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
    hangoffHopsRemaining = 0;
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

float PitchTracker::softHarmonicClarity (int tau, int minTau, int maxTau) const
{
    if (tau < minTau || tau > maxTau)
        return 0.0f;

    float score = 1.0f - juce::jlimit (0.0f, 0.99f, yinBuffer[static_cast<size_t> (tau)]);

    for (int harmonic = 2; harmonic <= 5; ++harmonic)
    {
        const int harmonicTau = tau / harmonic;
        if (harmonicTau < minTau)
            break;

        const float yin = yinBuffer[static_cast<size_t> (harmonicTau)];
        score *= (1.0f - juce::jlimit (0.0f, 0.99f, yin));
    }

    return score;
}

void PitchTracker::frameStats (const float* data, int numSamples, float& rms, float& contrast) const
{
    double sumSq = 0.0;
    double lowSum = 0.0;
    int lowCount = 0;
    double highSum = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        const float x = data[i];
        sumSq += static_cast<double> (x) * static_cast<double> (x);
        if ((i & 1) == 0)
        {
            lowSum += std::abs (x);
            ++lowCount;
        }
        if (i > 0)
            highSum += std::abs (x - data[i - 1]);
    }

    rms = static_cast<float> (std::sqrt (sumSq / juce::jmax (1, numSamples)) + 1.0e-8);
    const float low = static_cast<float> (lowSum / juce::jmax (1, lowCount) + 1.0e-8);
    const float high = static_cast<float> (highSum / juce::jmax (1, numSamples - 1) + 1.0e-8);
    contrast = juce::jlimit (-3.0f, 3.0f, std::log ((high + 1.0e-8f) / (low + 1.0e-8f)));
}

int PitchTracker::collectOctaveCandidates (int seedTau, float yinThreshold, int minTau, int maxTau,
                                           int* candidates, int maxCandidates) const
{
    int globalMinTau = seedTau;
    float globalMinYin = yinBuffer[static_cast<size_t> (seedTau)];

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

    const int seeds[2] { seedTau, globalMinTau };
    int numCandidates = 0;

    auto pushUnique = [&] (int tau)
    {
        tau = findLocalMinimumTau (tau, minTau, maxTau);
        if (tau < minTau || tau > maxTau)
            return;
        for (int i = 0; i < numCandidates; ++i)
            if (candidates[i] == tau)
                return;
        if (numCandidates < maxCandidates)
            candidates[numCandidates++] = tau;
    };

    for (int s = 0; s < 2; ++s)
    {
        const int seed = seeds[s];
        pushUnique (seed);
        for (int factor : { 2, 3 })
        {
            if (seed / factor >= minTau)
                pushUnique (seed / factor);
            if (seed * factor <= maxTau)
                pushUnique (seed * factor);
            if (numCandidates >= maxCandidates)
                return numCandidates;
        }
    }

    return numCandidates;
}

void PitchTracker::fillCandidateFeatures (int tau, int minTau, int maxTau,
                                          float prevHz, float rms, float contrast,
                                          float* features) const
{
    const float candHz = static_cast<float> (sampleRate / static_cast<double> (juce::jmax (1, tau)));
    const int halfTau = tau / 2;
    const int doubleTau = tau * 2;

    const float yinTau = yinBuffer[static_cast<size_t> (tau)];
    const float yinHalf = (halfTau >= minTau && halfTau <= maxTau)
                        ? yinBuffer[static_cast<size_t> (halfTau)] : 1.0f;
    const float yinDouble = (doubleTau >= minTau && doubleTau <= maxTau)
                          ? yinBuffer[static_cast<size_t> (doubleTau)] : 1.0f;

    const bool hasPrev = prevHz > minFrequency * 0.5f;
    float ratio = 0.0f;
    if (hasPrev)
        ratio = juce::jlimit (-2.0f, 2.0f, std::log2 (juce::jmax (candHz, 1.0e-6f) / juce::jmax (prevHz, 1.0e-6f)));

    const float tauNorm = static_cast<float> (tau - minTau)
                        / static_cast<float> (juce::jmax (1, maxTau - minTau));

    features[0] = yinTau;
    features[1] = yinHalf;
    features[2] = yinDouble;
    features[3] = softHarmonicClarity (tau, minTau, maxTau);
    features[4] = ratio;
    features[5] = rms;
    features[6] = contrast;
    features[7] = tauNorm;
    features[8] = 1.0f - juce::jlimit (0.0f, 1.0f, yinTau);
    features[9] = yinTau - yinHalf;
    features[10] = yinTau - yinDouble;
    features[11] = hasPrev ? 1.0f : 0.0f;
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
    int seedTau = minTau;
    bool foundSeed = false;

    for (int tau = minTau; tau <= maxTau; ++tau)
    {
        if (yinBuffer[static_cast<size_t> (tau)] < yinThreshold)
        {
            while (tau + 1 <= maxTau
                   && yinBuffer[static_cast<size_t> (tau + 1)] < yinBuffer[static_cast<size_t> (tau)])
                ++tau;

            seedTau = tau;
            foundSeed = true;
            break;
        }
    }

    if (! foundSeed)
    {
        float bestYin = 1.0f;
        for (int tau = minTau; tau <= maxTau; ++tau)
        {
            if (yinBuffer[static_cast<size_t> (tau)] < bestYin)
            {
                bestYin = yinBuffer[static_cast<size_t> (tau)];
                seedTau = tau;
            }
        }
    }

    int candidates[OctaveNet::kMaxCandidates] {};
    const int numCandidates = collectOctaveCandidates (seedTau, yinThreshold, minTau, maxTau,
                                                       candidates, OctaveNet::kMaxCandidates);
    if (numCandidates <= 0)
    {
        outConfidence = 0.0f;
        return 0.0f;
    }

    float rms = 0.0f;
    float contrast = 0.0f;
    frameStats (data, numSamples, rms, contrast);

    float featureMatrix[OctaveNet::kMaxCandidates][OctaveNet::kNumFeatures] {};
    for (int c = 0; c < numCandidates; ++c)
        fillCandidateFeatures (candidates[c], minTau, maxTau, smoothedFrequency,
                               rms, contrast, featureMatrix[c]);

    const float voicedThreshold = juce::jmap (confidenceThreshold, 0.1f, 0.99f, 0.35f, 0.55f);
    const auto netResult = OctaveNet::selectBest (featureMatrix, numCandidates, voicedThreshold);

    if (! netResult.voiced || netResult.bestCandidateIndex < 0)
    {
        outConfidence = netResult.voicedProbability * 0.5f;
        return 0.0f;
    }

    int bestTau = candidates[netResult.bestCandidateIndex];

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
    const float yinConfidence = juce::jlimit (0.0f, 1.0f, 1.0f - yinValue);
    outConfidence = juce::jlimit (0.0f, 1.0f, 0.5f * yinConfidence + 0.5f * netResult.voicedProbability);

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
    }

    const float minConfidence = getMinConfidenceThreshold();
    bool detectedVoiced = detected >= minFrequency
                       && detected <= maxFrequency
                       && confidence >= minConfidence;

    // Soft temporal consistency: reject large non-octave jumps only when confidence is weak.
    // Octave jumps in either direction are allowed (NN already scored against previous Hz).
    if (detectedVoiced && smoothedFrequency > minFrequency * 0.5f)
    {
        const float ratio = detected / smoothedFrequency;
        const float semitones = std::abs (12.0f * std::log2 (juce::jmax (ratio, 1.0e-6f)));
        const bool isOctaveJump = (ratio > 1.75f && ratio < 2.25f)
                               || (ratio > 0.45f && ratio < 0.55f);

        if (semitones > 2.0f && confidence < 0.50f && ! isOctaveJump)
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
            const bool isOctaveJump = (ratio > 1.75f && ratio < 2.25f)
                                   || (ratio > 0.45f && ratio < 0.55f);

            if (isOctaveJump)
            {
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
