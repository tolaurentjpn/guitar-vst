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
    void setRetriggerSensitivity (float sensitivity01);

    float processSample (float input) noexcept;
    bool isGateOpen() const noexcept { return gateOpen; }
    /** Returns true once for each pick onset while the gate is already open. */
    bool consumeOnset() noexcept;
    float getGateOutput() const noexcept { return gateOutput; }
    float getEnvelopeLinear() const noexcept { return envelope; }
    float getPeakHoldLinear() const noexcept { return peakHold; }
    float getEnvelopeDb() const noexcept;

private:
    double sampleRate = 44100.0;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float peakDecayCoeff = 0.0f;
    float slowEnvCoeff = 0.0f;
    float gateThresholdLinear = 0.01f;
    float retriggerSensitivity = 0.5f;
    float envelope = 0.0f;
    float peakHold = 0.0f;
    float slowEnvelope = 0.0f;
    bool gateOpen = false;
    bool onsetPending = false;
    float gateOutput = 0.0f;
    int samplesSinceOnset = 0;
    int refractorySamples = 1;

    static float dbToLinear (float db);
    static float msToCoeff (float ms, double sr);
};
