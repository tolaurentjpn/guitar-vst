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
}

int main()
{
    GuitarSynthAudioProcessor processor;
    processor.setPlayConfigDetails (2, 2, 48000.0, 512);
    processor.prepareToPlay (48000.0, 512);

    expectTrue ("Processor has input channels configured", processor.getTotalNumInputChannels() == 2);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;

    for (int block = 0; block < 20; ++block)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float sample = 0.15f * std::sin (juce::MathConstants<float>::twoPi * 220.0f
                                                     * static_cast<float> (block * buffer.getNumSamples() + i)
                                                     / 48000.0f);
            buffer.setSample (0, i, sample);
            buffer.setSample (1, i, 0.0f);
        }

        processor.processBlock (buffer, midi);
    }

    expectTrue ("processBlock measures non-zero input peak",
                processor.getDisplayedInputPeak() > 0.05f);
    expectTrue ("Pitch tracker detects injected sine tone",
                processor.getDisplayedFrequency() > 200.0f
                && processor.getDisplayedFrequency() < 240.0f);

    {
        auto& apvts = processor.getApvts();
        if (auto* gateParam = apvts.getParameter (GuitarSynthAudioProcessor::paramGateThreshold))
            gateParam->setValueNotifyingHost (gateParam->convertTo0to1 (0.0f));

        // Soft gate-close uses ADSR release; wait it out before asserting silence.
        for (int block = 0; block < 80; ++block)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const float sample = 0.15f * std::sin (juce::MathConstants<float>::twoPi * 220.0f
                                                         * static_cast<float> (block * buffer.getNumSamples() + i)
                                                         / 48000.0f);
                buffer.setSample (0, i, sample);
                buffer.setSample (1, i, 0.0f);
            }

            processor.processBlock (buffer, midi);
        }

        float outputPeak = 0.0f;
        for (int block = 0; block < 20; ++block)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const float sample = 0.15f * std::sin (juce::MathConstants<float>::twoPi * 220.0f
                                                         * static_cast<float> (block * buffer.getNumSamples() + i)
                                                         / 48000.0f);
                buffer.setSample (0, i, sample);
                buffer.setSample (1, i, 0.0f);
            }

            processor.processBlock (buffer, midi);

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                    outputPeak = juce::jmax (outputPeak, std::abs (buffer.getSample (ch, i)));
        }

        expectTrue ("Gate at 0 dB produces silence on instrument-level input", outputPeak < 1.0e-4f);
        expectTrue ("Gate at 0 dB stays closed on instrument-level input",
                    ! processor.getDisplayedGateOpen());
    }

    {
        auto& apvts = processor.getApvts();
        if (auto* gateParam = apvts.getParameter (GuitarSynthAudioProcessor::paramGateThreshold))
            gateParam->setValueNotifyingHost (gateParam->convertTo0to1 (-80.0f));

        float outputPeak = 0.0f;
        for (int block = 0; block < 200; ++block)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const float sample = 0.3f * std::sin (juce::MathConstants<float>::twoPi * 220.0f
                                                         * static_cast<float> (block * buffer.getNumSamples() + i)
                                                         / 48000.0f);
                buffer.setSample (0, i, sample);
                buffer.setSample (1, i, 0.0f);
            }

            processor.processBlock (buffer, midi);

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                    outputPeak = juce::jmax (outputPeak, std::abs (buffer.getSample (ch, i)));
        }

        expectTrue ("Synth produces audible output when gate opens and pitch is tracked",
                    outputPeak > 0.01f);
        expectTrue ("Pitch is tracked with permissive gate",
                    processor.getDisplayedFrequency() > 200.0f
                    && processor.getDisplayedFrequency() < 240.0f);
        std::cout << "Synth output peak: " << outputPeak << '\n';
    }

    {
        auto& apvts = processor.getApvts();
        if (auto* gateParam = apvts.getParameter (GuitarSynthAudioProcessor::paramGateThreshold))
            gateParam->setValueNotifyingHost (gateParam->convertTo0to1 (-48.0f));

        float outputPeak = 0.0f;
        int sampleCounter = 0;

        for (int block = 0; block < 120; ++block)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const int n = sampleCounter++;
                const float t = static_cast<float> (n) / 48000.0f;
                const float env = std::exp (-t * 2.5f);
                const float phase = juce::MathConstants<float>::twoPi * 220.0f * t;
                const float sample = env * (0.45f * std::sin (phase)
                                            + 0.35f * std::sin (2.0f * phase)
                                            + 0.12f * std::sin (3.0f * phase));
                buffer.setSample (0, i, sample);
                buffer.setSample (1, i, 0.0f);
            }

            processor.processBlock (buffer, midi);

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                    outputPeak = juce::jmax (outputPeak, std::abs (buffer.getSample (ch, i)));
        }

        expectTrue ("Plucked guitar-like tone produces synth output with default gate",
                    outputPeak > 0.01f);
        expectTrue ("Plucked guitar-like tone is pitch-tracked",
                    processor.getDisplayedFrequency() > 190.0f
                    && processor.getDisplayedFrequency() < 250.0f);
        std::cout << "Plucked guitar output peak: " << outputPeak << '\n';
    }

    {
        auto& apvts = processor.getApvts();
        if (auto* gateParam = apvts.getParameter (GuitarSynthAudioProcessor::paramGateThreshold))
            gateParam->setValueNotifyingHost (gateParam->convertTo0to1 (-80.0f));

        juce::AudioBuffer<float> silentBuffer (2, 512);
        silentBuffer.clear();
        float outputPeak = 0.0f;
        int sampleCounter = 0;

        for (int block = 0; block < 8; ++block)
            processor.processBlock (silentBuffer, midi);

        for (int block = 0; block < 40; ++block)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const int n = sampleCounter++;
                const float t = static_cast<float> (n) / 48000.0f;
                const float env = std::exp (-t * 4.0f);
                const float phase = juce::MathConstants<float>::twoPi * 220.0f * t;
                const float sample = env * (0.5f * std::sin (phase) + 0.3f * std::sin (2.0f * phase));
                buffer.setSample (0, i, sample);
                buffer.setSample (1, i, 0.0f);
            }

            processor.processBlock (buffer, midi);

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                    outputPeak = juce::jmax (outputPeak, std::abs (buffer.getSample (ch, i)));
        }

        expectTrue ("Warm pitch tracker produces synth output soon after note onset",
                    outputPeak > 0.01f);
        std::cout << "Warm tracker onset output peak: " << outputPeak << '\n';
    }

    {
        processor.requestOutputTestTone (0.5);
        float sinePeak = 0.0f;
        float synthPeak = 0.0f;
        bool sawForcedSynthPhase = false;

        for (int block = 0; block < 50; ++block)
        {
            buffer.clear();
            const bool forced = processor.isForcedSynthTestActive();
            processor.processBlock (buffer, midi);

            float blockPeak = 0.0f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                    blockPeak = juce::jmax (blockPeak, std::abs (buffer.getSample (ch, i)));

            if (forced)
            {
                sawForcedSynthPhase = true;
                synthPeak = juce::jmax (synthPeak, blockPeak);
            }
            else if (processor.isOutputTestToneActive())
            {
                sinePeak = juce::jmax (sinePeak, blockPeak);
            }
        }

        expectTrue ("Output test tone phase produces audible sine", sinePeak > 0.1f);
        expectTrue ("Forced SynthEngine test phase runs", sawForcedSynthPhase);
        expectTrue ("Forced SynthEngine test produces audible audio", synthPeak > 0.05f);
        std::cout << "Test tone sine peak: " << sinePeak
                  << ", forced synth peak: " << synthPeak << '\n';
    }

    {
        // 4-channel device layout: synth must appear on Main and Loop-back pairs.
        GuitarSynthAudioProcessor quadProcessor;
        quadProcessor.setPlayConfigDetails (2, 4, 48000.0, 256);
        quadProcessor.prepareToPlay (48000.0, 256);

        auto& apvts = quadProcessor.getApvts();
        if (auto* gateParam = apvts.getParameter (GuitarSynthAudioProcessor::paramGateThreshold))
            gateParam->setValueNotifyingHost (gateParam->convertTo0to1 (-80.0f));

        juce::AudioBuffer<float> quadBuffer (4, 256);
        juce::MidiBuffer quadMidi;
        float peakMain = 0.0f;
        float peakLoop = 0.0f;
        double squareSum = 0.0;
        int sampleCount = 0;

        for (int block = 0; block < 120; ++block)
        {
            for (int i = 0; i < quadBuffer.getNumSamples(); ++i)
            {
                const float sample = 0.35f * std::sin (juce::MathConstants<float>::twoPi * 220.0f
                                                         * static_cast<float> (block * quadBuffer.getNumSamples() + i)
                                                         / 48000.0f);
                quadBuffer.setSample (0, i, sample);
                quadBuffer.setSample (1, i, 0.0f);
                quadBuffer.setSample (2, i, 0.0f);
                quadBuffer.setSample (3, i, 0.0f);
            }

            quadProcessor.processBlock (quadBuffer, quadMidi);

            for (int i = 0; i < quadBuffer.getNumSamples(); ++i)
            {
                const float l = quadBuffer.getSample (0, i);
                const float r = quadBuffer.getSample (1, i);
                const float l2 = quadBuffer.getSample (2, i);
                const float r2 = quadBuffer.getSample (3, i);
                peakMain = juce::jmax (peakMain, std::abs (l), std::abs (r));
                peakLoop = juce::jmax (peakLoop, std::abs (l2), std::abs (r2));
                squareSum += static_cast<double> (l) * static_cast<double> (l);
                ++sampleCount;
            }
        }

        const float rms = sampleCount > 0 ? static_cast<float> (std::sqrt (squareSum / sampleCount)) : 0.0f;
        expectTrue ("4-channel layout produces main-out audio", peakMain > 0.05f);
        expectTrue ("4-channel layout mirrors audio to loop-back outs", peakLoop > 0.05f);
        expectTrue ("Sustained note keeps meaningful RMS (not just click peaks)", rms > 0.02f);
        expectTrue ("Displayed RMS tracks audible output",
                    quadProcessor.getDisplayedOutputRms() > 0.01f);
        std::cout << "4ch main peak: " << peakMain
                  << ", loop peak: " << peakLoop
                  << ", rms: " << rms
                  << ", displayed rms: " << quadProcessor.getDisplayedOutputRms() << '\n';
    }

    std::cout << "Input peak: " << processor.getDisplayedInputPeak()
              << ", frequency: " << processor.getDisplayedFrequency()
              << ", input channels: " << processor.getTotalNumInputChannels() << '\n';
    std::cout << testsRun << " tests run, " << testsFailed << " failed\n";
    return testsFailed == 0 ? 0 : 1;
}
