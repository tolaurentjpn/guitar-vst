#include "EnvelopeFollower.h"
#include <cmath>

void EnvelopeFollower::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    reset();
}

void EnvelopeFollower::reset()
{
    envelope = 0.0f;
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
    const float coeff = rectified > envelope ? attackCoeff : releaseCoeff;
    envelope = rectified + coeff * (envelope - rectified);

    if (envelope < gateThresholdLinear)
        return 0.0f;

    return juce::jlimit (0.0f, 1.0f, envelope / gateThresholdLinear);
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
