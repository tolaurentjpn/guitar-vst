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

        float outputPeak = 0.0f;
        for (int block = 0; block < 40; ++block)
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

    std::cout << "Input peak: " << processor.getDisplayedInputPeak()
              << ", frequency: " << processor.getDisplayedFrequency()
              << ", input channels: " << processor.getTotalNumInputChannels() << '\n';
    std::cout << testsRun << " tests run, " << testsFailed << " failed\n";
    return testsFailed == 0 ? 0 : 1;
}
