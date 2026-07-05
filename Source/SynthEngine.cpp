#include "SynthEngine.h"

namespace
{
    float waveformFunction (WaveformType type, float phase)
    {
        switch (type)
        {
            case WaveformType::sine:
                return std::sin (phase);
            case WaveformType::saw:
                return 2.0f * phase / juce::MathConstants<float>::twoPi - 1.0f;
            case WaveformType::square:
            default:
                return phase < juce::MathConstants<float>::pi ? 1.0f : -1.0f;
        }
    }
}

void SynthEngine::prepare (double newSampleRate, int maximumBlockSize)
{
    juce::ignoreUnused (maximumBlockSize);
    sampleRate = newSampleRate;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (maximumBlockSize);
    spec.numChannels = 1;

    oscillator.initialise ([] (float x) { return std::sin (x); });
    oscillator.prepare (spec);
    filter.prepare (spec);
    filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    updateFilter();
    reset();
}

void SynthEngine::reset()
{
    oscillator.reset();
    filter.reset();
    envelope = 0.0f;
    envStage = EnvStage::idle;
    currentFrequency = 440.0f;
    targetFrequency = 440.0f;
    activeVoiced = false;
}

void SynthEngine::setWaveform (WaveformType type)
{
    waveform = type;
    oscillator.initialise ([type] (float x) { return waveformFunction (type, x); });
}

void SynthEngine::setFilterCutoff (float hz)
{
    filter.setCutoffFrequency (juce::jlimit (20.0f, static_cast<float> (sampleRate * 0.45), hz));
}

void SynthEngine::setFilterResonance (float resonance)
{
    filter.setResonance (juce::jlimit (0.1f, 2.0f, resonance));
}

void SynthEngine::setAttackMs (float ms)
{
    attackCoeff = msToCoeff (ms);
}

void SynthEngine::setDecayMs (float ms)
{
    decayCoeff = msToCoeff (ms);
}

void SynthEngine::setSustainLevel (float level)
{
    sustainLevel = juce::jlimit (0.0f, 1.0f, level);
}

void SynthEngine::setReleaseMs (float ms)
{
    releaseCoeff = msToCoeff (ms);
}

void SynthEngine::setGlideMs (float ms)
{
    glideCoeff = msToCoeff (juce::jmax (0.0f, ms));
}

void SynthEngine::setMasterGain (float gain)
{
    masterGain = juce::jlimit (0.0f, 1.0f, gain);
}

void SynthEngine::setPitchState (float hz, bool isVoiced)
{
    if (isVoiced && hz > 0.0f)
    {
        targetFrequency = hz;
        if (! activeVoiced)
        {
            currentFrequency = hz;
            envStage = EnvStage::attack;
        }
        activeVoiced = true;
    }
    else if (activeVoiced)
    {
        activeVoiced = false;
        envStage = EnvStage::release;
    }
}

float SynthEngine::processSample (float gateLevel) noexcept
{
    if (glideCoeff <= 0.0f || ! activeVoiced)
        currentFrequency = targetFrequency;
    else
        currentFrequency += (targetFrequency - currentFrequency) * (1.0f - glideCoeff);

    oscillator.setFrequency (currentFrequency, true);
    return renderSample (gateLevel);
}

void SynthEngine::processBlock (juce::AudioBuffer<float>& buffer, const float* gateEnvelope, int numSamples)
{
    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : left;

    for (int i = 0; i < numSamples; ++i)
    {
        const float gate = gateEnvelope != nullptr ? gateEnvelope[i] : 1.0f;
        const float sample = processSample (gate);
        left[i] = sample;
        right[i] = sample;
    }
}

float SynthEngine::renderSample (float gateLevel) noexcept
{
    switch (envStage)
    {
        case EnvStage::attack:
            envelope += (1.0f - envelope) * (1.0f - attackCoeff);
            if (envelope >= 0.99f)
            {
                envelope = 1.0f;
                envStage = EnvStage::decay;
            }
            break;
        case EnvStage::decay:
            envelope += (sustainLevel - envelope) * (1.0f - decayCoeff);
            if (std::abs (envelope - sustainLevel) < 0.01f)
            {
                envelope = sustainLevel;
                envStage = EnvStage::sustain;
            }
            break;
        case EnvStage::sustain:
            envelope = sustainLevel;
            break;
        case EnvStage::release:
            envelope *= releaseCoeff;
            if (envelope < 0.001f)
            {
                envelope = 0.0f;
                envStage = EnvStage::idle;
            }
            break;
        case EnvStage::idle:
        default:
            envelope = 0.0f;
            break;
    }

    const float amp = envelope * juce::jlimit (0.0f, 1.0f, gateLevel) * masterGain;
    if (amp <= 0.0f)
        return 0.0f;

    const float osc = oscillator.processSample (0.0f);
    return filter.processSample (0, osc * amp);
}

void SynthEngine::updateFilter()
{
    filter.setCutoffFrequency (2000.0f);
    filter.setResonance (0.707f);
}

float SynthEngine::msToCoeff (float ms) const
{
    const float safeMs = juce::jmax (0.1f, ms);
    return std::exp (-1.0f / (0.001f * safeMs * static_cast<float> (sampleRate)));
}
