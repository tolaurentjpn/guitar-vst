#pragma once

#include "DistortionEffect.h"
#include "CompressorEffect.h"
#include "DelayEffect.h"
#include "ReverbEffect.h"
#include <array>

class EffectChain
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setOrder (const std::array<int, 4>& newOrder) noexcept;
    std::array<int, 4> getOrder() const noexcept { return order; }

    DistortionEffect& getDistortion() noexcept { return distortion; }
    CompressorEffect& getCompressor() noexcept { return compressor; }
    DelayEffect& getDelay() noexcept { return delay; }
    ReverbEffect& getReverb() noexcept { return reverb; }

    double getTailLengthSeconds() const noexcept;

    static constexpr const char* nameForType (int typeId) noexcept
    {
        switch (typeId)
        {
            case 0: return "Distortion";
            case 1: return "Compressor";
            case 2: return "Delay";
            case 3: return "Reverb";
            default: return "Effect";
        }
    }

private:
    EffectBase* effectForType (int typeId) noexcept;

    DistortionEffect distortion;
    CompressorEffect compressor;
    DelayEffect delay;
    ReverbEffect reverb;

    std::array<int, 4> order { 0, 1, 2, 3 };
};
