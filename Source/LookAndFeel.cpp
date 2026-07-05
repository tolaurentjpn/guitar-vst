#include "LookAndFeel.h"

GuitarLookAndFeel::GuitarLookAndFeel()
{
    background = juce::Colour (0xff121418);
    panel = juce::Colour (0xff1c2028);
    accent = juce::Colour (0xffe85d4c);
    text = juce::Colour (0xffe8eaed);

    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, panel.darker (0.2f));
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, accent.withAlpha (0.4f));
    setColour (juce::ComboBox::arrowColourId, accent);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.35f));
    setColour (juce::Label::textColourId, text);
}

void GuitarLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                          int x,
                                          int y,
                                          int width,
                                          int height,
                                          float sliderPosProportional,
                                          float rotaryStartAngle,
                                          float rotaryEndAngle,
                                          juce::Slider& slider)
{
    juce::ignoreUnused (slider);
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                                static_cast<float> (y),
                                                static_cast<float> (width),
                                                static_cast<float> (height))
                            .reduced (8.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getCentreY();
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    g.setColour (panel.brighter (0.15f));
    g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    g.setColour (accent.withAlpha (0.25f));
    g.drawEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 2.0f);

    juce::Path arc;
    arc.addCentredArc (centreX, centreY, radius * 0.82f, radius * 0.82f, 0.0f,
                       rotaryStartAngle, angle, true);
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRectangle (-2.0f, -radius * 0.75f, 4.0f, radius * 0.35f);
    g.setColour (text);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centreX, centreY));
}

void GuitarLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour (juce::Label::backgroundColourId));
    if (! label.isBeingEdited())
    {
        g.setColour (label.findColour (juce::Label::textColourId));
        g.setFont (label.getFont());
        g.drawFittedText (label.getText(),
                          label.getLocalBounds().reduced (2),
                          label.getJustificationType(),
                          1);
    }
}
