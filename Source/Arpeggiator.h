#pragma once

#include <JuceHeader.h>
#include <array>
#include <random>
#include <vector>

enum class ArpMode
{
    up = 0,
    down,
    upDown,
    random
};

enum class ArpChord
{
    note = 0,
    major,
    minor,
    maj7,
    min7,
    sus2,
    sus4
};

enum class ArpDivision
{
    quarter = 0,
    eighth,
    eighthTriplet,
    sixteenth,
    sixteenthTriplet,
    thirtySecond
};

struct ArpOutput
{
    float frequencyHz = 0.0f;
    bool voiced = false;
    bool shouldRetrigger = false;
};

class Arpeggiator
{
public:
    void prepare (double sampleRate);
    void reset();

    void setEnabled (bool shouldEnable) noexcept { enabled = shouldEnable; }
    void setSync (bool shouldSync) noexcept { sync = shouldSync; }
    void setRate (float hz) noexcept { rateHz = juce::jlimit (0.25f, 20.0f, hz); }
    void setDivision (int divisionIndex) noexcept;
    void setGate (float percent) noexcept { gatePercent = juce::jlimit (5.0f, 100.0f, percent); }
    void setMode (int modeIndex) noexcept;
    void setOctaves (int octaves) noexcept;
    void setChord (int chordIndex) noexcept;
    void setLatch (bool shouldLatch) noexcept;
    void setHostBpm (double bpm) noexcept { hostBpm = juce::jmax (1.0, bpm); }

    /** Advance one sample. When inactive, voiced is false and frequency is 0. */
    ArpOutput process (float rootHz, bool inputVoiced, bool inputGate);

    /** True while the arp is holding a sequence (including latch with gate closed). */
    bool isActive() const noexcept { return active; }

private:
    void rebuildPattern();
    void updateRoot (float rootHz);
    float currentFrequency() const;
    double samplesPerStep() const;
    void advanceStep();

    static int midiFromHz (float hz);
    static float hzFromMidi (int midi);

    double sampleRate = 44100.0;
    bool enabled = false;
    bool sync = false;
    bool latch = false;
    bool latched = false;
    bool active = false;

    float rateHz = 4.0f;
    float gatePercent = 50.0f;
    double hostBpm = 120.0;
    ArpDivision division = ArpDivision::eighth;
    ArpMode mode = ArpMode::up;
    ArpChord chord = ArpChord::major;
    int octaveCount = 1;

    int rootMidi = -1;
    int patternVersion = 0;
    int builtVersion = -1;

    std::vector<int> patternMidi;
    int stepIndex = 0;
    double samplesIntoStep = 0.0;
    bool stepStarted = false;

    std::mt19937 rng { std::random_device{}() };
};
