#include "DelayEffect.h"

void DelayEffect::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    const int maxSamples = static_cast<int> (std::ceil (maxDelayMs * 0.001 * sampleRate)) + 8;

    delayL.setMaximumDelayInSamples (maxSamples);
    delayR.setMaximumDelayInSamples (maxSamples);
    delayL.prepare (spec);
    delayR.prepare (spec);

    setTimeMs (350.0f);
    currentDelaySamples = targetDelaySamples;
    reset();
}

void DelayEffect::reset()
{
    delayL.reset();
    delayR.reset();
    dampStateL = 0.0f;
    dampStateR = 0.0f;
    currentDelaySamples = targetDelaySamples;
}

void DelayEffect::setTimeMs (float ms) noexcept
{
    const float clamped = juce::jlimit (1.0f, maxDelayMs, ms);
    targetDelaySamples = clamped * 0.001f * static_cast<float> (sampleRate);
}

double DelayEffect::getTailLengthSeconds() const noexcept
{
    if (bypassed || mix <= 0.001f)
        return 0.0;

    const float delaySec = currentDelaySamples / static_cast<float> (sampleRate);
    // Approximate feedback decay time to ~-60 dB
    const float fb = juce::jmax (0.01f, feedback);
    const double repeats = std::log (0.001) / std::log (static_cast<double> (fb));
    return juce::jmin (8.0, static_cast<double> (delaySec) * repeats);
}

void DelayEffect::process (juce::dsp::AudioBlock<float>& block)
{
    if (bypassed || block.getNumSamples() == 0)
        return;

    const auto numSamples = block.getNumSamples();
    const bool stereo = block.getNumChannels() > 1;

    float* left = block.getChannelPointer (0);
    float* right = stereo ? block.getChannelPointer (1) : left;

    // One-pole toward target delay time (avoid zipper / glitches)
    const float smooth = 1.0f - std::exp (-1.0f / (0.03f * static_cast<float> (sampleRate)));

    // Higher damping → darker feedback (lower cutoff)
    const float dampCutoff = juce::jmap (1.0f - damping, 800.0f, 12000.0f);
    const float dampCoeff = std::exp (-juce::MathConstants<float>::twoPi * dampCutoff
                                      / static_cast<float> (sampleRate));

    const float wet = mix;
    const float dry = 1.0f - mix;

    for (size_t i = 0; i < numSamples; ++i)
    {
        currentDelaySamples += smooth * (targetDelaySamples - currentDelaySamples);
        delayL.setDelay (currentDelaySamples);
        delayR.setDelay (currentDelaySamples);

        const float inL = left[i];
        const float inR = right[i];

        float delayedL = delayL.popSample (0);
        float delayedR = delayR.popSample (0);

        dampStateL += (1.0f - dampCoeff) * (delayedL - dampStateL);
        dampStateR += (1.0f - dampCoeff) * (delayedR - dampStateR);
        delayedL = dampStateL;
        delayedR = dampStateR;

        float fbInL = delayedL * feedback;
        float fbInR = delayedR * feedback;

        if (pingPong && stereo)
        {
            // Cross-feed for ping-pong
            const float tmp = fbInL;
            fbInL = fbInR;
            fbInR = tmp;
        }

        delayL.pushSample (0, inL + fbInL);
        delayR.pushSample (0, inR + fbInR);

        left[i] = dry * inL + wet * delayedL;
        if (stereo)
            right[i] = dry * inR + wet * delayedR;
    }
}
