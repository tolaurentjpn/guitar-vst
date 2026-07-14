#include "PluginEditor.h"
#include "EffectChain.h"

class GuitarSynthAudioProcessorEditor::LevelMeter : public juce::Component
{
public:
    void setLevel (float newLevel)
    {
        level = juce::jlimit (0.0f, 1.0f, newLevel);
        repaint();
    }

    void setAccent (juce::Colour c) { accent = c; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (juce::Colour (0xff10141a));
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (juce::Colour (0xff3a3f47));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        auto fill = bounds.reduced (2.0f);
        fill.setWidth (fill.getWidth() * level);
        g.setColour (accent);
        g.fillRoundedRectangle (fill, 3.0f);
    }

private:
    float level = 0.0f;
    juce::Colour accent { 0xffe85d4c };
};

class GuitarSynthAudioProcessorEditor::VoicedLed : public juce::Component
{
public:
    explicit VoicedLed (GuitarLookAndFeel& laf) : lookAndFeel (laf) {}

    void setActive (bool shouldBeActive)
    {
        if (active != shouldBeActive)
        {
            active = shouldBeActive;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const auto& image = active ? lookAndFeel.getLedOn() : lookAndFeel.getLedOff();
        if (! image.isNull())
        {
            g.drawImage (image, getLocalBounds().toFloat(), juce::RectanglePlacement::centred, false);
            return;
        }

        const auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (active ? juce::Colour (0xff4cd964) : juce::Colour (0xff3a3f47));
        g.fillEllipse (bounds);
    }

private:
    GuitarLookAndFeel& lookAndFeel;
    bool active = false;
};

class GuitarSynthAudioProcessorEditor::SynthPage : public juce::Component
{
public:
    explicit SynthPage (GuitarSynthAudioProcessorEditor& ed) : editor (ed) {}

    void paint (juce::Graphics& g) override
    {
        editor.paintSectionPanel (g, editor.oscPanelBounds, "OSCILLATORS");
        editor.paintSectionPanel (g, editor.waveformBounds, {});
        editor.paintSectionPanel (g, editor.playPanelBounds, "PLAY");
        editor.paintSectionPanel (g, editor.filterPanelBounds, "FILTERS");
        editor.paintSectionPanel (g, editor.envelopePanelBounds, "ENVELOPES");
    }

    void resized() override
    {
        editor.layoutSynthPage();
    }

private:
    GuitarSynthAudioProcessorEditor& editor;
};

class GuitarSynthAudioProcessorEditor::LfoPage : public juce::Component
{
public:
    explicit LfoPage (GuitarSynthAudioProcessorEditor& ed) : editor (ed) {}

    void paint (juce::Graphics& g) override
    {
        editor.paintSectionPanel (g, editor.lfo1PanelBounds, "LFO 1");
        editor.paintSectionPanel (g, editor.lfo2PanelBounds, "LFO 2");
    }

    void resized() override
    {
        editor.layoutLfoPage();
    }

private:
    GuitarSynthAudioProcessorEditor& editor;
};

class GuitarSynthAudioProcessorEditor::FilterEnvPage : public juce::Component
{
public:
    explicit FilterEnvPage (GuitarSynthAudioProcessorEditor& ed) : editor (ed) {}

    void paint (juce::Graphics& g) override
    {
        editor.paintSectionPanel (g, editor.filterEnv1PanelBounds, "FILTER ENV 1");
        editor.paintSectionPanel (g, editor.filterEnv2PanelBounds, "FILTER ENV 2");
    }

    void resized() override
    {
        editor.layoutFilterEnvPage();
    }

private:
    GuitarSynthAudioProcessorEditor& editor;
};

class GuitarSynthAudioProcessorEditor::FxPage : public juce::Component
{
public:
    explicit FxPage (GuitarSynthAudioProcessorEditor& ed) : editor (ed) {}

    void paint (juce::Graphics& g) override
    {
        editor.paintSectionPanel (g, editor.fxRackBounds, "FX CHAIN");
        editor.paintSectionPanel (g, editor.fxDetailBounds, "EFFECT");
    }

    void resized() override
    {
        editor.layoutFxPage();
    }

private:
    GuitarSynthAudioProcessorEditor& editor;
};

namespace
{
    void styleChoiceButton (juce::TextButton& button)
    {
        button.setClickingTogglesState (true);
        button.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1c2028));
        button.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe85d4c));
        button.setColour (juce::TextButton::textColourOffId, juce::Colours::whitesmoke);
        button.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }
}

GuitarSynthAudioProcessorEditor::GuitarSynthAudioProcessorEditor (GuitarSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      voicedIndicator (std::make_unique<VoicedLed> (lookAndFeel)),
      inputMeter (std::make_unique<LevelMeter>()),
      gateMeter (std::make_unique<LevelMeter>()),
      confidenceMeter (std::make_unique<LevelMeter>()),
      synthPage (std::make_unique<SynthPage> (*this)),
      lfoPage (std::make_unique<LfoPage> (*this)),
      filterEnvPage (std::make_unique<FilterEnvPage> (*this)),
      fxPage (std::make_unique<FxPage> (*this))
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    setResizeLimits (720, 600, 1400, 1100);

    titleLabel.setText ("GUITAR SYNTH", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    pitchLabel.setText ("--- Hz", juce::dontSendNotification);
    pitchLabel.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    pitchLabel.setJustificationType (juce::Justification::centred);
    pitchLabel.setColour (juce::Label::textColourId, juce::Colour (0xff7dffb0));
    addAndMakeVisible (pitchLabel);

    noteLabel.setText ("---", juce::dontSendNotification);
    noteLabel.setFont (juce::Font (juce::FontOptions (42.0f, juce::Font::bold)));
    noteLabel.setJustificationType (juce::Justification::centred);
    noteLabel.setColour (juce::Label::textColourId, juce::Colour (0xff9cffc4));
    addAndMakeVisible (noteLabel);

    confidenceLabel.setText ("Confidence", juce::dontSendNotification);
    confidenceLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (confidenceLabel);

    inputLevelLabel.setText ("Input", juce::dontSendNotification);
    inputLevelLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (inputLevelLabel);

    gateLevelLabel.setText ("Gate", juce::dontSendNotification);
    gateLevelLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (gateLevelLabel);

    inputHintLabel.setText ("", juce::dontSendNotification);
    inputHintLabel.setJustificationType (juce::Justification::centredLeft);
    inputHintLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffcc66));
    inputHintLabel.setFont (juce::Font (juce::FontOptions (13.0f)));
    addAndMakeVisible (inputHintLabel);

    latencyLabel.setText ("Latency: -- ms", juce::dontSendNotification);
    latencyLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (latencyLabel);

    presetLabel.setText ("Preset", juce::dontSendNotification);
    presetLabel.setJustificationType (juce::Justification::centredRight);
    presetLabel.setFont (juce::Font (juce::FontOptions (13.0f)));
    addAndMakeVisible (presetLabel);

    for (int i = 0; i < audioProcessor.getNumPrograms(); ++i)
        presetCombo.addItem (audioProcessor.getProgramName (i), i + 1);
    presetCombo.onChange = [this]
    {
        if (! updatingPresetCombo)
            audioProcessor.setCurrentProgram (presetCombo.getSelectedItemIndex());
    };
    addAndMakeVisible (presetCombo);

    auto styleSectionLabel = [this] (juce::Component& parent, juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, lookAndFeel.accentColour());
        parent.addAndMakeVisible (label);
    };

    styleSectionLabel (*synthPage, osc1WaveformLabel, "OSC 1");
    styleSectionLabel (*synthPage, osc2WaveformLabel, "OSC 2");
    styleSectionLabel (*synthPage, oscSectionLabel, "OSCILLATORS");
    styleSectionLabel (*synthPage, filterSectionLabel, "FILTERS");
    styleSectionLabel (*synthPage, osc1FilterLabel, "OSC 1");
    styleSectionLabel (*synthPage, osc2FilterLabel, "OSC 2");
    styleSectionLabel (*synthPage, envelopeSectionLabel, "ENVELOPES");
    styleSectionLabel (*synthPage, osc1EnvelopeLabel, "OSC 1");
    styleSectionLabel (*synthPage, osc2EnvelopeLabel, "OSC 2");
    styleSectionLabel (*synthPage, playSectionLabel, "PLAY");
    styleSectionLabel (*synthPage, octaveLabel, "OCTAVE");
    styleSectionLabel (*synthPage, osc1UnisonLabel, "UNISON");
    styleSectionLabel (*synthPage, osc2UnisonLabel, "UNISON");
    styleSectionLabel (*lfoPage, lfo1SectionLabel, "LFO 1 → OSC 1");
    styleSectionLabel (*lfoPage, lfo2SectionLabel, "LFO 2 → OSC 2");
    styleSectionLabel (*filterEnvPage, filterEnv1SectionLabel, "FILTER ENV 1");
    styleSectionLabel (*filterEnvPage, filterEnv2SectionLabel, "FILTER ENV 2");

    inputMeter->setAccent (juce::Colour (0xff4aa3ff));
    gateMeter->setAccent (juce::Colour (0xff4cd964));
    confidenceMeter->setAccent (lookAndFeel.accentColour());
    addAndMakeVisible (*inputMeter);
    addAndMakeVisible (*gateMeter);
    addAndMakeVisible (*confidenceMeter);
    addAndMakeVisible (*voicedIndicator);

    setupWaveformButton (*synthPage, sineButton, 0, 1001, GuitarSynthAudioProcessor::paramWaveform);
    setupWaveformButton (*synthPage, sawButton, 1, 1001, GuitarSynthAudioProcessor::paramWaveform);
    setupWaveformButton (*synthPage, squareButton, 2, 1001, GuitarSynthAudioProcessor::paramWaveform);

    setupWaveformButton (*synthPage, osc2SineButton, 0, 1002, GuitarSynthAudioProcessor::paramOsc2Waveform);
    setupWaveformButton (*synthPage, osc2SawButton, 1, 1002, GuitarSynthAudioProcessor::paramOsc2Waveform);
    setupWaveformButton (*synthPage, osc2SquareButton, 2, 1002, GuitarSynthAudioProcessor::paramOsc2Waveform);

    setupOctaveButton (*synthPage, octaveDownButton, 0);
    setupOctaveButton (*synthPage, octaveZeroButton, 1);
    setupOctaveButton (*synthPage, octaveUpButton, 2);

    setupLfoShapeButton (*lfoPage, lfo1SineButton, 0, 1003, GuitarSynthAudioProcessor::paramLfo1Shape);
    setupLfoShapeButton (*lfoPage, lfo1TriButton, 1, 1003, GuitarSynthAudioProcessor::paramLfo1Shape);
    setupLfoShapeButton (*lfoPage, lfo1SquareButton, 2, 1003, GuitarSynthAudioProcessor::paramLfo1Shape);
    setupLfoShapeButton (*lfoPage, lfo1SawButton, 3, 1003, GuitarSynthAudioProcessor::paramLfo1Shape);

    setupLfoShapeButton (*lfoPage, lfo2SineButton, 0, 1004, GuitarSynthAudioProcessor::paramLfo2Shape);
    setupLfoShapeButton (*lfoPage, lfo2TriButton, 1, 1004, GuitarSynthAudioProcessor::paramLfo2Shape);
    setupLfoShapeButton (*lfoPage, lfo2SquareButton, 2, 1004, GuitarSynthAudioProcessor::paramLfo2Shape);
    setupLfoShapeButton (*lfoPage, lfo2SawButton, 3, 1004, GuitarSynthAudioProcessor::paramLfo2Shape);

    styleChoiceButton (lfo1EnableButton);
    styleChoiceButton (lfo2EnableButton);
    styleChoiceButton (adsrSyncButton);
    styleChoiceButton (filterEnvSyncButton);
    lfoPage->addAndMakeVisible (lfo1EnableButton);
    lfoPage->addAndMakeVisible (lfo2EnableButton);
    synthPage->addAndMakeVisible (adsrSyncButton);
    filterEnvPage->addAndMakeVisible (filterEnvSyncButton);

    setupSlider (*synthPage, osc2MixSlider, osc2MixLabel, "Mix");
    setupSlider (*synthPage, osc2DetuneSlider, osc2DetuneLabel, "Detune");
    setupSlider (*synthPage, osc1UnisonVoicesSlider, osc1UnisonVoicesLabel, "Voices");
    setupSlider (*synthPage, osc1UnisonDetuneSlider, osc1UnisonDetuneLabel, "Detune");
    setupSlider (*synthPage, osc1UnisonSpreadSlider, osc1UnisonSpreadLabel, "Spread");
    setupSlider (*synthPage, osc1UnisonBlendSlider, osc1UnisonBlendLabel, "Blend");
    setupSlider (*synthPage, osc2UnisonVoicesSlider, osc2UnisonVoicesLabel, "Voices");
    setupSlider (*synthPage, osc2UnisonDetuneSlider, osc2UnisonDetuneLabel, "Detune");
    setupSlider (*synthPage, osc2UnisonSpreadSlider, osc2UnisonSpreadLabel, "Spread");
    setupSlider (*synthPage, osc2UnisonBlendSlider, osc2UnisonBlendLabel, "Blend");
    setupSlider (*synthPage, filterCutoffSlider, filterCutoffLabel, "Cutoff");
    setupSlider (*synthPage, filterResonanceSlider, filterResonanceLabel, "Resonance");
    setupSlider (*synthPage, osc2FilterCutoffSlider, osc2FilterCutoffLabel, "Cutoff");
    setupSlider (*synthPage, osc2FilterResonanceSlider, osc2FilterResonanceLabel, "Resonance");
    setupSlider (*synthPage, attackSlider, attackLabel, "Attack");
    setupSlider (*synthPage, decaySlider, decayLabel, "Decay");
    setupSlider (*synthPage, sustainSlider, sustainLabel, "Sustain");
    setupSlider (*synthPage, releaseSlider, releaseLabel, "Release");
    setupSlider (*synthPage, osc2AttackSlider, osc2AttackLabel, "Attack");
    setupSlider (*synthPage, osc2DecaySlider, osc2DecayLabel, "Decay");
    setupSlider (*synthPage, osc2SustainSlider, osc2SustainLabel, "Sustain");
    setupSlider (*synthPage, osc2ReleaseSlider, osc2ReleaseLabel, "Release");
    setupSlider (*synthPage, glideSlider, glideLabel, "Glide");
    setupSlider (*synthPage, masterSlider, masterLabel, "Master");
    setupSlider (*synthPage, trackingSlider, trackingLabel, "Sensitivity");
    setupSlider (*synthPage, gateSlider, gateLabel, "Gate");

    setupSlider (*lfoPage, lfo1RateSlider, lfo1RateLabel, "Rate");
    setupSlider (*lfoPage, lfo1FilterSlider, lfo1FilterLabel, "Cutoff");
    setupSlider (*lfoPage, lfo1ResonanceSlider, lfo1ResonanceLabel, "Resonance");
    setupSlider (*lfoPage, lfo1PitchSlider, lfo1PitchLabel, "Pitch");
    setupSlider (*lfoPage, lfo1AmpSlider, lfo1AmpLabel, "Amp");
    setupSlider (*lfoPage, lfo2RateSlider, lfo2RateLabel, "Rate");
    setupSlider (*lfoPage, lfo2FilterSlider, lfo2FilterLabel, "Cutoff");
    setupSlider (*lfoPage, lfo2ResonanceSlider, lfo2ResonanceLabel, "Resonance");
    setupSlider (*lfoPage, lfo2PitchSlider, lfo2PitchLabel, "Pitch");
    setupSlider (*lfoPage, lfo2AmpSlider, lfo2AmpLabel, "Amp");

    setupSlider (*filterEnvPage, filterEnv1AmountSlider, filterEnv1AmountLabel, "Amount");
    setupSlider (*filterEnvPage, filterEnv2AmountSlider, filterEnv2AmountLabel, "Amount");

    fxDetailTitleLabel.setText ("Distortion", juce::dontSendNotification);
    fxDetailTitleLabel.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
    fxDetailTitleLabel.setJustificationType (juce::Justification::centredLeft);
    fxDetailTitleLabel.setColour (juce::Label::textColourId, lookAndFeel.accentColour());
    fxPage->addAndMakeVisible (fxDetailTitleLabel);

    distModeLabel.setText ("Mode", juce::dontSendNotification);
    distModeLabel.setJustificationType (juce::Justification::centredLeft);
    distModeLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    fxPage->addAndMakeVisible (distModeLabel);

    styleChoiceButton (distSoftButton);
    styleChoiceButton (distHardButton);
    styleChoiceButton (distFoldButton);
    distSoftButton.setRadioGroupId (1010);
    distHardButton.setRadioGroupId (1010);
    distFoldButton.setRadioGroupId (1010);
    distSoftButton.onClick = [this] { setChoiceParameter (GuitarSynthAudioProcessor::paramDistMode, 0); syncDistModeButtons(); };
    distHardButton.onClick = [this] { setChoiceParameter (GuitarSynthAudioProcessor::paramDistMode, 1); syncDistModeButtons(); };
    distFoldButton.onClick = [this] { setChoiceParameter (GuitarSynthAudioProcessor::paramDistMode, 2); syncDistModeButtons(); };
    fxPage->addAndMakeVisible (distSoftButton);
    fxPage->addAndMakeVisible (distHardButton);
    fxPage->addAndMakeVisible (distFoldButton);

    styleChoiceButton (delayPingPongButton);
    fxPage->addAndMakeVisible (delayPingPongButton);

    setupSlider (*fxPage, distDriveSlider, distDriveLabel, "Drive");
    setupSlider (*fxPage, distToneSlider, distToneLabel, "Tone");
    setupSlider (*fxPage, distMixSlider, distMixLabel, "Mix");
    setupSlider (*fxPage, compThresholdSlider, compThresholdLabel, "Thresh");
    setupSlider (*fxPage, compRatioSlider, compRatioLabel, "Ratio");
    setupSlider (*fxPage, compAttackSlider, compAttackLabel, "Attack");
    setupSlider (*fxPage, compReleaseSlider, compReleaseLabel, "Release");
    setupSlider (*fxPage, compMakeupSlider, compMakeupLabel, "Makeup");
    setupSlider (*fxPage, compMixSlider, compMixLabel, "Mix");
    setupSlider (*fxPage, delayTimeSlider, delayTimeLabel, "Time");
    setupSlider (*fxPage, delayFeedbackSlider, delayFeedbackLabel, "Feedback");
    setupSlider (*fxPage, delayDampingSlider, delayDampingLabel, "Damping");
    setupSlider (*fxPage, delayMixSlider, delayMixLabel, "Mix");
    setupSlider (*fxPage, reverbSizeSlider, reverbSizeLabel, "Size");
    setupSlider (*fxPage, reverbDampingSlider, reverbDampingLabel, "Damping");
    setupSlider (*fxPage, reverbWidthSlider, reverbWidthLabel, "Width");
    setupSlider (*fxPage, reverbMixSlider, reverbMixLabel, "Mix");

    filterCutoffSlider.setSkewFactorFromMidPoint (800.0f);
    osc2FilterCutoffSlider.setSkewFactorFromMidPoint (800.0f);
    lfo1RateSlider.setSkewFactorFromMidPoint (2.0f);
    lfo2RateSlider.setSkewFactorFromMidPoint (2.0f);
    gateSlider.textFromValueFunction = [] (double value)
    {
        return juce::String (value, 1) + " dB";
    };
    osc2DetuneSlider.textFromValueFunction = [] (double value)
    {
        return juce::String (value, 1) + " c";
    };
    osc1UnisonVoicesSlider.setNumDecimalPlacesToDisplay (0);
    osc2UnisonVoicesSlider.setNumDecimalPlacesToDisplay (0);
    delayTimeSlider.setSkewFactorFromMidPoint (350.0f);
    compRatioSlider.setSkewFactorFromMidPoint (4.0f);
    compAttackSlider.setSkewFactorFromMidPoint (10.0f);
    compReleaseSlider.setSkewFactorFromMidPoint (100.0f);

    auto& apvts = audioProcessor.getApvts();
    osc2MixAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc2Mix, osc2MixSlider);
    osc2DetuneAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc2Detune, osc2DetuneSlider);
    osc1UnisonVoicesAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc1UnisonVoices, osc1UnisonVoicesSlider);
    osc1UnisonDetuneAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc1UnisonDetune, osc1UnisonDetuneSlider);
    osc1UnisonSpreadAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc1UnisonSpread, osc1UnisonSpreadSlider);
    osc1UnisonBlendAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc1UnisonBlend, osc1UnisonBlendSlider);
    osc2UnisonVoicesAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc2UnisonVoices, osc2UnisonVoicesSlider);
    osc2UnisonDetuneAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc2UnisonDetune, osc2UnisonDetuneSlider);
    osc2UnisonSpreadAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc2UnisonSpread, osc2UnisonSpreadSlider);
    osc2UnisonBlendAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc2UnisonBlend, osc2UnisonBlendSlider);
    filterCutoffAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramFilterCutoff, filterCutoffSlider);
    filterResonanceAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramFilterResonance, filterResonanceSlider);
    osc2FilterCutoffAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc2FilterCutoff, osc2FilterCutoffSlider);
    osc2FilterResonanceAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc2FilterResonance, osc2FilterResonanceSlider);
    attackAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramAttack, attackSlider);
    decayAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramDecay, decaySlider);
    sustainAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramSustain, sustainSlider);
    releaseAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramRelease, releaseSlider);
    osc2AttackAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc2Attack, osc2AttackSlider);
    osc2DecayAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc2Decay, osc2DecaySlider);
    osc2SustainAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc2Sustain, osc2SustainSlider);
    osc2ReleaseAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramOsc2Release, osc2ReleaseSlider);
    glideAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramGlide, glideSlider);
    masterAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramMasterGain, masterSlider);
    trackingAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramTrackingSensitivity, trackingSlider);
    gateAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramGateThreshold, gateSlider);
    lfo1RateAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramLfo1Rate, lfo1RateSlider);
    lfo1FilterAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramLfo1Filter, lfo1FilterSlider);
    lfo1ResonanceAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramLfo1Resonance, lfo1ResonanceSlider);
    lfo1PitchAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramLfo1Pitch, lfo1PitchSlider);
    lfo1AmpAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramLfo1Amp, lfo1AmpSlider);
    lfo2RateAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramLfo2Rate, lfo2RateSlider);
    lfo2FilterAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramLfo2Filter, lfo2FilterSlider);
    lfo2ResonanceAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramLfo2Resonance, lfo2ResonanceSlider);
    lfo2PitchAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramLfo2Pitch, lfo2PitchSlider);
    lfo2AmpAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramLfo2Amp, lfo2AmpSlider);
    filterEnv1AmountAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramFilterEnv1Amount, filterEnv1AmountSlider);
    filterEnv2AmountAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramFilterEnv2Amount, filterEnv2AmountSlider);
    lfo1EnableAttachment = std::make_unique<ButtonAttachment> (apvts, GuitarSynthAudioProcessor::paramLfo1Enabled, lfo1EnableButton);
    lfo2EnableAttachment = std::make_unique<ButtonAttachment> (apvts, GuitarSynthAudioProcessor::paramLfo2Enabled, lfo2EnableButton);
    adsrSyncAttachment = std::make_unique<ButtonAttachment> (apvts, GuitarSynthAudioProcessor::paramAdsrSync, adsrSyncButton);
    filterEnvSyncAttachment = std::make_unique<ButtonAttachment> (apvts, GuitarSynthAudioProcessor::paramFilterEnvSync, filterEnvSyncButton);

    distDriveAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramDistDrive, distDriveSlider);
    distToneAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramDistTone, distToneSlider);
    distMixAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramDistMix, distMixSlider);
    compThresholdAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramCompThreshold, compThresholdSlider);
    compRatioAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramCompRatio, compRatioSlider);
    compAttackAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramCompAttack, compAttackSlider);
    compReleaseAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramCompRelease, compReleaseSlider);
    compMakeupAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramCompMakeup, compMakeupSlider);
    compMixAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramCompMix, compMixSlider);
    delayTimeAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramDelayTime, delayTimeSlider);
    delayFeedbackAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramDelayFeedback, delayFeedbackSlider);
    delayDampingAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramDelayDamping, delayDampingSlider);
    delayMixAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramDelayMix, delayMixSlider);
    reverbSizeAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramReverbSize, reverbSizeSlider);
    reverbDampingAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramReverbDamping, reverbDampingSlider);
    reverbWidthAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramReverbWidth, reverbWidthSlider);
    reverbMixAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramReverbMix, reverbMixSlider);
    delayPingPongAttachment = std::make_unique<ButtonAttachment> (apvts, GuitarSynthAudioProcessor::paramDelayPingPong, delayPingPongButton);

    fxRack = std::make_unique<FxRackComponent> (apvts, lookAndFeel);
    fxRack->onSelectionChanged = [this]
    {
        syncFxDetailVisibility();
        if (fxPage != nullptr)
            fxPage->resized();
    };
    fxPage->addAndMakeVisible (*fxRack);

    filterEnv1Plot = std::make_unique<AdsrPlotEditor> (apvts,
                                                       GuitarSynthAudioProcessor::paramFilterEnv1Attack,
                                                       GuitarSynthAudioProcessor::paramFilterEnv1Decay,
                                                       GuitarSynthAudioProcessor::paramFilterEnv1Sustain,
                                                       GuitarSynthAudioProcessor::paramFilterEnv1Release);
    filterEnv2Plot = std::make_unique<AdsrPlotEditor> (apvts,
                                                       GuitarSynthAudioProcessor::paramFilterEnv2Attack,
                                                       GuitarSynthAudioProcessor::paramFilterEnv2Decay,
                                                       GuitarSynthAudioProcessor::paramFilterEnv2Sustain,
                                                       GuitarSynthAudioProcessor::paramFilterEnv2Release);
    filterEnv1Plot->setAccentColour (lookAndFeel.accentColour());
    filterEnv2Plot->setAccentColour (lookAndFeel.accentColour());
    filterEnvPage->addAndMakeVisible (*filterEnv1Plot);
    filterEnvPage->addAndMakeVisible (*filterEnv2Plot);

    tabs.setOutline (0);
    tabs.getTabbedButtonBar().setColour (juce::TabbedButtonBar::tabOutlineColourId, juce::Colours::transparentBlack);
    tabs.getTabbedButtonBar().setColour (juce::TabbedButtonBar::frontOutlineColourId, lookAndFeel.accentColour());
    tabs.addTab ("Synth", lookAndFeel.panelColour(), synthPage.get(), false);
    tabs.addTab ("Filter Env", lookAndFeel.panelColour(), filterEnvPage.get(), false);
    tabs.addTab ("LFO", lookAndFeel.panelColour(), lfoPage.get(), false);
    tabs.addTab ("FX", lookAndFeel.panelColour(), fxPage.get(), false);
    tabs.setColour (juce::TabbedComponent::backgroundColourId, juce::Colours::transparentBlack);
    tabs.setColour (juce::TabbedComponent::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (tabs);

    apvts.addParameterListener (GuitarSynthAudioProcessor::paramWaveform, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramOsc2Waveform, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramOsc2Octave, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramLfo1Shape, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramLfo2Shape, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramDistMode, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramAdsrSync, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramAttack, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramDecay, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramSustain, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramRelease, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramOsc2Attack, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramOsc2Decay, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramOsc2Sustain, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramOsc2Release, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramFilterEnvSync, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramFilterEnv1Attack, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramFilterEnv1Decay, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramFilterEnv1Sustain, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramFilterEnv1Release, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramFilterEnv1Amount, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramFilterEnv2Attack, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramFilterEnv2Decay, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramFilterEnv2Sustain, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramFilterEnv2Release, this);
    apvts.addParameterListener (GuitarSynthAudioProcessor::paramFilterEnv2Amount, this);

    syncWaveformButtons();
    syncOsc2WaveformButtons();
    syncOctaveButtons();
    syncLfoShapeButtons (1);
    syncLfoShapeButtons (2);
    syncDistModeButtons();
    syncFxDetailVisibility();
    syncPresetCombo();

    setSize (980, 860);
    startTimerHz (30);
}

GuitarSynthAudioProcessorEditor::~GuitarSynthAudioProcessorEditor()
{
    auto& apvts = audioProcessor.getApvts();
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramWaveform, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramOsc2Waveform, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramOsc2Octave, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramLfo1Shape, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramLfo2Shape, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramDistMode, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramAdsrSync, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramAttack, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramDecay, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramSustain, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramRelease, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramOsc2Attack, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramOsc2Decay, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramOsc2Sustain, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramOsc2Release, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramFilterEnvSync, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramFilterEnv1Attack, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramFilterEnv1Decay, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramFilterEnv1Sustain, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramFilterEnv1Release, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramFilterEnv1Amount, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramFilterEnv2Attack, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramFilterEnv2Decay, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramFilterEnv2Sustain, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramFilterEnv2Release, this);
    apvts.removeParameterListener (GuitarSynthAudioProcessor::paramFilterEnv2Amount, this);
    setLookAndFeel (nullptr);
}

void GuitarSynthAudioProcessorEditor::setupSlider (juce::Component& parent, juce::Slider& slider,
                                                   juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
    parent.addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (12.0f)));
    parent.addAndMakeVisible (label);
}

void GuitarSynthAudioProcessorEditor::setupWaveformButton (juce::Component& parent, juce::ImageButton& button,
                                                           int waveformIndex, int radioGroupId,
                                                           const juce::String& paramId)
{
    const auto off = lookAndFeel.getWaveformButtonImage (waveformIndex, false);
    const auto on = lookAndFeel.getWaveformButtonImage (waveformIndex, true);
    button.setImages (false, true, true,
                      off, 1.0f, juce::Colours::transparentBlack,
                      off, 1.0f, juce::Colours::transparentBlack,
                      on, 1.0f, juce::Colours::transparentBlack);
    button.setClickingTogglesState (true);
    button.setRadioGroupId (radioGroupId);
    button.onClick = [this, waveformIndex, paramId]
    {
        setChoiceParameter (paramId, waveformIndex);
    };
    parent.addAndMakeVisible (button);
}

void GuitarSynthAudioProcessorEditor::setupOctaveButton (juce::Component& parent, juce::TextButton& button,
                                                         int octaveIndex)
{
    styleChoiceButton (button);
    button.setRadioGroupId (1005);
    button.onClick = [this, octaveIndex]
    {
        setChoiceParameter (GuitarSynthAudioProcessor::paramOsc2Octave, octaveIndex);
        syncOctaveButtons();
    };
    parent.addAndMakeVisible (button);
}

void GuitarSynthAudioProcessorEditor::setupLfoShapeButton (juce::Component& parent, juce::TextButton& button,
                                                           int shapeIndex, int radioGroupId,
                                                           const juce::String& paramId)
{
    styleChoiceButton (button);
    button.setRadioGroupId (radioGroupId);
    button.onClick = [this, shapeIndex, paramId]
    {
        setChoiceParameter (paramId, shapeIndex);
        if (paramId == GuitarSynthAudioProcessor::paramLfo1Shape)
            syncLfoShapeButtons (1);
        else
            syncLfoShapeButtons (2);
    };
    parent.addAndMakeVisible (button);
}

void GuitarSynthAudioProcessorEditor::setChoiceParameter (const juce::String& paramId, int index)
{
    if (auto* param = audioProcessor.getApvts().getParameter (paramId))
    {
        const float normalised = param->convertTo0to1 (static_cast<float> (index));
        param->beginChangeGesture();
        param->setValueNotifyingHost (normalised);
        param->endChangeGesture();
    }
}

void GuitarSynthAudioProcessorEditor::syncWaveformButtons()
{
    const int index = juce::roundToInt (audioProcessor.getApvts()
                                            .getRawParameterValue (GuitarSynthAudioProcessor::paramWaveform)
                                            ->load());
    sineButton.setToggleState (index == 0, juce::dontSendNotification);
    sawButton.setToggleState (index == 1, juce::dontSendNotification);
    squareButton.setToggleState (index == 2, juce::dontSendNotification);

    auto apply = [this] (juce::ImageButton& button, int waveformIndex, bool active)
    {
        const auto off = lookAndFeel.getWaveformButtonImage (waveformIndex, false);
        const auto on = lookAndFeel.getWaveformButtonImage (waveformIndex, true);
        const auto& img = active ? on : off;
        button.setImages (false, true, true,
                          img, 1.0f, juce::Colours::transparentBlack,
                          img, 1.0f, juce::Colours::transparentBlack,
                          on, 1.0f, juce::Colours::transparentBlack);
    };
    apply (sineButton, 0, index == 0);
    apply (sawButton, 1, index == 1);
    apply (squareButton, 2, index == 2);
}

void GuitarSynthAudioProcessorEditor::syncOsc2WaveformButtons()
{
    const int index = juce::roundToInt (audioProcessor.getApvts()
                                            .getRawParameterValue (GuitarSynthAudioProcessor::paramOsc2Waveform)
                                            ->load());
    osc2SineButton.setToggleState (index == 0, juce::dontSendNotification);
    osc2SawButton.setToggleState (index == 1, juce::dontSendNotification);
    osc2SquareButton.setToggleState (index == 2, juce::dontSendNotification);

    auto apply = [this] (juce::ImageButton& button, int waveformIndex, bool active)
    {
        const auto off = lookAndFeel.getWaveformButtonImage (waveformIndex, false);
        const auto on = lookAndFeel.getWaveformButtonImage (waveformIndex, true);
        const auto& img = active ? on : off;
        button.setImages (false, true, true,
                          img, 1.0f, juce::Colours::transparentBlack,
                          img, 1.0f, juce::Colours::transparentBlack,
                          on, 1.0f, juce::Colours::transparentBlack);
    };
    apply (osc2SineButton, 0, index == 0);
    apply (osc2SawButton, 1, index == 1);
    apply (osc2SquareButton, 2, index == 2);
}

void GuitarSynthAudioProcessorEditor::syncOctaveButtons()
{
    const int index = juce::roundToInt (audioProcessor.getApvts()
                                            .getRawParameterValue (GuitarSynthAudioProcessor::paramOsc2Octave)
                                            ->load());
    octaveDownButton.setToggleState (index == 0, juce::dontSendNotification);
    octaveZeroButton.setToggleState (index == 1, juce::dontSendNotification);
    octaveUpButton.setToggleState (index == 2, juce::dontSendNotification);
}

void GuitarSynthAudioProcessorEditor::syncLfoShapeButtons (int lfoIndex)
{
    const auto* paramId = (lfoIndex == 1) ? GuitarSynthAudioProcessor::paramLfo1Shape
                                          : GuitarSynthAudioProcessor::paramLfo2Shape;
    const int index = juce::roundToInt (audioProcessor.getApvts().getRawParameterValue (paramId)->load());

    if (lfoIndex == 1)
    {
        lfo1SineButton.setToggleState (index == 0, juce::dontSendNotification);
        lfo1TriButton.setToggleState (index == 1, juce::dontSendNotification);
        lfo1SquareButton.setToggleState (index == 2, juce::dontSendNotification);
        lfo1SawButton.setToggleState (index == 3, juce::dontSendNotification);
    }
    else
    {
        lfo2SineButton.setToggleState (index == 0, juce::dontSendNotification);
        lfo2TriButton.setToggleState (index == 1, juce::dontSendNotification);
        lfo2SquareButton.setToggleState (index == 2, juce::dontSendNotification);
        lfo2SawButton.setToggleState (index == 3, juce::dontSendNotification);
    }
}

void GuitarSynthAudioProcessorEditor::syncDistModeButtons()
{
    const int index = juce::roundToInt (audioProcessor.getApvts()
                                            .getRawParameterValue (GuitarSynthAudioProcessor::paramDistMode)
                                            ->load());
    distSoftButton.setToggleState (index == 0, juce::dontSendNotification);
    distHardButton.setToggleState (index == 1, juce::dontSendNotification);
    distFoldButton.setToggleState (index == 2, juce::dontSendNotification);
}

void GuitarSynthAudioProcessorEditor::syncFxDetailVisibility()
{
    const int type = fxRack != nullptr ? fxRack->getSelectedType() : 0;

    const bool showDist = type == 0;
    const bool showComp = type == 1;
    const bool showDelay = type == 2;
    const bool showReverb = type == 3;

    fxDetailTitleLabel.setText (EffectChain::nameForType (type), juce::dontSendNotification);

    distModeLabel.setVisible (showDist);
    distSoftButton.setVisible (showDist);
    distHardButton.setVisible (showDist);
    distFoldButton.setVisible (showDist);
    distDriveSlider.setVisible (showDist);
    distDriveLabel.setVisible (showDist);
    distToneSlider.setVisible (showDist);
    distToneLabel.setVisible (showDist);
    distMixSlider.setVisible (showDist);
    distMixLabel.setVisible (showDist);

    compThresholdSlider.setVisible (showComp);
    compThresholdLabel.setVisible (showComp);
    compRatioSlider.setVisible (showComp);
    compRatioLabel.setVisible (showComp);
    compAttackSlider.setVisible (showComp);
    compAttackLabel.setVisible (showComp);
    compReleaseSlider.setVisible (showComp);
    compReleaseLabel.setVisible (showComp);
    compMakeupSlider.setVisible (showComp);
    compMakeupLabel.setVisible (showComp);
    compMixSlider.setVisible (showComp);
    compMixLabel.setVisible (showComp);

    delayTimeSlider.setVisible (showDelay);
    delayTimeLabel.setVisible (showDelay);
    delayFeedbackSlider.setVisible (showDelay);
    delayFeedbackLabel.setVisible (showDelay);
    delayDampingSlider.setVisible (showDelay);
    delayDampingLabel.setVisible (showDelay);
    delayMixSlider.setVisible (showDelay);
    delayMixLabel.setVisible (showDelay);
    delayPingPongButton.setVisible (showDelay);

    reverbSizeSlider.setVisible (showReverb);
    reverbSizeLabel.setVisible (showReverb);
    reverbDampingSlider.setVisible (showReverb);
    reverbDampingLabel.setVisible (showReverb);
    reverbWidthSlider.setVisible (showReverb);
    reverbWidthLabel.setVisible (showReverb);
    reverbMixSlider.setVisible (showReverb);
    reverbMixLabel.setVisible (showReverb);
}

void GuitarSynthAudioProcessorEditor::syncPresetCombo()
{
    updatingPresetCombo = true;
    presetCombo.setSelectedItemIndex (audioProcessor.getCurrentProgram(), juce::dontSendNotification);
    updatingPresetCombo = false;
}

void GuitarSynthAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::Component::SafePointer<GuitarSynthAudioProcessorEditor> safeThis (this);
    juce::MessageManager::callAsync ([safeThis, parameterID, newValue]
    {
        if (safeThis == nullptr)
            return;

        if (parameterID == GuitarSynthAudioProcessor::paramWaveform)
            safeThis->syncWaveformButtons();
        else if (parameterID == GuitarSynthAudioProcessor::paramOsc2Waveform)
            safeThis->syncOsc2WaveformButtons();
        else if (parameterID == GuitarSynthAudioProcessor::paramOsc2Octave)
            safeThis->syncOctaveButtons();
        else if (parameterID == GuitarSynthAudioProcessor::paramLfo1Shape)
            safeThis->syncLfoShapeButtons (1);
        else if (parameterID == GuitarSynthAudioProcessor::paramLfo2Shape)
            safeThis->syncLfoShapeButtons (2);
        else if (parameterID == GuitarSynthAudioProcessor::paramDistMode)
            safeThis->syncDistModeButtons();
        else if (parameterID == GuitarSynthAudioProcessor::paramAdsrSync)
        {
            if (newValue > 0.5f)
                safeThis->copyAdsrFromOsc1ToOsc2();
        }
        else if (parameterID == GuitarSynthAudioProcessor::paramAttack
                 || parameterID == GuitarSynthAudioProcessor::paramDecay
                 || parameterID == GuitarSynthAudioProcessor::paramSustain
                 || parameterID == GuitarSynthAudioProcessor::paramRelease
                 || parameterID == GuitarSynthAudioProcessor::paramOsc2Attack
                 || parameterID == GuitarSynthAudioProcessor::paramOsc2Decay
                 || parameterID == GuitarSynthAudioProcessor::paramOsc2Sustain
                 || parameterID == GuitarSynthAudioProcessor::paramOsc2Release)
        {
            safeThis->mirrorAdsrParameters (parameterID);
        }
        else if (parameterID == GuitarSynthAudioProcessor::paramFilterEnvSync)
        {
            if (newValue > 0.5f)
                safeThis->copyFilterEnvFromOsc1ToOsc2();
        }
        else if (parameterID == GuitarSynthAudioProcessor::paramFilterEnv1Attack
                 || parameterID == GuitarSynthAudioProcessor::paramFilterEnv1Decay
                 || parameterID == GuitarSynthAudioProcessor::paramFilterEnv1Sustain
                 || parameterID == GuitarSynthAudioProcessor::paramFilterEnv1Release
                 || parameterID == GuitarSynthAudioProcessor::paramFilterEnv1Amount
                 || parameterID == GuitarSynthAudioProcessor::paramFilterEnv2Attack
                 || parameterID == GuitarSynthAudioProcessor::paramFilterEnv2Decay
                 || parameterID == GuitarSynthAudioProcessor::paramFilterEnv2Sustain
                 || parameterID == GuitarSynthAudioProcessor::paramFilterEnv2Release
                 || parameterID == GuitarSynthAudioProcessor::paramFilterEnv2Amount)
        {
            safeThis->mirrorFilterEnvParameters (parameterID);
        }
    });
}

void GuitarSynthAudioProcessorEditor::copyAdsrFromOsc1ToOsc2()
{
    if (mirroringAdsr)
        return;

    mirroringAdsr = true;
    auto& apvts = audioProcessor.getApvts();

    auto copyParam = [&apvts] (const char* fromId, const char* toId)
    {
        if (auto* from = apvts.getParameter (fromId))
            if (auto* to = apvts.getParameter (toId))
                to->setValueNotifyingHost (from->getValue());
    };

    copyParam (GuitarSynthAudioProcessor::paramAttack, GuitarSynthAudioProcessor::paramOsc2Attack);
    copyParam (GuitarSynthAudioProcessor::paramDecay, GuitarSynthAudioProcessor::paramOsc2Decay);
    copyParam (GuitarSynthAudioProcessor::paramSustain, GuitarSynthAudioProcessor::paramOsc2Sustain);
    copyParam (GuitarSynthAudioProcessor::paramRelease, GuitarSynthAudioProcessor::paramOsc2Release);
    mirroringAdsr = false;
}

void GuitarSynthAudioProcessorEditor::mirrorAdsrParameters (const juce::String& sourceParamId)
{
    if (mirroringAdsr)
        return;

    if (*audioProcessor.getApvts().getRawParameterValue (GuitarSynthAudioProcessor::paramAdsrSync) <= 0.5f)
        return;

    const char* targetId = nullptr;
    if (sourceParamId == GuitarSynthAudioProcessor::paramAttack)
        targetId = GuitarSynthAudioProcessor::paramOsc2Attack;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramDecay)
        targetId = GuitarSynthAudioProcessor::paramOsc2Decay;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramSustain)
        targetId = GuitarSynthAudioProcessor::paramOsc2Sustain;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramRelease)
        targetId = GuitarSynthAudioProcessor::paramOsc2Release;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramOsc2Attack)
        targetId = GuitarSynthAudioProcessor::paramAttack;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramOsc2Decay)
        targetId = GuitarSynthAudioProcessor::paramDecay;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramOsc2Sustain)
        targetId = GuitarSynthAudioProcessor::paramSustain;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramOsc2Release)
        targetId = GuitarSynthAudioProcessor::paramRelease;

    if (targetId == nullptr)
        return;

    auto& apvts = audioProcessor.getApvts();
    auto* source = apvts.getParameter (sourceParamId);
    auto* target = apvts.getParameter (targetId);
    if (source == nullptr || target == nullptr)
        return;

    if (std::abs (source->getValue() - target->getValue()) < 1.0e-6f)
        return;

    mirroringAdsr = true;
    target->setValueNotifyingHost (source->getValue());
    mirroringAdsr = false;
}

void GuitarSynthAudioProcessorEditor::copyFilterEnvFromOsc1ToOsc2()
{
    if (mirroringFilterEnv)
        return;

    mirroringFilterEnv = true;
    auto& apvts = audioProcessor.getApvts();

    auto copyParam = [&apvts] (const char* fromId, const char* toId)
    {
        if (auto* from = apvts.getParameter (fromId))
            if (auto* to = apvts.getParameter (toId))
                to->setValueNotifyingHost (from->getValue());
    };

    copyParam (GuitarSynthAudioProcessor::paramFilterEnv1Attack, GuitarSynthAudioProcessor::paramFilterEnv2Attack);
    copyParam (GuitarSynthAudioProcessor::paramFilterEnv1Decay, GuitarSynthAudioProcessor::paramFilterEnv2Decay);
    copyParam (GuitarSynthAudioProcessor::paramFilterEnv1Sustain, GuitarSynthAudioProcessor::paramFilterEnv2Sustain);
    copyParam (GuitarSynthAudioProcessor::paramFilterEnv1Release, GuitarSynthAudioProcessor::paramFilterEnv2Release);
    copyParam (GuitarSynthAudioProcessor::paramFilterEnv1Amount, GuitarSynthAudioProcessor::paramFilterEnv2Amount);
    mirroringFilterEnv = false;
}

void GuitarSynthAudioProcessorEditor::mirrorFilterEnvParameters (const juce::String& sourceParamId)
{
    if (mirroringFilterEnv)
        return;

    if (*audioProcessor.getApvts().getRawParameterValue (GuitarSynthAudioProcessor::paramFilterEnvSync) <= 0.5f)
        return;

    const char* targetId = nullptr;
    if (sourceParamId == GuitarSynthAudioProcessor::paramFilterEnv1Attack)
        targetId = GuitarSynthAudioProcessor::paramFilterEnv2Attack;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramFilterEnv1Decay)
        targetId = GuitarSynthAudioProcessor::paramFilterEnv2Decay;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramFilterEnv1Sustain)
        targetId = GuitarSynthAudioProcessor::paramFilterEnv2Sustain;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramFilterEnv1Release)
        targetId = GuitarSynthAudioProcessor::paramFilterEnv2Release;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramFilterEnv1Amount)
        targetId = GuitarSynthAudioProcessor::paramFilterEnv2Amount;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramFilterEnv2Attack)
        targetId = GuitarSynthAudioProcessor::paramFilterEnv1Attack;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramFilterEnv2Decay)
        targetId = GuitarSynthAudioProcessor::paramFilterEnv1Decay;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramFilterEnv2Sustain)
        targetId = GuitarSynthAudioProcessor::paramFilterEnv1Sustain;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramFilterEnv2Release)
        targetId = GuitarSynthAudioProcessor::paramFilterEnv1Release;
    else if (sourceParamId == GuitarSynthAudioProcessor::paramFilterEnv2Amount)
        targetId = GuitarSynthAudioProcessor::paramFilterEnv1Amount;

    if (targetId == nullptr)
        return;

    auto& apvts = audioProcessor.getApvts();
    auto* source = apvts.getParameter (sourceParamId);
    auto* target = apvts.getParameter (targetId);
    if (source == nullptr || target == nullptr)
        return;

    if (std::abs (source->getValue() - target->getValue()) < 1.0e-6f)
        return;

    mirroringFilterEnv = true;
    target->setValueNotifyingHost (source->getValue());
    mirroringFilterEnv = false;
}

void GuitarSynthAudioProcessorEditor::placeKnob (juce::Rectangle<int> area, juce::Slider& slider, juce::Label& label)
{
    auto labelArea = area.removeFromBottom (20);
    slider.setBounds (area.reduced (4));
    label.setBounds (labelArea);
}

juce::String GuitarSynthAudioProcessorEditor::frequencyToNoteName (float frequency)
{
    if (frequency <= 0.0f)
        return "---";

    const float midi = 69.0f + 12.0f * std::log2 (frequency / 440.0f);
    const int note = juce::roundToInt (midi);
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int nameIndex = ((note % 12) + 12) % 12;
    const int octave = note / 12 - 1;
    return juce::String (names[nameIndex]) + juce::String (octave);
}

void GuitarSynthAudioProcessorEditor::paintSectionPanel (juce::Graphics& g,
                                                         juce::Rectangle<int> bounds,
                                                         const juce::String& title)
{
    juce::ignoreUnused (title);
    g.setColour (lookAndFeel.panelColour().withAlpha (0.92f));
    g.fillRoundedRectangle (bounds.toFloat(), 12.0f);
    g.setColour (lookAndFeel.accentColour().withAlpha (0.28f));
    g.drawRoundedRectangle (bounds.toFloat(), 12.0f, 1.4f);
}

void GuitarSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto& bg = lookAndFeel.getPanelBackground();
    if (! bg.isNull())
        g.drawImage (bg, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit, false);
    else
        g.fillAll (lookAndFeel.backgroundColour());

    paintSectionPanel (g, headerBounds, {});
    paintSectionPanel (g, displayBounds, {});
    paintSectionPanel (g, meterBounds, {});

    auto lcd = displayBounds.reduced (10).toFloat();
    g.setColour (lookAndFeel.lcdColour());
    g.fillRoundedRectangle (lcd, 10.0f);
    g.setColour (juce::Colour (0xff4cd964).withAlpha (0.25f));
    g.drawRoundedRectangle (lcd, 10.0f, 1.2f);
}

void GuitarSynthAudioProcessorEditor::layoutSynthPage()
{
    auto bounds = synthPage->getLocalBounds().reduced (8);
    if (bounds.getWidth() < 32 || bounds.getHeight() < 32)
        return;

    // Taller top row to fit unison knob rows under each oscillator section.
    const int topHeight = juce::jlimit (300, 380, bounds.getHeight() * 45 / 100);
    auto topRow = bounds.removeFromTop (topHeight);
    oscPanelBounds = topRow.removeFromLeft (juce::jmax (300, topRow.getWidth() * 2 / 5));
    topRow.removeFromLeft (10);
    waveformBounds = topRow.removeFromLeft (juce::jmax (200, topRow.getWidth() / 2));
    topRow.removeFromLeft (10);
    playPanelBounds = topRow;

    {
        auto panel = oscPanelBounds.reduced (8);
        oscSectionLabel.setBounds (panel.removeFromTop (18));
        panel.removeFromTop (2);

        osc2WaveformLabel.setBounds (panel.removeFromTop (14));
        auto osc2Wave = panel.removeFromTop (juce::jmin (36, panel.getHeight() / 6));
        const int btnW = osc2Wave.getWidth() / 3;
        osc2SineButton.setBounds (osc2Wave.removeFromLeft (btnW).reduced (3));
        osc2SawButton.setBounds (osc2Wave.removeFromLeft (btnW).reduced (3));
        osc2SquareButton.setBounds (osc2Wave.reduced (3));

        panel.removeFromTop (2);
        octaveLabel.setBounds (panel.removeFromTop (14));
        auto octaveRow = panel.removeFromTop (28);
        const int octW = octaveRow.getWidth() / 3;
        octaveDownButton.setBounds (octaveRow.removeFromLeft (octW).reduced (2));
        octaveZeroButton.setBounds (octaveRow.removeFromLeft (octW).reduced (2));
        octaveUpButton.setBounds (octaveRow.reduced (2));

        panel.removeFromTop (4);
        auto mixRow = panel.removeFromTop (juce::jmax (100, panel.getHeight() / 3));
        const int mixKnobW = mixRow.getWidth() / 2;
        placeKnob (mixRow.removeFromLeft (mixKnobW), osc2MixSlider, osc2MixLabel);
        placeKnob (mixRow, osc2DetuneSlider, osc2DetuneLabel);

        panel.removeFromTop (2);
        osc2UnisonLabel.setBounds (panel.removeFromTop (14));
        const int uniW = panel.getWidth() / 4;
        placeKnob (panel.removeFromLeft (uniW), osc2UnisonVoicesSlider, osc2UnisonVoicesLabel);
        placeKnob (panel.removeFromLeft (uniW), osc2UnisonDetuneSlider, osc2UnisonDetuneLabel);
        placeKnob (panel.removeFromLeft (uniW), osc2UnisonSpreadSlider, osc2UnisonSpreadLabel);
        placeKnob (panel, osc2UnisonBlendSlider, osc2UnisonBlendLabel);
    }

    {
        auto wave = waveformBounds.reduced (8);
        osc1WaveformLabel.setBounds (wave.removeFromTop (18));
        wave.removeFromTop (4);
        auto waveRow = wave.removeFromTop (juce::jmin (44, wave.getHeight() / 4));
        const int btnW = waveRow.getWidth() / 3;
        sineButton.setBounds (waveRow.removeFromLeft (btnW).reduced (3));
        sawButton.setBounds (waveRow.removeFromLeft (btnW).reduced (3));
        squareButton.setBounds (waveRow.reduced (3));

        wave.removeFromTop (8);
        osc1UnisonLabel.setBounds (wave.removeFromTop (16));
        const int uniW = wave.getWidth() / 4;
        placeKnob (wave.removeFromLeft (uniW), osc1UnisonVoicesSlider, osc1UnisonVoicesLabel);
        placeKnob (wave.removeFromLeft (uniW), osc1UnisonDetuneSlider, osc1UnisonDetuneLabel);
        placeKnob (wave.removeFromLeft (uniW), osc1UnisonSpreadSlider, osc1UnisonSpreadLabel);
        placeKnob (wave, osc1UnisonBlendSlider, osc1UnisonBlendLabel);
    }

    {
        auto panel = playPanelBounds.reduced (8);
        playSectionLabel.setBounds (panel.removeFromTop (20));
        panel.removeFromTop (4);
        const int knobW = panel.getWidth() / 4;
        placeKnob (panel.removeFromLeft (knobW), glideSlider, glideLabel);
        placeKnob (panel.removeFromLeft (knobW), masterSlider, masterLabel);
        placeKnob (panel.removeFromLeft (knobW), trackingSlider, trackingLabel);
        placeKnob (panel, gateSlider, gateLabel);
    }

    bounds.removeFromTop (10);
    const int panelGap = 10;
    filterPanelBounds = bounds.removeFromLeft ((bounds.getWidth() - panelGap) / 2);
    bounds.removeFromLeft (panelGap);
    envelopePanelBounds = bounds;

    {
        auto panel = filterPanelBounds.reduced (10);
        filterSectionLabel.setBounds (panel.removeFromTop (20));
        panel.removeFromTop (2);

        auto left = panel.removeFromLeft (panel.getWidth() / 2).reduced (2);
        auto right = panel.reduced (2);

        osc1FilterLabel.setBounds (left.removeFromTop (16));
        const int leftKnobW = left.getWidth() / 2;
        placeKnob (left.removeFromLeft (leftKnobW), filterCutoffSlider, filterCutoffLabel);
        placeKnob (left, filterResonanceSlider, filterResonanceLabel);

        osc2FilterLabel.setBounds (right.removeFromTop (16));
        const int rightKnobW = right.getWidth() / 2;
        placeKnob (right.removeFromLeft (rightKnobW), osc2FilterCutoffSlider, osc2FilterCutoffLabel);
        placeKnob (right, osc2FilterResonanceSlider, osc2FilterResonanceLabel);
    }

    {
        auto panel = envelopePanelBounds.reduced (10);
        auto titleRow = panel.removeFromTop (22);
        envelopeSectionLabel.setBounds (titleRow.removeFromLeft (titleRow.getWidth() - 72));
        adsrSyncButton.setBounds (titleRow.reduced (2, 0));
        panel.removeFromTop (2);

        auto top = panel.removeFromTop (panel.getHeight() / 2).reduced (0, 2);
        auto bottom = panel.reduced (0, 2);

        osc1EnvelopeLabel.setBounds (top.removeFromTop (16));
        const int knW = top.getWidth() / 4;
        placeKnob (top.removeFromLeft (knW), attackSlider, attackLabel);
        placeKnob (top.removeFromLeft (knW), decaySlider, decayLabel);
        placeKnob (top.removeFromLeft (knW), sustainSlider, sustainLabel);
        placeKnob (top, releaseSlider, releaseLabel);

        osc2EnvelopeLabel.setBounds (bottom.removeFromTop (16));
        const int knW2 = bottom.getWidth() / 4;
        placeKnob (bottom.removeFromLeft (knW2), osc2AttackSlider, osc2AttackLabel);
        placeKnob (bottom.removeFromLeft (knW2), osc2DecaySlider, osc2DecayLabel);
        placeKnob (bottom.removeFromLeft (knW2), osc2SustainSlider, osc2SustainLabel);
        placeKnob (bottom, osc2ReleaseSlider, osc2ReleaseLabel);
    }
}

void GuitarSynthAudioProcessorEditor::layoutLfoPage()
{
    auto bounds = lfoPage->getLocalBounds().reduced (12);
    if (bounds.getWidth() < 32 || bounds.getHeight() < 32)
        return;
    const int gap = 12;
    lfo1PanelBounds = bounds.removeFromLeft ((bounds.getWidth() - gap) / 2);
    bounds.removeFromLeft (gap);
    lfo2PanelBounds = bounds;

    auto layoutLfoColumn = [this] (juce::Rectangle<int> col,
                                   juce::Label& sectionLabel,
                                   juce::TextButton& enableBtn,
                                   juce::TextButton& sinBtn, juce::TextButton& triBtn,
                                   juce::TextButton& sqrBtn, juce::TextButton& sawBtn,
                                   juce::Slider& rate, juce::Label& rateLbl,
                                   juce::Slider& cutoff, juce::Label& cutoffLbl,
                                   juce::Slider& res, juce::Label& resLbl,
                                   juce::Slider& pitch, juce::Label& pitchLbl,
                                   juce::Slider& amp, juce::Label& ampLbl)
    {
        auto panel = col.reduced (10);
        auto titleRow = panel.removeFromTop (26);
        sectionLabel.setBounds (titleRow.removeFromLeft (titleRow.getWidth() - 56));
        enableBtn.setBounds (titleRow.reduced (2, 1));
        panel.removeFromTop (6);

        auto shapeRow = panel.removeFromTop (32);
        const int sw = shapeRow.getWidth() / 4;
        sinBtn.setBounds (shapeRow.removeFromLeft (sw).reduced (2));
        triBtn.setBounds (shapeRow.removeFromLeft (sw).reduced (2));
        sqrBtn.setBounds (shapeRow.removeFromLeft (sw).reduced (2));
        sawBtn.setBounds (shapeRow.reduced (2));

        panel.removeFromTop (16);
        auto topKnobs = panel.removeFromTop (juce::jmax (120, panel.getHeight() / 2));
        const int knobW = topKnobs.getWidth() / 3;
        placeKnob (topKnobs.removeFromLeft (knobW), rate, rateLbl);
        placeKnob (topKnobs.removeFromLeft (knobW), cutoff, cutoffLbl);
        placeKnob (topKnobs, res, resLbl);

        panel.removeFromTop (8);
        const int bottomW = panel.getWidth() / 2;
        placeKnob (panel.removeFromLeft (bottomW), pitch, pitchLbl);
        placeKnob (panel, amp, ampLbl);
    };

    layoutLfoColumn (lfo1PanelBounds, lfo1SectionLabel, lfo1EnableButton,
                     lfo1SineButton, lfo1TriButton, lfo1SquareButton, lfo1SawButton,
                     lfo1RateSlider, lfo1RateLabel,
                     lfo1FilterSlider, lfo1FilterLabel,
                     lfo1ResonanceSlider, lfo1ResonanceLabel,
                     lfo1PitchSlider, lfo1PitchLabel,
                     lfo1AmpSlider, lfo1AmpLabel);

    layoutLfoColumn (lfo2PanelBounds, lfo2SectionLabel, lfo2EnableButton,
                     lfo2SineButton, lfo2TriButton, lfo2SquareButton, lfo2SawButton,
                     lfo2RateSlider, lfo2RateLabel,
                     lfo2FilterSlider, lfo2FilterLabel,
                     lfo2ResonanceSlider, lfo2ResonanceLabel,
                     lfo2PitchSlider, lfo2PitchLabel,
                     lfo2AmpSlider, lfo2AmpLabel);
}

void GuitarSynthAudioProcessorEditor::layoutFilterEnvPage()
{
    auto bounds = filterEnvPage->getLocalBounds().reduced (12);
    if (bounds.getWidth() < 32 || bounds.getHeight() < 32)
        return;

    const int gap = 12;
    filterEnv1PanelBounds = bounds.removeFromLeft ((bounds.getWidth() - gap) / 2);
    bounds.removeFromLeft (gap);
    filterEnv2PanelBounds = bounds;

    auto layoutEnvColumn = [this] (juce::Rectangle<int> col,
                                   juce::Label& sectionLabel,
                                   bool showSync,
                                   AdsrPlotEditor* plot,
                                   juce::Slider& amount, juce::Label& amountLbl)
    {
        auto panel = col.reduced (10);
        auto titleRow = panel.removeFromTop (26);
        if (showSync)
        {
            sectionLabel.setBounds (titleRow.removeFromLeft (titleRow.getWidth() - 72));
            filterEnvSyncButton.setBounds (titleRow.reduced (2, 1));
        }
        else
        {
            sectionLabel.setBounds (titleRow);
        }

        panel.removeFromTop (8);
        auto knobArea = panel.removeFromBottom (juce::jmax (120, panel.getHeight() / 4));
        placeKnob (knobArea.withSizeKeepingCentre (juce::jmin (140, knobArea.getWidth()), knobArea.getHeight()),
                   amount, amountLbl);

        panel.removeFromBottom (8);
        if (plot != nullptr)
            plot->setBounds (panel);
    };

    layoutEnvColumn (filterEnv1PanelBounds, filterEnv1SectionLabel, true,
                     filterEnv1Plot.get(), filterEnv1AmountSlider, filterEnv1AmountLabel);
    layoutEnvColumn (filterEnv2PanelBounds, filterEnv2SectionLabel, false,
                     filterEnv2Plot.get(), filterEnv2AmountSlider, filterEnv2AmountLabel);
}

void GuitarSynthAudioProcessorEditor::layoutFxPage()
{
    auto bounds = fxPage->getLocalBounds().reduced (12);
    if (bounds.getWidth() < 32 || bounds.getHeight() < 32)
        return;

    const int gap = 12;
    fxRackBounds = bounds.removeFromTop (110);
    bounds.removeFromTop (gap);
    fxDetailBounds = bounds;

    if (fxRack != nullptr)
        fxRack->setBounds (fxRackBounds.reduced (10).withTrimmedTop (22));

    auto detail = fxDetailBounds.reduced (12).withTrimmedTop (8);
    fxDetailTitleLabel.setBounds (detail.removeFromTop (24));
    detail.removeFromTop (8);

    const int type = fxRack != nullptr ? fxRack->getSelectedType() : 0;

    if (type == 0)
    {
        auto modeRow = detail.removeFromTop (32);
        distModeLabel.setBounds (modeRow.removeFromLeft (48));
        const int bw = modeRow.getWidth() / 3;
        distSoftButton.setBounds (modeRow.removeFromLeft (bw).reduced (2));
        distHardButton.setBounds (modeRow.removeFromLeft (bw).reduced (2));
        distFoldButton.setBounds (modeRow.reduced (2));
        detail.removeFromTop (12);
        const int kw = detail.getWidth() / 3;
        placeKnob (detail.removeFromLeft (kw), distDriveSlider, distDriveLabel);
        placeKnob (detail.removeFromLeft (kw), distToneSlider, distToneLabel);
        placeKnob (detail, distMixSlider, distMixLabel);
    }
    else if (type == 1)
    {
        auto top = detail.removeFromTop (juce::jmax (130, detail.getHeight() / 2));
        const int kw = top.getWidth() / 3;
        placeKnob (top.removeFromLeft (kw), compThresholdSlider, compThresholdLabel);
        placeKnob (top.removeFromLeft (kw), compRatioSlider, compRatioLabel);
        placeKnob (top, compAttackSlider, compAttackLabel);
        detail.removeFromTop (8);
        const int bw = detail.getWidth() / 3;
        placeKnob (detail.removeFromLeft (bw), compReleaseSlider, compReleaseLabel);
        placeKnob (detail.removeFromLeft (bw), compMakeupSlider, compMakeupLabel);
        placeKnob (detail, compMixSlider, compMixLabel);
    }
    else if (type == 2)
    {
        auto top = detail.removeFromTop (juce::jmax (130, detail.getHeight() * 2 / 3));
        const int kw = top.getWidth() / 4;
        placeKnob (top.removeFromLeft (kw), delayTimeSlider, delayTimeLabel);
        placeKnob (top.removeFromLeft (kw), delayFeedbackSlider, delayFeedbackLabel);
        placeKnob (top.removeFromLeft (kw), delayDampingSlider, delayDampingLabel);
        placeKnob (top, delayMixSlider, delayMixLabel);
        detail.removeFromTop (8);
        delayPingPongButton.setBounds (detail.removeFromTop (32).withSizeKeepingCentre (120, 28));
    }
    else
    {
        const int kw = detail.getWidth() / 4;
        placeKnob (detail.removeFromLeft (kw), reverbSizeSlider, reverbSizeLabel);
        placeKnob (detail.removeFromLeft (kw), reverbDampingSlider, reverbDampingLabel);
        placeKnob (detail.removeFromLeft (kw), reverbWidthSlider, reverbWidthLabel);
        placeKnob (detail, reverbMixSlider, reverbMixLabel);
    }
}

void GuitarSynthAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16);

    headerBounds = bounds.removeFromTop (56);
    {
        auto header = headerBounds.reduced (12, 8);
        titleLabel.setBounds (header.removeFromLeft (220));
        latencyLabel.setBounds (header.removeFromRight (150));
        voicedIndicator->setBounds (header.removeFromRight (40).reduced (4));
        header.removeFromRight (8);
        presetCombo.setBounds (header.removeFromRight (180).reduced (0, 2));
        presetLabel.setBounds (header.removeFromRight (56));
    }

    bounds.removeFromTop (8);
    auto monitor = bounds.removeFromTop (120);
    displayBounds = monitor.removeFromLeft (juce::jmax (240, monitor.getWidth() * 2 / 5));
    monitor.removeFromLeft (8);
    meterBounds = monitor;

    {
        auto display = displayBounds.reduced (12);
        noteLabel.setBounds (display.removeFromTop (48));
        pitchLabel.setBounds (display.removeFromTop (32));
        confidenceLabel.setBounds (display.removeFromTop (16));
        confidenceMeter->setBounds (display.removeFromTop (12).reduced (0, 1));
    }

    {
        auto meters = meterBounds.reduced (10);
        inputLevelLabel.setBounds (meters.removeFromTop (16));
        inputMeter->setBounds (meters.removeFromTop (14).reduced (0, 1));
        meters.removeFromTop (6);
        gateLevelLabel.setBounds (meters.removeFromTop (16));
        gateMeter->setBounds (meters.removeFromTop (14).reduced (0, 1));
        meters.removeFromTop (6);
        inputHintLabel.setBounds (meters);
    }

    bounds.removeFromTop (8);
    tabs.setBounds (bounds);
}

void GuitarSynthAudioProcessorEditor::timerCallback()
{
    const float frequency = audioProcessor.getDisplayedFrequency();
    const float confidence = audioProcessor.getDisplayedConfidence();
    const bool voiced = audioProcessor.getDisplayedVoiced();
    const float inputPeak = audioProcessor.getDisplayedInputPeak();
    const bool gateOpen = audioProcessor.getDisplayedGateOpen();
    const float gateEnvelopeDb = audioProcessor.getDisplayedGateEnvelopeDb();

    pitchLabel.setText (voiced && frequency > 0.0f
                            ? juce::String (frequency, 1) + " Hz"
                            : juce::String ("--- Hz"),
                        juce::dontSendNotification);
    noteLabel.setText (frequencyToNoteName (frequency), juce::dontSendNotification);
    confidenceLabel.setText ("Confidence  " + juce::String (juce::roundToInt (confidence * 100.0f)) + "%",
                             juce::dontSendNotification);
    confidenceMeter->setLevel (confidence);

    gateLevelLabel.setText ("Gate  " + juce::String (gateOpen ? "open" : "closed")
                            + "  (" + juce::String (gateEnvelopeDb, 1) + " dB)",
                            juce::dontSendNotification);
    gateLevelLabel.setColour (juce::Label::textColourId,
                              gateOpen ? juce::Colour (0xff4cd964) : lookAndFeel.textColour());

    const float gateLevel = juce::jmap (juce::jlimit (-80.0f, 0.0f, gateEnvelopeDb), -80.0f, 0.0f, 0.0f, 1.0f);
    gateMeter->setLevel (gateLevel);

    if (inputPeak > 1.0e-6f)
    {
        const float inputDb = 20.0f * std::log10 (inputPeak);
        inputLevelLabel.setText ("Input  " + juce::String (inputDb, 1) + " dBFS ("
                                 + juce::String (audioProcessor.getConfiguredInputChannels()) + " ch)",
                                 juce::dontSendNotification);
        inputMeter->setLevel (juce::jmap (juce::jlimit (-60.0f, 0.0f, inputDb), -60.0f, 0.0f, 0.0f, 1.0f));

        if (gateOpen && ! voiced)
            inputHintLabel.setText ("Gate open on noise — raise Gate (try -40 dB or higher)",
                                    juce::dontSendNotification);
        else
            inputHintLabel.setText ("", juce::dontSendNotification);
    }
    else if (audioProcessor.getConfiguredInputChannels() == 0)
    {
        inputLevelLabel.setText ("Input  no channels", juce::dontSendNotification);
        inputMeter->setLevel (0.0f);
        inputHintLabel.setText ("Allow Microphone in System Settings, then restart",
                                juce::dontSendNotification);
    }
    else
    {
        inputLevelLabel.setText ("Input  silent (" + juce::String (audioProcessor.getConfiguredInputChannels()) + " ch)",
                                 juce::dontSendNotification);
        inputMeter->setLevel (0.0f);
        inputHintLabel.setText ("No signal — check Audio Settings device/channels and interface gain",
                                juce::dontSendNotification);
    }

    latencyLabel.setText ("Latency: " + juce::String (audioProcessor.getDisplayedLatencyMs(), 1) + " ms",
                          juce::dontSendNotification);

    voicedIndicator->setActive (voiced);
    syncPresetCombo();
}
