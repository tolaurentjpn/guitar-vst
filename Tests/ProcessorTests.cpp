#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <cmath>
#include <iostream>

namespace
{
    int testsRun = 0;
    int testsFailed = 0;

    void expectTrue (const char* name, bool condition)
    {
        ++testsRun;
        if (! condition)
        {
            ++testsFailed;
            std::cerr << "FAIL: " << name << '\n';
        }
    }

    void fillTone (juce::AudioBuffer<float>& buffer, float frequency, double sampleRate, int baseSample)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float sample = 0.15f * std::sin (juce::MathConstants<float>::twoPi * frequency
                                                   * static_cast<float> (baseSample + i)
                                                   / static_cast<float> (sampleRate));
            buffer.setSample (0, i, sample);
            if (buffer.getNumChannels() > 1)
                buffer.setSample (1, i, 0.0f);
        }
    }

    void fillSilence (juce::AudioBuffer<float>& buffer)
    {
        buffer.clear();
    }

    float bufferPeak (const juce::AudioBuffer<float>& buffer)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                peak = juce::jmax (peak, std::abs (buffer.getSample (ch, i)));
        return peak;
    }
}

int main()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;

    GuitarSynthAudioProcessor processor;
    processor.setPlayConfigDetails (2, 2, sampleRate, blockSize);
    processor.prepareToPlay (sampleRate, blockSize);

    expectTrue ("Processor has input channels configured", processor.getTotalNumInputChannels() == 2);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    for (int block = 0; block < 20; ++block)
    {
        fillTone (buffer, 220.0f, sampleRate, block * blockSize);
        processor.processBlock (buffer, midi);
    }

    expectTrue ("processBlock measures non-zero input peak",
                processor.getDisplayedInputPeak() > 0.05f);
    expectTrue ("Pitch tracker detects injected sine tone",
                processor.getDisplayedFrequency() > 200.0f
                && processor.getDisplayedFrequency() < 240.0f);

    {
        // Fresh processor so we test a never-open gate, not a note-off release tail.
        GuitarSynthAudioProcessor gatedProc;
        gatedProc.setPlayConfigDetails (2, 2, sampleRate, blockSize);
        gatedProc.prepareToPlay (sampleRate, blockSize);

        auto& apvts = gatedProc.getApvts();
        if (auto* gateParam = apvts.getParameter (GuitarSynthAudioProcessor::paramGateThreshold))
            gateParam->setValueNotifyingHost (gateParam->convertTo0to1 (0.0f));

        float outputPeak = 0.0f;
        for (int block = 0; block < 40; ++block)
        {
            fillTone (buffer, 220.0f, sampleRate, block * blockSize);
            gatedProc.processBlock (buffer, midi);
            outputPeak = juce::jmax (outputPeak, bufferPeak (buffer));
        }

        expectTrue ("Gate at 0 dB produces silence on instrument-level input", outputPeak < 1.0e-4f);
        expectTrue ("Gate at 0 dB stays closed on instrument-level input",
                    ! gatedProc.getDisplayedGateOpen());
    }

    // Fresh processor for release / transition tests (reset gate).
    {
        GuitarSynthAudioProcessor releaseProc;
        releaseProc.setPlayConfigDetails (2, 2, sampleRate, blockSize);
        releaseProc.prepareToPlay (sampleRate, blockSize);

        auto& apvts = releaseProc.getApvts();
        if (auto* releaseParam = apvts.getParameter (GuitarSynthAudioProcessor::paramRelease))
            releaseParam->setValueNotifyingHost (releaseParam->convertTo0to1 (300.0f));
        if (auto* gateParam = apvts.getParameter (GuitarSynthAudioProcessor::paramGateThreshold))
            gateParam->setValueNotifyingHost (gateParam->convertTo0to1 (-48.0f));

        for (int block = 0; block < 30; ++block)
        {
            fillTone (buffer, 220.0f, sampleRate, block * blockSize);
            releaseProc.processBlock (buffer, midi);
        }

        expectTrue ("Pre-gate-close note is sounding", bufferPeak (buffer) > 1.0e-3f);

        fillSilence (buffer);
        releaseProc.processBlock (buffer, midi);
        const float releasePeak = bufferPeak (buffer);
        expectTrue ("Gate close keeps audible release (not instantaneous hard mute)",
                    releasePeak > 1.0e-4f);
    }

    {
        GuitarSynthAudioProcessor transitionProc;
        transitionProc.setPlayConfigDetails (2, 2, sampleRate, blockSize);
        transitionProc.prepareToPlay (sampleRate, blockSize);

        for (int block = 0; block < 30; ++block)
        {
            fillTone (buffer, 220.0f, sampleRate, block * blockSize);
            transitionProc.processBlock (buffer, midi);
        }

        expectTrue ("Transition setup detects A3",
                    transitionProc.getDisplayedFrequency() > 190.0f
                    && transitionProc.getDisplayedFrequency() < 250.0f);

        float minPeakDuringTransition = 1.0f;
        for (int block = 0; block < 40; ++block)
        {
            fillTone (buffer, 110.0f, sampleRate, (30 + block) * blockSize);
            transitionProc.processBlock (buffer, midi);
            minPeakDuringTransition = juce::jmin (minPeakDuringTransition, bufferPeak (buffer));
        }

        expectTrue ("A3->A2 transition tracks near 110 Hz",
                    transitionProc.getDisplayedFrequency() > 95.0f
                    && transitionProc.getDisplayedFrequency() < 125.0f);
        expectTrue ("A3->A2 with gate open has no hard-mute silence gap",
                    minPeakDuringTransition > 1.0e-5f);
    }

    {
        GuitarSynthAudioProcessor dualOscProc;
        dualOscProc.setPlayConfigDetails (2, 2, sampleRate, blockSize);
        dualOscProc.prepareToPlay (sampleRate, blockSize);

        auto& apvts = dualOscProc.getApvts();
        if (auto* mixParam = apvts.getParameter (GuitarSynthAudioProcessor::paramOsc2Mix))
            mixParam->setValueNotifyingHost (mixParam->convertTo0to1 (0.5f));
        if (auto* detuneParam = apvts.getParameter (GuitarSynthAudioProcessor::paramOsc2Detune))
            detuneParam->setValueNotifyingHost (detuneParam->convertTo0to1 (50.0f));
        if (auto* osc2Cutoff = apvts.getParameter (GuitarSynthAudioProcessor::paramOsc2FilterCutoff))
            osc2Cutoff->setValueNotifyingHost (osc2Cutoff->convertTo0to1 (800.0f));
        if (auto* filterMod = apvts.getParameter (GuitarSynthAudioProcessor::paramLfo1Filter))
            filterMod->setValueNotifyingHost (filterMod->convertTo0to1 (0.4f));
        if (auto* resMod = apvts.getParameter (GuitarSynthAudioProcessor::paramLfo1Resonance))
            resMod->setValueNotifyingHost (resMod->convertTo0to1 (0.3f));
        if (auto* lfo2Filter = apvts.getParameter (GuitarSynthAudioProcessor::paramLfo2Filter))
            lfo2Filter->setValueNotifyingHost (lfo2Filter->convertTo0to1 (-0.3f));
        if (auto* osc2Attack = apvts.getParameter (GuitarSynthAudioProcessor::paramOsc2Attack))
            osc2Attack->setValueNotifyingHost (osc2Attack->convertTo0to1 (40.0f));
        if (auto* osc2Release = apvts.getParameter (GuitarSynthAudioProcessor::paramOsc2Release))
            osc2Release->setValueNotifyingHost (osc2Release->convertTo0to1 (400.0f));

        float dualPeak = 0.0f;
        for (int block = 0; block < 30; ++block)
        {
            fillTone (buffer, 220.0f, sampleRate, block * blockSize);
            dualOscProc.processBlock (buffer, midi);
            dualPeak = juce::jmax (dualPeak, bufferPeak (buffer));
        }

        expectTrue ("Dual osc + per-osc filters/LFOs produce audible output", dualPeak > 1.0e-3f);
        expectTrue ("Dual osc path still tracks pitch",
                    dualOscProc.getDisplayedFrequency() > 200.0f
                    && dualOscProc.getDisplayedFrequency() < 240.0f);
    }

    {
        GuitarSynthAudioProcessor unisonProc;
        unisonProc.setPlayConfigDetails (2, 2, sampleRate, blockSize);
        unisonProc.prepareToPlay (sampleRate, blockSize);

        auto& apvts = unisonProc.getApvts();
        if (auto* voices = apvts.getParameter (GuitarSynthAudioProcessor::paramOsc1UnisonVoices))
            voices->setValueNotifyingHost (voices->convertTo0to1 (7.0f));
        if (auto* detune = apvts.getParameter (GuitarSynthAudioProcessor::paramOsc1UnisonDetune))
            detune->setValueNotifyingHost (detune->convertTo0to1 (0.4f));
        if (auto* spread = apvts.getParameter (GuitarSynthAudioProcessor::paramOsc1UnisonSpread))
            spread->setValueNotifyingHost (spread->convertTo0to1 (1.0f));
        if (auto* mix = apvts.getParameter (GuitarSynthAudioProcessor::paramOsc2Mix))
            mix->setValueNotifyingHost (mix->convertTo0to1 (0.0f));

        float unisonPeak = 0.0f;
        float stereoDiff = 0.0f;
        for (int block = 0; block < 40; ++block)
        {
            fillTone (buffer, 220.0f, sampleRate, block * blockSize);
            unisonProc.processBlock (buffer, midi);
            unisonPeak = juce::jmax (unisonPeak, bufferPeak (buffer));

            float blockDiff = 0.0f;
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                blockDiff = juce::jmax (blockDiff,
                                        std::abs (buffer.getSample (0, i) - buffer.getSample (1, i)));
            stereoDiff = juce::jmax (stereoDiff, blockDiff);
        }

        expectTrue ("Unison voices with detune produce audible output", unisonPeak > 1.0e-3f);
        expectTrue ("Unison stereo spread yields L/R difference", stereoDiff > 1.0e-4f);
        expectTrue ("Unison path still tracks pitch",
                    unisonProc.getDisplayedFrequency() > 200.0f
                    && unisonProc.getDisplayedFrequency() < 240.0f);
    }

    {
        GuitarSynthAudioProcessor filterEnvProc;
        filterEnvProc.setPlayConfigDetails (2, 2, sampleRate, blockSize);
        filterEnvProc.prepareToPlay (sampleRate, blockSize);

        auto& apvts = filterEnvProc.getApvts();
        if (auto* cutoff = apvts.getParameter (GuitarSynthAudioProcessor::paramFilterCutoff))
            cutoff->setValueNotifyingHost (cutoff->convertTo0to1 (200.0f));
        if (auto* amount = apvts.getParameter (GuitarSynthAudioProcessor::paramFilterEnv1Amount))
            amount->setValueNotifyingHost (amount->convertTo0to1 (0.9f));
        if (auto* attack = apvts.getParameter (GuitarSynthAudioProcessor::paramFilterEnv1Attack))
            attack->setValueNotifyingHost (attack->convertTo0to1 (5.0f));
        if (auto* decay = apvts.getParameter (GuitarSynthAudioProcessor::paramFilterEnv1Decay))
            decay->setValueNotifyingHost (decay->convertTo0to1 (400.0f));
        if (auto* mix = apvts.getParameter (GuitarSynthAudioProcessor::paramOsc2Mix))
            mix->setValueNotifyingHost (mix->convertTo0to1 (0.0f));

        float filterEnvPeak = 0.0f;
        for (int block = 0; block < 40; ++block)
        {
            fillTone (buffer, 220.0f, sampleRate, block * blockSize);
            filterEnvProc.processBlock (buffer, midi);
            filterEnvPeak = juce::jmax (filterEnvPeak, bufferPeak (buffer));
        }

        expectTrue ("Filter env amount keeps patch audible under low cutoff", filterEnvPeak > 1.0e-3f);
    }

    {
        GuitarSynthAudioProcessor presetProc;
        presetProc.setPlayConfigDetails (2, 2, sampleRate, blockSize);
        presetProc.prepareToPlay (sampleRate, blockSize);

        expectTrue ("Factory preset bank has programs", presetProc.getNumPrograms() >= 10);
        expectTrue ("Init program name is set", presetProc.getProgramName (0) == "Init");
        expectTrue ("SuperSaw program name is set", presetProc.getProgramName (4) == "SuperSaw");

        const float gateBefore = presetProc.getApvts().getRawParameterValue (
            GuitarSynthAudioProcessor::paramGateThreshold)->load();
        const float trackBefore = presetProc.getApvts().getRawParameterValue (
            GuitarSynthAudioProcessor::paramTrackingSensitivity)->load();

        presetProc.setCurrentProgram (4); // SuperSaw

        expectTrue ("SuperSaw sets Osc1 voices > 1",
                    presetProc.getApvts().getRawParameterValue (
                        GuitarSynthAudioProcessor::paramOsc1UnisonVoices)->load() > 1.5f);
        expectTrue ("Preset load preserves Gate",
                    std::abs (presetProc.getApvts().getRawParameterValue (
                        GuitarSynthAudioProcessor::paramGateThreshold)->load() - gateBefore) < 0.01f);
        expectTrue ("Preset load preserves Tracking",
                    std::abs (presetProc.getApvts().getRawParameterValue (
                        GuitarSynthAudioProcessor::paramTrackingSensitivity)->load() - trackBefore) < 0.01f);

        float presetPeak = 0.0f;
        for (int block = 0; block < 30; ++block)
        {
            fillTone (buffer, 220.0f, sampleRate, block * blockSize);
            presetProc.processBlock (buffer, midi);
            presetPeak = juce::jmax (presetPeak, bufferPeak (buffer));
        }

        expectTrue ("SuperSaw preset produces audible output", presetPeak > 1.0e-3f);
    }

    {
        GuitarSynthAudioProcessor fxProc;
        fxProc.setPlayConfigDetails (2, 2, sampleRate, blockSize);
        fxProc.prepareToPlay (sampleRate, blockSize);

        expectTrue ("FX bypassed reports near-zero tail", fxProc.getTailLengthSeconds() < 0.01);

        auto& apvts = fxProc.getApvts();
        if (auto* delayOn = apvts.getParameter (GuitarSynthAudioProcessor::paramDelayEnabled))
            delayOn->setValueNotifyingHost (1.0f);
        if (auto* delayMix = apvts.getParameter (GuitarSynthAudioProcessor::paramDelayMix))
            delayMix->setValueNotifyingHost (delayMix->convertTo0to1 (0.5f));
        if (auto* delayTime = apvts.getParameter (GuitarSynthAudioProcessor::paramDelayTime))
            delayTime->setValueNotifyingHost (delayTime->convertTo0to1 (400.0f));
        if (auto* delayFb = apvts.getParameter (GuitarSynthAudioProcessor::paramDelayFeedback))
            delayFb->setValueNotifyingHost (delayFb->convertTo0to1 (0.5f));

        // Push params through one process call
        fillTone (buffer, 220.0f, sampleRate, 0);
        fxProc.processBlock (buffer, midi);
        expectTrue ("Delay enabled reports non-zero tail", fxProc.getTailLengthSeconds() > 0.1);

        // Reorder: Delay first
        if (auto* o0 = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (GuitarSynthAudioProcessor::paramFxOrder0)))
            o0->setValueNotifyingHost (o0->convertTo0to1 (2.0f));
        if (auto* o1 = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (GuitarSynthAudioProcessor::paramFxOrder1)))
            o1->setValueNotifyingHost (o1->convertTo0to1 (0.0f));
        if (auto* o2 = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (GuitarSynthAudioProcessor::paramFxOrder2)))
            o2->setValueNotifyingHost (o2->convertTo0to1 (1.0f));
        if (auto* o3 = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (GuitarSynthAudioProcessor::paramFxOrder3)))
            o3->setValueNotifyingHost (o3->convertTo0to1 (3.0f));

        float fxPeak = 0.0f;
        for (int block = 0; block < 30; ++block)
        {
            fillTone (buffer, 220.0f, sampleRate, block * blockSize);
            fxProc.processBlock (buffer, midi);
            fxPeak = juce::jmax (fxPeak, bufferPeak (buffer));
        }
        expectTrue ("FX chain with delay reorder stays audible", fxPeak > 1.0e-3f);

        // Distortion alone should not crash / silence
        if (auto* distOn = apvts.getParameter (GuitarSynthAudioProcessor::paramDistEnabled))
            distOn->setValueNotifyingHost (1.0f);
        if (auto* delayOff = apvts.getParameter (GuitarSynthAudioProcessor::paramDelayEnabled))
            delayOff->setValueNotifyingHost (0.0f);

        float distPeak = 0.0f;
        for (int block = 0; block < 20; ++block)
        {
            fillTone (buffer, 220.0f, sampleRate, block * blockSize);
            fxProc.processBlock (buffer, midi);
            distPeak = juce::jmax (distPeak, bufferPeak (buffer));
        }
        expectTrue ("Distortion enabled stays audible", distPeak > 1.0e-3f);
    }

    std::cout << "Input peak: " << processor.getDisplayedInputPeak()
              << ", frequency: " << processor.getDisplayedFrequency()
              << ", input channels: " << processor.getTotalNumInputChannels() << '\n';
    std::cout << testsRun << " tests run, " << testsFailed << " failed\n";
    return testsFailed == 0 ? 0 : 1;
}
