#include "PitchTracker.h"
#include <cmath>
#include <algorithm>

void PitchTracker::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    pyinDecoder.prepare (sampleRate, minFrequency, maxFrequency, windowSize, hopSize);
    reset();
}

void PitchTracker::reset()
{
    ringBuffer.assign (static_cast<size_t> (windowSize), 0.0f);
    analysisBuffer.assign (static_cast<size_t> (windowSize), 0.0f);
    writeIndex = 0;
    samplesSinceHop = 0;
    smoothedFrequency = 0.0f;
    lastConfidence = 0.0f;
    voiced = false;
    hangoffHopsRemaining = 0;
    pyinDecoder.reset();
}

float PitchTracker::getMinConfidenceThreshold() const noexcept
{
    return juce::jmap (confidenceThreshold, 0.1f, 0.99f, 0.08f, 0.28f);
}

void PitchTracker::clearVoicing() noexcept
{
    voiced = false;
    smoothedFrequency = 0.0f;
    lastConfidence = 0.0f;
    lastCandidateFrequency = 0.0f;
    lastCandidateConfidence = 0.0f;
    hangoffHopsRemaining = 0;
}

void PitchTracker::flush() noexcept
{
    std::fill (ringBuffer.begin(), ringBuffer.end(), 0.0f);
    writeIndex = 0;
    samplesSinceHop = 0;
    clearVoicing();
    pyinDecoder.reset();
}

void PitchTracker::setWindowSize (int newWindowSize)
{
    windowSize = juce::jmax (256, newWindowSize);
    ringBuffer.assign (static_cast<size_t> (windowSize), 0.0f);
    analysisBuffer.assign (static_cast<size_t> (windowSize), 0.0f);
    writeIndex = 0;
    samplesSinceHop = 0;
    pyinDecoder.prepare (sampleRate, minFrequency, maxFrequency, windowSize, hopSize);
}

void PitchTracker::setHopSize (int newHopSize)
{
    hopSize = juce::jmax (32, newHopSize);
    pyinDecoder.prepare (sampleRate, minFrequency, maxFrequency, windowSize, hopSize);
}

void PitchTracker::setMinFrequency (float hz)
{
    minFrequency = juce::jlimit (40.0f, 500.0f, hz);
    pyinDecoder.prepare (sampleRate, minFrequency, maxFrequency, windowSize, hopSize);
}

void PitchTracker::setMaxFrequency (float hz)
{
    maxFrequency = juce::jlimit (200.0f, 4000.0f, hz);
    pyinDecoder.prepare (sampleRate, minFrequency, maxFrequency, windowSize, hopSize);
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
    return windowSize / 2 + hopSize / 2 + hopSize * (PyinDecoder::viterbiFrames / 2);
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

void PitchTracker::runAnalysis()
{
    for (int i = 0; i < windowSize; ++i)
    {
        const int index = (writeIndex + i) % windowSize;
        analysisBuffer[static_cast<size_t> (i)] = ringBuffer[static_cast<size_t> (index)];
    }

    float confidence = 0.0f;
    float detected = 0.0f;
    pyinDecoder.processFrame (analysisBuffer.data(), windowSize, detected, confidence);

    lastCandidateFrequency = pyinDecoder.getLastBestCandidateFrequency();
    lastCandidateConfidence = pyinDecoder.getLastBestCandidateProbability();

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

    if (detectedVoiced && smoothedFrequency > minFrequency * 0.5f)
    {
        const float ratio = detected / smoothedFrequency;
        const float semitones = std::abs (12.0f * std::log2 (juce::jmax (ratio, 1.0e-6f)));

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
