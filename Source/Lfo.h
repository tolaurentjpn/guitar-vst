#pragma once

#include <JuceHeader.h>

enum class LfoShape
{
    sine = 0,
    triangle,
    square,
    saw
};

class Lfo
{
public:
    void prepare (double sampleRate);
    void reset() noexcept;

    void setRateHz (float hz);
    void setShape (LfoShape newShape);

    float processSample() noexcept;

private:
    double sampleRate = 44100.0;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float rateHz = 1.0f;
    LfoShape shape = LfoShape::sine;

    void updatePhaseIncrement() noexcept;
};
