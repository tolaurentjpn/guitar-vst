#pragma once

#include <JuceHeader.h>

namespace PresetBank
{
    constexpr int numPresets = 10;

    juce::String getName (int index);
    void applyTo (juce::AudioProcessorValueTreeState& apvts, int index);
}
