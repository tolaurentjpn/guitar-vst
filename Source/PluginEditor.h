#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "AdsrPlotEditor.h"
#include "FxRackComponent.h"

class GuitarSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer,
                                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit GuitarSynthAudioProcessorEditor (GuitarSynthAudioProcessor&);
    ~GuitarSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class LevelMeter;
    class VoicedLed;
    class SynthPage;
    class LfoPage;
    class FilterEnvPage;
    class FxPage;

    void timerCallback() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void setupSlider (juce::Component& parent, juce::Slider& slider, juce::Label& label,
                      const juce::String& text);
    void setupWaveformButton (juce::Component& parent, juce::ImageButton& button, int waveformIndex,
                              int radioGroupId, const juce::String& paramId);
    void setupOctaveButton (juce::Component& parent, juce::TextButton& button, int octaveIndex);
    void setupLfoShapeButton (juce::Component& parent, juce::TextButton& button, int shapeIndex,
                              int radioGroupId, const juce::String& paramId);
    void setChoiceParameter (const juce::String& paramId, int index);
    void syncWaveformButtons();
    void syncOsc2WaveformButtons();
    void syncOctaveButtons();
    void syncLfoShapeButtons (int lfoIndex);
    void syncDistModeButtons();
    void syncPresetCombo();
    void syncFxDetailVisibility();
    void placeKnob (juce::Rectangle<int> area, juce::Slider& slider, juce::Label& label);
    void paintSectionPanel (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title);
    void layoutSynthPage();
    void layoutLfoPage();
    void layoutFilterEnvPage();
    void layoutFxPage();
    void mirrorAdsrParameters (const juce::String& sourceParamId);
    void mirrorFilterEnvParameters (const juce::String& sourceParamId);
    void copyAdsrFromOsc1ToOsc2();
    void copyFilterEnvFromOsc1ToOsc2();
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
    juce::Label presetLabel;
    juce::ComboBox presetCombo;

    juce::Label osc1WaveformLabel;
    juce::Label osc2WaveformLabel;
    juce::Label oscSectionLabel;
    juce::Label filterSectionLabel;
    juce::Label osc1FilterLabel;
    juce::Label osc2FilterLabel;
    juce::Label envelopeSectionLabel;
    juce::Label osc1EnvelopeLabel;
    juce::Label osc2EnvelopeLabel;
    juce::Label playSectionLabel;
    juce::Label lfo1SectionLabel;
    juce::Label lfo2SectionLabel;
    juce::Label octaveLabel;
    juce::Label osc1UnisonLabel;
    juce::Label osc2UnisonLabel;
    juce::Label filterEnv1SectionLabel;
    juce::Label filterEnv2SectionLabel;

    std::unique_ptr<VoicedLed> voicedIndicator;
    std::unique_ptr<LevelMeter> inputMeter;
    std::unique_ptr<LevelMeter> gateMeter;
    std::unique_ptr<LevelMeter> confidenceMeter;

    std::unique_ptr<SynthPage> synthPage;
    std::unique_ptr<LfoPage> lfoPage;
    std::unique_ptr<FilterEnvPage> filterEnvPage;
    std::unique_ptr<FxPage> fxPage;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };

    std::unique_ptr<FxRackComponent> fxRack;

    juce::ImageButton sineButton;
    juce::ImageButton sawButton;
    juce::ImageButton squareButton;
    juce::ImageButton osc2SineButton;
    juce::ImageButton osc2SawButton;
    juce::ImageButton osc2SquareButton;

    juce::TextButton octaveDownButton { "-1" };
    juce::TextButton octaveZeroButton { "0" };
    juce::TextButton octaveUpButton { "+1" };

    juce::TextButton lfo1SineButton { "Sin" };
    juce::TextButton lfo1TriButton { "Tri" };
    juce::TextButton lfo1SquareButton { "Sqr" };
    juce::TextButton lfo1SawButton { "Saw" };
    juce::TextButton lfo1EnableButton { "On" };
    juce::TextButton lfo2SineButton { "Sin" };
    juce::TextButton lfo2TriButton { "Tri" };
    juce::TextButton lfo2SquareButton { "Sqr" };
    juce::TextButton lfo2SawButton { "Saw" };
    juce::TextButton lfo2EnableButton { "On" };
    juce::TextButton adsrSyncButton { "Sync" };
    juce::TextButton filterEnvSyncButton { "Sync" };

    juce::TextButton distSoftButton { "Soft" };
    juce::TextButton distHardButton { "Hard" };
    juce::TextButton distFoldButton { "Fold" };
    juce::TextButton delayPingPongButton { "Ping-Pong" };

    juce::Slider osc2MixSlider;
    juce::Slider osc2DetuneSlider;
    juce::Slider osc1UnisonVoicesSlider;
    juce::Slider osc1UnisonDetuneSlider;
    juce::Slider osc1UnisonSpreadSlider;
    juce::Slider osc1UnisonBlendSlider;
    juce::Slider osc2UnisonVoicesSlider;
    juce::Slider osc2UnisonDetuneSlider;
    juce::Slider osc2UnisonSpreadSlider;
    juce::Slider osc2UnisonBlendSlider;
    juce::Slider filterCutoffSlider;
    juce::Slider filterResonanceSlider;
    juce::Slider osc2FilterCutoffSlider;
    juce::Slider osc2FilterResonanceSlider;
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    juce::Slider osc2AttackSlider;
    juce::Slider osc2DecaySlider;
    juce::Slider osc2SustainSlider;
    juce::Slider osc2ReleaseSlider;
    juce::Slider glideSlider;
    juce::Slider masterSlider;
    juce::Slider trackingSlider;
    juce::Slider gateSlider;
    juce::Slider lfo1RateSlider;
    juce::Slider lfo1FilterSlider;
    juce::Slider lfo1ResonanceSlider;
    juce::Slider lfo1PitchSlider;
    juce::Slider lfo1AmpSlider;
    juce::Slider lfo2RateSlider;
    juce::Slider lfo2FilterSlider;
    juce::Slider lfo2ResonanceSlider;
    juce::Slider lfo2PitchSlider;
    juce::Slider lfo2AmpSlider;
    juce::Slider filterEnv1AmountSlider;
    juce::Slider filterEnv2AmountSlider;

    juce::Slider distDriveSlider;
    juce::Slider distToneSlider;
    juce::Slider distMixSlider;
    juce::Slider compThresholdSlider;
    juce::Slider compRatioSlider;
    juce::Slider compAttackSlider;
    juce::Slider compReleaseSlider;
    juce::Slider compMakeupSlider;
    juce::Slider compMixSlider;
    juce::Slider delayTimeSlider;
    juce::Slider delayFeedbackSlider;
    juce::Slider delayDampingSlider;
    juce::Slider delayMixSlider;
    juce::Slider reverbSizeSlider;
    juce::Slider reverbDampingSlider;
    juce::Slider reverbWidthSlider;
    juce::Slider reverbMixSlider;

    juce::Label osc2MixLabel;
    juce::Label osc2DetuneLabel;
    juce::Label osc1UnisonVoicesLabel;
    juce::Label osc1UnisonDetuneLabel;
    juce::Label osc1UnisonSpreadLabel;
    juce::Label osc1UnisonBlendLabel;
    juce::Label osc2UnisonVoicesLabel;
    juce::Label osc2UnisonDetuneLabel;
    juce::Label osc2UnisonSpreadLabel;
    juce::Label osc2UnisonBlendLabel;
    juce::Label filterCutoffLabel;
    juce::Label filterResonanceLabel;
    juce::Label osc2FilterCutoffLabel;
    juce::Label osc2FilterResonanceLabel;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;
    juce::Label osc2AttackLabel;
    juce::Label osc2DecayLabel;
    juce::Label osc2SustainLabel;
    juce::Label osc2ReleaseLabel;
    juce::Label glideLabel;
    juce::Label masterLabel;
    juce::Label trackingLabel;
    juce::Label gateLabel;
    juce::Label lfo1RateLabel;
    juce::Label lfo1FilterLabel;
    juce::Label lfo1ResonanceLabel;
    juce::Label lfo1PitchLabel;
    juce::Label lfo1AmpLabel;
    juce::Label lfo2RateLabel;
    juce::Label lfo2FilterLabel;
    juce::Label lfo2ResonanceLabel;
    juce::Label lfo2PitchLabel;
    juce::Label lfo2AmpLabel;
    juce::Label filterEnv1AmountLabel;
    juce::Label filterEnv2AmountLabel;

    juce::Label fxDetailTitleLabel;
    juce::Label distDriveLabel;
    juce::Label distToneLabel;
    juce::Label distMixLabel;
    juce::Label distModeLabel;
    juce::Label compThresholdLabel;
    juce::Label compRatioLabel;
    juce::Label compAttackLabel;
    juce::Label compReleaseLabel;
    juce::Label compMakeupLabel;
    juce::Label compMixLabel;
    juce::Label delayTimeLabel;
    juce::Label delayFeedbackLabel;
    juce::Label delayDampingLabel;
    juce::Label delayMixLabel;
    juce::Label reverbSizeLabel;
    juce::Label reverbDampingLabel;
    juce::Label reverbWidthLabel;
    juce::Label reverbMixLabel;

    std::unique_ptr<AdsrPlotEditor> filterEnv1Plot;
    std::unique_ptr<AdsrPlotEditor> filterEnv2Plot;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> osc2MixAttachment;
    std::unique_ptr<SliderAttachment> osc2DetuneAttachment;
    std::unique_ptr<SliderAttachment> osc1UnisonVoicesAttachment;
    std::unique_ptr<SliderAttachment> osc1UnisonDetuneAttachment;
    std::unique_ptr<SliderAttachment> osc1UnisonSpreadAttachment;
    std::unique_ptr<SliderAttachment> osc1UnisonBlendAttachment;
    std::unique_ptr<SliderAttachment> osc2UnisonVoicesAttachment;
    std::unique_ptr<SliderAttachment> osc2UnisonDetuneAttachment;
    std::unique_ptr<SliderAttachment> osc2UnisonSpreadAttachment;
    std::unique_ptr<SliderAttachment> osc2UnisonBlendAttachment;
    std::unique_ptr<SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<SliderAttachment> filterResonanceAttachment;
    std::unique_ptr<SliderAttachment> osc2FilterCutoffAttachment;
    std::unique_ptr<SliderAttachment> osc2FilterResonanceAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> decayAttachment;
    std::unique_ptr<SliderAttachment> sustainAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> osc2AttackAttachment;
    std::unique_ptr<SliderAttachment> osc2DecayAttachment;
    std::unique_ptr<SliderAttachment> osc2SustainAttachment;
    std::unique_ptr<SliderAttachment> osc2ReleaseAttachment;
    std::unique_ptr<SliderAttachment> glideAttachment;
    std::unique_ptr<SliderAttachment> masterAttachment;
    std::unique_ptr<SliderAttachment> trackingAttachment;
    std::unique_ptr<SliderAttachment> gateAttachment;
    std::unique_ptr<SliderAttachment> lfo1RateAttachment;
    std::unique_ptr<SliderAttachment> lfo1FilterAttachment;
    std::unique_ptr<SliderAttachment> lfo1ResonanceAttachment;
    std::unique_ptr<SliderAttachment> lfo1PitchAttachment;
    std::unique_ptr<SliderAttachment> lfo1AmpAttachment;
    std::unique_ptr<SliderAttachment> lfo2RateAttachment;
    std::unique_ptr<SliderAttachment> lfo2FilterAttachment;
    std::unique_ptr<SliderAttachment> lfo2ResonanceAttachment;
    std::unique_ptr<SliderAttachment> lfo2PitchAttachment;
    std::unique_ptr<SliderAttachment> lfo2AmpAttachment;
    std::unique_ptr<SliderAttachment> filterEnv1AmountAttachment;
    std::unique_ptr<SliderAttachment> filterEnv2AmountAttachment;
    std::unique_ptr<ButtonAttachment> lfo1EnableAttachment;
    std::unique_ptr<ButtonAttachment> lfo2EnableAttachment;
    std::unique_ptr<ButtonAttachment> adsrSyncAttachment;
    std::unique_ptr<ButtonAttachment> filterEnvSyncAttachment;

    std::unique_ptr<SliderAttachment> distDriveAttachment;
    std::unique_ptr<SliderAttachment> distToneAttachment;
    std::unique_ptr<SliderAttachment> distMixAttachment;
    std::unique_ptr<SliderAttachment> compThresholdAttachment;
    std::unique_ptr<SliderAttachment> compRatioAttachment;
    std::unique_ptr<SliderAttachment> compAttackAttachment;
    std::unique_ptr<SliderAttachment> compReleaseAttachment;
    std::unique_ptr<SliderAttachment> compMakeupAttachment;
    std::unique_ptr<SliderAttachment> compMixAttachment;
    std::unique_ptr<SliderAttachment> delayTimeAttachment;
    std::unique_ptr<SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr<SliderAttachment> delayDampingAttachment;
    std::unique_ptr<SliderAttachment> delayMixAttachment;
    std::unique_ptr<SliderAttachment> reverbSizeAttachment;
    std::unique_ptr<SliderAttachment> reverbDampingAttachment;
    std::unique_ptr<SliderAttachment> reverbWidthAttachment;
    std::unique_ptr<SliderAttachment> reverbMixAttachment;
    std::unique_ptr<ButtonAttachment> delayPingPongAttachment;

    bool mirroringAdsr = false;
    bool mirroringFilterEnv = false;
    bool updatingPresetCombo = false;

    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> displayBounds;
    juce::Rectangle<int> meterBounds;
    juce::Rectangle<int> waveformBounds;
    juce::Rectangle<int> oscPanelBounds;
    juce::Rectangle<int> filterPanelBounds;
    juce::Rectangle<int> envelopePanelBounds;
    juce::Rectangle<int> playPanelBounds;
    juce::Rectangle<int> lfo1PanelBounds;
    juce::Rectangle<int> lfo2PanelBounds;
    juce::Rectangle<int> filterEnv1PanelBounds;
    juce::Rectangle<int> filterEnv2PanelBounds;
    juce::Rectangle<int> fxRackBounds;
    juce::Rectangle<int> fxDetailBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarSynthAudioProcessorEditor)
};
