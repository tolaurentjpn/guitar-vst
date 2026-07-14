#pragma once

#include <JuceHeader.h>

enum class WaveformType
{
    sine = 0,
    saw,
    square
};

class SynthEngine
{
public:
    void prepare (double sampleRate, int maximumBlockSize);
    void reset();

    void setWaveform (WaveformType type);
    void setFilterCutoff (float hz);
    void setFilterResonance (float resonance);
    void setAttackMs (float ms);
    void setDecayMs (float ms);
    void setSustainLevel (float level);
    void setReleaseMs (float ms);
    void setGlideMs (float ms);
    void setMasterGain (float gain);

    void setPitchState (float hz, bool trackingActive);
    void muteImmediately() noexcept;
    float processSample() noexcept;
    void processBlock (juce::AudioBuffer<float>& buffer, const float* gateEnvelope, int numSamples);

private:
    void hardMute() noexcept;
    float renderSample() noexcept;
    void updateFilter();
    float msToCoeff (float ms) const;

    double sampleRate = 44100.0;
    WaveformType waveform = WaveformType::saw;

    juce::dsp::Oscillator<float> oscillator;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::dsp::ProcessSpec spec {};

    float currentFrequency = 440.0f;
    float targetFrequency = 440.0f;
    float glideCoeff = 0.0f;

    float attackCoeff = 0.0f;
    float decayCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float sustainLevel = 0.7f;

    enum class EnvStage { idle, attack, decay, sustain, release };
    EnvStage envStage = EnvStage::idle;
    float envelope = 0.0f;
    float masterGain = 0.8f;
    float lastFilterCutoff = -1.0f;
    float lastFilterResonance = -1.0f;

    bool activeVoiced = false;
};
