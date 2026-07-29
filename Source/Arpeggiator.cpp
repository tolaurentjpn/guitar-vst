#include "Arpeggiator.h"
#include <cmath>

namespace
{
    const std::array<int, 1> kNoteIntervals { 0 };
    const std::array<int, 3> kMajorIntervals { 0, 4, 7 };
    const std::array<int, 3> kMinorIntervals { 0, 3, 7 };
    const std::array<int, 4> kMaj7Intervals { 0, 4, 7, 11 };
    const std::array<int, 4> kMin7Intervals { 0, 3, 7, 10 };
    const std::array<int, 3> kSus2Intervals { 0, 2, 7 };
    const std::array<int, 3> kSus4Intervals { 0, 5, 7 };

    template <typename Array>
    void appendChordOctaves (std::vector<int>& dest, int rootMidi, int octaves, const Array& intervals)
    {
        const int octaveSpan = juce::jlimit (1, 4, octaves);
        for (int oct = 0; oct < octaveSpan; ++oct)
            for (int interval : intervals)
                dest.push_back (rootMidi + interval + oct * 12);
    }

    double beatsForDivision (ArpDivision division)
    {
        switch (division)
        {
            case ArpDivision::quarter:           return 1.0;
            case ArpDivision::eighth:            return 0.5;
            case ArpDivision::eighthTriplet:     return 1.0 / 3.0;
            case ArpDivision::sixteenth:         return 0.25;
            case ArpDivision::sixteenthTriplet:  return 1.0 / 6.0;
            case ArpDivision::thirtySecond:      return 0.125;
        }
        return 0.5;
    }
}

void Arpeggiator::prepare (double newSampleRate)
{
    sampleRate = juce::jmax (1.0, newSampleRate);
    reset();
}

void Arpeggiator::reset()
{
    latched = false;
    active = false;
    rootMidi = -1;
    patternMidi.clear();
    builtVersion = -1;
    stepIndex = 0;
    samplesIntoStep = 0.0;
    stepStarted = false;
}

void Arpeggiator::setDivision (int divisionIndex) noexcept
{
    division = static_cast<ArpDivision> (juce::jlimit (0, 5, divisionIndex));
}

void Arpeggiator::setMode (int modeIndex) noexcept
{
    const auto newMode = static_cast<ArpMode> (juce::jlimit (0, 3, modeIndex));
    if (newMode == mode)
        return;
    mode = newMode;
    ++patternVersion;
}

void Arpeggiator::setOctaves (int octaves) noexcept
{
    const int clamped = juce::jlimit (1, 4, octaves);
    if (clamped == octaveCount)
        return;
    octaveCount = clamped;
    ++patternVersion;
}

void Arpeggiator::setChord (int chordIndex) noexcept
{
    const auto newChord = static_cast<ArpChord> (juce::jlimit (0, 6, chordIndex));
    if (newChord == chord)
        return;
    chord = newChord;
    ++patternVersion;
}

void Arpeggiator::setLatch (bool shouldLatch) noexcept
{
    latch = shouldLatch;
    if (! latch)
        latched = false;
}

int Arpeggiator::midiFromHz (float hz)
{
    if (hz <= 0.0f)
        return -1;
    return juce::roundToInt (69.0 + 12.0 * std::log2 (static_cast<double> (hz) / 440.0));
}

float Arpeggiator::hzFromMidi (int midi)
{
    return static_cast<float> (440.0 * std::pow (2.0, (static_cast<double> (midi) - 69.0) / 12.0));
}

void Arpeggiator::updateRoot (float rootHz)
{
    const int midi = midiFromHz (rootHz);
    if (midi < 0)
        return;

    if (rootMidi < 0 || std::abs (midi - rootMidi) >= 1)
    {
        rootMidi = midi;
        ++patternVersion;
        stepIndex = 0;
        samplesIntoStep = 0.0;
        stepStarted = false;
    }
}

void Arpeggiator::rebuildPattern()
{
    patternMidi.clear();
    if (rootMidi < 0)
    {
        builtVersion = patternVersion;
        return;
    }

    std::vector<int> ascending;
    switch (chord)
    {
        case ArpChord::note:  appendChordOctaves (ascending, rootMidi, octaveCount, kNoteIntervals); break;
        case ArpChord::major: appendChordOctaves (ascending, rootMidi, octaveCount, kMajorIntervals); break;
        case ArpChord::minor: appendChordOctaves (ascending, rootMidi, octaveCount, kMinorIntervals); break;
        case ArpChord::maj7:  appendChordOctaves (ascending, rootMidi, octaveCount, kMaj7Intervals); break;
        case ArpChord::min7:  appendChordOctaves (ascending, rootMidi, octaveCount, kMin7Intervals); break;
        case ArpChord::sus2:  appendChordOctaves (ascending, rootMidi, octaveCount, kSus2Intervals); break;
        case ArpChord::sus4:  appendChordOctaves (ascending, rootMidi, octaveCount, kSus4Intervals); break;
    }

    if (ascending.empty())
    {
        builtVersion = patternVersion;
        return;
    }

    switch (mode)
    {
        case ArpMode::up:
            patternMidi = std::move (ascending);
            break;

        case ArpMode::down:
            patternMidi.assign (ascending.rbegin(), ascending.rend());
            break;

        case ArpMode::upDown:
            patternMidi = ascending;
            if (ascending.size() > 1)
                for (int i = static_cast<int> (ascending.size()) - 2; i >= 1; --i)
                    patternMidi.push_back (ascending[static_cast<size_t> (i)]);
            break;

        case ArpMode::random:
            patternMidi = std::move (ascending);
            break;
    }

    if (stepIndex >= static_cast<int> (patternMidi.size()))
        stepIndex = 0;

    builtVersion = patternVersion;
}

double Arpeggiator::samplesPerStep() const
{
    double hz = static_cast<double> (rateHz);
    if (sync)
    {
        const double bpm = hostBpm > 0.0 ? hostBpm : 120.0;
        const double beats = beatsForDivision (division);
        hz = (bpm / 60.0) / juce::jmax (1.0e-6, beats);
    }

    hz = juce::jlimit (0.25, 40.0, hz);
    return sampleRate / hz;
}

float Arpeggiator::currentFrequency() const
{
    if (patternMidi.empty() || stepIndex < 0 || stepIndex >= static_cast<int> (patternMidi.size()))
        return 0.0f;
    return hzFromMidi (patternMidi[static_cast<size_t> (stepIndex)]);
}

void Arpeggiator::advanceStep()
{
    if (patternMidi.empty())
        return;

    if (mode == ArpMode::random)
    {
        if (patternMidi.size() == 1)
        {
            stepIndex = 0;
        }
        else
        {
            std::uniform_int_distribution<int> dist (0, static_cast<int> (patternMidi.size()) - 1);
            int next = stepIndex;
            for (int attempt = 0; attempt < 8 && next == stepIndex; ++attempt)
                next = dist (rng);
            stepIndex = next;
        }
    }
    else
    {
        stepIndex = (stepIndex + 1) % static_cast<int> (patternMidi.size());
    }
}

ArpOutput Arpeggiator::process (float rootHz, bool inputVoiced, bool inputGate)
{
    ArpOutput out;

    if (! enabled)
    {
        if (active)
            reset();
        return out;
    }

    if (inputVoiced && rootHz > 0.0f)
    {
        updateRoot (rootHz);
        if (latch)
            latched = true;
    }

    const bool holdByLatch = latch && latched && rootMidi >= 0;
    active = (inputGate && rootMidi >= 0) || holdByLatch;

    if (! active)
    {
        samplesIntoStep = 0.0;
        stepStarted = false;
        return out;
    }

    if (builtVersion != patternVersion)
        rebuildPattern();

    if (patternMidi.empty())
        return out;

    const double stepLen = juce::jmax (1.0, samplesPerStep());
    const double gateSamples = stepLen * (static_cast<double> (gatePercent) / 100.0);

    if (! stepStarted)
    {
        stepStarted = true;
        samplesIntoStep = 0.0;
        out.shouldRetrigger = true;
    }
    else
    {
        samplesIntoStep += 1.0;
        if (samplesIntoStep >= stepLen)
        {
            samplesIntoStep -= stepLen;
            advanceStep();
            out.shouldRetrigger = true;
        }
    }

    if (samplesIntoStep < gateSamples)
    {
        out.frequencyHz = currentFrequency();
        out.voiced = out.frequencyHz > 0.0f;
    }

    return out;
}
