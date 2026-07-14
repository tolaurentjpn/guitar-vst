#include "LookAndFeel.h"
#include "BinaryData.h"

namespace
{
    void drawVectorKnob (juce::Graphics& g,
                         juce::Rectangle<float> bounds,
                         float sliderPosProportional,
                         float rotaryStartAngle,
                         float rotaryEndAngle,
                         juce::Colour panel,
                         juce::Colour accent,
                         juce::Colour text)
    {
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
}

juce::Image GuitarLookAndFeel::loadBinaryImage (const char* data, int dataSize)
{
    return juce::ImageFileFormat::loadFrom (data, static_cast<size_t> (dataSize));
}

GuitarLookAndFeel::GuitarLookAndFeel()
{
    background = juce::Colour (0xff121418);
    panel = juce::Colour (0xff1c2028);
    accent = juce::Colour (0xffe85d4c);
    text = juce::Colour (0xffe8eaed);
    lcd = juce::Colour (0xff10201a);

    panelBackground = loadBinaryImage (BinaryData::panel_bg_png, BinaryData::panel_bg_pngSize);
    knobBase = loadBinaryImage (BinaryData::knob_base_png, BinaryData::knob_base_pngSize);
    knobPointer = loadBinaryImage (BinaryData::knob_pointer_png, BinaryData::knob_pointer_pngSize);
    btnSine = loadBinaryImage (BinaryData::btn_sine_png, BinaryData::btn_sine_pngSize);
    btnSineOn = loadBinaryImage (BinaryData::btn_sine_on_png, BinaryData::btn_sine_on_pngSize);
    btnSaw = loadBinaryImage (BinaryData::btn_saw_png, BinaryData::btn_saw_pngSize);
    btnSawOn = loadBinaryImage (BinaryData::btn_saw_on_png, BinaryData::btn_saw_on_pngSize);
    btnSquare = loadBinaryImage (BinaryData::btn_square_png, BinaryData::btn_square_pngSize);
    btnSquareOn = loadBinaryImage (BinaryData::btn_square_on_png, BinaryData::btn_square_on_pngSize);
    ledOn = loadBinaryImage (BinaryData::led_on_png, BinaryData::led_on_pngSize);
    ledOff = loadBinaryImage (BinaryData::led_off_png, BinaryData::led_off_pngSize);

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
    setColour (juce::TextButton::buttonColourId, panel.brighter (0.1f));
    setColour (juce::TextButton::buttonOnColourId, accent);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
}

juce::Image GuitarLookAndFeel::getWaveformButtonImage (int waveformIndex, bool active) const
{
    switch (waveformIndex)
    {
        case 0:  return active ? btnSineOn : btnSine;
        case 1:  return active ? btnSawOn : btnSaw;
        default: return active ? btnSquareOn : btnSquare;
    }
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
    auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                          static_cast<float> (y),
                                          static_cast<float> (width),
                                          static_cast<float> (height))
                      .reduced (6.0f);

    const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    bounds = bounds.withSizeKeepingCentre (diameter, diameter);

    if (knobBase.isNull() || knobPointer.isNull())
    {
        drawVectorKnob (g, bounds, sliderPosProportional, rotaryStartAngle, rotaryEndAngle, panel, accent, text);
        return;
    }

    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    g.setOpacity (1.0f);
    g.drawImage (knobBase, bounds, juce::RectanglePlacement::centred, false);

    {
        const float radius = diameter * 0.42f;
        juce::Path arc;
        arc.addCentredArc (bounds.getCentreX(), bounds.getCentreY(), radius, radius, 0.0f,
                           rotaryStartAngle, angle, true);
        g.setColour (accent.withAlpha (0.9f));
        g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Pointer asset points toward -Y (12 o'clock); JUCE 0 rad is +X, so subtract pi/2.
    {
        const float scale = diameter / static_cast<float> (knobPointer.getWidth());
        auto transform = juce::AffineTransform::translation (-knobPointer.getWidth() * 0.5f,
                                                             -knobPointer.getHeight() * 0.5f)
                             .scaled (scale, scale)
                             .rotated (angle - juce::MathConstants<float>::halfPi)
                             .translated (bounds.getCentreX(), bounds.getCentreY());

        g.drawImageTransformed (knobPointer, transform, false);
    }
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
                          juce::jmax (1, label.getFont().getHeight() > 16.0f ? 2 : 1));
    }
}

void GuitarLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                              juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    auto fill = backgroundColour;

    if (shouldDrawButtonAsDown || button.getToggleState())
        fill = accent;
    else if (shouldDrawButtonAsHighlighted)
        fill = fill.brighter (0.12f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (accent.withAlpha (button.getToggleState() ? 0.9f : 0.35f));
    g.drawRoundedRectangle (bounds, 8.0f, 1.5f);
}
