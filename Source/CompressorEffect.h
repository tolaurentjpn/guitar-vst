#pragma once

#include "EffectBase.h"

class CompressorEffect : public EffectBase
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::dsp::AudioBlock<float>& block) override;

    void setThresholdDb (float db) noexcept { thresholdDb = db; }
    void setRatio (float r) noexcept { ratio = juce::jmax (1.0f, r); }
    void setAttackMs (float ms) noexcept { attackMs = juce::jmax (0.1f, ms); }
    void setReleaseMs (float ms) noexcept { releaseMs = juce::jmax (1.0f, ms); }
    void setMakeupDb (float db) noexcept { makeupDb = db; }
    void setMix (float mix01) noexcept { mix = juce::jlimit (0.0f, 1.0f, mix01); }

private:
    void updateCompressorSettings();

    juce::dsp::Compressor<float> compressor;
    juce::HeapBlock<float> dryScratch;
    size_t dryScratchCapacity = 0;

    float thresholdDb = -18.0f;
    float ratio = 4.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
    float makeupDb = 0.0f;
    float mix = 1.0f;
};
