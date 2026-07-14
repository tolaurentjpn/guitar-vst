#include "EffectChain.h"

void EffectChain::prepare (const juce::dsp::ProcessSpec& spec)
{
    distortion.prepare (spec);
    compressor.prepare (spec);
    delay.prepare (spec);
    reverb.prepare (spec);
}

void EffectChain::reset()
{
    distortion.reset();
    compressor.reset();
    delay.reset();
    reverb.reset();
}

EffectBase* EffectChain::effectForType (int typeId) noexcept
{
    switch (typeId)
    {
        case 0: return &distortion;
        case 1: return &compressor;
        case 2: return &delay;
        case 3: return &reverb;
        default: return nullptr;
    }
}

void EffectChain::setOrder (const std::array<int, 4>& newOrder) noexcept
{
    // Validate permutation of 0..3; ignore invalid writes
    bool seen[4] = { false, false, false, false };
    for (int id : newOrder)
    {
        if (id < 0 || id > 3 || seen[id])
            return;
        seen[id] = true;
    }

    order = newOrder;
}

void EffectChain::process (juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumSamples() == 0 || buffer.getNumChannels() == 0)
        return;

    juce::dsp::AudioBlock<float> block (buffer);

    for (int typeId : order)
    {
        if (auto* fx = effectForType (typeId))
            fx->process (block);
    }
}

double EffectChain::getTailLengthSeconds() const noexcept
{
    double maxTail = 0.0;
    maxTail = juce::jmax (maxTail, delay.getTailLengthSeconds());
    maxTail = juce::jmax (maxTail, reverb.getTailLengthSeconds());
    return maxTail;
}
