#include "CompressorEffect.h"

void CompressorEffect::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    compressor.prepare (spec);
    dryScratchCapacity = static_cast<size_t> (spec.maximumBlockSize)
                       * juce::jmax ((size_t) 1, static_cast<size_t> (spec.numChannels));
    dryScratch.allocate (dryScratchCapacity, true);
    updateCompressorSettings();
    reset();
}

void CompressorEffect::reset()
{
    compressor.reset();
}

void CompressorEffect::updateCompressorSettings()
{
    compressor.setThreshold (thresholdDb);
    compressor.setRatio (ratio);
    compressor.setAttack (attackMs);
    compressor.setRelease (releaseMs);
}

void CompressorEffect::process (juce::dsp::AudioBlock<float>& block)
{
    if (bypassed || block.getNumSamples() == 0)
        return;

    updateCompressorSettings();

    const auto numChannels = block.getNumChannels();
    const auto numSamples = block.getNumSamples();
    const size_t needed = numChannels * numSamples;

    if (needed > dryScratchCapacity)
    {
        dryScratchCapacity = needed;
        dryScratch.allocate (dryScratchCapacity, true);
    }

    for (size_t ch = 0; ch < numChannels; ++ch)
        juce::FloatVectorOperations::copy (dryScratch.getData() + ch * numSamples,
                                           block.getChannelPointer (ch),
                                           static_cast<int> (numSamples));

    juce::dsp::ProcessContextReplacing<float> context (block);
    compressor.process (context);

    const float makeupLin = juce::Decibels::decibelsToGain (makeupDb);
    const float wet = mix;
    const float dry = 1.0f - mix;

    for (size_t ch = 0; ch < numChannels; ++ch)
    {
        float* data = block.getChannelPointer (ch);
        const float* dryData = dryScratch.getData() + ch * numSamples;

        for (size_t i = 0; i < numSamples; ++i)
            data[i] = dry * dryData[i] + wet * (data[i] * makeupLin);
    }
}
