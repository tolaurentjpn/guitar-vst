#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"

class GuitarSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit GuitarSynthAudioProcessorEditor (GuitarSynthAudioProcessor&);
    ~GuitarSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text);
    static juce::String frequencyToNoteName (float frequency);

    GuitarSynthAudioProcessor& audioProcessor;
    GuitarLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label pitchLabel;
    juce::Label noteLabel;
    juce::Label confidenceLabel;
    juce::Label inputLevelLabel;
    juce::Label gateLevelLabel;
    juce::Label inputHintLabel;
    juce::Label latencyLabel;
    std::unique_ptr<juce::Component> voicedIndicator;

    juce::ComboBox waveformBox;
    juce::Label waveformLabel;

    juce::Slider filterCutoffSlider;
    juce::Slider filterResonanceSlider;
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    juce::Slider glideSlider;
    juce::Slider masterSlider;
    juce::Slider trackingSlider;
    juce::Slider gateSlider;

    juce::Label filterCutoffLabel;
    juce::Label filterResonanceLabel;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;
    juce::Label glideLabel;
    juce::Label masterLabel;
    juce::Label trackingLabel;
    juce::Label gateLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<SliderAttachment> filterResonanceAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> decayAttachment;
    std::unique_ptr<SliderAttachment> sustainAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> glideAttachment;
    std::unique_ptr<SliderAttachment> masterAttachment;
    std::unique_ptr<SliderAttachment> trackingAttachment;
    std::unique_ptr<SliderAttachment> gateAttachment;
    std::unique_ptr<ComboAttachment> waveformAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarSynthAudioProcessorEditor)
};
