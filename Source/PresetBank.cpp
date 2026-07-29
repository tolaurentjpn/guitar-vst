#include "PresetBank.h"
#include "PluginProcessor.h"

namespace
{
    void setFloat (juce::AudioProcessorValueTreeState& apvts, const char* id, float value)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    }

    void setChoice (juce::AudioProcessorValueTreeState& apvts, const char* id, int index)
    {
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
            choice->setValueNotifyingHost (choice->convertTo0to1 (static_cast<float> (index)));
        else if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (index)));
    }

    void setBool (juce::AudioProcessorValueTreeState& apvts, const char* id, bool value)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (value ? 1.0f : 0.0f);
    }

    void setInt (juce::AudioProcessorValueTreeState& apvts, const char* id, int value)
    {
        if (auto* intParam = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (id)))
            intParam->setValueNotifyingHost (intParam->convertTo0to1 (static_cast<float> (value)));
        else if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (value)));
    }

    // FX type IDs: 0 Distortion, 1 Compressor, 2 Delay, 3 Reverb
    void setFxOrder (juce::AudioProcessorValueTreeState& apvts, int a, int b, int c, int d)
    {
        using P = GuitarSynthAudioProcessor;
        setInt (apvts, P::paramFxOrder0, a);
        setInt (apvts, P::paramFxOrder1, b);
        setInt (apvts, P::paramFxOrder2, c);
        setInt (apvts, P::paramFxOrder3, d);
    }

    void setAdsr (juce::AudioProcessorValueTreeState& apvts,
                  const char* a, const char* d, const char* s, const char* r,
                  float attack, float decay, float sustain, float release)
    {
        setFloat (apvts, a, attack);
        setFloat (apvts, d, decay);
        setFloat (apvts, s, sustain);
        setFloat (apvts, r, release);
    }

    void setLfoOff (juce::AudioProcessorValueTreeState& apvts, bool isLfo1)
    {
        using P = GuitarSynthAudioProcessor;
        if (isLfo1)
        {
            setBool (apvts, P::paramLfo1Enabled, false);
            setFloat (apvts, P::paramLfo1Rate, 2.0f);
            setChoice (apvts, P::paramLfo1Shape, 0);
            setFloat (apvts, P::paramLfo1Filter, 0.0f);
            setFloat (apvts, P::paramLfo1Resonance, 0.0f);
            setFloat (apvts, P::paramLfo1Pitch, 0.0f);
            setFloat (apvts, P::paramLfo1Amp, 0.0f);
            setFloat (apvts, P::paramLfo1Pwm, 0.0f);
        }
        else
        {
            setBool (apvts, P::paramLfo2Enabled, false);
            setFloat (apvts, P::paramLfo2Rate, 0.5f);
            setChoice (apvts, P::paramLfo2Shape, 1);
            setFloat (apvts, P::paramLfo2Filter, 0.0f);
            setFloat (apvts, P::paramLfo2Resonance, 0.0f);
            setFloat (apvts, P::paramLfo2Pitch, 0.0f);
            setFloat (apvts, P::paramLfo2Amp, 0.0f);
            setFloat (apvts, P::paramLfo2Pwm, 0.0f);
        }
    }

    void applyCommonDefaults (juce::AudioProcessorValueTreeState& apvts)
    {
        using P = GuitarSynthAudioProcessor;

        setBool (apvts, P::paramAdsrSync, false);
        setBool (apvts, P::paramFilterEnvSync, false);
        setLfoOff (apvts, true);
        setLfoOff (apvts, false);

        setFloat (apvts, P::paramMasterGain, 0.78f);
        setFloat (apvts, P::paramOsc1PhaseRandom, 1.0f);
        setFloat (apvts, P::paramOsc2PhaseRandom, 1.0f);
        setFloat (apvts, P::paramOsc1UnisonBlend, 0.8f);
        setFloat (apvts, P::paramOsc2UnisonBlend, 0.8f);
        setFloat (apvts, P::paramOsc1UnisonDetune, 0.35f);
        setFloat (apvts, P::paramOsc2UnisonDetune, 0.35f);
        setFloat (apvts, P::paramOsc1PulseWidth, 0.5f);
        setFloat (apvts, P::paramOsc2PulseWidth, 0.5f);
        setFloat (apvts, P::paramSubLevel, 0.0f);
        setFloat (apvts, P::paramNoiseMix, 0.0f);

        setBool (apvts, P::paramChorusEnabled, false);
        setFloat (apvts, P::paramChorusRate, 0.8f);
        setFloat (apvts, P::paramChorusDepth, 0.35f);
        setFloat (apvts, P::paramChorusMix, 0.4f);

        setBool (apvts, P::paramArpEnabled, false);
        setBool (apvts, P::paramArpSync, false);
        setFloat (apvts, P::paramArpRate, 4.0f);
        setChoice (apvts, P::paramArpDivision, 1);
        setFloat (apvts, P::paramArpGate, 50.0f);
        setChoice (apvts, P::paramArpMode, 0);
        setInt (apvts, P::paramArpOctaves, 1);
        setChoice (apvts, P::paramArpChord, 1);
        setBool (apvts, P::paramArpLatch, false);

        setBool (apvts, P::paramDistEnabled, false);
        setChoice (apvts, P::paramDistMode, 0);
        setFloat (apvts, P::paramDistDrive, 0.35f);
        setFloat (apvts, P::paramDistTone, 0.65f);
        setFloat (apvts, P::paramDistMix, 1.0f);

        setBool (apvts, P::paramCompEnabled, false);
        setFloat (apvts, P::paramCompThreshold, -18.0f);
        setFloat (apvts, P::paramCompRatio, 4.0f);
        setFloat (apvts, P::paramCompAttack, 10.0f);
        setFloat (apvts, P::paramCompRelease, 100.0f);
        setFloat (apvts, P::paramCompMakeup, 0.0f);
        setFloat (apvts, P::paramCompMix, 1.0f);

        setBool (apvts, P::paramDelayEnabled, false);
        setFloat (apvts, P::paramDelayTime, 350.0f);
        setFloat (apvts, P::paramDelayFeedback, 0.35f);
        setFloat (apvts, P::paramDelayDamping, 0.35f);
        setFloat (apvts, P::paramDelayMix, 0.35f);
        setBool (apvts, P::paramDelayPingPong, false);

        setBool (apvts, P::paramReverbEnabled, false);
        setFloat (apvts, P::paramReverbSize, 0.45f);
        setFloat (apvts, P::paramReverbDamping, 0.4f);
        setFloat (apvts, P::paramReverbWidth, 1.0f);
        setFloat (apvts, P::paramReverbMix, 0.25f);

        setFxOrder (apvts, 0, 1, 2, 3);
    }
}

juce::String PresetBank::getName (int index)
{
    static const char* names[numPresets] = {
        "Init",
        "Soft Bass",
        "Growl Bass",
        "Saw Lead",
        "SuperSaw",
        "Warm Pad",
        "Pluck",
        "Keys",
        "Organ",
        "Filter Sweep",
        "Zimmer Brass",
        "Zimmer Pad",
        "Jupiter Brass",
        "Jupiter Strings"
    };

    if (index < 0 || index >= numPresets)
        return {};

    return names[index];
}

void PresetBank::applyTo (juce::AudioProcessorValueTreeState& apvts, int index)
{
    if (index < 0 || index >= numPresets)
        return;

    using P = GuitarSynthAudioProcessor;
    applyCommonDefaults (apvts);

    // Tracking / Gate / Retrigger intentionally left unchanged.

    switch (index)
    {
        case 0: // Init — clean dual-saw starting point (PolyBLEP-friendly open tone)
            setChoice (apvts, P::paramWaveform, 1);
            setChoice (apvts, P::paramOsc2Waveform, 1);
            setFloat (apvts, P::paramOsc2Mix, 0.35f);
            setChoice (apvts, P::paramOsc2Octave, 1);
            setFloat (apvts, P::paramOsc2Detune, 3.0f);
            setFloat (apvts, P::paramSubLevel, 0.12f);
            setInt (apvts, P::paramOsc1UnisonVoices, 1);
            setInt (apvts, P::paramOsc2UnisonVoices, 1);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.35f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.35f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 0.0f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.0f);
            setFloat (apvts, P::paramOsc1UnisonBlend, 0.8f);
            setFloat (apvts, P::paramOsc2UnisonBlend, 0.8f);
            setFloat (apvts, P::paramOsc1PhaseRandom, 1.0f);
            setFloat (apvts, P::paramOsc2PhaseRandom, 1.0f);
            setFloat (apvts, P::paramFilterCutoff, 3200.0f);
            setFloat (apvts, P::paramFilterResonance, 0.75f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 3000.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 0.72f);
            setBool (apvts, P::paramAdsrSync, true);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     8.0f, 180.0f, 0.75f, 220.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     8.0f, 180.0f, 0.75f, 220.0f);
            setBool (apvts, P::paramFilterEnvSync, true);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     8.0f, 180.0f, 0.5f, 220.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.0f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     8.0f, 180.0f, 0.5f, 220.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.0f);
            setFloat (apvts, P::paramGlide, 25.0f);
            setFloat (apvts, P::paramMasterGain, 0.8f);
            break;

        case 1: // Soft Bass — sine + bandlimited sub, round saw body
            setChoice (apvts, P::paramWaveform, 0);       // sine fundamental
            setChoice (apvts, P::paramOsc2Waveform, 1);   // saw body
            setFloat (apvts, P::paramOsc2Mix, 0.24f);
            setChoice (apvts, P::paramOsc2Octave, 0);     // -1
            setFloat (apvts, P::paramOsc2Detune, 4.0f);
            setFloat (apvts, P::paramSubLevel, 0.48f);
            setInt (apvts, P::paramOsc1UnisonVoices, 1);
            setInt (apvts, P::paramOsc2UnisonVoices, 2);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.1f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.18f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 0.0f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.2f);
            setFloat (apvts, P::paramOsc1UnisonBlend, 0.7f);
            setFloat (apvts, P::paramOsc2UnisonBlend, 0.65f);
            setFloat (apvts, P::paramOsc1PhaseRandom, 0.3f);
            setFloat (apvts, P::paramOsc2PhaseRandom, 0.85f);
            setFloat (apvts, P::paramFilterCutoff, 520.0f);
            setFloat (apvts, P::paramFilterResonance, 0.58f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 780.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 0.5f);
            setBool (apvts, P::paramAdsrSync, true);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     4.0f, 240.0f, 0.88f, 200.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     4.0f, 240.0f, 0.88f, 200.0f);
            setBool (apvts, P::paramFilterEnvSync, true);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     2.0f, 140.0f, 0.22f, 160.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.42f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     2.0f, 140.0f, 0.22f, 160.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.32f);
            setFloat (apvts, P::paramGlide, 48.0f);
            setFloat (apvts, P::paramMasterGain, 0.72f);

            setBool (apvts, P::paramCompEnabled, true);
            setFloat (apvts, P::paramCompThreshold, -22.0f);
            setFloat (apvts, P::paramCompRatio, 3.5f);
            setFloat (apvts, P::paramCompAttack, 18.0f);
            setFloat (apvts, P::paramCompRelease, 140.0f);
            setFloat (apvts, P::paramCompMakeup, 3.0f);
            setFloat (apvts, P::paramCompMix, 1.0f);

            setBool (apvts, P::paramReverbEnabled, true);
            setFloat (apvts, P::paramReverbSize, 0.28f);
            setFloat (apvts, P::paramReverbDamping, 0.55f);
            setFloat (apvts, P::paramReverbWidth, 0.7f);
            setFloat (apvts, P::paramReverbMix, 0.12f);

            setFxOrder (apvts, 1, 0, 2, 3); // Comp → Dist → Delay → Reverb
            break;

        case 2: // Growl Bass — PWM square growl + bandlimited sub
            setChoice (apvts, P::paramWaveform, 1);
            setChoice (apvts, P::paramOsc2Waveform, 2);
            setFloat (apvts, P::paramOsc2Mix, 0.42f);
            setChoice (apvts, P::paramOsc2Octave, 0);
            setFloat (apvts, P::paramOsc2Detune, 14.0f);
            setFloat (apvts, P::paramOsc2PulseWidth, 0.22f);
            setFloat (apvts, P::paramSubLevel, 0.42f);
            setInt (apvts, P::paramOsc1UnisonVoices, 3);
            setInt (apvts, P::paramOsc2UnisonVoices, 2);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.22f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.16f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 0.25f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.15f);
            setFloat (apvts, P::paramOsc1UnisonBlend, 0.75f);
            setFloat (apvts, P::paramOsc2UnisonBlend, 0.7f);
            setFloat (apvts, P::paramOsc1PhaseRandom, 0.95f);
            setFloat (apvts, P::paramOsc2PhaseRandom, 0.7f);
            setFloat (apvts, P::paramFilterCutoff, 380.0f);
            setFloat (apvts, P::paramFilterResonance, 1.35f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 560.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 1.05f);
            setBool (apvts, P::paramAdsrSync, true);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     3.0f, 260.0f, 0.72f, 180.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     3.0f, 260.0f, 0.72f, 180.0f);
            setBool (apvts, P::paramFilterEnvSync, false);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     1.5f, 160.0f, 0.12f, 140.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.78f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     2.0f, 200.0f, 0.18f, 150.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.58f);
            setFloat (apvts, P::paramGlide, 28.0f);
            setFloat (apvts, P::paramMasterGain, 0.66f);

            setBool (apvts, P::paramLfo1Enabled, true);
            setFloat (apvts, P::paramLfo1Rate, 0.35f);
            setChoice (apvts, P::paramLfo1Shape, 1); // triangle
            setFloat (apvts, P::paramLfo1Filter, 0.12f);
            setFloat (apvts, P::paramLfo1Resonance, 0.08f);

            setBool (apvts, P::paramLfo2Enabled, true);
            setFloat (apvts, P::paramLfo2Rate, 0.55f);
            setChoice (apvts, P::paramLfo2Shape, 0);
            setFloat (apvts, P::paramLfo2Pwm, 0.28f);

            setBool (apvts, P::paramDistEnabled, true);
            setChoice (apvts, P::paramDistMode, 1); // Hard
            setFloat (apvts, P::paramDistDrive, 0.42f);
            setFloat (apvts, P::paramDistTone, 0.48f);
            setFloat (apvts, P::paramDistMix, 0.7f);

            setBool (apvts, P::paramCompEnabled, true);
            setFloat (apvts, P::paramCompThreshold, -16.0f);
            setFloat (apvts, P::paramCompRatio, 5.0f);
            setFloat (apvts, P::paramCompAttack, 8.0f);
            setFloat (apvts, P::paramCompRelease, 90.0f);
            setFloat (apvts, P::paramCompMakeup, 2.5f);
            setFloat (apvts, P::paramCompMix, 1.0f);

            setBool (apvts, P::paramReverbEnabled, true);
            setFloat (apvts, P::paramReverbSize, 0.22f);
            setFloat (apvts, P::paramReverbDamping, 0.75f);
            setFloat (apvts, P::paramReverbWidth, 0.55f);
            setFloat (apvts, P::paramReverbMix, 0.08f);

            setFxOrder (apvts, 0, 1, 2, 3); // Dist → Comp → Delay → Reverb
            break;

        case 3: // Saw Lead — bright PolyBLEP stack with slap delay + vibrato
            setChoice (apvts, P::paramWaveform, 1);
            setChoice (apvts, P::paramOsc2Waveform, 1);
            setFloat (apvts, P::paramOsc2Mix, 0.32f);
            setChoice (apvts, P::paramOsc2Octave, 1);
            setFloat (apvts, P::paramOsc2Detune, 9.0f);
            setFloat (apvts, P::paramSubLevel, 0.08f);
            setInt (apvts, P::paramOsc1UnisonVoices, 4);
            setInt (apvts, P::paramOsc2UnisonVoices, 3);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.3f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.24f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 0.68f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.55f);
            setFloat (apvts, P::paramOsc1UnisonBlend, 0.82f);
            setFloat (apvts, P::paramOsc2UnisonBlend, 0.78f);
            setFloat (apvts, P::paramOsc1PhaseRandom, 1.0f);
            setFloat (apvts, P::paramOsc2PhaseRandom, 1.0f);
            setFloat (apvts, P::paramFilterCutoff, 4500.0f);
            setFloat (apvts, P::paramFilterResonance, 0.85f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 4000.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 0.78f);
            setBool (apvts, P::paramAdsrSync, true);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     10.0f, 220.0f, 0.82f, 320.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     10.0f, 220.0f, 0.82f, 320.0f);
            setBool (apvts, P::paramFilterEnvSync, true);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     8.0f, 280.0f, 0.48f, 340.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.48f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     8.0f, 280.0f, 0.48f, 340.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.4f);
            setFloat (apvts, P::paramGlide, 60.0f);
            setFloat (apvts, P::paramMasterGain, 0.74f);

            setBool (apvts, P::paramLfo1Enabled, true);
            setFloat (apvts, P::paramLfo1Rate, 5.2f);
            setChoice (apvts, P::paramLfo1Shape, 0); // sine vibrato
            setFloat (apvts, P::paramLfo1Pitch, 0.18f);
            setFloat (apvts, P::paramLfo1Filter, 0.06f);

            setBool (apvts, P::paramLfo2Enabled, true);
            setFloat (apvts, P::paramLfo2Rate, 5.0f);
            setChoice (apvts, P::paramLfo2Shape, 0);
            setFloat (apvts, P::paramLfo2Pitch, 0.14f);

            setBool (apvts, P::paramDistEnabled, true);
            setChoice (apvts, P::paramDistMode, 0); // Soft
            setFloat (apvts, P::paramDistDrive, 0.22f);
            setFloat (apvts, P::paramDistTone, 0.78f);
            setFloat (apvts, P::paramDistMix, 0.42f);

            setBool (apvts, P::paramCompEnabled, true);
            setFloat (apvts, P::paramCompThreshold, -20.0f);
            setFloat (apvts, P::paramCompRatio, 3.0f);
            setFloat (apvts, P::paramCompAttack, 12.0f);
            setFloat (apvts, P::paramCompRelease, 110.0f);
            setFloat (apvts, P::paramCompMakeup, 1.5f);

            setBool (apvts, P::paramDelayEnabled, true);
            setFloat (apvts, P::paramDelayTime, 280.0f);
            setFloat (apvts, P::paramDelayFeedback, 0.28f);
            setFloat (apvts, P::paramDelayDamping, 0.28f);
            setFloat (apvts, P::paramDelayMix, 0.28f);
            setBool (apvts, P::paramDelayPingPong, true);

            setBool (apvts, P::paramReverbEnabled, true);
            setFloat (apvts, P::paramReverbSize, 0.42f);
            setFloat (apvts, P::paramReverbDamping, 0.28f);
            setFloat (apvts, P::paramReverbWidth, 1.0f);
            setFloat (apvts, P::paramReverbMix, 0.22f);

            setFxOrder (apvts, 0, 1, 2, 3);
            break;

        case 4: // SuperSaw — wide PolyBLEP stack with light chorus + hall
            setChoice (apvts, P::paramWaveform, 1);
            setChoice (apvts, P::paramOsc2Waveform, 1);
            setFloat (apvts, P::paramOsc2Mix, 0.5f);
            setChoice (apvts, P::paramOsc2Octave, 1);
            setFloat (apvts, P::paramOsc2Detune, 8.0f);
            setFloat (apvts, P::paramSubLevel, 0.1f);
            setInt (apvts, P::paramOsc1UnisonVoices, 8);
            setInt (apvts, P::paramOsc2UnisonVoices, 7);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.5f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.42f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 1.0f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.95f);
            setFloat (apvts, P::paramOsc1UnisonBlend, 0.9f);
            setFloat (apvts, P::paramOsc2UnisonBlend, 0.88f);
            setFloat (apvts, P::paramOsc1PhaseRandom, 1.0f);
            setFloat (apvts, P::paramOsc2PhaseRandom, 1.0f);
            setFloat (apvts, P::paramFilterCutoff, 6500.0f);
            setFloat (apvts, P::paramFilterResonance, 0.55f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 5800.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 0.5f);
            setBool (apvts, P::paramAdsrSync, true);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     18.0f, 320.0f, 0.88f, 420.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     18.0f, 320.0f, 0.88f, 420.0f);
            setBool (apvts, P::paramFilterEnvSync, true);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     14.0f, 450.0f, 0.58f, 480.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.28f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     14.0f, 450.0f, 0.58f, 480.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.22f);
            setFloat (apvts, P::paramGlide, 32.0f);
            setFloat (apvts, P::paramMasterGain, 0.64f);

            setBool (apvts, P::paramLfo1Enabled, true);
            setFloat (apvts, P::paramLfo1Rate, 0.18f);
            setChoice (apvts, P::paramLfo1Shape, 0);
            setFloat (apvts, P::paramLfo1Filter, 0.12f);

            setBool (apvts, P::paramLfo2Enabled, true);
            setFloat (apvts, P::paramLfo2Rate, 0.22f);
            setChoice (apvts, P::paramLfo2Shape, 1);
            setFloat (apvts, P::paramLfo2Filter, 0.1f);
            setFloat (apvts, P::paramLfo2Amp, 0.05f);

            setBool (apvts, P::paramChorusEnabled, true);
            setFloat (apvts, P::paramChorusRate, 0.45f);
            setFloat (apvts, P::paramChorusDepth, 0.32f);
            setFloat (apvts, P::paramChorusMix, 0.28f);

            setBool (apvts, P::paramCompEnabled, true);
            setFloat (apvts, P::paramCompThreshold, -18.0f);
            setFloat (apvts, P::paramCompRatio, 4.5f);
            setFloat (apvts, P::paramCompAttack, 15.0f);
            setFloat (apvts, P::paramCompRelease, 160.0f);
            setFloat (apvts, P::paramCompMakeup, 2.0f);

            setBool (apvts, P::paramDelayEnabled, true);
            setFloat (apvts, P::paramDelayTime, 420.0f);
            setFloat (apvts, P::paramDelayFeedback, 0.32f);
            setFloat (apvts, P::paramDelayDamping, 0.28f);
            setFloat (apvts, P::paramDelayMix, 0.22f);
            setBool (apvts, P::paramDelayPingPong, true);

            setBool (apvts, P::paramReverbEnabled, true);
            setFloat (apvts, P::paramReverbSize, 0.58f);
            setFloat (apvts, P::paramReverbDamping, 0.25f);
            setFloat (apvts, P::paramReverbWidth, 1.0f);
            setFloat (apvts, P::paramReverbMix, 0.3f);

            setFxOrder (apvts, 1, 0, 2, 3); // Comp first to glue stack
            break;

        case 5: // Warm Pad — triangle/saw bloom with soft noise + chorus
            setChoice (apvts, P::paramWaveform, 3); // triangle
            setChoice (apvts, P::paramOsc2Waveform, 1);
            setFloat (apvts, P::paramOsc2Mix, 0.48f);
            setChoice (apvts, P::paramOsc2Octave, 2); // +1
            setFloat (apvts, P::paramOsc2Detune, 12.0f);
            setFloat (apvts, P::paramNoiseMix, 0.1f);
            setFloat (apvts, P::paramSubLevel, 0.08f);
            setInt (apvts, P::paramOsc1UnisonVoices, 5);
            setInt (apvts, P::paramOsc2UnisonVoices, 4);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.32f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.28f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 0.92f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.88f);
            setFloat (apvts, P::paramOsc1UnisonBlend, 0.72f);
            setFloat (apvts, P::paramOsc2UnisonBlend, 0.7f);
            setFloat (apvts, P::paramOsc1PhaseRandom, 1.0f);
            setFloat (apvts, P::paramOsc2PhaseRandom, 1.0f);
            setFloat (apvts, P::paramFilterCutoff, 2400.0f);
            setFloat (apvts, P::paramFilterResonance, 0.48f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 3400.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 0.42f);
            setBool (apvts, P::paramAdsrSync, false);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     420.0f, 900.0f, 0.92f, 2200.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     520.0f, 1000.0f, 0.9f, 2500.0f);
            setBool (apvts, P::paramFilterEnvSync, false);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     160.0f, 800.0f, 0.62f, 1200.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.52f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     190.0f, 850.0f, 0.58f, 1300.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.45f);
            setFloat (apvts, P::paramGlide, 90.0f);
            setFloat (apvts, P::paramMasterGain, 0.68f);

            setBool (apvts, P::paramLfo1Enabled, true);
            setFloat (apvts, P::paramLfo1Rate, 0.12f);
            setChoice (apvts, P::paramLfo1Shape, 0);
            setFloat (apvts, P::paramLfo1Filter, 0.28f);
            setFloat (apvts, P::paramLfo1Resonance, 0.1f);
            setFloat (apvts, P::paramLfo1Amp, 0.08f);

            setBool (apvts, P::paramLfo2Enabled, true);
            setFloat (apvts, P::paramLfo2Rate, 0.19f);
            setChoice (apvts, P::paramLfo2Shape, 1);
            setFloat (apvts, P::paramLfo2Filter, 0.22f);
            setFloat (apvts, P::paramLfo2Pitch, 0.04f);
            setFloat (apvts, P::paramLfo2Amp, 0.1f);

            setBool (apvts, P::paramChorusEnabled, true);
            setFloat (apvts, P::paramChorusRate, 0.4f);
            setFloat (apvts, P::paramChorusDepth, 0.45f);
            setFloat (apvts, P::paramChorusMix, 0.4f);

            setBool (apvts, P::paramCompEnabled, true);
            setFloat (apvts, P::paramCompThreshold, -24.0f);
            setFloat (apvts, P::paramCompRatio, 2.5f);
            setFloat (apvts, P::paramCompAttack, 30.0f);
            setFloat (apvts, P::paramCompRelease, 220.0f);
            setFloat (apvts, P::paramCompMakeup, 1.0f);
            setFloat (apvts, P::paramCompMix, 0.85f);

            setBool (apvts, P::paramDelayEnabled, true);
            setFloat (apvts, P::paramDelayTime, 520.0f);
            setFloat (apvts, P::paramDelayFeedback, 0.42f);
            setFloat (apvts, P::paramDelayDamping, 0.35f);
            setFloat (apvts, P::paramDelayMix, 0.3f);
            setBool (apvts, P::paramDelayPingPong, true);

            setBool (apvts, P::paramReverbEnabled, true);
            setFloat (apvts, P::paramReverbSize, 0.78f);
            setFloat (apvts, P::paramReverbDamping, 0.22f);
            setFloat (apvts, P::paramReverbWidth, 1.0f);
            setFloat (apvts, P::paramReverbMix, 0.42f);

            setFxOrder (apvts, 1, 2, 3, 0); // Comp → Delay → Reverb → Dist(off)
            break;

        case 6: // Pluck — snappy PolyBLEP pluck with PWM sparkle
            setChoice (apvts, P::paramWaveform, 1);
            setChoice (apvts, P::paramOsc2Waveform, 2);
            setFloat (apvts, P::paramOsc2Mix, 0.22f);
            setChoice (apvts, P::paramOsc2Octave, 2);
            setFloat (apvts, P::paramOsc2Detune, 5.0f);
            setFloat (apvts, P::paramOsc2PulseWidth, 0.18f);
            setInt (apvts, P::paramOsc1UnisonVoices, 2);
            setInt (apvts, P::paramOsc2UnisonVoices, 1);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.14f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.1f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 0.4f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.2f);
            setFloat (apvts, P::paramOsc1UnisonBlend, 0.78f);
            setFloat (apvts, P::paramOsc2UnisonBlend, 0.75f);
            setFloat (apvts, P::paramOsc1PhaseRandom, 0.55f);
            setFloat (apvts, P::paramOsc2PhaseRandom, 0.4f);
            setFloat (apvts, P::paramFilterCutoff, 2800.0f);
            setFloat (apvts, P::paramFilterResonance, 1.05f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 4500.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 0.85f);
            setBool (apvts, P::paramAdsrSync, false);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     1.5f, 130.0f, 0.12f, 110.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     1.5f, 90.0f, 0.08f, 90.0f);
            setBool (apvts, P::paramFilterEnvSync, false);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     1.0f, 85.0f, 0.04f, 70.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.82f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     1.0f, 70.0f, 0.03f, 60.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.7f);
            setFloat (apvts, P::paramGlide, 4.0f);
            setFloat (apvts, P::paramMasterGain, 0.76f);

            setBool (apvts, P::paramDistEnabled, true);
            setChoice (apvts, P::paramDistMode, 0);
            setFloat (apvts, P::paramDistDrive, 0.22f);
            setFloat (apvts, P::paramDistTone, 0.78f);
            setFloat (apvts, P::paramDistMix, 0.4f);

            setBool (apvts, P::paramCompEnabled, true);
            setFloat (apvts, P::paramCompThreshold, -14.0f);
            setFloat (apvts, P::paramCompRatio, 4.0f);
            setFloat (apvts, P::paramCompAttack, 2.0f);
            setFloat (apvts, P::paramCompRelease, 60.0f);
            setFloat (apvts, P::paramCompMakeup, 1.5f);

            setBool (apvts, P::paramDelayEnabled, true);
            setFloat (apvts, P::paramDelayTime, 95.0f);
            setFloat (apvts, P::paramDelayFeedback, 0.18f);
            setFloat (apvts, P::paramDelayDamping, 0.55f);
            setFloat (apvts, P::paramDelayMix, 0.2f);
            setBool (apvts, P::paramDelayPingPong, false);

            setBool (apvts, P::paramReverbEnabled, true);
            setFloat (apvts, P::paramReverbSize, 0.35f);
            setFloat (apvts, P::paramReverbDamping, 0.55f);
            setFloat (apvts, P::paramReverbWidth, 0.9f);
            setFloat (apvts, P::paramReverbMix, 0.18f);

            setFxOrder (apvts, 0, 1, 2, 3);
            break;

        case 7: // Keys — electric-piano character, soft amp, hall
            setChoice (apvts, P::paramWaveform, 0);
            setChoice (apvts, P::paramOsc2Waveform, 1);
            setFloat (apvts, P::paramOsc2Mix, 0.34f);
            setChoice (apvts, P::paramOsc2Octave, 2);
            setFloat (apvts, P::paramOsc2Detune, 3.5f);
            setInt (apvts, P::paramOsc1UnisonVoices, 2);
            setInt (apvts, P::paramOsc2UnisonVoices, 2);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.1f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.12f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 0.35f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.3f);
            setFloat (apvts, P::paramOsc1UnisonBlend, 0.7f);
            setFloat (apvts, P::paramOsc2UnisonBlend, 0.68f);
            setFloat (apvts, P::paramOsc1PhaseRandom, 0.25f); // more pitch-stable hammer
            setFloat (apvts, P::paramOsc2PhaseRandom, 0.6f);
            setFloat (apvts, P::paramFilterCutoff, 3000.0f);
            setFloat (apvts, P::paramFilterResonance, 0.62f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 3800.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 0.55f);
            setBool (apvts, P::paramAdsrSync, false);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     2.5f, 380.0f, 0.32f, 480.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     2.5f, 260.0f, 0.2f, 360.0f);
            setBool (apvts, P::paramFilterEnvSync, false);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     1.5f, 240.0f, 0.18f, 320.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.55f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     1.5f, 180.0f, 0.12f, 280.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.48f);
            setFloat (apvts, P::paramGlide, 8.0f);
            setFloat (apvts, P::paramMasterGain, 0.76f);

            setBool (apvts, P::paramLfo1Enabled, true);
            setFloat (apvts, P::paramLfo1Rate, 4.8f);
            setChoice (apvts, P::paramLfo1Shape, 0);
            setFloat (apvts, P::paramLfo1Pitch, 0.06f);

            setBool (apvts, P::paramChorusEnabled, true);
            setFloat (apvts, P::paramChorusRate, 0.7f);
            setFloat (apvts, P::paramChorusDepth, 0.22f);
            setFloat (apvts, P::paramChorusMix, 0.2f);

            setBool (apvts, P::paramCompEnabled, true);
            setFloat (apvts, P::paramCompThreshold, -20.0f);
            setFloat (apvts, P::paramCompRatio, 3.2f);
            setFloat (apvts, P::paramCompAttack, 5.0f);
            setFloat (apvts, P::paramCompRelease, 120.0f);
            setFloat (apvts, P::paramCompMakeup, 1.5f);

            setBool (apvts, P::paramDelayEnabled, true);
            setFloat (apvts, P::paramDelayTime, 320.0f);
            setFloat (apvts, P::paramDelayFeedback, 0.22f);
            setFloat (apvts, P::paramDelayDamping, 0.5f);
            setFloat (apvts, P::paramDelayMix, 0.18f);
            setBool (apvts, P::paramDelayPingPong, true);

            setBool (apvts, P::paramReverbEnabled, true);
            setFloat (apvts, P::paramReverbSize, 0.5f);
            setFloat (apvts, P::paramReverbDamping, 0.42f);
            setFloat (apvts, P::paramReverbWidth, 1.0f);
            setFloat (apvts, P::paramReverbMix, 0.28f);

            setFxOrder (apvts, 1, 0, 2, 3);
            break;

        case 8: // Organ — bandlimited square drawbar + sine, PWM + rotary
            setChoice (apvts, P::paramWaveform, 2);
            setChoice (apvts, P::paramOsc2Waveform, 0);
            setFloat (apvts, P::paramOsc2Mix, 0.45f);
            setChoice (apvts, P::paramOsc2Octave, 2);
            setFloat (apvts, P::paramOsc2Detune, 1.5f);
            setFloat (apvts, P::paramOsc1PulseWidth, 0.42f);
            setFloat (apvts, P::paramSubLevel, 0.15f);
            setInt (apvts, P::paramOsc1UnisonVoices, 1);
            setInt (apvts, P::paramOsc2UnisonVoices, 1);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.05f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.05f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 0.05f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.08f);
            setFloat (apvts, P::paramOsc1UnisonBlend, 0.5f);
            setFloat (apvts, P::paramOsc2UnisonBlend, 0.5f);
            setFloat (apvts, P::paramOsc1PhaseRandom, 0.15f);
            setFloat (apvts, P::paramOsc2PhaseRandom, 0.2f);
            setFloat (apvts, P::paramFilterCutoff, 5200.0f);
            setFloat (apvts, P::paramFilterResonance, 0.4f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 6000.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 0.35f);
            setBool (apvts, P::paramAdsrSync, true);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     12.0f, 80.0f, 0.96f, 120.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     12.0f, 80.0f, 0.96f, 120.0f);
            setBool (apvts, P::paramFilterEnvSync, true);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     10.0f, 100.0f, 0.8f, 120.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.08f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     10.0f, 100.0f, 0.8f, 120.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.06f);
            setFloat (apvts, P::paramGlide, 0.0f);
            setFloat (apvts, P::paramMasterGain, 0.72f);

            setBool (apvts, P::paramLfo1Enabled, true);
            setFloat (apvts, P::paramLfo1Rate, 6.2f);
            setChoice (apvts, P::paramLfo1Shape, 0);
            setFloat (apvts, P::paramLfo1Pitch, 0.22f);
            setFloat (apvts, P::paramLfo1Amp, 0.12f);
            setFloat (apvts, P::paramLfo1Filter, 0.05f);
            setFloat (apvts, P::paramLfo1Pwm, 0.18f);

            setBool (apvts, P::paramLfo2Enabled, true);
            setFloat (apvts, P::paramLfo2Rate, 5.8f);
            setChoice (apvts, P::paramLfo2Shape, 1);
            setFloat (apvts, P::paramLfo2Pitch, 0.16f);
            setFloat (apvts, P::paramLfo2Amp, 0.1f);

            setBool (apvts, P::paramChorusEnabled, true);
            setFloat (apvts, P::paramChorusRate, 0.9f);
            setFloat (apvts, P::paramChorusDepth, 0.25f);
            setFloat (apvts, P::paramChorusMix, 0.22f);

            setBool (apvts, P::paramDistEnabled, true);
            setChoice (apvts, P::paramDistMode, 0);
            setFloat (apvts, P::paramDistDrive, 0.25f);
            setFloat (apvts, P::paramDistTone, 0.62f);
            setFloat (apvts, P::paramDistMix, 0.35f);

            setBool (apvts, P::paramCompEnabled, true);
            setFloat (apvts, P::paramCompThreshold, -18.0f);
            setFloat (apvts, P::paramCompRatio, 3.0f);
            setFloat (apvts, P::paramCompAttack, 20.0f);
            setFloat (apvts, P::paramCompRelease, 100.0f);
            setFloat (apvts, P::paramCompMakeup, 1.0f);

            setBool (apvts, P::paramDelayEnabled, true);
            setFloat (apvts, P::paramDelayTime, 38.0f);
            setFloat (apvts, P::paramDelayFeedback, 0.12f);
            setFloat (apvts, P::paramDelayDamping, 0.3f);
            setFloat (apvts, P::paramDelayMix, 0.15f);
            setBool (apvts, P::paramDelayPingPong, false);

            setBool (apvts, P::paramReverbEnabled, true);
            setFloat (apvts, P::paramReverbSize, 0.4f);
            setFloat (apvts, P::paramReverbDamping, 0.48f);
            setFloat (apvts, P::paramReverbWidth, 0.85f);
            setFloat (apvts, P::paramReverbMix, 0.2f);

            setFxOrder (apvts, 0, 1, 2, 3);
            break;

        case 9: // Filter Sweep — wide PolyBLEP stack, long open
            setChoice (apvts, P::paramWaveform, 1);
            setChoice (apvts, P::paramOsc2Waveform, 1);
            setFloat (apvts, P::paramOsc2Mix, 0.4f);
            setChoice (apvts, P::paramOsc2Octave, 1);
            setFloat (apvts, P::paramOsc2Detune, 18.0f);
            setFloat (apvts, P::paramSubLevel, 0.12f);
            setInt (apvts, P::paramOsc1UnisonVoices, 5);
            setInt (apvts, P::paramOsc2UnisonVoices, 4);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.42f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.36f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 0.8f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.75f);
            setFloat (apvts, P::paramOsc1UnisonBlend, 0.84f);
            setFloat (apvts, P::paramOsc2UnisonBlend, 0.8f);
            setFloat (apvts, P::paramOsc1PhaseRandom, 1.0f);
            setFloat (apvts, P::paramOsc2PhaseRandom, 1.0f);
            setFloat (apvts, P::paramFilterCutoff, 160.0f);
            setFloat (apvts, P::paramFilterResonance, 1.4f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 200.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 1.25f);
            setBool (apvts, P::paramAdsrSync, true);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     35.0f, 450.0f, 0.72f, 650.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     35.0f, 450.0f, 0.72f, 650.0f);
            setBool (apvts, P::paramFilterEnvSync, false);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     120.0f, 1400.0f, 0.12f, 900.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.95f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     140.0f, 1500.0f, 0.1f, 950.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.9f);
            setFloat (apvts, P::paramGlide, 50.0f);
            setFloat (apvts, P::paramMasterGain, 0.68f);

            setBool (apvts, P::paramLfo1Enabled, true);
            setFloat (apvts, P::paramLfo1Rate, 0.28f);
            setChoice (apvts, P::paramLfo1Shape, 3); // saw for unidirectional crawl
            setFloat (apvts, P::paramLfo1Resonance, 0.18f);
            setFloat (apvts, P::paramLfo1Filter, 0.08f);

            setBool (apvts, P::paramLfo2Enabled, true);
            setFloat (apvts, P::paramLfo2Rate, 0.35f);
            setChoice (apvts, P::paramLfo2Shape, 1);
            setFloat (apvts, P::paramLfo2Resonance, 0.12f);
            setFloat (apvts, P::paramLfo2Filter, 0.06f);

            setBool (apvts, P::paramDistEnabled, true);
            setChoice (apvts, P::paramDistMode, 2); // Fold — harmonics as filter opens
            setFloat (apvts, P::paramDistDrive, 0.32f);
            setFloat (apvts, P::paramDistTone, 0.62f);
            setFloat (apvts, P::paramDistMix, 0.38f);

            setBool (apvts, P::paramCompEnabled, true);
            setFloat (apvts, P::paramCompThreshold, -16.0f);
            setFloat (apvts, P::paramCompRatio, 4.0f);
            setFloat (apvts, P::paramCompAttack, 10.0f);
            setFloat (apvts, P::paramCompRelease, 150.0f);
            setFloat (apvts, P::paramCompMakeup, 2.0f);

            setBool (apvts, P::paramDelayEnabled, true);
            setFloat (apvts, P::paramDelayTime, 480.0f);
            setFloat (apvts, P::paramDelayFeedback, 0.48f);
            setFloat (apvts, P::paramDelayDamping, 0.38f);
            setFloat (apvts, P::paramDelayMix, 0.35f);
            setBool (apvts, P::paramDelayPingPong, true);

            setBool (apvts, P::paramReverbEnabled, true);
            setFloat (apvts, P::paramReverbSize, 0.72f);
            setFloat (apvts, P::paramReverbDamping, 0.28f);
            setFloat (apvts, P::paramReverbWidth, 1.0f);
            setFloat (apvts, P::paramReverbMix, 0.38f);

            setFxOrder (apvts, 0, 1, 2, 3);
            break;

        case 10: // Zimmer Brass — cinematic horn; open PolyBLEP saws + sub air
            setChoice (apvts, P::paramWaveform, 1); // saw
            setChoice (apvts, P::paramOsc2Waveform, 1);
            setFloat (apvts, P::paramOsc2Mix, 0.55f);
            setChoice (apvts, P::paramOsc2Octave, 1); // 0
            setFloat (apvts, P::paramOsc2Detune, 20.0f);
            setFloat (apvts, P::paramSubLevel, 0.42f);
            setFloat (apvts, P::paramNoiseMix, 0.12f);
            setInt (apvts, P::paramOsc1UnisonVoices, 6);
            setInt (apvts, P::paramOsc2UnisonVoices, 5);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.52f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.46f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 0.78f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.72f);
            setFloat (apvts, P::paramOsc1UnisonBlend, 0.84f);
            setFloat (apvts, P::paramOsc2UnisonBlend, 0.82f);
            setFloat (apvts, P::paramFilterCutoff, 1200.0f);
            setFloat (apvts, P::paramFilterResonance, 1.05f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 1500.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 0.95f);
            setBool (apvts, P::paramAdsrSync, true);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     48.0f, 260.0f, 0.8f, 520.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     48.0f, 260.0f, 0.8f, 520.0f);
            setBool (apvts, P::paramFilterEnvSync, true);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     22.0f, 380.0f, 0.4f, 480.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.78f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     22.0f, 380.0f, 0.4f, 480.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.7f);
            setFloat (apvts, P::paramGlide, 40.0f);
            setFloat (apvts, P::paramMasterGain, 0.68f);

            setBool (apvts, P::paramLfo1Enabled, true);
            setFloat (apvts, P::paramLfo1Rate, 0.12f);
            setChoice (apvts, P::paramLfo1Shape, 0);
            setFloat (apvts, P::paramLfo1Filter, 0.08f);

            setBool (apvts, P::paramDistEnabled, true);
            setChoice (apvts, P::paramDistMode, 0);
            setFloat (apvts, P::paramDistDrive, 0.22f);
            setFloat (apvts, P::paramDistTone, 0.55f);
            setFloat (apvts, P::paramDistMix, 0.38f);

            setBool (apvts, P::paramCompEnabled, true);
            setFloat (apvts, P::paramCompThreshold, -16.0f);
            setFloat (apvts, P::paramCompRatio, 3.5f);
            setFloat (apvts, P::paramCompAttack, 8.0f);
            setFloat (apvts, P::paramCompRelease, 120.0f);
            setFloat (apvts, P::paramCompMakeup, 1.5f);

            setBool (apvts, P::paramReverbEnabled, true);
            setFloat (apvts, P::paramReverbSize, 0.58f);
            setFloat (apvts, P::paramReverbDamping, 0.32f);
            setFloat (apvts, P::paramReverbWidth, 1.0f);
            setFloat (apvts, P::paramReverbMix, 0.3f);

            setFxOrder (apvts, 0, 1, 2, 3);
            break;

        case 11: // Zimmer Pad — long bloom; softer LP noise + open triangle/saw
            setChoice (apvts, P::paramWaveform, 3); // triangle
            setChoice (apvts, P::paramOsc2Waveform, 1); // saw
            setFloat (apvts, P::paramOsc2Mix, 0.5f);
            setChoice (apvts, P::paramOsc2Octave, 2); // +1
            setFloat (apvts, P::paramOsc2Detune, 14.0f);
            setFloat (apvts, P::paramNoiseMix, 0.18f);
            setFloat (apvts, P::paramSubLevel, 0.18f);
            setInt (apvts, P::paramOsc1UnisonVoices, 5);
            setInt (apvts, P::paramOsc2UnisonVoices, 6);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.35f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.4f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 0.98f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.95f);
            setFloat (apvts, P::paramOsc1UnisonBlend, 0.7f);
            setFloat (apvts, P::paramOsc2UnisonBlend, 0.72f);
            setFloat (apvts, P::paramFilterCutoff, 2000.0f);
            setFloat (apvts, P::paramFilterResonance, 0.45f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 3200.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 0.4f);
            setBool (apvts, P::paramAdsrSync, false);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     2200.0f, 1800.0f, 0.94f, 4500.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     2800.0f, 2000.0f, 0.9f, 5200.0f);
            setBool (apvts, P::paramFilterEnvSync, false);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     1500.0f, 2500.0f, 0.7f, 4000.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.4f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     1800.0f, 2800.0f, 0.65f, 4500.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.32f);
            setFloat (apvts, P::paramGlide, 80.0f);
            setFloat (apvts, P::paramMasterGain, 0.64f);

            setBool (apvts, P::paramLfo1Enabled, true);
            setFloat (apvts, P::paramLfo1Rate, 0.08f);
            setChoice (apvts, P::paramLfo1Shape, 0);
            setFloat (apvts, P::paramLfo1Filter, 0.25f);
            setFloat (apvts, P::paramLfo1Amp, 0.08f);

            setBool (apvts, P::paramLfo2Enabled, true);
            setFloat (apvts, P::paramLfo2Rate, 0.11f);
            setChoice (apvts, P::paramLfo2Shape, 1);
            setFloat (apvts, P::paramLfo2Filter, 0.2f);
            setFloat (apvts, P::paramLfo2Pitch, 0.04f);

            setBool (apvts, P::paramChorusEnabled, true);
            setFloat (apvts, P::paramChorusRate, 0.32f);
            setFloat (apvts, P::paramChorusDepth, 0.6f);
            setFloat (apvts, P::paramChorusMix, 0.55f);

            setBool (apvts, P::paramDelayEnabled, true);
            setFloat (apvts, P::paramDelayTime, 620.0f);
            setFloat (apvts, P::paramDelayFeedback, 0.42f);
            setFloat (apvts, P::paramDelayDamping, 0.32f);
            setFloat (apvts, P::paramDelayMix, 0.28f);
            setBool (apvts, P::paramDelayPingPong, true);

            setBool (apvts, P::paramReverbEnabled, true);
            setFloat (apvts, P::paramReverbSize, 0.85f);
            setFloat (apvts, P::paramReverbDamping, 0.2f);
            setFloat (apvts, P::paramReverbWidth, 1.0f);
            setFloat (apvts, P::paramReverbMix, 0.45f);

            setFxOrder (apvts, 1, 2, 3, 0);
            break;

        case 12: // Jupiter Brass — saw + PolyBLEP PWM square, open brass bite
            setChoice (apvts, P::paramWaveform, 1); // saw
            setChoice (apvts, P::paramOsc2Waveform, 2); // square
            setFloat (apvts, P::paramOsc2Mix, 0.45f);
            setChoice (apvts, P::paramOsc2Octave, 1);
            setFloat (apvts, P::paramOsc2Detune, 7.0f);
            setFloat (apvts, P::paramOsc1PulseWidth, 0.5f);
            setFloat (apvts, P::paramOsc2PulseWidth, 0.24f);
            setFloat (apvts, P::paramSubLevel, 0.28f);
            setInt (apvts, P::paramOsc1UnisonVoices, 2);
            setInt (apvts, P::paramOsc2UnisonVoices, 2);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.2f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.16f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 0.4f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.35f);
            setFloat (apvts, P::paramFilterCutoff, 2200.0f);
            setFloat (apvts, P::paramFilterResonance, 0.95f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 2600.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 0.85f);
            setBool (apvts, P::paramAdsrSync, true);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     12.0f, 220.0f, 0.72f, 280.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     12.0f, 220.0f, 0.72f, 280.0f);
            setBool (apvts, P::paramFilterEnvSync, true);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     8.0f, 300.0f, 0.32f, 260.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.72f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     8.0f, 300.0f, 0.32f, 260.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.58f);
            setFloat (apvts, P::paramGlide, 18.0f);
            setFloat (apvts, P::paramMasterGain, 0.7f);

            setBool (apvts, P::paramLfo2Enabled, true);
            setFloat (apvts, P::paramLfo2Rate, 0.45f);
            setChoice (apvts, P::paramLfo2Shape, 0);
            setFloat (apvts, P::paramLfo2Pwm, 0.42f);

            setBool (apvts, P::paramChorusEnabled, true);
            setFloat (apvts, P::paramChorusRate, 0.55f);
            setFloat (apvts, P::paramChorusDepth, 0.45f);
            setFloat (apvts, P::paramChorusMix, 0.4f);

            setBool (apvts, P::paramReverbEnabled, true);
            setFloat (apvts, P::paramReverbSize, 0.42f);
            setFloat (apvts, P::paramReverbDamping, 0.35f);
            setFloat (apvts, P::paramReverbWidth, 0.95f);
            setFloat (apvts, P::paramReverbMix, 0.22f);

            setFxOrder (apvts, 0, 1, 2, 3);
            break;

        case 13: // Jupiter Strings — PWM PolyBLEP ensemble + rich chorus
            setChoice (apvts, P::paramWaveform, 2); // square + PWM
            setChoice (apvts, P::paramOsc2Waveform, 3); // triangle
            setFloat (apvts, P::paramOsc2Mix, 0.52f);
            setChoice (apvts, P::paramOsc2Octave, 1);
            setFloat (apvts, P::paramOsc2Detune, 9.0f);
            setFloat (apvts, P::paramOsc1PulseWidth, 0.2f);
            setFloat (apvts, P::paramSubLevel, 0.12f);
            setFloat (apvts, P::paramNoiseMix, 0.04f);
            setInt (apvts, P::paramOsc1UnisonVoices, 4);
            setInt (apvts, P::paramOsc2UnisonVoices, 4);
            setFloat (apvts, P::paramOsc1UnisonDetune, 0.3f);
            setFloat (apvts, P::paramOsc2UnisonDetune, 0.25f);
            setFloat (apvts, P::paramOsc1UnisonSpread, 0.9f);
            setFloat (apvts, P::paramOsc2UnisonSpread, 0.85f);
            setFloat (apvts, P::paramOsc1UnisonBlend, 0.75f);
            setFloat (apvts, P::paramOsc2UnisonBlend, 0.72f);
            setFloat (apvts, P::paramFilterCutoff, 2800.0f);
            setFloat (apvts, P::paramFilterResonance, 0.48f);
            setFloat (apvts, P::paramOsc2FilterCutoff, 3600.0f);
            setFloat (apvts, P::paramOsc2FilterResonance, 0.42f);
            setBool (apvts, P::paramAdsrSync, true);
            setAdsr (apvts, P::paramAttack, P::paramDecay, P::paramSustain, P::paramRelease,
                     480.0f, 900.0f, 0.9f, 1800.0f);
            setAdsr (apvts, P::paramOsc2Attack, P::paramOsc2Decay, P::paramOsc2Sustain, P::paramOsc2Release,
                     520.0f, 950.0f, 0.88f, 1900.0f);
            setBool (apvts, P::paramFilterEnvSync, true);
            setAdsr (apvts, P::paramFilterEnv1Attack, P::paramFilterEnv1Decay,
                     P::paramFilterEnv1Sustain, P::paramFilterEnv1Release,
                     220.0f, 1000.0f, 0.58f, 1300.0f);
            setFloat (apvts, P::paramFilterEnv1Amount, 0.32f);
            setAdsr (apvts, P::paramFilterEnv2Attack, P::paramFilterEnv2Decay,
                     P::paramFilterEnv2Sustain, P::paramFilterEnv2Release,
                     240.0f, 1050.0f, 0.55f, 1400.0f);
            setFloat (apvts, P::paramFilterEnv2Amount, 0.28f);
            setFloat (apvts, P::paramGlide, 35.0f);
            setFloat (apvts, P::paramMasterGain, 0.66f);

            setBool (apvts, P::paramLfo1Enabled, true);
            setFloat (apvts, P::paramLfo1Rate, 0.22f);
            setChoice (apvts, P::paramLfo1Shape, 0);
            setFloat (apvts, P::paramLfo1Pwm, 0.62f);
            setFloat (apvts, P::paramLfo1Filter, 0.12f);

            setBool (apvts, P::paramLfo2Enabled, true);
            setFloat (apvts, P::paramLfo2Rate, 0.16f);
            setChoice (apvts, P::paramLfo2Shape, 1);
            setFloat (apvts, P::paramLfo2Filter, 0.14f);
            setFloat (apvts, P::paramLfo2Amp, 0.05f);

            setBool (apvts, P::paramChorusEnabled, true);
            setFloat (apvts, P::paramChorusRate, 0.5f);
            setFloat (apvts, P::paramChorusDepth, 0.7f);
            setFloat (apvts, P::paramChorusMix, 0.6f);

            setBool (apvts, P::paramDelayEnabled, true);
            setFloat (apvts, P::paramDelayTime, 380.0f);
            setFloat (apvts, P::paramDelayFeedback, 0.28f);
            setFloat (apvts, P::paramDelayDamping, 0.35f);
            setFloat (apvts, P::paramDelayMix, 0.18f);
            setBool (apvts, P::paramDelayPingPong, false);

            setBool (apvts, P::paramReverbEnabled, true);
            setFloat (apvts, P::paramReverbSize, 0.65f);
            setFloat (apvts, P::paramReverbDamping, 0.28f);
            setFloat (apvts, P::paramReverbWidth, 1.0f);
            setFloat (apvts, P::paramReverbMix, 0.34f);

            setFxOrder (apvts, 1, 2, 3, 0);
            break;

        default:
            break;
    }
}
