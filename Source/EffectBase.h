#pragma once

#include <JuceHeader.h>

enum class FxType : int
{
    Distortion = 0,
    Compressor = 1,
    Delay = 2,
    Reverb = 3,
    Count = 4
};

class EffectBase
{
public:
    virtual ~EffectBase() = default;

    virtual void prepare (const juce::dsp::ProcessSpec& spec) = 0;
    virtual void reset() = 0;
    virtual void process (juce::dsp::AudioBlock<float>& block) = 0;

    void setBypassed (bool shouldBypass) noexcept { bypassed = shouldBypass; }
    bool isBypassed() const noexcept { return bypassed; }

    virtual double getTailLengthSeconds() const noexcept { return 0.0; }

protected:
    bool bypassed = true;
    double sampleRate = 44100.0;
};
