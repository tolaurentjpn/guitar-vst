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

    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    juce::Colour accentColour() const { return accent; }
    juce::Colour panelColour() const { return panel; }
    juce::Colour backgroundColour() const { return background; }
    juce::Colour textColour() const { return text; }
    juce::Colour lcdColour() const { return lcd; }

    const juce::Image& getPanelBackground() const { return panelBackground; }
    const juce::Image& getLedOn() const { return ledOn; }
    const juce::Image& getLedOff() const { return ledOff; }

    juce::Image getWaveformButtonImage (int waveformIndex, bool active) const;

private:
    static juce::Image loadBinaryImage (const char* data, int dataSize);

    juce::Colour background;
    juce::Colour panel;
    juce::Colour accent;
    juce::Colour text;
    juce::Colour lcd;

    juce::Image panelBackground;
    juce::Image knobBase;
    juce::Image knobPointer;
    juce::Image btnSine;
    juce::Image btnSineOn;
    juce::Image btnSaw;
    juce::Image btnSawOn;
    juce::Image btnSquare;
    juce::Image btnSquareOn;
    juce::Image ledOn;
    juce::Image ledOff;
};
