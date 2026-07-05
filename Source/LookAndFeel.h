#pragma once

#include <JuceHeader.h>

class GuitarLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GuitarLookAndFeel();

    void drawRotarySlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawLabel (juce::Graphics& g, juce::Label& label) override;

    juce::Colour accentColour() const { return accent; }
    juce::Colour panelColour() const { return panel; }

private:
    juce::Colour background;
    juce::Colour panel;
    juce::Colour accent;
    juce::Colour text;
};
