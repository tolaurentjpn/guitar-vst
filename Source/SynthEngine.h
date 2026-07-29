#pragma once

#include <JuceHeader.h>
#include "Lfo.h"
#include <array>
#include <random>

enum class WaveformType
{
    sine = 0,
    saw,
    square,
    triangle
};

class SynthEngine
{
public:
    static constexpr int maxUnison = 8;

    void prepare (double sampleRate, int maximumBlockSize);
    void reset();

    void setWaveform (WaveformType type);
    void setOsc2Waveform (WaveformType type);
    void setOsc2Mix (float mix);
    void setOsc2Octave (int octave);
    void setOsc2DetuneCents (float cents);

    void setOsc1PulseWidth (float width01);
    void setOsc2PulseWidth (float width01);
    void setSubLevel (float level01);
    void setNoiseMix (float mix01);

    void setOsc1UnisonVoices (int voices);
    void setOsc1UnisonDetune (float amount01);
    void setOsc1UnisonSpread (float amount01);
    void setOsc1UnisonBlend (float amount01);
    void setOsc1PhaseRandom (float amount01);

    void setOsc2UnisonVoices (int voices);
    void setOsc2UnisonDetune (float amount01);
    void setOsc2UnisonSpread (float amount01);
    void setOsc2UnisonBlend (float amount01);
    void setOsc2PhaseRandom (float amount01);

    void setOsc1FilterCutoff (float hz);
    void setOsc1FilterResonance (float resonance);
    void setOsc2FilterCutoff (float hz);
    void setOsc2FilterResonance (float resonance);

    void setOsc1AttackMs (float ms);
    void setOsc1DecayMs (float ms);
    void setOsc1SustainLevel (float level);
    void setOsc1ReleaseMs (float ms);

    void setOsc2AttackMs (float ms);
    void setOsc2DecayMs (float ms);
    void setOsc2SustainLevel (float level);
    void setOsc2ReleaseMs (float ms);

    void setFilterEnv1AttackMs (float ms);
    void setFilterEnv1DecayMs (float ms);
    void setFilterEnv1SustainLevel (float level);
    void setFilterEnv1ReleaseMs (float ms);
    void setFilterEnv1Amount (float amount);

    void setFilterEnv2AttackMs (float ms);
    void setFilterEnv2DecayMs (float ms);
    void setFilterEnv2SustainLevel (float level);
    void setFilterEnv2ReleaseMs (float ms);
    void setFilterEnv2Amount (float amount);

    void setGlideMs (float ms);
    void setMasterGain (float gain);

    void setLfo1Enabled (bool enabled);
    void setLfo1Rate (float hz);
    void setLfo1Shape (LfoShape shape);
    void setLfo1FilterAmount (float amount);
    void setLfo1ResonanceAmount (float amount);
    void setLfo1PitchAmount (float amount);
    void setLfo1AmpAmount (float amount);
    void setLfo1PwmAmount (float amount);

    void setLfo2Enabled (bool enabled);
    void setLfo2Rate (float hz);
    void setLfo2Shape (LfoShape shape);
    void setLfo2FilterAmount (float amount);
    void setLfo2ResonanceAmount (float amount);
    void setLfo2PitchAmount (float amount);
    void setLfo2AmpAmount (float amount);
    void setLfo2PwmAmount (float amount);

    void setPitchState (float hz, bool trackingActive);
    /** Re-attack amp/filter envelopes and reset phases while a note is already sounding. */
    void retrigger() noexcept;
    void muteImmediately() noexcept;
    bool isIdle() const noexcept;
    void processSample (float& left, float& right) noexcept;
    void processBlock (juce::AudioBuffer<float>& buffer, const float* gateEnvelope, int numSamples);

private:
    enum class EnvStage { idle, attack, decay, sustain, release };

    struct AmpEnvelope
    {
        float attackCoeff = 0.0f;
        float decayCoeff = 0.0f;
        float releaseCoeff = 0.0f;
        float sustainLevel = 0.7f;
        EnvStage stage = EnvStage::idle;
        float value = 0.0f;

        void hardReset() noexcept
        {
            stage = EnvStage::idle;
            value = 0.0f;
        }

        void advance() noexcept;
    };

    /** Phase in [0, 1); triState holds leaky-integrator memory for triangle. */
    struct PhaseOsc
    {
        float phase = 0.0f;
        float triState = 0.0f;
    };

    void hardMute() noexcept;
    void renderSample (float osc1BaseFreq, float osc2BaseFreq,
                       float osc1AmpScale, float osc2AmpScale,
                       float osc1Pulse, float osc2Pulse,
                       float& left, float& right) noexcept;
    void resetUnisonPhases (bool randomizeOsc1, bool randomizeOsc2);
    void resetOscStack (std::array<PhaseOsc, maxUnison>& stack, float phaseRandom01);
    float msToCoeff (float ms) const;
    float clampCutoff (float hz) const noexcept;
    void updateNoiseFilterCoeff() noexcept;
    static float voiceDetuneCents (int voiceIndex, int numVoices, float maxDetuneCents) noexcept;
    static float voicePan (int voiceIndex, int numVoices, float spread01) noexcept;
    static float voiceBlendGain (int voiceIndex, int numVoices, float blend01) noexcept;
    static float polyBlep (float t, float dt) noexcept;
    static float renderSquareBlep (float phase01, float dt, float pulseWidth) noexcept;
    static float renderWave (WaveformType type, float phase01, float dt,
                             float pulseWidth, float& triState) noexcept;
    static float modulatedPulseWidth (float baseWidth, float lfoAmount, float lfoValue) noexcept;

    double sampleRate = 44100.0;
    WaveformType waveform = WaveformType::saw;
    WaveformType osc2Waveform = WaveformType::saw;

    std::array<PhaseOsc, maxUnison> osc1Stack {};
    std::array<PhaseOsc, maxUnison> osc2Stack {};
    juce::dsp::StateVariableTPTFilter<float> filter1;
    juce::dsp::StateVariableTPTFilter<float> filter2;
    juce::dsp::ProcessSpec spec {};

    Lfo lfo1;
    Lfo lfo2;

    float currentFrequency = 440.0f;
    float targetFrequency = 440.0f;
    float glideCoeff = 0.0f;

    float osc2Mix = 0.35f;
    int osc2Octave = 0;
    float osc2DetuneCents = 0.0f;

    float osc1PulseWidth = 0.5f;
    float osc2PulseWidth = 0.5f;
    float subLevel = 0.0f;
    float noiseMix = 0.0f;
    float subPhase = 0.0f;
    float noiseLpState = 0.0f;
    float noiseLpCoeff = 0.0f;

    int osc1UnisonVoices = 1;
    float osc1UnisonDetune = 0.35f;
    float osc1UnisonSpread = 0.0f;
    float osc1UnisonBlend = 0.8f;
    float osc1PhaseRandom = 1.0f;

    int osc2UnisonVoices = 1;
    float osc2UnisonDetune = 0.35f;
    float osc2UnisonSpread = 0.0f;
    float osc2UnisonBlend = 0.8f;
    float osc2PhaseRandom = 1.0f;

    float osc1CutoffHz = 2200.0f;
    float osc1Resonance = 0.85f;
    float osc2CutoffHz = 2200.0f;
    float osc2Resonance = 0.85f;

    bool lfo1Enabled = true;
    bool lfo2Enabled = true;
    float lfo1FilterAmount = 0.0f;
    float lfo1ResonanceAmount = 0.0f;
    float lfo1PitchAmount = 0.0f;
    float lfo1AmpAmount = 0.0f;
    float lfo1PwmAmount = 0.0f;
    float lfo2FilterAmount = 0.0f;
    float lfo2ResonanceAmount = 0.0f;
    float lfo2PitchAmount = 0.0f;
    float lfo2AmpAmount = 0.0f;
    float lfo2PwmAmount = 0.0f;

    AmpEnvelope env1;
    AmpEnvelope env2;
    AmpEnvelope filterEnv1;
    AmpEnvelope filterEnv2;
    float filterEnv1Amount = 0.0f;
    float filterEnv2Amount = 0.0f;

    float masterGain = 0.8f;
    bool activeVoiced = false;

    std::mt19937 rng { std::random_device{}() };
    std::uniform_real_distribution<float> noiseDist { -1.0f, 1.0f };
};
