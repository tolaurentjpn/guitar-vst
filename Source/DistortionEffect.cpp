#include "DistortionEffect.h"

void DistortionEffect::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    reset();
}

void DistortionEffect::reset()
{
    toneStateL = 0.0f;
    toneStateR = 0.0f;
}

float DistortionEffect::shapeSample (float x, DistortionMode mode, float driveGain) noexcept
{
    const float driven = x * driveGain;

    switch (mode)
    {
        case DistortionMode::Hard:
            return juce::jlimit (-1.0f, 1.0f, driven);

        case DistortionMode::Fold:
        {
            float y = driven;
            for (int n = 0; n < 16 && (y > 1.0f || y < -1.0f); ++n)
            {
                if (y > 1.0f)
                    y = 2.0f - y;
                else
                    y = -2.0f - y;
            }
            return juce::jlimit (-1.0f, 1.0f, y);
        }

        case DistortionMode::Soft:
        default:
            return std::tanh (driven);
    }
}

void DistortionEffect::process (juce::dsp::AudioBlock<float>& block)
{
    if (bypassed || block.getNumSamples() == 0)
        return;

    const float driveGain = juce::jmap (drive, 1.0f, 24.0f);
    // Approximate makeup so louder drive doesn't explode the level.
    const float makeup = 1.0f / std::sqrt (juce::jmax (1.0f, driveGain * 0.35f));
    const float wet = mix;
    const float dry = 1.0f - mix;

    // Tone: 1-pole LPF cutoff from ~400 Hz (dark) to ~12 kHz (bright)
    const float cutoffHz = juce::jmap (tone, 400.0f, 12000.0f);
    toneCoeff = std::exp (-juce::MathConstants<float>::twoPi * cutoffHz / static_cast<float> (sampleRate));

    const auto numChannels = juce::jmin ((size_t) 2, block.getNumChannels());
    const auto numSamples = block.getNumSamples();

    for (size_t i = 0; i < numSamples; ++i)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            float* data = block.getChannelPointer (ch);
            const float in = data[i];
            float shaped = shapeSample (in, mode, driveGain) * makeup;

            float& toneState = (ch == 0) ? toneStateL : toneStateR;
            toneState += (1.0f - toneCoeff) * (shaped - toneState);
            shaped = toneState;

            data[i] = dry * in + wet * shaped;
        }
    }
}
