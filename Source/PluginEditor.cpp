#include "PluginEditor.h"

namespace
{
    class VoicedLed : public juce::Component
    {
    public:
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
            const auto bounds = getLocalBounds().toFloat().reduced (2.0f);
            g.setColour (active ? juce::Colour (0xff4cd964) : juce::Colour (0xff3a3f47));
            g.fillEllipse (bounds);
            g.setColour (juce::Colours::white.withAlpha (0.15f));
            g.drawEllipse (bounds, 1.0f);
        }

    private:
        bool active = false;
    };
}

GuitarSynthAudioProcessorEditor::GuitarSynthAudioProcessorEditor (GuitarSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      voicedIndicator (std::make_unique<VoicedLed>())
{
    setLookAndFeel (&lookAndFeel);
    setSize (820, 520);
    setResizable (true, true);
    setResizeLimits (640, 420, 1200, 800);

    titleLabel.setText ("GUITAR SYNTH", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    pitchLabel.setText ("--- Hz", juce::dontSendNotification);
    pitchLabel.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold)));
    pitchLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (pitchLabel);

    noteLabel.setText ("---", juce::dontSendNotification);
    noteLabel.setFont (juce::Font (juce::FontOptions (18.0f)));
    noteLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (noteLabel);

    confidenceLabel.setText ("Confidence: 0%", juce::dontSendNotification);
    confidenceLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (confidenceLabel);

    inputLevelLabel.setText ("Input: -inf dB", juce::dontSendNotification);
    inputLevelLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (inputLevelLabel);

    gateLevelLabel.setText ("Gate: closed", juce::dontSendNotification);
    gateLevelLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (gateLevelLabel);

    inputHintLabel.setText ("", juce::dontSendNotification);
    inputHintLabel.setJustificationType (juce::Justification::centredLeft);
    inputHintLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffcc66));
    addAndMakeVisible (inputHintLabel);

    latencyLabel.setText ("Latency: -- ms", juce::dontSendNotification);
    latencyLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (latencyLabel);

    addAndMakeVisible (*voicedIndicator);

    waveformBox.addItem ("Sine", 1);
    waveformBox.addItem ("Saw", 2);
    waveformBox.addItem ("Square", 3);
    addAndMakeVisible (waveformBox);
    waveformLabel.setText ("Waveform", juce::dontSendNotification);
    waveformLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (waveformLabel);

    setupSlider (filterCutoffSlider, filterCutoffLabel, "Cutoff");
    setupSlider (filterResonanceSlider, filterResonanceLabel, "Resonance");
    setupSlider (attackSlider, attackLabel, "Attack");
    setupSlider (decaySlider, decayLabel, "Decay");
    setupSlider (sustainSlider, sustainLabel, "Sustain");
    setupSlider (releaseSlider, releaseLabel, "Release");
    setupSlider (glideSlider, glideLabel, "Glide");
    setupSlider (masterSlider, masterLabel, "Master");
    setupSlider (trackingSlider, trackingLabel, "Tracking");
    setupSlider (gateSlider, gateLabel, "Gate");

    filterCutoffSlider.setSkewFactorFromMidPoint (800.0f);
    gateSlider.textFromValueFunction = [] (double value)
    {
        return juce::String (value, 1) + " dB";
    };

    auto& apvts = audioProcessor.getApvts();
    waveformAttachment = std::make_unique<ComboAttachment> (apvts, GuitarSynthAudioProcessor::paramWaveform, waveformBox);
    filterCutoffAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramFilterCutoff, filterCutoffSlider);
    filterResonanceAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramFilterResonance, filterResonanceSlider);
    attackAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramAttack, attackSlider);
    decayAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramDecay, decaySlider);
    sustainAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramSustain, sustainSlider);
    releaseAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramRelease, releaseSlider);
    glideAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramGlide, glideSlider);
    masterAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramMasterGain, masterSlider);
    trackingAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramTrackingSensitivity, trackingSlider);
    gateAttachment = std::make_unique<SliderAttachment> (apvts, GuitarSynthAudioProcessor::paramGateThreshold, gateSlider);

    startTimerHz (30);
}

GuitarSynthAudioProcessorEditor::~GuitarSynthAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void GuitarSynthAudioProcessorEditor::setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
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

void GuitarSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (lookAndFeel.panelColour().darker (0.35f));

    auto header = getLocalBounds().removeFromTop (72).reduced (16, 10);
    g.setColour (lookAndFeel.panelColour());
    g.fillRoundedRectangle (header.toFloat(), 10.0f);
    g.setColour (lookAndFeel.accentColour().withAlpha (0.35f));
    g.drawRoundedRectangle (header.toFloat(), 10.0f, 1.5f);

    auto display = juce::Rectangle<int> (240, 92, 340, 130);
    g.setColour (lookAndFeel.panelColour().brighter (0.05f));
    g.fillRoundedRectangle (display.toFloat(), 12.0f);
    g.setColour (lookAndFeel.accentColour());
    g.drawRoundedRectangle (display.toFloat(), 12.0f, 1.5f);
}

void GuitarSynthAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16);

    auto header = bounds.removeFromTop (72);
    titleLabel.setBounds (header.removeFromLeft (260));
    latencyLabel.setBounds (header.removeFromRight (180));
    voicedIndicator->setBounds (header.removeFromRight (28).reduced (4));

    bounds.removeFromTop (12);
    auto display = bounds.removeFromTop (130);
    pitchLabel.setBounds (display.withTrimmedLeft (120).withTrimmedRight (120).removeFromTop (42));
    noteLabel.setBounds (display.withTrimmedLeft (180).withTrimmedRight (180).removeFromTop (72));
    inputLevelLabel.setBounds (display.removeFromLeft (160).removeFromBottom (28));
    gateLevelLabel.setBounds (display.removeFromRight (160).removeFromBottom (28));
    confidenceLabel.setBounds (display.removeFromBottom (28));
    inputHintLabel.setBounds (bounds.removeFromTop (22));

    bounds.removeFromTop (8);
    auto knobRow1 = bounds.removeFromTop (150);
    const int knobWidth = knobRow1.getWidth() / 5;
    auto placeKnob = [] (juce::Rectangle<int> area, juce::Slider& slider, juce::Label& label)
    {
        auto labelArea = area.removeFromBottom (22);
        slider.setBounds (area.reduced (4));
        label.setBounds (labelArea);
    };

    placeKnob (knobRow1.removeFromLeft (knobWidth), filterCutoffSlider, filterCutoffLabel);
    placeKnob (knobRow1.removeFromLeft (knobWidth), filterResonanceSlider, filterResonanceLabel);
    placeKnob (knobRow1.removeFromLeft (knobWidth), attackSlider, attackLabel);
    placeKnob (knobRow1.removeFromLeft (knobWidth), decaySlider, decayLabel);
    placeKnob (knobRow1, sustainSlider, sustainLabel);

    bounds.removeFromTop (4);
    auto knobRow2 = bounds.removeFromTop (150);
    const int knobWidth2 = knobRow2.getWidth() / 6;
    placeKnob (knobRow2.removeFromLeft (knobWidth2), releaseSlider, releaseLabel);
    placeKnob (knobRow2.removeFromLeft (knobWidth2), glideSlider, glideLabel);
    placeKnob (knobRow2.removeFromLeft (knobWidth2), masterSlider, masterLabel);
    placeKnob (knobRow2.removeFromLeft (knobWidth2), trackingSlider, trackingLabel);
    placeKnob (knobRow2.removeFromLeft (knobWidth2), gateSlider, gateLabel);

    auto waveformArea = knobRow2.reduced (8);
    waveformLabel.setBounds (waveformArea.removeFromTop (22));
    waveformBox.setBounds (waveformArea.reduced (8, 4));
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
    confidenceLabel.setText ("Confidence: " + juce::String (juce::roundToInt (confidence * 100.0f)) + "%",
                             juce::dontSendNotification);
    gateLevelLabel.setText ("Gate: " + juce::String (gateOpen ? "open" : "closed")
                            + " (" + juce::String (gateEnvelopeDb, 1) + " dB)",
                            juce::dontSendNotification);
    gateLevelLabel.setColour (juce::Label::textColourId,
                              gateOpen ? juce::Colour (0xff4cd964) : juce::Colours::lightgrey);

    if (inputPeak > 1.0e-6f)
    {
        const float inputDb = 20.0f * std::log10 (inputPeak);
        inputLevelLabel.setText ("Input: " + juce::String (inputDb, 1) + " dBFS ("
                                 + juce::String (audioProcessor.getConfiguredInputChannels()) + " ch)",
                                 juce::dontSendNotification);
        if (gateOpen && ! voiced)
            inputHintLabel.setText ("Gate is open on input noise — raise the Gate knob (try -40 dB or higher)",
                                    juce::dontSendNotification);
        else
            inputHintLabel.setText ("", juce::dontSendNotification);
    }
    else if (audioProcessor.getConfiguredInputChannels() == 0)
    {
        inputLevelLabel.setText ("Input: no channels", juce::dontSendNotification);
        inputHintLabel.setText ("macOS blocked audio input — allow Guitar Synth under System Settings → Privacy → Microphone, then restart",
                                juce::dontSendNotification);
    }
    else
    {
        inputLevelLabel.setText ("Input: silent (" + juce::String (audioProcessor.getConfiguredInputChannels()) + " ch)",
                                 juce::dontSendNotification);
        inputHintLabel.setText ("No signal on active inputs — check Audio Settings device/channels and interface gain",
                                juce::dontSendNotification);
    }
    latencyLabel.setText ("Latency: " + juce::String (audioProcessor.getDisplayedLatencyMs(), 1) + " ms",
                          juce::dontSendNotification);

    if (auto* led = dynamic_cast<VoicedLed*> (voicedIndicator.get()))
        led->setActive (voiced);
}
