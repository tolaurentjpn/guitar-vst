#pragma once

#include <JuceHeader.h>

/** Drag-editable ADSR curve bound to four APVTS float parameters. */
class AdsrPlotEditor : public juce::Component,
                       private juce::AudioProcessorValueTreeState::Listener
{
public:
    AdsrPlotEditor (juce::AudioProcessorValueTreeState& state,
                    const juce::String& attackId,
                    const juce::String& decayId,
                    const juce::String& sustainId,
                    const juce::String& releaseId);
    ~AdsrPlotEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

    void setAccentColour (juce::Colour c) { accent = c; repaint(); }

private:
    enum class Handle { none, attack, decaySustain, release };

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void refreshFromParams();
    void writeParams();
    juce::Point<float> attackPoint() const;
    juce::Point<float> decayPoint() const;
    juce::Point<float> releasePoint() const;
    juce::Rectangle<float> plotBounds() const;
    Handle hitTestHandle (juce::Point<float> pos) const;

    juce::AudioProcessorValueTreeState& apvts;
    juce::String attackParamId, decayParamId, sustainParamId, releaseParamId;

    float attackMs = 8.0f;
    float decayMs = 180.0f;
    float sustainLevel = 0.5f;
    float releaseMs = 220.0f;

    Handle activeHandle = Handle::none;
    juce::Colour accent { 0xffe85d4c };
};
