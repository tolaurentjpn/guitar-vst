#pragma once

#include <JuceHeader.h>
#include "PitchTracker.h"
#include "EnvelopeFollower.h"
#include "SynthEngine.h"
#include "ChorusEffect.h"
#include "Arpeggiator.h"
#include "EffectChain.h"

class GuitarSynthAudioProcessor : public juce::AudioProcessor
{
public:
    GuitarSynthAudioProcessor();
    ~GuitarSynthAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getApvts() { return apvts; }

    float getDisplayedFrequency() const noexcept { return displayedFrequency.load(); }
    float getDisplayedConfidence() const noexcept { return displayedConfidence.load(); }
    bool getDisplayedVoiced() const noexcept { return displayedVoiced.load(); }
    double getDisplayedLatencyMs() const noexcept { return displayedLatencyMs.load(); }

    float getDisplayedInputPeak() const noexcept { return displayedInputPeak.load(); }
    float getDisplayedGateEnvelopeDb() const noexcept { return displayedGateEnvelopeDb.load(); }
    bool getDisplayedGateOpen() const noexcept { return displayedGateOpen.load(); }
    int getConfiguredInputChannels() const noexcept { return getTotalNumInputChannels(); }

    static juce::String getParameterId (const char* id) { return juce::String (id); }

    static constexpr const char* paramWaveform = "waveform";
    static constexpr const char* paramOsc2Waveform = "osc2Waveform";
    static constexpr const char* paramOsc2Mix = "osc2Mix";
    static constexpr const char* paramOsc2Octave = "osc2Octave";
    static constexpr const char* paramOsc2Detune = "osc2Detune";
    static constexpr const char* paramOsc1PulseWidth = "osc1PulseWidth";
    static constexpr const char* paramOsc2PulseWidth = "osc2PulseWidth";
    static constexpr const char* paramSubLevel = "subLevel";
    static constexpr const char* paramNoiseMix = "noiseMix";

    static constexpr const char* paramOsc1UnisonVoices = "osc1UnisonVoices";
    static constexpr const char* paramOsc1UnisonDetune = "osc1UnisonDetune";
    static constexpr const char* paramOsc1UnisonSpread = "osc1UnisonSpread";
    static constexpr const char* paramOsc1UnisonBlend = "osc1UnisonBlend";
    static constexpr const char* paramOsc1PhaseRandom = "osc1PhaseRandom";
    static constexpr const char* paramOsc2UnisonVoices = "osc2UnisonVoices";
    static constexpr const char* paramOsc2UnisonDetune = "osc2UnisonDetune";
    static constexpr const char* paramOsc2UnisonSpread = "osc2UnisonSpread";
    static constexpr const char* paramOsc2UnisonBlend = "osc2UnisonBlend";
    static constexpr const char* paramOsc2PhaseRandom = "osc2PhaseRandom";

    static constexpr const char* paramFilterCutoff = "filterCutoff";
    static constexpr const char* paramFilterResonance = "filterResonance";
    static constexpr const char* paramOsc2FilterCutoff = "osc2FilterCutoff";
    static constexpr const char* paramOsc2FilterResonance = "osc2FilterResonance";

    static constexpr const char* paramAttack = "attack";
    static constexpr const char* paramDecay = "decay";
    static constexpr const char* paramSustain = "sustain";
    static constexpr const char* paramRelease = "release";
    static constexpr const char* paramOsc2Attack = "osc2Attack";
    static constexpr const char* paramOsc2Decay = "osc2Decay";
    static constexpr const char* paramOsc2Sustain = "osc2Sustain";
    static constexpr const char* paramOsc2Release = "osc2Release";

    static constexpr const char* paramFilterEnv1Attack = "filterEnv1Attack";
    static constexpr const char* paramFilterEnv1Decay = "filterEnv1Decay";
    static constexpr const char* paramFilterEnv1Sustain = "filterEnv1Sustain";
    static constexpr const char* paramFilterEnv1Release = "filterEnv1Release";
    static constexpr const char* paramFilterEnv1Amount = "filterEnv1Amount";
    static constexpr const char* paramFilterEnv2Attack = "filterEnv2Attack";
    static constexpr const char* paramFilterEnv2Decay = "filterEnv2Decay";
    static constexpr const char* paramFilterEnv2Sustain = "filterEnv2Sustain";
    static constexpr const char* paramFilterEnv2Release = "filterEnv2Release";
    static constexpr const char* paramFilterEnv2Amount = "filterEnv2Amount";
    static constexpr const char* paramFilterEnvSync = "filterEnvSync";

    static constexpr const char* paramGlide = "glide";
    static constexpr const char* paramMasterGain = "masterGain";
    static constexpr const char* paramTrackingSensitivity = "trackingSensitivity";
    static constexpr const char* paramGateThreshold = "gateThreshold";
    static constexpr const char* paramRetriggerSensitivity = "retriggerSensitivity";
    static constexpr const char* paramAdsrSync = "adsrSync";
    static constexpr const char* paramLfo1Enabled = "lfo1Enabled";
    static constexpr const char* paramLfo1Rate = "lfo1Rate";
    static constexpr const char* paramLfo1Shape = "lfo1Shape";
    static constexpr const char* paramLfo1Filter = "lfo1Filter";
    static constexpr const char* paramLfo1Resonance = "lfo1Resonance";
    static constexpr const char* paramLfo1Pitch = "lfo1Pitch";
    static constexpr const char* paramLfo1Amp = "lfo1Amp";
    static constexpr const char* paramLfo1Pwm = "lfo1Pwm";
    static constexpr const char* paramLfo2Enabled = "lfo2Enabled";
    static constexpr const char* paramLfo2Rate = "lfo2Rate";
    static constexpr const char* paramLfo2Shape = "lfo2Shape";
    static constexpr const char* paramLfo2Filter = "lfo2Filter";
    static constexpr const char* paramLfo2Resonance = "lfo2Resonance";
    static constexpr const char* paramLfo2Pitch = "lfo2Pitch";
    static constexpr const char* paramLfo2Amp = "lfo2Amp";
    static constexpr const char* paramLfo2Pwm = "lfo2Pwm";

    static constexpr const char* paramChorusEnabled = "chorusEnabled";
    static constexpr const char* paramChorusRate = "chorusRate";
    static constexpr const char* paramChorusDepth = "chorusDepth";
    static constexpr const char* paramChorusMix = "chorusMix";

    static constexpr const char* paramArpEnabled = "arpEnabled";
    static constexpr const char* paramArpSync = "arpSync";
    static constexpr const char* paramArpRate = "arpRate";
    static constexpr const char* paramArpDivision = "arpDivision";
    static constexpr const char* paramArpGate = "arpGate";
    static constexpr const char* paramArpMode = "arpMode";
    static constexpr const char* paramArpOctaves = "arpOctaves";
    static constexpr const char* paramArpChord = "arpChord";
    static constexpr const char* paramArpLatch = "arpLatch";

    static constexpr const char* paramDistEnabled = "distEnabled";
    static constexpr const char* paramDistMode = "distMode";
    static constexpr const char* paramDistDrive = "distDrive";
    static constexpr const char* paramDistTone = "distTone";
    static constexpr const char* paramDistMix = "distMix";

    static constexpr const char* paramCompEnabled = "compEnabled";
    static constexpr const char* paramCompThreshold = "compThreshold";
    static constexpr const char* paramCompRatio = "compRatio";
    static constexpr const char* paramCompAttack = "compAttack";
    static constexpr const char* paramCompRelease = "compRelease";
    static constexpr const char* paramCompMakeup = "compMakeup";
    static constexpr const char* paramCompMix = "compMix";

    static constexpr const char* paramDelayEnabled = "delayEnabled";
    static constexpr const char* paramDelayTime = "delayTime";
    static constexpr const char* paramDelayFeedback = "delayFeedback";
    static constexpr const char* paramDelayDamping = "delayDamping";
    static constexpr const char* paramDelayMix = "delayMix";
    static constexpr const char* paramDelayPingPong = "delayPingPong";

    static constexpr const char* paramReverbEnabled = "reverbEnabled";
    static constexpr const char* paramReverbSize = "reverbSize";
    static constexpr const char* paramReverbDamping = "reverbDamping";
    static constexpr const char* paramReverbWidth = "reverbWidth";
    static constexpr const char* paramReverbMix = "reverbMix";

    static constexpr const char* paramFxOrder0 = "fxOrder0";
    static constexpr const char* paramFxOrder1 = "fxOrder1";
    static constexpr const char* paramFxOrder2 = "fxOrder2";
    static constexpr const char* paramFxOrder3 = "fxOrder3";

    EffectChain& getEffectChain() noexcept { return effectChain; }

private:
    void updateRealtimeParameters();

    juce::AudioProcessorValueTreeState apvts;

    PitchTracker pitchTracker;
    EnvelopeFollower envelopeFollower;
    SynthEngine synthEngine;
    ChorusEffect chorusEffect;
    Arpeggiator arpeggiator;
    EffectChain effectChain;

    juce::dsp::IIR::Filter<float> highPassFilter;
    juce::dsp::ProcessSpec spec {};

    int currentProgram = 0;

    std::atomic<float> displayedFrequency { 0.0f };
    std::atomic<float> displayedConfidence { 0.0f };
    std::atomic<bool> displayedVoiced { false };
    std::atomic<float> displayedInputPeak { 0.0f };
    std::atomic<float> displayedGateEnvelopeDb { -100.0f };
    std::atomic<bool> displayedGateOpen { false };
    std::atomic<double> displayedLatencyMs { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarSynthAudioProcessor)
};
