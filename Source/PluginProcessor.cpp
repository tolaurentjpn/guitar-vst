#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PresetBank.h"
#include <vector>

namespace
{
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            GuitarSynthAudioProcessor::paramWaveform, "Osc 1 Waveform",
            juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 1));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            GuitarSynthAudioProcessor::paramOsc2Waveform, "Osc 2 Waveform",
            juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 1));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc2Mix, "Osc 2 Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            GuitarSynthAudioProcessor::paramOsc2Octave, "Osc 2 Octave",
            juce::StringArray { "-1", "0", "+1" }, 1));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc2Detune, "Osc 2 Detune",
            juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, "cents"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc1PulseWidth, "Osc 1 Pulse Width",
            juce::NormalisableRange<float> (0.05f, 0.5f, 0.001f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc2PulseWidth, "Osc 2 Pulse Width",
            juce::NormalisableRange<float> (0.05f, 0.5f, 0.001f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramSubLevel, "Sub Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramNoiseMix, "Noise Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterInt> (
            GuitarSynthAudioProcessor::paramOsc1UnisonVoices, "Osc 1 Voices", 1, 8, 1));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc1UnisonDetune, "Osc 1 Unison Detune",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc1UnisonSpread, "Osc 1 Spread",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc1UnisonBlend, "Osc 1 Blend",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc1PhaseRandom, "Osc 1 Phase Random",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterInt> (
            GuitarSynthAudioProcessor::paramOsc2UnisonVoices, "Osc 2 Voices", 1, 8, 1));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc2UnisonDetune, "Osc 2 Unison Detune",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc2UnisonSpread, "Osc 2 Spread",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc2UnisonBlend, "Osc 2 Blend",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc2PhaseRandom, "Osc 2 Phase Random",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterCutoff, "Osc 1 Cutoff",
            juce::NormalisableRange<float> (80.0f, 8000.0f, 0.01f, 0.35f), 2200.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterResonance, "Osc 1 Resonance",
            juce::NormalisableRange<float> (0.1f, 2.0f, 0.001f, 0.5f), 0.85f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc2FilterCutoff, "Osc 2 Cutoff",
            juce::NormalisableRange<float> (80.0f, 8000.0f, 0.01f, 0.35f), 2200.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc2FilterResonance, "Osc 2 Resonance",
            juce::NormalisableRange<float> (0.1f, 2.0f, 0.001f, 0.5f), 0.85f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramAttack, "Osc 1 Attack",
            juce::NormalisableRange<float> (1.0f, 5000.0f, 0.1f, 0.35f), 8.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramDecay, "Osc 1 Decay",
            juce::NormalisableRange<float> (10.0f, 4000.0f, 0.1f, 0.35f), 180.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramSustain, "Osc 1 Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.75f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramRelease, "Osc 1 Release",
            juce::NormalisableRange<float> (10.0f, 8000.0f, 0.1f, 0.35f), 220.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc2Attack, "Osc 2 Attack",
            juce::NormalisableRange<float> (1.0f, 5000.0f, 0.1f, 0.35f), 8.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc2Decay, "Osc 2 Decay",
            juce::NormalisableRange<float> (10.0f, 4000.0f, 0.1f, 0.35f), 180.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc2Sustain, "Osc 2 Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.75f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramOsc2Release, "Osc 2 Release",
            juce::NormalisableRange<float> (10.0f, 8000.0f, 0.1f, 0.35f), 220.0f, "ms"));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterEnv1Attack, "Filter Env 1 Attack",
            juce::NormalisableRange<float> (1.0f, 5000.0f, 0.1f, 0.35f), 8.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterEnv1Decay, "Filter Env 1 Decay",
            juce::NormalisableRange<float> (10.0f, 4000.0f, 0.1f, 0.35f), 180.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterEnv1Sustain, "Filter Env 1 Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterEnv1Release, "Filter Env 1 Release",
            juce::NormalisableRange<float> (10.0f, 8000.0f, 0.1f, 0.35f), 220.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterEnv1Amount, "Filter Env 1 Amount",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterEnv2Attack, "Filter Env 2 Attack",
            juce::NormalisableRange<float> (1.0f, 5000.0f, 0.1f, 0.35f), 8.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterEnv2Decay, "Filter Env 2 Decay",
            juce::NormalisableRange<float> (10.0f, 4000.0f, 0.1f, 0.35f), 180.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterEnv2Sustain, "Filter Env 2 Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterEnv2Release, "Filter Env 2 Release",
            juce::NormalisableRange<float> (10.0f, 8000.0f, 0.1f, 0.35f), 220.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterEnv2Amount, "Filter Env 2 Amount",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            GuitarSynthAudioProcessor::paramFilterEnvSync, "Filter Env Sync", false));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramGlide, "Glide",
            juce::NormalisableRange<float> (0.0f, 300.0f, 0.1f, 0.45f), 25.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramMasterGain, "Master",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramTrackingSensitivity, "Tracking",
            juce::NormalisableRange<float> (0.1f, 0.99f, 0.001f), 0.55f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramGateThreshold, "Gate",
            juce::NormalisableRange<float> (-80.0f, 0.0f, 0.1f), -48.0f, "dB"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramRetriggerSensitivity, "Retrigger",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            GuitarSynthAudioProcessor::paramAdsrSync, "ADSR Sync", false));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            GuitarSynthAudioProcessor::paramLfo1Enabled, "LFO 1 On", true));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramLfo1Rate, "LFO 1 Rate",
            juce::NormalisableRange<float> (0.05f, 20.0f, 0.001f, 0.35f), 2.0f, "Hz"));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            GuitarSynthAudioProcessor::paramLfo1Shape, "LFO 1 Shape",
            juce::StringArray { "Sine", "Triangle", "Square", "Saw" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramLfo1Filter, "LFO 1 → Osc1 Cutoff",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramLfo1Resonance, "LFO 1 → Osc1 Resonance",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramLfo1Pitch, "LFO 1 → Osc1 Pitch",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramLfo1Amp, "LFO 1 → Osc1 Amp",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramLfo1Pwm, "LFO 1 → Osc1 PWM",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            GuitarSynthAudioProcessor::paramLfo2Enabled, "LFO 2 On", true));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramLfo2Rate, "LFO 2 Rate",
            juce::NormalisableRange<float> (0.05f, 20.0f, 0.001f, 0.35f), 0.5f, "Hz"));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            GuitarSynthAudioProcessor::paramLfo2Shape, "LFO 2 Shape",
            juce::StringArray { "Sine", "Triangle", "Square", "Saw" }, 1));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramLfo2Filter, "LFO 2 → Osc2 Cutoff",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramLfo2Resonance, "LFO 2 → Osc2 Resonance",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramLfo2Pitch, "LFO 2 → Osc2 Pitch",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramLfo2Amp, "LFO 2 → Osc2 Amp",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramLfo2Pwm, "LFO 2 → Osc2 PWM",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            GuitarSynthAudioProcessor::paramChorusEnabled, "Chorus On", false));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramChorusRate, "Chorus Rate",
            juce::NormalisableRange<float> (0.05f, 5.0f, 0.001f, 0.4f), 0.8f, "Hz"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramChorusDepth, "Chorus Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramChorusMix, "Chorus Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.4f));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            GuitarSynthAudioProcessor::paramArpEnabled, "Arp On", false));
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            GuitarSynthAudioProcessor::paramArpSync, "Arp Sync", false));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramArpRate, "Arp Rate",
            juce::NormalisableRange<float> (0.25f, 20.0f, 0.001f, 0.4f), 4.0f, "Hz"));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            GuitarSynthAudioProcessor::paramArpDivision, "Arp Division",
            juce::StringArray { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 1));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramArpGate, "Arp Gate",
            juce::NormalisableRange<float> (5.0f, 100.0f, 0.1f), 50.0f, "%"));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            GuitarSynthAudioProcessor::paramArpMode, "Arp Mode",
            juce::StringArray { "Up", "Down", "UpDown", "Random" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterInt> (
            GuitarSynthAudioProcessor::paramArpOctaves, "Arp Octaves", 1, 4, 1));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            GuitarSynthAudioProcessor::paramArpChord, "Arp Chord",
            juce::StringArray { "Note", "Major", "Minor", "Maj7", "Min7", "Sus2", "Sus4" }, 1));
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            GuitarSynthAudioProcessor::paramArpLatch, "Arp Latch", false));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            GuitarSynthAudioProcessor::paramDistEnabled, "Distortion On", false));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            GuitarSynthAudioProcessor::paramDistMode, "Distortion Mode",
            juce::StringArray { "Soft", "Hard", "Fold" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramDistDrive, "Distortion Drive",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramDistTone, "Distortion Tone",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.65f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramDistMix, "Distortion Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            GuitarSynthAudioProcessor::paramCompEnabled, "Compressor On", false));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramCompThreshold, "Comp Threshold",
            juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -18.0f, "dB"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramCompRatio, "Comp Ratio",
            juce::NormalisableRange<float> (1.0f, 20.0f, 0.01f, 0.4f), 4.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramCompAttack, "Comp Attack",
            juce::NormalisableRange<float> (0.1f, 100.0f, 0.01f, 0.4f), 10.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramCompRelease, "Comp Release",
            juce::NormalisableRange<float> (10.0f, 1000.0f, 0.1f, 0.4f), 100.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramCompMakeup, "Comp Makeup",
            juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 0.0f, "dB"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramCompMix, "Comp Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            GuitarSynthAudioProcessor::paramDelayEnabled, "Delay On", false));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramDelayTime, "Delay Time",
            juce::NormalisableRange<float> (1.0f, 2000.0f, 0.1f, 0.4f), 350.0f, "ms"));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramDelayFeedback, "Delay Feedback",
            juce::NormalisableRange<float> (0.0f, 0.95f, 0.001f), 0.35f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramDelayDamping, "Delay Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramDelayMix, "Delay Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            GuitarSynthAudioProcessor::paramDelayPingPong, "Delay Ping-Pong", false));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            GuitarSynthAudioProcessor::paramReverbEnabled, "Reverb On", false));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramReverbSize, "Reverb Size",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.45f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramReverbDamping, "Reverb Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.4f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramReverbWidth, "Reverb Width",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramReverbMix, "Reverb Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f));

        params.push_back (std::make_unique<juce::AudioParameterInt> (
            GuitarSynthAudioProcessor::paramFxOrder0, "FX Order 0", 0, 3, 0));
        params.push_back (std::make_unique<juce::AudioParameterInt> (
            GuitarSynthAudioProcessor::paramFxOrder1, "FX Order 1", 0, 3, 1));
        params.push_back (std::make_unique<juce::AudioParameterInt> (
            GuitarSynthAudioProcessor::paramFxOrder2, "FX Order 2", 0, 3, 2));
        params.push_back (std::make_unique<juce::AudioParameterInt> (
            GuitarSynthAudioProcessor::paramFxOrder3, "FX Order 3", 0, 3, 3));

        return { params.begin(), params.end() };
    }
}

GuitarSynthAudioProcessor::GuitarSynthAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createLayout())
{
}

GuitarSynthAudioProcessor::~GuitarSynthAudioProcessor() = default;

int GuitarSynthAudioProcessor::getNumPrograms()
{
    return PresetBank::numPresets;
}

int GuitarSynthAudioProcessor::getCurrentProgram()
{
    return currentProgram;
}

void GuitarSynthAudioProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= PresetBank::numPresets)
        return;

    currentProgram = index;
    PresetBank::applyTo (apvts, index);
}

const juce::String GuitarSynthAudioProcessor::getProgramName (int index)
{
    return PresetBank::getName (index);
}

double GuitarSynthAudioProcessor::getTailLengthSeconds() const
{
    return juce::jmax (chorusEffect.getTailLengthSeconds(), effectChain.getTailLengthSeconds());
}

void GuitarSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 1;

    pitchTracker.prepare (sampleRate);
    envelopeFollower.prepare (sampleRate);
    synthEngine.prepare (sampleRate, samplesPerBlock);
    arpeggiator.prepare (sampleRate);

    highPassFilter.prepare (spec);
    highPassFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 80.0);

    juce::dsp::ProcessSpec fxSpec;
    fxSpec.sampleRate = sampleRate;
    fxSpec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    fxSpec.numChannels = 2;
    chorusEffect.prepare (fxSpec);
    effectChain.prepare (fxSpec);

    updateRealtimeParameters();
    setLatencySamples (pitchTracker.getLatencySamples());
    displayedLatencyMs.store (1000.0 * static_cast<double> (pitchTracker.getLatencySamples()) / sampleRate);
}

void GuitarSynthAudioProcessor::releaseResources()
{
    chorusEffect.reset();
    arpeggiator.reset();
    effectChain.reset();
}

bool GuitarSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    const auto& in = layouts.getMainInputChannelSet();
    if (in == juce::AudioChannelSet::disabled())
        return true;

    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo())
        return false;

    if (in == juce::AudioChannelSet::mono())
        return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();

    return out == juce::AudioChannelSet::stereo();
}

void GuitarSynthAudioProcessor::updateRealtimeParameters()
{
    const auto waveformIndex = static_cast<int> (*apvts.getRawParameterValue (paramWaveform));
    synthEngine.setWaveform (static_cast<WaveformType> (juce::jlimit (0, 3, waveformIndex)));

    const auto osc2WaveformIndex = static_cast<int> (*apvts.getRawParameterValue (paramOsc2Waveform));
    synthEngine.setOsc2Waveform (static_cast<WaveformType> (juce::jlimit (0, 3, osc2WaveformIndex)));
    synthEngine.setOsc2Mix (*apvts.getRawParameterValue (paramOsc2Mix));
    synthEngine.setOsc2Octave (static_cast<int> (*apvts.getRawParameterValue (paramOsc2Octave)) - 1);
    synthEngine.setOsc2DetuneCents (*apvts.getRawParameterValue (paramOsc2Detune));
    synthEngine.setOsc1PulseWidth (*apvts.getRawParameterValue (paramOsc1PulseWidth));
    synthEngine.setOsc2PulseWidth (*apvts.getRawParameterValue (paramOsc2PulseWidth));
    synthEngine.setSubLevel (*apvts.getRawParameterValue (paramSubLevel));
    synthEngine.setNoiseMix (*apvts.getRawParameterValue (paramNoiseMix));

    synthEngine.setOsc1UnisonVoices (static_cast<int> (*apvts.getRawParameterValue (paramOsc1UnisonVoices)));
    synthEngine.setOsc1UnisonDetune (*apvts.getRawParameterValue (paramOsc1UnisonDetune));
    synthEngine.setOsc1UnisonSpread (*apvts.getRawParameterValue (paramOsc1UnisonSpread));
    synthEngine.setOsc1UnisonBlend (*apvts.getRawParameterValue (paramOsc1UnisonBlend));
    synthEngine.setOsc1PhaseRandom (*apvts.getRawParameterValue (paramOsc1PhaseRandom));

    synthEngine.setOsc2UnisonVoices (static_cast<int> (*apvts.getRawParameterValue (paramOsc2UnisonVoices)));
    synthEngine.setOsc2UnisonDetune (*apvts.getRawParameterValue (paramOsc2UnisonDetune));
    synthEngine.setOsc2UnisonSpread (*apvts.getRawParameterValue (paramOsc2UnisonSpread));
    synthEngine.setOsc2UnisonBlend (*apvts.getRawParameterValue (paramOsc2UnisonBlend));
    synthEngine.setOsc2PhaseRandom (*apvts.getRawParameterValue (paramOsc2PhaseRandom));

    synthEngine.setOsc1FilterCutoff (*apvts.getRawParameterValue (paramFilterCutoff));
    synthEngine.setOsc1FilterResonance (*apvts.getRawParameterValue (paramFilterResonance));
    synthEngine.setOsc2FilterCutoff (*apvts.getRawParameterValue (paramOsc2FilterCutoff));
    synthEngine.setOsc2FilterResonance (*apvts.getRawParameterValue (paramOsc2FilterResonance));

    synthEngine.setOsc1AttackMs (*apvts.getRawParameterValue (paramAttack));
    synthEngine.setOsc1DecayMs (*apvts.getRawParameterValue (paramDecay));
    synthEngine.setOsc1SustainLevel (*apvts.getRawParameterValue (paramSustain));
    synthEngine.setOsc1ReleaseMs (*apvts.getRawParameterValue (paramRelease));
    synthEngine.setOsc2AttackMs (*apvts.getRawParameterValue (paramOsc2Attack));
    synthEngine.setOsc2DecayMs (*apvts.getRawParameterValue (paramOsc2Decay));
    synthEngine.setOsc2SustainLevel (*apvts.getRawParameterValue (paramOsc2Sustain));
    synthEngine.setOsc2ReleaseMs (*apvts.getRawParameterValue (paramOsc2Release));

    synthEngine.setFilterEnv1AttackMs (*apvts.getRawParameterValue (paramFilterEnv1Attack));
    synthEngine.setFilterEnv1DecayMs (*apvts.getRawParameterValue (paramFilterEnv1Decay));
    synthEngine.setFilterEnv1SustainLevel (*apvts.getRawParameterValue (paramFilterEnv1Sustain));
    synthEngine.setFilterEnv1ReleaseMs (*apvts.getRawParameterValue (paramFilterEnv1Release));
    synthEngine.setFilterEnv1Amount (*apvts.getRawParameterValue (paramFilterEnv1Amount));
    synthEngine.setFilterEnv2AttackMs (*apvts.getRawParameterValue (paramFilterEnv2Attack));
    synthEngine.setFilterEnv2DecayMs (*apvts.getRawParameterValue (paramFilterEnv2Decay));
    synthEngine.setFilterEnv2SustainLevel (*apvts.getRawParameterValue (paramFilterEnv2Sustain));
    synthEngine.setFilterEnv2ReleaseMs (*apvts.getRawParameterValue (paramFilterEnv2Release));
    synthEngine.setFilterEnv2Amount (*apvts.getRawParameterValue (paramFilterEnv2Amount));

    synthEngine.setGlideMs (*apvts.getRawParameterValue (paramGlide));
    synthEngine.setMasterGain (*apvts.getRawParameterValue (paramMasterGain));

    synthEngine.setLfo1Enabled (*apvts.getRawParameterValue (paramLfo1Enabled) > 0.5f);
    synthEngine.setLfo1Rate (*apvts.getRawParameterValue (paramLfo1Rate));
    synthEngine.setLfo1Shape (static_cast<LfoShape> (juce::jlimit (0, 3, static_cast<int> (*apvts.getRawParameterValue (paramLfo1Shape)))));
    synthEngine.setLfo1FilterAmount (*apvts.getRawParameterValue (paramLfo1Filter));
    synthEngine.setLfo1ResonanceAmount (*apvts.getRawParameterValue (paramLfo1Resonance));
    synthEngine.setLfo1PitchAmount (*apvts.getRawParameterValue (paramLfo1Pitch));
    synthEngine.setLfo1AmpAmount (*apvts.getRawParameterValue (paramLfo1Amp));
    synthEngine.setLfo1PwmAmount (*apvts.getRawParameterValue (paramLfo1Pwm));

    synthEngine.setLfo2Enabled (*apvts.getRawParameterValue (paramLfo2Enabled) > 0.5f);
    synthEngine.setLfo2Rate (*apvts.getRawParameterValue (paramLfo2Rate));
    synthEngine.setLfo2Shape (static_cast<LfoShape> (juce::jlimit (0, 3, static_cast<int> (*apvts.getRawParameterValue (paramLfo2Shape)))));
    synthEngine.setLfo2FilterAmount (*apvts.getRawParameterValue (paramLfo2Filter));
    synthEngine.setLfo2ResonanceAmount (*apvts.getRawParameterValue (paramLfo2Resonance));
    synthEngine.setLfo2PitchAmount (*apvts.getRawParameterValue (paramLfo2Pitch));
    synthEngine.setLfo2AmpAmount (*apvts.getRawParameterValue (paramLfo2Amp));
    synthEngine.setLfo2PwmAmount (*apvts.getRawParameterValue (paramLfo2Pwm));

    pitchTracker.setConfidenceThreshold (*apvts.getRawParameterValue (paramTrackingSensitivity));
    pitchTracker.setSmoothing (0.15f + (1.0f - *apvts.getRawParameterValue (paramTrackingSensitivity)) * 0.6f);

    envelopeFollower.setAttackMs (2.0f);
    envelopeFollower.setReleaseMs (180.0f);
    envelopeFollower.setGateThreshold (*apvts.getRawParameterValue (paramGateThreshold));
    envelopeFollower.setRetriggerSensitivity (*apvts.getRawParameterValue (paramRetriggerSensitivity));

    chorusEffect.setBypassed (*apvts.getRawParameterValue (paramChorusEnabled) < 0.5f);
    chorusEffect.setRate (*apvts.getRawParameterValue (paramChorusRate));
    chorusEffect.setDepth (*apvts.getRawParameterValue (paramChorusDepth));
    chorusEffect.setMix (*apvts.getRawParameterValue (paramChorusMix));

    arpeggiator.setEnabled (*apvts.getRawParameterValue (paramArpEnabled) > 0.5f);
    arpeggiator.setSync (*apvts.getRawParameterValue (paramArpSync) > 0.5f);
    arpeggiator.setRate (*apvts.getRawParameterValue (paramArpRate));
    arpeggiator.setDivision (static_cast<int> (*apvts.getRawParameterValue (paramArpDivision)));
    arpeggiator.setGate (*apvts.getRawParameterValue (paramArpGate));
    arpeggiator.setMode (static_cast<int> (*apvts.getRawParameterValue (paramArpMode)));
    arpeggiator.setOctaves (static_cast<int> (*apvts.getRawParameterValue (paramArpOctaves)));
    arpeggiator.setChord (static_cast<int> (*apvts.getRawParameterValue (paramArpChord)));
    arpeggiator.setLatch (*apvts.getRawParameterValue (paramArpLatch) > 0.5f);

    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
            if (auto hostBpm = position->getBpm())
                bpm = *hostBpm;
    arpeggiator.setHostBpm (bpm);

    auto& dist = effectChain.getDistortion();
    dist.setBypassed (*apvts.getRawParameterValue (paramDistEnabled) < 0.5f);
    dist.setMode (static_cast<DistortionMode> (juce::jlimit (0, 2, static_cast<int> (*apvts.getRawParameterValue (paramDistMode)))));
    dist.setDrive (*apvts.getRawParameterValue (paramDistDrive));
    dist.setTone (*apvts.getRawParameterValue (paramDistTone));
    dist.setMix (*apvts.getRawParameterValue (paramDistMix));

    auto& comp = effectChain.getCompressor();
    comp.setBypassed (*apvts.getRawParameterValue (paramCompEnabled) < 0.5f);
    comp.setThresholdDb (*apvts.getRawParameterValue (paramCompThreshold));
    comp.setRatio (*apvts.getRawParameterValue (paramCompRatio));
    comp.setAttackMs (*apvts.getRawParameterValue (paramCompAttack));
    comp.setReleaseMs (*apvts.getRawParameterValue (paramCompRelease));
    comp.setMakeupDb (*apvts.getRawParameterValue (paramCompMakeup));
    comp.setMix (*apvts.getRawParameterValue (paramCompMix));

    auto& delayFx = effectChain.getDelay();
    delayFx.setBypassed (*apvts.getRawParameterValue (paramDelayEnabled) < 0.5f);
    delayFx.setTimeMs (*apvts.getRawParameterValue (paramDelayTime));
    delayFx.setFeedback (*apvts.getRawParameterValue (paramDelayFeedback));
    delayFx.setDamping (*apvts.getRawParameterValue (paramDelayDamping));
    delayFx.setMix (*apvts.getRawParameterValue (paramDelayMix));
    delayFx.setPingPong (*apvts.getRawParameterValue (paramDelayPingPong) > 0.5f);

    auto& reverbFx = effectChain.getReverb();
    reverbFx.setBypassed (*apvts.getRawParameterValue (paramReverbEnabled) < 0.5f);
    reverbFx.setSize (*apvts.getRawParameterValue (paramReverbSize));
    reverbFx.setDamping (*apvts.getRawParameterValue (paramReverbDamping));
    reverbFx.setWidth (*apvts.getRawParameterValue (paramReverbWidth));
    reverbFx.setMix (*apvts.getRawParameterValue (paramReverbMix));

    effectChain.setOrder ({
        static_cast<int> (*apvts.getRawParameterValue (paramFxOrder0)),
        static_cast<int> (*apvts.getRawParameterValue (paramFxOrder1)),
        static_cast<int> (*apvts.getRawParameterValue (paramFxOrder2)),
        static_cast<int> (*apvts.getRawParameterValue (paramFxOrder3))
    });
}

void GuitarSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    updateRealtimeParameters();

    const int numSamples = buffer.getNumSamples();

    if (getTotalNumInputChannels() == 0)
    {
        displayedInputPeak.store (0.0f);
        buffer.clear();
        return;
    }

    std::vector<float> inputSamples (static_cast<size_t> (numSamples));
    const float* inputCh0 = buffer.getReadPointer (0);
    const float* inputCh1 = getTotalNumInputChannels() > 1 ? buffer.getReadPointer (1) : nullptr;

    float inputPeak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = inputCh0[i];
        if (inputCh1 != nullptr && std::abs (inputCh1[i]) > std::abs (sample))
            sample = inputCh1[i];

        inputSamples[static_cast<size_t> (i)] = sample;
        inputPeak = juce::jmax (inputPeak, std::abs (sample));
    }

    displayedInputPeak.store (inputPeak);
    buffer.clear();

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : left;

    bool trackingActive = false;
    bool gateWasOpen = false;
    const bool arpEnabled = *apvts.getRawParameterValue (paramArpEnabled) > 0.5f;
    if (! arpEnabled && arpeggiator.isActive())
        arpeggiator.reset();

    for (int i = 0; i < numSamples; ++i)
    {
        const float hpSample = highPassFilter.processSample (inputSamples[static_cast<size_t> (i)]);
        envelopeFollower.processSample (hpSample);
        const bool gateOpen = envelopeFollower.isGateOpen();

        float trackedHz = 0.0f;
        trackingActive = false;

        if (gateOpen)
        {
            pitchTracker.pushSample (hpSample);

            trackingActive = pitchTracker.isVoiced()
                          && pitchTracker.getConfidence() >= pitchTracker.getMinConfidenceThreshold()
                                     + (1.0f - juce::jlimit (0.0f, 1.0f, envelopeFollower.getEnvelopeLinear() * 8.0f)) * 0.12f;
            trackedHz = pitchTracker.getFrequency();
        }
        else
        {
            if (gateWasOpen)
                pitchTracker.flush();
            else
                pitchTracker.clearVoicing();
        }

        if (arpEnabled)
        {
            const auto arp = arpeggiator.process (trackedHz, trackingActive, gateOpen);

            if (arpeggiator.isActive() || arp.voiced || arp.shouldRetrigger)
            {
                synthEngine.setPitchState (arp.frequencyHz, arp.voiced);
                if (arp.shouldRetrigger && arp.voiced)
                    synthEngine.retrigger();
            }
            else if (! gateOpen)
            {
                synthEngine.setPitchState (0.0f, false);
                trackingActive = false;

                if (synthEngine.isIdle())
                {
                    gateWasOpen = gateOpen;
                    left[i] = 0.0f;
                    right[i] = 0.0f;
                    continue;
                }
            }
            else
            {
                // Gate open but arp not yet active (waiting for pitch): keep silent path.
                synthEngine.setPitchState (0.0f, false);
                if (synthEngine.isIdle())
                {
                    gateWasOpen = gateOpen;
                    left[i] = 0.0f;
                    right[i] = 0.0f;
                    continue;
                }
            }
        }
        else if (gateOpen)
        {
            synthEngine.setPitchState (trackedHz, trackingActive);

            if (envelopeFollower.consumeOnset() && trackingActive)
                synthEngine.retrigger();
        }
        else
        {
            synthEngine.setPitchState (0.0f, false);
            trackingActive = false;

            if (synthEngine.isIdle())
            {
                gateWasOpen = gateOpen;
                left[i] = 0.0f;
                right[i] = 0.0f;
                continue;
            }
        }

        gateWasOpen = gateOpen;

        float l = 0.0f, r = 0.0f;
        synthEngine.processSample (l, r);
        left[i] = l;
        right[i] = r;
    }

    chorusEffect.process (buffer);
    effectChain.process (buffer);

    displayedFrequency.store (pitchTracker.getFrequency());
    displayedConfidence.store (pitchTracker.getConfidence());
    displayedVoiced.store (trackingActive);
    displayedGateOpen.store (envelopeFollower.isGateOpen());
    displayedGateEnvelopeDb.store (envelopeFollower.getEnvelopeDb());
    displayedLatencyMs.store (1000.0 * static_cast<double> (getLatencySamples()) / getSampleRate());
}

juce::AudioProcessorEditor* GuitarSynthAudioProcessor::createEditor()
{
    return new GuitarSynthAudioProcessorEditor (*this);
}

void GuitarSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState().createXml())
    {
        state->setAttribute ("program", currentProgram);
        copyXmlToBinary (*state, destData);
    }
}

void GuitarSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        currentProgram = xml->getIntAttribute ("program", 0);
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GuitarSynthAudioProcessor();
}
