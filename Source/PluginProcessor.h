#pragma once

#include <JuceHeader.h>
#include <vector>
#include "PitchTracker.h"
#include "EnvelopeFollower.h"
#include "SynthEngine.h"

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
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getApvts() { return apvts; }

    float getDisplayedFrequency() const noexcept { return displayedFrequency.load(); }
    float getDisplayedConfidence() const noexcept { return displayedConfidence.load(); }
    bool getDisplayedVoiced() const noexcept { return displayedVoiced.load(); }
    double getDisplayedLatencyMs() const noexcept { return displayedLatencyMs.load(); }

    float getDisplayedInputPeak() const noexcept { return displayedInputPeak.load(); }
    float getDisplayedInputPeakCh0() const noexcept { return displayedInputPeakCh0.load(); }
    float getDisplayedInputPeakCh1() const noexcept { return displayedInputPeakCh1.load(); }
    float getDisplayedOutputPeak() const noexcept { return displayedOutputPeak.load(); }
    float getDisplayedOutputRms() const noexcept { return displayedOutputRms.load(); }
    float getDisplayedGateEnvelopeDb() const noexcept { return displayedGateEnvelopeDb.load(); }
    bool getDisplayedGateOpen() const noexcept { return displayedGateOpen.load(); }
    int getConfiguredInputChannels() const noexcept { return getTotalNumInputChannels(); }
    int getConfiguredOutputChannels() const noexcept { return getTotalNumOutputChannels(); }
    juce::String getBusLayoutDescription() const;

    /** Plays diagnostics: sine, then forced synth, through the real output path. */
    void requestOutputTestTone (double seconds) noexcept;
    bool isOutputTestToneActive() const noexcept;
    bool isForcedSynthTestActive() const noexcept;

    static juce::String getParameterId (const char* id) { return juce::String (id); }

    static constexpr const char* paramWaveform = "waveform";
    static constexpr const char* paramFilterCutoff = "filterCutoff";
    static constexpr const char* paramFilterResonance = "filterResonance";
    static constexpr const char* paramAttack = "attack";
    static constexpr const char* paramDecay = "decay";
    static constexpr const char* paramSustain = "sustain";
    static constexpr const char* paramRelease = "release";
    static constexpr const char* paramGlide = "glide";
    static constexpr const char* paramMasterGain = "masterGain";
    static constexpr const char* paramTrackingSensitivity = "trackingSensitivity";
    static constexpr const char* paramGateThreshold = "gateThreshold";

private:
    void updateRealtimeParameters();

    juce::AudioProcessorValueTreeState apvts;

    PitchTracker pitchTracker;
    EnvelopeFollower envelopeFollower;
    SynthEngine synthEngine;

    juce::dsp::IIR::Filter<float> gateHighPassFilter;
    juce::dsp::ProcessSpec spec {};

    float latchedPitchHz = 0.0f;
    bool gateWasOpen = false;
    int gateOpenSampleCount = 0;
    int gateClosedSampleCount = 0;
    int preRollWriteIndex = 0;
    float pitchLevelEstimate = 0.0f;
    int64_t totalSamplesProcessed = 0;
    int64_t testToneUntilSample = 0;
    int64_t testSynthUntilSample = 0;
    double testTonePhase = 0.0;

    std::vector<float> inputPreRoll;

    std::atomic<float> displayedFrequency { 0.0f };
    std::atomic<float> displayedConfidence { 0.0f };
    std::atomic<bool> displayedVoiced { false };
    std::atomic<float> displayedInputPeak { 0.0f };
    std::atomic<float> displayedInputPeakCh0 { 0.0f };
    std::atomic<float> displayedInputPeakCh1 { 0.0f };
    std::atomic<float> displayedOutputPeak { 0.0f };
    std::atomic<float> displayedOutputRms { 0.0f };
    std::atomic<float> displayedGateEnvelopeDb { -100.0f };
    std::atomic<bool> displayedGateOpen { false };
    std::atomic<double> displayedLatencyMs { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarSynthAudioProcessor)
};
