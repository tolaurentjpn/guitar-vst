#include "EnvelopeFollower.h"
#include <cmath>

void EnvelopeFollower::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    peakDecayCoeff = msToCoeff (15.0f, sampleRate);
    reset();
}

void EnvelopeFollower::reset()
{
    envelope = 0.0f;
    peakHold = 0.0f;
    gateOpen = false;
    gateOutput = 0.0f;
}

void EnvelopeFollower::setAttackMs (float attackMs)
{
    attackCoeff = msToCoeff (attackMs, sampleRate);
}

void EnvelopeFollower::setReleaseMs (float releaseMs)
{
    releaseCoeff = msToCoeff (releaseMs, sampleRate);
}

void EnvelopeFollower::setGateThreshold (float thresholdDb)
{
    gateThresholdLinear = dbToLinear (thresholdDb);
}

float EnvelopeFollower::processSample (float input) noexcept
{
    const float rectified = std::abs (input);
    peakHold = juce::jmax (rectified, peakHold * peakDecayCoeff);

    const float coeff = rectified > envelope ? attackCoeff : releaseCoeff;
    envelope = rectified + coeff * (envelope - rectified);

    const float openThreshold = gateThresholdLinear;
    const float closeThreshold = gateThresholdLinear * 0.35f;

    if (! gateOpen)
    {
        // Open on transient peaks (picked notes), not slow envelope buildup from noise floor.
        if (peakHold >= openThreshold)
            gateOpen = true;
    }
    else if (envelope < closeThreshold
             || (peakHold < openThreshold * 0.6f && envelope < openThreshold * 0.45f))
    {
        gateOpen = false;
        peakHold = 0.0f;
    }

    const float target = gateOpen ? 1.0f : 0.0f;
    const float gateCloseCoeff = msToCoeff (20.0f, sampleRate);
    const float slewCoeff = gateOpen ? attackCoeff : gateCloseCoeff;
    gateOutput = target + slewCoeff * (gateOutput - target);

    return gateOutput;
}

float EnvelopeFollower::getEnvelopeDb() const noexcept
{
    return envelope > 1.0e-8f ? 20.0f * std::log10 (envelope) : -100.0f;
}

float EnvelopeFollower::dbToLinear (float db)
{
    return std::pow (10.0f, db / 20.0f);
}

float EnvelopeFollower::msToCoeff (float ms, double sr)
{
    const float safeMs = juce::jmax (0.1f, ms);
    return std::exp (-1.0f / (0.001f * safeMs * static_cast<float> (sr)));
}
