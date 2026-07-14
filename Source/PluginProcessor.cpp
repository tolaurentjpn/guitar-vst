#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <vector>

namespace
{
    void writeSampleToAllChannels (juce::AudioBuffer<float>& buffer, int sampleIndex, float sample) noexcept
    {
        const int numChannels = buffer.getNumChannels();
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample (ch, sampleIndex, sample);
    }

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
            0.35f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            GuitarSynthAudioProcessor::paramGateThreshold,
            "Gate",
            juce::NormalisableRange<float> (-80.0f, 0.0f, 0.1f),
            -48.0f,
            "dB"));

        return { params.begin(), params.end() };
    }
}

void GuitarSynthAudioProcessor::requestOutputTestTone (double seconds) noexcept
{
    const double sr = getSampleRate() > 0.0 ? getSampleRate() : 48000.0;
    const double totalSeconds = juce::jmax (0.4, seconds);
    const int64_t totalSamples = static_cast<int64_t> (totalSeconds * sr);
    // First half: raw sine. Second half: SynthEngine (verifies filter/ADSR path).
    testToneUntilSample = totalSamplesProcessed + totalSamples / 2;
    testSynthUntilSample = totalSamplesProcessed + totalSamples;
    testTonePhase = 0.0;
    synthEngine.muteImmediately();
}

bool GuitarSynthAudioProcessor::isOutputTestToneActive() const noexcept
{
    return totalSamplesProcessed < testSynthUntilSample;
}

bool GuitarSynthAudioProcessor::isForcedSynthTestActive() const noexcept
{
    return totalSamplesProcessed >= testToneUntilSample
        && totalSamplesProcessed < testSynthUntilSample;
}

juce::String GuitarSynthAudioProcessor::getBusLayoutDescription() const
{
    const auto inSet = getBusesLayout().getMainInputChannelSet();
    const auto outSet = getBusesLayout().getMainOutputChannelSet();
    const auto describe = [] (const juce::AudioChannelSet& set) -> juce::String
    {
        if (set == juce::AudioChannelSet::disabled())
            return "off";
        if (set == juce::AudioChannelSet::mono())
            return "mono";
        if (set == juce::AudioChannelSet::stereo())
            return "stereo";
        return juce::String (set.size()) + "ch";
    };

    return describe (inSet) + "→" + describe (outSet)
         + " (" + juce::String (getTotalNumInputChannels()) + "i/"
         + juce::String (getTotalNumOutputChannels()) + "o)";
}

GuitarSynthAudioProcessor::GuitarSynthAudioProcessor()
    : AudioProcessor (BusesProperties()
                          // Guitar is mono. Prefer mono-in → stereo-out so CoreAudio maps
                          // cleanly to Audient Main L/R (headphones), not a 4-ch discrete bus.
                          .withInput ("Input", juce::AudioChannelSet::mono(), true)
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

    gateHighPassFilter.prepare (spec);
    gateHighPassFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 80.0);
    inputPreRoll.assign (static_cast<size_t> (pitchTracker.getWindowSize()), 0.0f);
    preRollWriteIndex = 0;
    gateWasOpen = false;
    gateClosedSampleCount = 0;
    totalSamplesProcessed = 0;
    testToneUntilSample = 0;
    testSynthUntilSample = 0;
    testTonePhase = 0.0;

    updateRealtimeParameters();
    setLatencySamples (pitchTracker.getLatencySamples());
    displayedLatencyMs.store (1000.0 * static_cast<double> (pitchTracker.getLatencySamples()) / sampleRate);
}

void GuitarSynthAudioProcessor::releaseResources() {}

bool GuitarSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const bool outOk = out == juce::AudioChannelSet::mono()
                    || out == juce::AudioChannelSet::stereo()
                    || out == juce::AudioChannelSet::discreteChannels (4)
                    || out == juce::AudioChannelSet::quadraphonic();
    if (! outOk)
        return false;

    const auto& in = layouts.getMainInputChannelSet();
    if (in == juce::AudioChannelSet::disabled())
        return true;

    // Accept mono or stereo input (DI may appear as either depending on device settings).
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
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
    envelopeFollower.setReleaseMs (180.0f);
    envelopeFollower.setGateThreshold (*apvts.getRawParameterValue (paramGateThreshold));
}

void GuitarSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    updateRealtimeParameters();

    const int numSamples = buffer.getNumSamples();
    const int numInputChannels = getTotalNumInputChannels();
    const int numBufferChannels = buffer.getNumChannels();
    const bool playTestTone = totalSamplesProcessed < testToneUntilSample;
    const bool playForcedSynth = ! playTestTone && totalSamplesProcessed < testSynthUntilSample;
    const double sampleRate = getSampleRate() > 0.0 ? getSampleRate() : 48000.0;
    const double testTonePhaseDelta = juce::MathConstants<double>::twoPi * 440.0 / sampleRate;

    std::vector<float> inputSamples (static_cast<size_t> (numSamples), 0.0f);
    float inputPeak = 0.0f;
    float inputPeakCh0 = 0.0f;
    float inputPeakCh1 = 0.0f;

    // Scan every channel present in the shared IO buffer before we clear it.
    // With mono→stereo layouts JUCE may still park device DI on channel 0 or 1.
    if (numInputChannels > 0 && numBufferChannels > 0)
    {
        const int channelsToScan = juce::jmin (numBufferChannels,
                                               juce::jmax (numInputChannels, 2));

        for (int i = 0; i < numSamples; ++i)
        {
            float sample = 0.0f;
            for (int ch = 0; ch < channelsToScan; ++ch)
            {
                const float chSample = buffer.getSample (ch, i);
                if (ch == 0)
                    inputPeakCh0 = juce::jmax (inputPeakCh0, std::abs (chSample));
                else if (ch == 1)
                    inputPeakCh1 = juce::jmax (inputPeakCh1, std::abs (chSample));

                if (std::abs (chSample) > std::abs (sample))
                    sample = chSample;
            }

            inputSamples[static_cast<size_t> (i)] = sample;
            inputPeak = juce::jmax (inputPeak, std::abs (sample));
        }
    }

    displayedInputPeak.store (inputPeak);
    displayedInputPeakCh0.store (inputPeakCh0);
    displayedInputPeakCh1.store (inputPeakCh1);

    buffer.clear();

    if (numBufferChannels <= 0)
    {
        totalSamplesProcessed += numSamples;
        return;
    }

    totalSamplesProcessed += numSamples;

    // Diagnostic tones bypass gate/pitch so users can verify speakers/headphones.
    if (playTestTone || playForcedSynth)
    {
        float outputPeak = 0.0f;
        double outputSquareSum = 0.0;

        if (playTestTone)
        {
            synthEngine.muteImmediately();

            for (int i = 0; i < numSamples; ++i)
            {
                const float sample = 0.35f * static_cast<float> (std::sin (testTonePhase));
                testTonePhase += testTonePhaseDelta;
                if (testTonePhase >= juce::MathConstants<double>::twoPi)
                    testTonePhase -= juce::MathConstants<double>::twoPi;

                writeSampleToAllChannels (buffer, i, sample);
                outputPeak = juce::jmax (outputPeak, std::abs (sample));
                outputSquareSum += static_cast<double> (sample) * static_cast<double> (sample);
            }

            displayedFrequency.store (440.0f);
        }
        else
        {
            // Second half: drive the real SynthEngine so filter/ADSR/Master are verified.
            for (int i = 0; i < numSamples; ++i)
            {
                synthEngine.setPitchState (220.0f, true);
                const float sample = synthEngine.processSample();
                writeSampleToAllChannels (buffer, i, sample);
                outputPeak = juce::jmax (outputPeak, std::abs (sample));
                outputSquareSum += static_cast<double> (sample) * static_cast<double> (sample);
            }

            displayedFrequency.store (220.0f);
        }

        displayedConfidence.store (1.0f);
        displayedVoiced.store (true);
        displayedOutputPeak.store (outputPeak);
        displayedOutputRms.store (numSamples > 0
                                      ? static_cast<float> (std::sqrt (outputSquareSum / numSamples))
                                      : 0.0f);
        displayedGateOpen.store (envelopeFollower.isGateOpen());
        displayedGateEnvelopeDb.store (envelopeFollower.getEnvelopeDb());
        displayedLatencyMs.store (1000.0 * static_cast<double> (getLatencySamples()) / sampleRate);
        return;
    }

    constexpr int gateCloseClearSamples = 3840; // ~80 ms at 48 kHz
    float outputPeak = 0.0f;
    double outputSquareSum = 0.0;
    bool trackingActive = false;

    for (int i = 0; i < numSamples; ++i)
    {
        const float rawSample = inputSamples[static_cast<size_t> (i)];

        const float gateSample = gateHighPassFilter.processSample (rawSample);
        envelopeFollower.processSample (gateSample);
        const bool gateOpen = envelopeFollower.isGateOpen();

        pitchLevelEstimate = juce::jmax (pitchLevelEstimate * 0.9995f, std::abs (rawSample));
        const float pitchGain = juce::jlimit (1.0f, 12.0f, 0.08f / juce::jmax (pitchLevelEstimate, 1.0e-4f));
        const float gainedSample = rawSample * pitchGain;

        // Keep pitch analysis warm while gated so notes lock faster on attack.
        pitchTracker.pushSample (gainedSample);
        inputPreRoll[static_cast<size_t> (preRollWriteIndex)] = gainedSample;
        preRollWriteIndex = (preRollWriteIndex + 1) % static_cast<int> (inputPreRoll.size());

        if (! gateOpen)
        {
            if (gateWasOpen)
                gateClosedSampleCount = 0;

            ++gateClosedSampleCount;
            gateWasOpen = false;
            gateOpenSampleCount = 0;

            if (gateClosedSampleCount >= gateCloseClearSamples)
            {
                pitchTracker.flush();
                latchedPitchHz = 0.0f;
                pitchLevelEstimate = 0.0f;
                gateClosedSampleCount = 0;
                synthEngine.muteImmediately();
            }
            else
            {
                // Brief gate dips use ADSR release — hard-muting every closed sample
                // was zeroing the envelope and leaving only sparse attack clicks.
                synthEngine.setPitchState (0.0f, false);
            }

            const float sample = synthEngine.processSample();
            writeSampleToAllChannels (buffer, i, sample);
            outputPeak = juce::jmax (outputPeak, std::abs (sample));
            outputSquareSum += static_cast<double> (sample) * static_cast<double> (sample);
            continue;
        }

        gateWasOpen = true;
        gateClosedSampleCount = 0;
        ++gateOpenSampleCount;

        const float frequency = pitchTracker.getFrequency();
        const float candidateHz = pitchTracker.getCandidateFrequency();
        const float confidence = pitchTracker.getConfidence();
        const float candidateConf = pitchTracker.getCandidateConfidence();
        const float minConfidence = pitchTracker.getMinConfidenceThreshold();

        const bool reliablePitch = pitchTracker.isVoiced()
                                && frequency > 70.0f
                                && confidence >= minConfidence * 0.65f;

        float synthHz = 0.0f;

        if (reliablePitch)
            synthHz = frequency;
        else if (frequency > 70.0f && confidence >= minConfidence * 0.4f)
            synthHz = frequency;
        else if (candidateHz > 70.0f && candidateConf >= minConfidence * 0.35f)
            synthHz = candidateHz;
        else if (latchedPitchHz > 70.0f && (pitchTracker.isVoiced() || confidence >= minConfidence * 0.25f))
            synthHz = latchedPitchHz;

        if (reliablePitch || (frequency > 70.0f && confidence >= minConfidence * 0.5f))
            latchedPitchHz = frequency > 70.0f ? frequency : synthHz;

        // Gate open + usable pitch → sound. Do not require per-sample amplitude
        // (zero crossings used to briefly unvoice and retrigger/silence the synth).
        trackingActive = synthHz > 70.0f
                      && (reliablePitch
                          || pitchTracker.isVoiced()
                          || confidence >= minConfidence * 0.35f
                          || candidateConf >= minConfidence * 0.35f
                          || latchedPitchHz > 70.0f);

        const float outputHz = trackingActive ? synthHz : 0.0f;
        synthEngine.setPitchState (outputHz, trackingActive);
        const float sample = synthEngine.processSample();
        writeSampleToAllChannels (buffer, i, sample);
        outputPeak = juce::jmax (outputPeak, std::abs (sample));
        outputSquareSum += static_cast<double> (sample) * static_cast<double> (sample);
    }

    const float displayHz = pitchTracker.getFrequency() > 70.0f
                              ? pitchTracker.getFrequency()
                              : (latchedPitchHz > 70.0f ? latchedPitchHz : pitchTracker.getCandidateFrequency());

    displayedFrequency.store (displayHz);
    displayedConfidence.store (pitchTracker.getConfidence());
    displayedVoiced.store (trackingActive);
    displayedInputPeak.store (inputPeak);
    displayedOutputPeak.store (outputPeak);
    displayedOutputRms.store (numSamples > 0
                                  ? static_cast<float> (std::sqrt (outputSquareSum / numSamples))
                                  : 0.0f);
    displayedGateOpen.store (envelopeFollower.isGateOpen());
    displayedGateEnvelopeDb.store (envelopeFollower.getEnvelopeDb());
    displayedLatencyMs.store (1000.0 * static_cast<double> (getLatencySamples()) / sampleRate);
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
