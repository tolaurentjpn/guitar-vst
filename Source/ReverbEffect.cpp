#include "ReverbEffect.h"

void ReverbEffect::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    reverb.prepare (spec);
    updateParams();
    reset();
}

void ReverbEffect::reset()
{
    reverb.reset();
}

void ReverbEffect::updateParams()
{
    params.roomSize = roomSize;
    params.damping = damping;
    params.width = width;
    params.freezeMode = 0.0f;
    params.wetLevel = mix;
    params.dryLevel = 1.0f - mix;
    reverb.setParameters (params);
}

double ReverbEffect::getTailLengthSeconds() const noexcept
{
    if (bypassed || mix <= 0.001f)
        return 0.0;

    // Freeverb-style decay scales with room size; cap for host reporting
    return juce::jmap (static_cast<double> (roomSize), 1.5, 6.0);
}

void ReverbEffect::process (juce::dsp::AudioBlock<float>& block)
{
    if (bypassed || block.getNumSamples() == 0)
        return;

    updateParams();

    juce::dsp::ProcessContextReplacing<float> context (block);
    reverb.process (context);
}
