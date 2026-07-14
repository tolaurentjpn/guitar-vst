#include "SynthEngine.h"

namespace
{
    float waveformFunction (WaveformType type, float phase)
    {
        // JUCE Oscillator passes phase in [-pi, pi].
        switch (type)
        {
            case WaveformType::sine:
                return std::sin (phase);
            case WaveformType::saw:
                return phase / juce::MathConstants<float>::pi;
            case WaveformType::square:
            default:
                return phase < 0.0f ? -1.0f : 1.0f;
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
    hardMute();
    currentFrequency = 440.0f;
    targetFrequency = 440.0f;
}

void SynthEngine::setWaveform (WaveformType type)
{
    if (type == waveform && oscillator.isInitialised())
        return;

    waveform = type;
    oscillator.initialise ([type] (float x) { return waveformFunction (type, x); });
}

void SynthEngine::setFilterCutoff (float hz)
{
    const float clamped = juce::jlimit (20.0f, static_cast<float> (sampleRate * 0.45), hz);
    if (clamped == lastFilterCutoff)
        return;

    lastFilterCutoff = clamped;
    filter.setCutoffFrequency (clamped);
}

void SynthEngine::setFilterResonance (float resonance)
{
    const float clamped = juce::jlimit (0.1f, 2.0f, resonance);
    if (clamped == lastFilterResonance)
        return;

    lastFilterResonance = clamped;
    filter.setResonance (clamped);
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
    if (ms <= 0.0f)
        glideCoeff = 0.0f;
    else
        glideCoeff = msToCoeff (ms);
}

void SynthEngine::setMasterGain (float gain)
{
    masterGain = juce::jlimit (0.0f, 1.0f, gain);
}

void SynthEngine::hardMute() noexcept
{
    activeVoiced = false;
    envelope = 0.0f;
    envStage = EnvStage::idle;
    oscillator.reset();
    filter.reset();
}

void SynthEngine::muteImmediately() noexcept
{
    hardMute();
}

void SynthEngine::setPitchState (float hz, bool trackingActive)
{
    const bool hasPitch = trackingActive && hz > 0.0f;

    if (hasPitch)
    {
        const bool significantJump = activeVoiced
                                    && targetFrequency > 0.0f
                                    && std::abs (std::log2 (hz / targetFrequency)) > (50.0f / 1200.0f);

        targetFrequency = hz;

        if (envStage == EnvStage::idle)
        {
            currentFrequency = hz;
            envStage = EnvStage::attack;
            oscillator.reset();
            filter.reset();
            envelope = 0.0f;
            activeVoiced = true;
        }
        else if (envStage == EnvStage::release)
        {
            // Resume without retriggering the oscillator — avoids silencing on brief
            // unvoiced gaps / zero-crossing flicker.
            envStage = envelope < sustainLevel ? EnvStage::attack : EnvStage::sustain;
            activeVoiced = true;
        }
        else if (significantJump && glideCoeff <= 0.0f)
        {
            currentFrequency = hz;
            oscillator.reset();
        }

        activeVoiced = true;
    }
    else if (activeVoiced && envStage != EnvStage::release && envStage != EnvStage::idle)
    {
        envStage = EnvStage::release;
    }
}

float SynthEngine::processSample() noexcept
{
    if (envStage == EnvStage::idle)
        return 0.0f;

    if (glideCoeff <= 0.0f)
        currentFrequency = targetFrequency;
    else
        currentFrequency += (targetFrequency - currentFrequency) * (1.0f - glideCoeff);

    oscillator.setFrequency (currentFrequency, true);
    return renderSample();
}

void SynthEngine::processBlock (juce::AudioBuffer<float>& buffer, const float* gateEnvelope, int numSamples)
{
    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : left;

    for (int i = 0; i < numSamples; ++i)
    {
        juce::ignoreUnused (gateEnvelope);
        const float sample = processSample();
        left[i] = sample;
        right[i] = sample;
    }
}

float SynthEngine::renderSample() noexcept
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
            envelope += (0.0f - envelope) * (1.0f - releaseCoeff);
            if (envelope <= 0.001f)
                hardMute();
            break;
        case EnvStage::idle:
        default:
            envelope = 0.0f;
            break;
    }

    const float amp = envelope * masterGain;
    if (amp <= 0.0f)
        return 0.0f;

    const float osc = oscillator.processSample (0.0f);
    return juce::jlimit (-1.0f, 1.0f, filter.processSample (0, osc * amp));
}

void SynthEngine::updateFilter()
{
    lastFilterCutoff = 2000.0f;
    lastFilterResonance = 0.707f;
    filter.setCutoffFrequency (lastFilterCutoff);
    filter.setResonance (lastFilterResonance);
}

float SynthEngine::msToCoeff (float ms) const
{
    const float safeMs = juce::jmax (0.1f, ms);
    return std::exp (-1.0f / (0.001f * safeMs * static_cast<float> (sampleRate)));
}
