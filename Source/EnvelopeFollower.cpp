#include "EnvelopeFollower.h"
#include <cmath>

void EnvelopeFollower::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    peakDecayCoeff = msToCoeff (15.0f, sampleRate);
    slowEnvCoeff = msToCoeff (45.0f, sampleRate);
    refractorySamples = juce::jmax (1, static_cast<int> (0.040 * sampleRate));
    reset();
}

void EnvelopeFollower::reset()
{
    envelope = 0.0f;
    peakHold = 0.0f;
    slowEnvelope = 0.0f;
    gateOpen = false;
    onsetPending = false;
    gateOutput = 0.0f;
    samplesSinceOnset = refractorySamples;
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

void EnvelopeFollower::setRetriggerSensitivity (float sensitivity01)
{
    retriggerSensitivity = juce::jlimit (0.0f, 1.0f, sensitivity01);
}

bool EnvelopeFollower::consumeOnset() noexcept
{
    if (! onsetPending)
        return false;

    onsetPending = false;
    return true;
}

float EnvelopeFollower::processSample (float input) noexcept
{
    const float rectified = std::abs (input);
    peakHold = juce::jmax (rectified, peakHold * peakDecayCoeff);

    const float coeff = rectified > envelope ? attackCoeff : releaseCoeff;
    envelope = rectified + coeff * (envelope - rectified);

    const float openThreshold = gateThresholdLinear;
    const float closeThreshold = gateThresholdLinear * 0.35f;
    const bool wasGateOpen = gateOpen;

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
        onsetPending = false;
    }

    if (gateOpen)
    {
        if (! wasGateOpen)
        {
            // First note-on is handled by setPitchState; arm refractory so the same
            // pick edge does not also fire a same-note retrigger.
            samplesSinceOnset = 0;
            onsetPending = false;
        }
        else if (retriggerSensitivity > 0.0f
                 && samplesSinceOnset >= refractorySamples)
        {
            // Higher sensitivity => smaller rise required to count as a pick.
            const float riseRatio = juce::jmap (retriggerSensitivity, 1.0f, 0.0f, 1.25f, 3.0f);
            const float minDelta = juce::jmap (retriggerSensitivity, 1.0f, 0.0f, 0.015f, 0.08f);
            const float floor = juce::jmax (openThreshold, slowEnvelope);
            const bool ampRise = peakHold > floor * riseRatio
                              && peakHold > slowEnvelope + minDelta;

            if (ampRise)
            {
                onsetPending = true;
                samplesSinceOnset = 0;
            }
        }

        ++samplesSinceOnset;
    }
    else
    {
        samplesSinceOnset = refractorySamples;
    }

    // Slow follower trails peaks so the next pluck stands out against sustain.
    if (peakHold > slowEnvelope)
        slowEnvelope = peakHold;
    else
        slowEnvelope = peakHold + slowEnvCoeff * (slowEnvelope - peakHold);

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
