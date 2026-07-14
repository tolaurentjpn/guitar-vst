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

    std::cout << "Input peak: " << processor.getDisplayedInputPeak()
              << ", frequency: " << processor.getDisplayedFrequency()
              << ", input channels: " << processor.getTotalNumInputChannels() << '\n';
    std::cout << testsRun << " tests run, " << testsFailed << " failed\n";
    return testsFailed == 0 ? 0 : 1;
}
