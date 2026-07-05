#pragma once

#include <JuceHeader.h>
#include <vector>

class PitchTracker
{
public:
    static constexpr int defaultWindowSize = 512;
    static constexpr int defaultHopSize = 128;
    static constexpr float defaultMinFrequency = 80.0f;
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

private:
    float computeYinPitch (const float* data, int numSamples, float& outConfidence);
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
    int unvoicedHoldCounter = 0;

    std::vector<float> yinBuffer;
    std::vector<float> analysisBuffer;
};
