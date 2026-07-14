#pragma once

#include "EffectBase.h"

class ReverbEffect : public EffectBase
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::dsp::AudioBlock<float>& block) override;
    double getTailLengthSeconds() const noexcept override;

    void setSize (float size01) noexcept { roomSize = juce::jlimit (0.0f, 1.0f, size01); }
    void setDamping (float damp01) noexcept { damping = juce::jlimit (0.0f, 1.0f, damp01); }
    void setWidth (float width01) noexcept { width = juce::jlimit (0.0f, 1.0f, width01); }
    void setMix (float mix01) noexcept { mix = juce::jlimit (0.0f, 1.0f, mix01); }

private:
    void updateParams();

    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters params;

    float roomSize = 0.45f;
    float damping = 0.4f;
    float width = 1.0f;
    float mix = 0.25f;
};
