#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <vector>

namespace
{
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            GuitarSynthAudioProcessor::paramWaveform,
            "Waveform",
            juce::StringArray { "Sine", "Saw", "Square" },
            1));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterCutoff,
            "Filter Cutoff",
            juce::NormalisableRange<float> (80.0f, 8000.0f, 0.01f, 0.35f),
            2200.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramFilterResonance,
            "Filter Resonance",
            juce::NormalisableRange<float> (0.1f, 2.0f, 0.001f, 0.5f),
            0.85f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramAttack,
            "Attack",
            juce::NormalisableRange<float> (1.0f, 200.0f, 0.1f, 0.4f),
            8.0f,
            "ms"));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramDecay,
            "Decay",
            juce::NormalisableRange<float> (10.0f, 1000.0f, 0.1f, 0.4f),
            180.0f,
            "ms"));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramSustain,
            "Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
            0.75f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramRelease,
            "Release",
            juce::NormalisableRange<float> (10.0f, 2000.0f, 0.1f, 0.4f),
            220.0f,
            "ms"));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramGlide,
            "Glide",
            juce::NormalisableRange<float> (0.0f, 300.0f, 0.1f, 0.45f),
            25.0f,
            "ms"));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramMasterGain,
            "Master",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
            0.8f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramTrackingSensitivity,
            "Tracking",
            juce::NormalisableRange<float> (0.1f, 0.99f, 0.001f),
            0.55f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramGateThreshold,
            "Gate",
            juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
            -58.0f,
            "dB"));

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

void GuitarSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 1;

    pitchTracker.prepare (sampleRate);
    envelopeFollower.prepare (sampleRate);
    synthEngine.prepare (sampleRate, samplesPerBlock);

    highPassFilter.prepare (spec);
    highPassFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 80.0);

    updateRealtimeParameters();
    setLatencySamples (pitchTracker.getLatencySamples());
    displayedLatencyMs.store (1000.0 * static_cast<double> (pitchTracker.getLatencySamples()) / sampleRate);
}

void GuitarSynthAudioProcessor::releaseResources() {}

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
    synthEngine.setWaveform (static_cast<WaveformType> (juce::jlimit (0, 2, waveformIndex)));
    synthEngine.setFilterCutoff (*apvts.getRawParameterValue (paramFilterCutoff));
    synthEngine.setFilterResonance (*apvts.getRawParameterValue (paramFilterResonance));
    synthEngine.setAttackMs (*apvts.getRawParameterValue (paramAttack));
    synthEngine.setDecayMs (*apvts.getRawParameterValue (paramDecay));
    synthEngine.setSustainLevel (*apvts.getRawParameterValue (paramSustain));
    synthEngine.setReleaseMs (*apvts.getRawParameterValue (paramRelease));
    synthEngine.setGlideMs (*apvts.getRawParameterValue (paramGlide));
    synthEngine.setMasterGain (*apvts.getRawParameterValue (paramMasterGain));

    pitchTracker.setConfidenceThreshold (*apvts.getRawParameterValue (paramTrackingSensitivity));
    pitchTracker.setSmoothing (0.15f + (1.0f - *apvts.getRawParameterValue (paramTrackingSensitivity)) * 0.6f);

    envelopeFollower.setAttackMs (2.0f);
    envelopeFollower.setReleaseMs (80.0f);
    envelopeFollower.setGateThreshold (*apvts.getRawParameterValue (paramGateThreshold));
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

    std::vector<float> gateEnvelope (static_cast<size_t> (numSamples));

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : left;

    for (int i = 0; i < numSamples; ++i)
    {
        const float hpSample = highPassFilter.processSample (inputSamples[static_cast<size_t> (i)]);
        pitchTracker.pushSample (hpSample);
        gateEnvelope[static_cast<size_t> (i)] = envelopeFollower.processSample (hpSample);

        synthEngine.setPitchState (pitchTracker.getFrequency(), pitchTracker.isVoiced());
        const float sample = synthEngine.processSample (gateEnvelope[static_cast<size_t> (i)]);
        left[i] = sample;
        right[i] = sample;
    }

    displayedFrequency.store (pitchTracker.getFrequency());
    displayedConfidence.store (pitchTracker.getConfidence());
    displayedVoiced.store (pitchTracker.isVoiced());
    displayedLatencyMs.store (1000.0 * static_cast<double> (getLatencySamples()) / getSampleRate());
}

juce::AudioProcessorEditor* GuitarSynthAudioProcessor::createEditor()
{
    return new GuitarSynthAudioProcessorEditor (*this);
}

void GuitarSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState().createXml())
        copyXmlToBinary (*state, destData);
}

void GuitarSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GuitarSynthAudioProcessor();
}
