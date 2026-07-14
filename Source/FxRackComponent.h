#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include <array>
#include <functional>

/** Horizontal chain strip: 4 reorderable FX cards with bypass + selection. */
class FxRackComponent : public juce::Component,
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    FxRackComponent (juce::AudioProcessorValueTreeState& state, GuitarLookAndFeel& laf);
    ~FxRackComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

    int getSelectedType() const noexcept { return selectedType; }
    void setSelectedType (int typeId);
    std::function<void()> onSelectionChanged;

    void refreshFromApvts();

private:
    struct Card
    {
        juce::Rectangle<int> bounds;
        int typeId = 0;
        juce::TextButton bypass { "On" };
        juce::TextButton moveLeft { "<" };
        juce::TextButton moveRight { ">" };
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    };

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void rebuildOrderFromApvts();
    void writeOrderToApvts();
    void moveSlot (int slotIndex, int delta);
    int hitTestCard (juce::Point<int> pos) const;
    const char* enabledParamForType (int typeId) const;
    juce::String labelForType (int typeId) const;

    juce::AudioProcessorValueTreeState& apvts;
    GuitarLookAndFeel& lookAndFeel;

    std::array<int, 4> order { 0, 1, 2, 3 };
    std::array<Card, 4> cards;
    int selectedType = 0;

    int dragSlot = -1;
    int dragOverSlot = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxRackComponent)
};
