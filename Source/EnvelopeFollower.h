#pragma once

#include <JuceHeader.h>

class EnvelopeFollower
{
public:
    void prepare (double sampleRate);
    void reset();

    void setAttackMs (float attackMs);
    void setReleaseMs (float releaseMs);
    void setGateThreshold (float thresholdDb);

    float processSample (float input) noexcept;
    bool isGateOpen() const noexcept { return gateOpen; }
    float getGateOutput() const noexcept { return gateOutput; }
    float getEnvelopeLinear() const noexcept { return envelope; }
    float getPeakHoldLinear() const noexcept { return peakHold; }
    float getEnvelopeDb() const noexcept;

private:
    double sampleRate = 44100.0;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float peakDecayCoeff = 0.0f;
    float gateThresholdLinear = 0.01f;
    float envelope = 0.0f;
    float peakHold = 0.0f;
    bool gateOpen = false;
    float gateOutput = 0.0f;

    static float dbToLinear (float db);
    static float msToCoeff (float ms, double sr);
};
