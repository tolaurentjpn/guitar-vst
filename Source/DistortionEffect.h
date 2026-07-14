#pragma once

#include "EffectBase.h"

enum class DistortionMode : int
{
    Soft = 0,
    Hard = 1,
    Fold = 2
};

class DistortionEffect : public EffectBase
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::dsp::AudioBlock<float>& block) override;

    void setMode (DistortionMode newMode) noexcept { mode = newMode; }
    void setDrive (float drive01) noexcept { drive = juce::jlimit (0.0f, 1.0f, drive01); }
    void setTone (float tone01) noexcept { tone = juce::jlimit (0.0f, 1.0f, tone01); }
    void setMix (float mix01) noexcept { mix = juce::jlimit (0.0f, 1.0f, mix01); }

private:
    static float shapeSample (float x, DistortionMode mode, float driveGain) noexcept;

    DistortionMode mode = DistortionMode::Soft;
    float drive = 0.35f;
    float tone = 0.65f;
    float mix = 1.0f;

    float toneStateL = 0.0f;
    float toneStateR = 0.0f;
    float toneCoeff = 0.0f;
};
