#pragma once

#include <JuceHeader.h>
#include <vector>
#include "OctaveNet.h"

class PitchTracker
{
public:
    static constexpr int defaultWindowSize = 2048;
    static constexpr int defaultHopSize = 64;
    static constexpr float defaultMinFrequency = 70.0f;
    static constexpr float defaultMaxFrequency = 1200.0f;

    void prepare (double sampleRate);
    void reset();

    void setWindowSize (int newWindowSize);
    void setHopSize (int newHopSize);
    void setMinFrequency (float hz);
    void setMaxFrequency (float hz);
    void setConfidenceThreshold (float threshold);
    void setSmoothing (float smoothingAmount);

    int getWindowSize() const noexcept { return windowSize; }
    int getHopSize() const noexcept { return hopSize; }
    int getLatencySamples() const noexcept;

    void pushSample (float sample);

    float getFrequency() const noexcept { return smoothedFrequency; }
    float getConfidence() const noexcept { return lastConfidence; }
    bool isVoiced() const noexcept { return voiced; }
    float getMinConfidenceThreshold() const noexcept;
    void clearVoicing() noexcept;
    void flush() noexcept;

private:
    float computeYinPitch (const float* data, int numSamples, float& outConfidence);
    int findLocalMinimumTau (int centreTau, int minTau, int maxTau) const;
    float softHarmonicClarity (int tau, int minTau, int maxTau) const;
    int collectOctaveCandidates (int seedTau, float yinThreshold, int minTau, int maxTau,
                                 int* candidates, int maxCandidates) const;
    void fillCandidateFeatures (int tau, int minTau, int maxTau,
                                float prevHz, float rms, float contrast,
                                float* features) const;
    void frameStats (const float* data, int numSamples, float& rms, float& contrast) const;
    void runAnalysis();

    double sampleRate = 44100.0;
    int windowSize = defaultWindowSize;
    int hopSize = defaultHopSize;
    float minFrequency = defaultMinFrequency;
    float maxFrequency = defaultMaxFrequency;
    float confidenceThreshold = 0.85f;
    float smoothing = 0.35f;

    std::vector<float> ringBuffer;
    int writeIndex = 0;
    int samplesSinceHop = 0;

    float smoothedFrequency = 0.0f;
    float lastConfidence = 0.0f;
    bool voiced = false;
    int hangoffHopsRemaining = 0;

    static constexpr int maxHangoffHops = 10;

    std::vector<float> yinBuffer;
    std::vector<float> analysisBuffer;
};
