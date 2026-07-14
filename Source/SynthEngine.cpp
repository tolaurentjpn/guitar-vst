#include "SynthEngine.h"
#include <cmath>

void SynthEngine::AmpEnvelope::advance() noexcept
{
    switch (stage)
    {
        case EnvStage::attack:
            value += (1.0f - value) * (1.0f - attackCoeff);
            if (value >= 0.99f)
            {
                value = 1.0f;
                stage = EnvStage::decay;
            }
            break;
        case EnvStage::decay:
            value += (sustainLevel - value) * (1.0f - decayCoeff);
            if (std::abs (value - sustainLevel) < 0.01f)
            {
                value = sustainLevel;
                stage = EnvStage::sustain;
            }
            break;
        case EnvStage::sustain:
            value = sustainLevel;
            break;
        case EnvStage::release:
            value += (0.0f - value) * (1.0f - releaseCoeff);
            if (value <= 0.001f)
                hardReset();
            break;
        case EnvStage::idle:
        default:
            value = 0.0f;
            break;
    }
}

float SynthEngine::renderWave (WaveformType type, float phase) noexcept
{
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

float SynthEngine::voiceDetuneCents (int voiceIndex, int numVoices, float maxDetuneCents) noexcept
{
    if (numVoices <= 1)
        return 0.0f;

    const float t = static_cast<float> (voiceIndex) / static_cast<float> (numVoices - 1);
    return (t * 2.0f - 1.0f) * maxDetuneCents;
}

float SynthEngine::voicePan (int voiceIndex, int numVoices, float spread01) noexcept
{
    if (numVoices <= 1 || spread01 <= 0.0f)
        return 0.0f;

    const float t = static_cast<float> (voiceIndex) / static_cast<float> (numVoices - 1);
    return (t * 2.0f - 1.0f) * spread01;
}

float SynthEngine::voiceBlendGain (int voiceIndex, int numVoices, float blend01) noexcept
{
    if (numVoices <= 1)
        return 1.0f;

    const float t = static_cast<float> (voiceIndex) / static_cast<float> (numVoices - 1);
    const float edgeWeight = std::abs (t * 2.0f - 1.0f);
    const float centerWeight = 1.0f - edgeWeight;
    return juce::jmap (blend01, centerWeight, edgeWeight * 0.85f + 0.15f);
}

void SynthEngine::prepare (double newSampleRate, int maximumBlockSize)
{
    juce::ignoreUnused (maximumBlockSize);
    sampleRate = newSampleRate;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (maximumBlockSize);
    spec.numChannels = 1;

    filter1.prepare (spec);
    filter1.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    filter1.setCutoffFrequency (osc1CutoffHz);
    filter1.setResonance (osc1Resonance);

    filter2.prepare (spec);
    filter2.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    filter2.setCutoffFrequency (osc2CutoffHz);
    filter2.setResonance (osc2Resonance);

    lfo1.prepare (sampleRate);
    lfo2.prepare (sampleRate);
    reset();
}

void SynthEngine::reset()
{
    hardMute();
    currentFrequency = 440.0f;
    targetFrequency = 440.0f;
    lfo1.reset();
    lfo2.reset();
}

void SynthEngine::setWaveform (WaveformType type) { waveform = type; }
void SynthEngine::setOsc2Waveform (WaveformType type) { osc2Waveform = type; }

void SynthEngine::setOsc2Mix (float mix) { osc2Mix = juce::jlimit (0.0f, 1.0f, mix); }
void SynthEngine::setOsc2Octave (int octave) { osc2Octave = juce::jlimit (-1, 1, octave); }
void SynthEngine::setOsc2DetuneCents (float cents) { osc2DetuneCents = juce::jlimit (-100.0f, 100.0f, cents); }

void SynthEngine::setOsc1UnisonVoices (int voices) { osc1UnisonVoices = juce::jlimit (1, maxUnison, voices); }
void SynthEngine::setOsc1UnisonDetune (float amount01) { osc1UnisonDetune = juce::jlimit (0.0f, 1.0f, amount01); }
void SynthEngine::setOsc1UnisonSpread (float amount01) { osc1UnisonSpread = juce::jlimit (0.0f, 1.0f, amount01); }
void SynthEngine::setOsc1UnisonBlend (float amount01) { osc1UnisonBlend = juce::jlimit (0.0f, 1.0f, amount01); }
void SynthEngine::setOsc1PhaseRandom (float amount01) { osc1PhaseRandom = juce::jlimit (0.0f, 1.0f, amount01); }

void SynthEngine::setOsc2UnisonVoices (int voices) { osc2UnisonVoices = juce::jlimit (1, maxUnison, voices); }
void SynthEngine::setOsc2UnisonDetune (float amount01) { osc2UnisonDetune = juce::jlimit (0.0f, 1.0f, amount01); }
void SynthEngine::setOsc2UnisonSpread (float amount01) { osc2UnisonSpread = juce::jlimit (0.0f, 1.0f, amount01); }
void SynthEngine::setOsc2UnisonBlend (float amount01) { osc2UnisonBlend = juce::jlimit (0.0f, 1.0f, amount01); }
void SynthEngine::setOsc2PhaseRandom (float amount01) { osc2PhaseRandom = juce::jlimit (0.0f, 1.0f, amount01); }

void SynthEngine::setOsc1FilterCutoff (float hz) { osc1CutoffHz = clampCutoff (hz); }
void SynthEngine::setOsc1FilterResonance (float resonance) { osc1Resonance = juce::jlimit (0.1f, 2.0f, resonance); }
void SynthEngine::setOsc2FilterCutoff (float hz) { osc2CutoffHz = clampCutoff (hz); }
void SynthEngine::setOsc2FilterResonance (float resonance) { osc2Resonance = juce::jlimit (0.1f, 2.0f, resonance); }

void SynthEngine::setOsc1AttackMs (float ms) { env1.attackCoeff = msToCoeff (ms); }
void SynthEngine::setOsc1DecayMs (float ms) { env1.decayCoeff = msToCoeff (ms); }
void SynthEngine::setOsc1SustainLevel (float level) { env1.sustainLevel = juce::jlimit (0.0f, 1.0f, level); }
void SynthEngine::setOsc1ReleaseMs (float ms) { env1.releaseCoeff = msToCoeff (ms); }

void SynthEngine::setOsc2AttackMs (float ms) { env2.attackCoeff = msToCoeff (ms); }
void SynthEngine::setOsc2DecayMs (float ms) { env2.decayCoeff = msToCoeff (ms); }
void SynthEngine::setOsc2SustainLevel (float level) { env2.sustainLevel = juce::jlimit (0.0f, 1.0f, level); }
void SynthEngine::setOsc2ReleaseMs (float ms) { env2.releaseCoeff = msToCoeff (ms); }

void SynthEngine::setFilterEnv1AttackMs (float ms) { filterEnv1.attackCoeff = msToCoeff (ms); }
void SynthEngine::setFilterEnv1DecayMs (float ms) { filterEnv1.decayCoeff = msToCoeff (ms); }
void SynthEngine::setFilterEnv1SustainLevel (float level) { filterEnv1.sustainLevel = juce::jlimit (0.0f, 1.0f, level); }
void SynthEngine::setFilterEnv1ReleaseMs (float ms) { filterEnv1.releaseCoeff = msToCoeff (ms); }
void SynthEngine::setFilterEnv1Amount (float amount) { filterEnv1Amount = juce::jlimit (-1.0f, 1.0f, amount); }

void SynthEngine::setFilterEnv2AttackMs (float ms) { filterEnv2.attackCoeff = msToCoeff (ms); }
void SynthEngine::setFilterEnv2DecayMs (float ms) { filterEnv2.decayCoeff = msToCoeff (ms); }
void SynthEngine::setFilterEnv2SustainLevel (float level) { filterEnv2.sustainLevel = juce::jlimit (0.0f, 1.0f, level); }
void SynthEngine::setFilterEnv2ReleaseMs (float ms) { filterEnv2.releaseCoeff = msToCoeff (ms); }
void SynthEngine::setFilterEnv2Amount (float amount) { filterEnv2Amount = juce::jlimit (-1.0f, 1.0f, amount); }

void SynthEngine::setGlideMs (float ms) { glideCoeff = msToCoeff (juce::jmax (0.0f, ms)); }
void SynthEngine::setMasterGain (float gain) { masterGain = juce::jlimit (0.0f, 1.0f, gain); }

void SynthEngine::setLfo1Enabled (bool enabled) { lfo1Enabled = enabled; }
void SynthEngine::setLfo1Rate (float hz) { lfo1.setRateHz (hz); }
void SynthEngine::setLfo1Shape (LfoShape shape) { lfo1.setShape (shape); }
void SynthEngine::setLfo1FilterAmount (float amount) { lfo1FilterAmount = juce::jlimit (-1.0f, 1.0f, amount); }
void SynthEngine::setLfo1ResonanceAmount (float amount) { lfo1ResonanceAmount = juce::jlimit (-1.0f, 1.0f, amount); }
void SynthEngine::setLfo1PitchAmount (float amount) { lfo1PitchAmount = juce::jlimit (-1.0f, 1.0f, amount); }
void SynthEngine::setLfo1AmpAmount (float amount) { lfo1AmpAmount = juce::jlimit (-1.0f, 1.0f, amount); }

void SynthEngine::setLfo2Enabled (bool enabled) { lfo2Enabled = enabled; }
void SynthEngine::setLfo2Rate (float hz) { lfo2.setRateHz (hz); }
void SynthEngine::setLfo2Shape (LfoShape shape) { lfo2.setShape (shape); }
void SynthEngine::setLfo2FilterAmount (float amount) { lfo2FilterAmount = juce::jlimit (-1.0f, 1.0f, amount); }
void SynthEngine::setLfo2ResonanceAmount (float amount) { lfo2ResonanceAmount = juce::jlimit (-1.0f, 1.0f, amount); }
void SynthEngine::setLfo2PitchAmount (float amount) { lfo2PitchAmount = juce::jlimit (-1.0f, 1.0f, amount); }
void SynthEngine::setLfo2AmpAmount (float amount) { lfo2AmpAmount = juce::jlimit (-1.0f, 1.0f, amount); }

bool SynthEngine::isIdle() const noexcept
{
    return env1.stage == EnvStage::idle && env2.stage == EnvStage::idle;
}

void SynthEngine::resetOscStack (std::array<PhaseOsc, maxUnison>& stack, float phaseRandom01)
{
    std::uniform_real_distribution<float> dist (-juce::MathConstants<float>::pi,
                                                juce::MathConstants<float>::pi);
    for (auto& osc : stack)
        osc.phase = phaseRandom01 > 0.0f ? dist (rng) * phaseRandom01 : 0.0f;
}

void SynthEngine::resetUnisonPhases (bool randomizeOsc1, bool randomizeOsc2)
{
    if (randomizeOsc1)
        resetOscStack (osc1Stack, osc1PhaseRandom);
    if (randomizeOsc2)
        resetOscStack (osc2Stack, osc2PhaseRandom);
}

void SynthEngine::hardMute() noexcept
{
    activeVoiced = false;
    env1.hardReset();
    env2.hardReset();
    filterEnv1.hardReset();
    filterEnv2.hardReset();
    resetUnisonPhases (true, true);
    filter1.reset();
    filter2.reset();
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

        if (isIdle())
        {
            currentFrequency = hz;
            env1.stage = EnvStage::attack;
            env2.stage = EnvStage::attack;
            filterEnv1.stage = EnvStage::attack;
            filterEnv2.stage = EnvStage::attack;
            resetUnisonPhases (true, true);
            filter1.reset();
            filter2.reset();
            activeVoiced = true;
        }
        else if (env1.stage == EnvStage::release || env2.stage == EnvStage::release)
        {
            currentFrequency = hz;
            env1.stage = EnvStage::attack;
            env2.stage = EnvStage::attack;
            filterEnv1.stage = EnvStage::attack;
            filterEnv2.stage = EnvStage::attack;
            activeVoiced = true;
        }
        else if (significantJump && glideCoeff <= 0.0f)
        {
            currentFrequency = hz;
            resetUnisonPhases (true, true);
        }

        activeVoiced = true;
    }
    else if (activeVoiced)
    {
        if (env1.stage != EnvStage::release && env1.stage != EnvStage::idle)
            env1.stage = EnvStage::release;
        if (env2.stage != EnvStage::release && env2.stage != EnvStage::idle)
            env2.stage = EnvStage::release;
        if (filterEnv1.stage != EnvStage::release && filterEnv1.stage != EnvStage::idle)
            filterEnv1.stage = EnvStage::release;
        if (filterEnv2.stage != EnvStage::release && filterEnv2.stage != EnvStage::idle)
            filterEnv2.stage = EnvStage::release;
        activeVoiced = false;
    }
}

float SynthEngine::clampCutoff (float hz) const noexcept
{
    return juce::jlimit (20.0f, static_cast<float> (sampleRate * 0.45), hz);
}

void SynthEngine::processSample (float& left, float& right) noexcept
{
    const float lfo1Raw = lfo1.processSample();
    const float lfo2Raw = lfo2.processSample();
    const float lfo1Value = lfo1Enabled ? lfo1Raw : 0.0f;
    const float lfo2Value = lfo2Enabled ? lfo2Raw : 0.0f;

    if (isIdle())
    {
        left = 0.0f;
        right = 0.0f;
        return;
    }

    if (glideCoeff <= 0.0f)
        currentFrequency = targetFrequency;
    else
        currentFrequency += (targetFrequency - currentFrequency) * (1.0f - glideCoeff);

    const float osc1PitchFactor = std::exp2 (lfo1PitchAmount * lfo1Value * 100.0f / 1200.0f);
    const float osc2PitchFactor = std::exp2 (lfo2PitchAmount * lfo2Value * 100.0f / 1200.0f);
    const float octaveFactor = std::exp2 (static_cast<float> (osc2Octave));
    const float osc2OffsetFactor = std::exp2 (osc2DetuneCents / 1200.0f);

    const float nyquistLimit = static_cast<float> (sampleRate * 0.45);
    const float osc1BaseFreq = juce::jlimit (20.0f, nyquistLimit, currentFrequency * osc1PitchFactor);
    const float osc2BaseFreq = juce::jlimit (20.0f, nyquistLimit,
                                             currentFrequency * octaveFactor * osc2OffsetFactor * osc2PitchFactor);

    filterEnv1.advance();
    filterEnv2.advance();

    const float osc1CutoffMod = std::exp2 (filterEnv1Amount * filterEnv1.value * 4.0f)
                              * std::exp2 (4.0f * lfo1FilterAmount * lfo1Value);
    const float osc2CutoffMod = std::exp2 (filterEnv2Amount * filterEnv2.value * 4.0f)
                              * std::exp2 (4.0f * lfo2FilterAmount * lfo2Value);

    filter1.setCutoffFrequency (clampCutoff (osc1CutoffHz * osc1CutoffMod));
    filter2.setCutoffFrequency (clampCutoff (osc2CutoffHz * osc2CutoffMod));

    filter1.setResonance (juce::jlimit (0.1f, 2.0f,
                                         osc1Resonance + 1.5f * lfo1ResonanceAmount * lfo1Value));
    filter2.setResonance (juce::jlimit (0.1f, 2.0f,
                                         osc2Resonance + 1.5f * lfo2ResonanceAmount * lfo2Value));

    const float osc1AmpScale = juce::jlimit (0.0f, 1.5f, 1.0f + 0.8f * lfo1AmpAmount * lfo1Value);
    const float osc2AmpScale = juce::jlimit (0.0f, 1.5f, 1.0f + 0.8f * lfo2AmpAmount * lfo2Value);

    renderSample (osc1BaseFreq, osc2BaseFreq, osc1AmpScale, osc2AmpScale, left, right);
}

void SynthEngine::processBlock (juce::AudioBuffer<float>& buffer, const float* gateEnvelope, int numSamples)
{
    auto* leftPtr = buffer.getWritePointer (0);
    auto* rightPtr = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : leftPtr;

    for (int i = 0; i < numSamples; ++i)
    {
        juce::ignoreUnused (gateEnvelope);
        float l = 0.0f, r = 0.0f;
        processSample (l, r);
        leftPtr[i] = l;
        rightPtr[i] = r;
    }
}

void SynthEngine::renderSample (float osc1BaseFreq, float osc2BaseFreq,
                                float osc1AmpScale, float osc2AmpScale,
                                float& left, float& right) noexcept
{
    env1.advance();
    env2.advance();

    if (isIdle())
    {
        hardMute();
        left = 0.0f;
        right = 0.0f;
        return;
    }

    const float twoPi = juce::MathConstants<float>::twoPi;
    const float pi = juce::MathConstants<float>::pi;
    const float phaseIncScale = twoPi / static_cast<float> (sampleRate);
    const float nyquistLimit = static_cast<float> (sampleRate * 0.45);

    const float osc1DetuneCentsMax = osc1UnisonDetune * 200.0f;
    const float osc2DetuneCentsMax = osc2UnisonDetune * 200.0f;
    const float osc1Norm = 1.0f / std::sqrt (static_cast<float> (osc1UnisonVoices));
    const float osc2Norm = 1.0f / std::sqrt (static_cast<float> (osc2UnisonVoices));

    float stack1L = 0.0f, stack1R = 0.0f;
    for (int v = 0; v < osc1UnisonVoices; ++v)
    {
        const float cents = voiceDetuneCents (v, osc1UnisonVoices, osc1DetuneCentsMax);
        const float freq = juce::jlimit (20.0f, nyquistLimit, osc1BaseFreq * std::exp2 (cents / 1200.0f));
        auto& osc = osc1Stack[static_cast<size_t> (v)];
        const float sample = renderWave (waveform, osc.phase);
        osc.phase += freq * phaseIncScale;
        while (osc.phase > pi) osc.phase -= twoPi;
        while (osc.phase < -pi) osc.phase += twoPi;

        const float gain = voiceBlendGain (v, osc1UnisonVoices, osc1UnisonBlend) * osc1Norm;
        const float pan = voicePan (v, osc1UnisonVoices, osc1UnisonSpread);
        stack1L += sample * gain * (0.5f * (1.0f - pan));
        stack1R += sample * gain * (0.5f * (1.0f + pan));
    }

    float stack2L = 0.0f, stack2R = 0.0f;
    for (int v = 0; v < osc2UnisonVoices; ++v)
    {
        const float cents = voiceDetuneCents (v, osc2UnisonVoices, osc2DetuneCentsMax);
        const float freq = juce::jlimit (20.0f, nyquistLimit, osc2BaseFreq * std::exp2 (cents / 1200.0f));
        auto& osc = osc2Stack[static_cast<size_t> (v)];
        const float sample = renderWave (osc2Waveform, osc.phase);
        osc.phase += freq * phaseIncScale;
        while (osc.phase > pi) osc.phase -= twoPi;
        while (osc.phase < -pi) osc.phase += twoPi;

        const float gain = voiceBlendGain (v, osc2UnisonVoices, osc2UnisonBlend) * osc2Norm;
        const float pan = voicePan (v, osc2UnisonVoices, osc2UnisonSpread);
        stack2L += sample * gain * (0.5f * (1.0f - pan));
        stack2R += sample * gain * (0.5f * (1.0f + pan));
    }

    // One filter per osc on mono sum; restore stereo with pre-filter L/R magnitude weights.
    const float mono1 = stack1L + stack1R;
    const float mono2 = stack2L + stack2R;
    const float filtered1 = filter1.processSample (0, mono1 * osc1AmpScale) * env1.value;
    const float filtered2 = filter2.processSample (0, mono2 * osc2AmpScale) * env2.value;

    const float sum1 = std::abs (stack1L) + std::abs (stack1R);
    const float sum2 = std::abs (stack2L) + std::abs (stack2R);
    const float w1L = sum1 > 1.0e-8f ? std::abs (stack1L) / sum1 : 0.5f;
    const float w1R = sum1 > 1.0e-8f ? std::abs (stack1R) / sum1 : 0.5f;
    const float w2L = sum2 > 1.0e-8f ? std::abs (stack2L) / sum2 : 0.5f;
    const float w2R = sum2 > 1.0e-8f ? std::abs (stack2R) / sum2 : 0.5f;

    const float out1L = filtered1 * w1L * 2.0f;
    const float out1R = filtered1 * w1R * 2.0f;
    const float out2L = filtered2 * w2L * 2.0f;
    const float out2R = filtered2 * w2R * 2.0f;

    left = (out1L * (1.0f - osc2Mix) + out2L * osc2Mix) * masterGain;
    right = (out1R * (1.0f - osc2Mix) + out2R * osc2Mix) * masterGain;
}

float SynthEngine::msToCoeff (float ms) const
{
    const float safeMs = juce::jmax (0.1f, ms);
    return std::exp (-1.0f / (0.001f * safeMs * static_cast<float> (sampleRate)));
}
