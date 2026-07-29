#pragma once

#include "EffectBase.h"

class ChorusEffect : public EffectBase
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::dsp::AudioBlock<float>& block) override;
    /** Process a full stereo AudioBuffer (used as the dedicated pre-rack stage). */
    void process (juce::AudioBuffer<float>& buffer);

    void setRate (float hz) noexcept { rateHz = juce::jlimit (0.05f, 5.0f, hz); }
    void setDepth (float depth01) noexcept { depth = juce::jlimit (0.0f, 1.0f, depth01); }
    void setMix (float mix01) noexcept { mix = juce::jlimit (0.0f, 1.0f, mix01); }

private:
    void updateParams();

    juce::dsp::Chorus<float> chorus;

    float rateHz = 0.8f;
    float depth = 0.35f;
    float mix = 0.4f;
};
