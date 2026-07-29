#include "ChorusEffect.h"

void ChorusEffect::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    chorus.prepare (spec);
    updateParams();
    reset();
}

void ChorusEffect::reset()
{
    chorus.reset();
}

void ChorusEffect::updateParams()
{
    chorus.setRate (rateHz);
    chorus.setDepth (depth);
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.1f);
    chorus.setMix (mix);
}

void ChorusEffect::process (juce::dsp::AudioBlock<float>& block)
{
    if (bypassed || block.getNumSamples() == 0)
        return;

    updateParams();
    chorus.process (juce::dsp::ProcessContextReplacing<float> (block));
}

void ChorusEffect::process (juce::AudioBuffer<float>& buffer)
{
    if (bypassed || buffer.getNumSamples() == 0)
        return;

    updateParams();
    juce::dsp::AudioBlock<float> block (buffer);
    chorus.process (juce::dsp::ProcessContextReplacing<float> (block));
}
