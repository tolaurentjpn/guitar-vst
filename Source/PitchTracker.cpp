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
    unvoicedHoldCounter = 0;
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
    const float detected = computeYinPitch (analysisBuffer.data(), windowSize, confidence);
    lastConfidence = confidence;

    const float minConfidence = juce::jmap (confidenceThreshold, 0.1f, 0.99f, 0.12f, 0.45f);
    const bool detectedVoiced = detected >= minFrequency
                             && detected <= maxFrequency
                             && confidence >= minConfidence;

    if (detectedVoiced)
    {
        unvoicedHoldCounter = hopSize * 2;
        voiced = true;

        if (smoothedFrequency <= 0.0f)
            smoothedFrequency = detected;
        else
            smoothedFrequency = smoothing * smoothedFrequency + (1.0f - smoothing) * detected;
    }
    else if (unvoicedHoldCounter > 0)
    {
        unvoicedHoldCounter -= hopSize;
        voiced = true;
    }
    else
    {
        voiced = false;
        smoothedFrequency *= 0.95f;
        if (smoothedFrequency < minFrequency * 0.5f)
            smoothedFrequency = 0.0f;
    }
}
