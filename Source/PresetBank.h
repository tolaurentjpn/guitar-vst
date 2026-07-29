#pragma once

#include <JuceHeader.h>

namespace PresetBank
{
    constexpr int numPresets = 14;

    juce::String getName (int index);
    void applyTo (juce::AudioProcessorValueTreeState& apvts, int index);
}
