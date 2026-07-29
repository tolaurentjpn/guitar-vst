#include "Lfo.h"

void Lfo::prepare (double newSampleRate)
{
    sampleRate = juce::jmax (1.0, newSampleRate);
    updatePhaseIncrement();
}

void Lfo::reset() noexcept
{
    phase = 0.0f;
}

void Lfo::setRateHz (float hz)
{
    rateHz = juce::jlimit (0.01f, 40.0f, hz);
    updatePhaseIncrement();
}

void Lfo::setShape (LfoShape newShape)
{
    shape = newShape;
}

void Lfo::updatePhaseIncrement() noexcept
{
    phaseIncrement = rateHz / static_cast<float> (sampleRate);
}

float Lfo::processSample() noexcept
{
    float value = 0.0f;

    switch (shape)
    {
        case LfoShape::sine:
            value = std::sin (phase * juce::MathConstants<float>::twoPi);
            break;
        case LfoShape::triangle:
            value = (phase < 0.5f)
                        ? (4.0f * phase - 1.0f)
                        : (3.0f - 4.0f * phase);
            break;
        case LfoShape::square:
            value = phase < 0.5f ? 1.0f : -1.0f;
            break;
        case LfoShape::saw:
        default:
            value = 2.0f * phase - 1.0f;
            break;
    }

    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;

    return value;
}
