#pragma once

#include "EffectBase.h"

class DelayEffect : public EffectBase
{
public:
    static constexpr float maxDelayMs = 2000.0f;

    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::dsp::AudioBlock<float>& block) override;
    double getTailLengthSeconds() const noexcept override;

    void setTimeMs (float ms) noexcept;
    void setFeedback (float fb) noexcept { feedback = juce::jlimit (0.0f, 0.95f, fb); }
    void setDamping (float damp01) noexcept { damping = juce::jlimit (0.0f, 1.0f, damp01); }
    void setMix (float mix01) noexcept { mix = juce::jlimit (0.0f, 1.0f, mix01); }
    void setPingPong (bool enabled) noexcept { pingPong = enabled; }

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayL { 1 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayR { 1 };

    float targetDelaySamples = 0.0f;
    float currentDelaySamples = 0.0f;
    float feedback = 0.35f;
    float damping = 0.35f;
    float mix = 0.35f;
    bool pingPong = false;

    float dampStateL = 0.0f;
    float dampStateR = 0.0f;
};
