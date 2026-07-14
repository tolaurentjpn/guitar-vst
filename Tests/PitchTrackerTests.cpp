#include "PitchTracker.h"
#include <cmath>
#include <iostream>

namespace
{
    int testsRun = 0;
    int testsFailed = 0;

    void expectTrue (const char* name, bool condition)
    {
        ++testsRun;
        if (! condition)
        {
            ++testsFailed;
            std::cerr << "FAIL: " << name << '\n';
        }
    }

    float harmonicRichSample (double sampleRate, float frequency, int sampleIndex)
    {
        const float t = static_cast<float> (sampleIndex) / static_cast<float> (sampleRate);
        const float phase = juce::MathConstants<float>::twoPi * frequency * t;
        return 0.55f * std::sin (phase)
             + 0.35f * std::sin (2.0f * phase)
             + 0.10f * std::sin (3.0f * phase);
    }

    float missingFundamentalSample (double sampleRate, float fundamentalHz, int sampleIndex)
    {
        const float t = static_cast<float> (sampleIndex) / static_cast<float> (sampleRate);
        const float phase = juce::MathConstants<float>::twoPi * fundamentalHz * t;
        // No fundamental — only harmonics (common guitar pickup timbre).
        return 0.60f * std::sin (2.0f * phase)
             + 0.30f * std::sin (3.0f * phase)
             + 0.10f * std::sin (4.0f * phase);
    }

    float strongSecondSample (double sampleRate, float fundamentalHz, int sampleIndex)
    {
        const float t = static_cast<float> (sampleIndex) / static_cast<float> (sampleRate);
        const float phase = juce::MathConstants<float>::twoPi * fundamentalHz * t;
        return 0.25f * std::sin (phase)
             + 0.65f * std::sin (2.0f * phase)
             + 0.10f * std::sin (3.0f * phase);
    }

    void feedTone (PitchTracker& tracker, double sampleRate, float frequency, int numSamples,
                   int& sampleIndex,
                   float (*sampleFn) (double, float, int) = harmonicRichSample)
    {
        for (int i = 0; i < numSamples; ++i)
            tracker.pushSample (sampleFn (sampleRate, frequency, sampleIndex++));
    }

    float detectFrequency (double sampleRate, float targetFrequency)
    {
        PitchTracker tracker;
        tracker.prepare (sampleRate);

        const int settleSamples = tracker.getWindowSize() * 4;
        int sampleIndex = 0;
        feedTone (tracker, sampleRate, targetFrequency, settleSamples, sampleIndex);

        return tracker.getFrequency();
    }

    float detectCustomTone (double sampleRate,
                            float (*sampleFn) (double, float, int),
                            float targetFrequency)
    {
        PitchTracker tracker;
        tracker.prepare (sampleRate);

        const int settleSamples = tracker.getWindowSize() * 4;
        int sampleIndex = 0;
        feedTone (tracker, sampleRate, targetFrequency, settleSamples, sampleIndex, sampleFn);

        return tracker.getFrequency();
    }
}

int main()
{
    constexpr double sampleRate = 48000.0;

    const float openA = detectFrequency (sampleRate, 110.0f);
    expectTrue ("Open A (110 Hz) not detected an octave high",
                openA > 95.0f && openA < 125.0f);

    const float twelfthFretA = detectFrequency (sampleRate, 220.0f);
    expectTrue ("12th fret A (220 Hz) not detected an octave low",
                twelfthFretA > 190.0f && twelfthFretA < 250.0f);

    const float openE = detectFrequency (sampleRate, 82.41f);
    expectTrue ("Open low E (~82 Hz) not detected an octave high",
                openE > 70.0f && openE < 95.0f);

    const float twelfthFretE = detectFrequency (sampleRate, 164.81f);
    expectTrue ("12th fret low E (~165 Hz) not detected an octave low",
                twelfthFretE > 145.0f && twelfthFretE < 185.0f);

    const float pureTone = detectFrequency (sampleRate, 220.0f);
    expectTrue ("Pure harmonic tone still tracks near 220 Hz",
                pureTone > 200.0f && pureTone < 240.0f);

    const float a3MissingFundamental = detectCustomTone (sampleRate, missingFundamentalSample, 220.0f);
    expectTrue ("A3 (220 Hz) missing fundamental not detected an octave low",
                a3MissingFundamental > 190.0f && a3MissingFundamental < 250.0f);

    const float a3StrongSecond = detectCustomTone (sampleRate, strongSecondSample, 220.0f);
    expectTrue ("A3 (220 Hz) strong 2nd harmonic not detected an octave low",
                a3StrongSecond > 190.0f && a3StrongSecond < 250.0f);

    const float g3 = detectFrequency (sampleRate, 196.0f);
    expectTrue ("G3 (196 Hz) not detected an octave low",
                g3 > 170.0f && g3 < 220.0f);

    const float b3 = detectFrequency (sampleRate, 246.94f);
    expectTrue ("B3 (~247 Hz) not detected an octave low",
                b3 > 215.0f && b3 < 280.0f);

    const float e4 = detectCustomTone (sampleRate, missingFundamentalSample, 329.63f);
    expectTrue ("E4 (~330 Hz) missing fundamental not detected an octave low",
                e4 > 290.0f && e4 < 370.0f);

    // Transition sticky tests: A3 -> A2 and A2 -> A3 must settle on the new note.
    {
        PitchTracker tracker;
        tracker.prepare (sampleRate);
        int sampleIndex = 0;
        const int settle = tracker.getWindowSize() * 4;

        feedTone (tracker, sampleRate, 220.0f, settle, sampleIndex);
        expectTrue ("Pre-transition A3 is near 220 Hz",
                    tracker.getFrequency() > 190.0f && tracker.getFrequency() < 250.0f);

        feedTone (tracker, sampleRate, 110.0f, settle, sampleIndex);
        expectTrue ("A3->A2 transition settles near 110 Hz (not stuck at 220)",
                    tracker.getFrequency() > 95.0f && tracker.getFrequency() < 125.0f);
    }

    {
        PitchTracker tracker;
        tracker.prepare (sampleRate);
        int sampleIndex = 0;
        const int settle = tracker.getWindowSize() * 4;

        feedTone (tracker, sampleRate, 110.0f, settle, sampleIndex);
        expectTrue ("Pre-transition A2 is near 110 Hz",
                    tracker.getFrequency() > 95.0f && tracker.getFrequency() < 125.0f);

        feedTone (tracker, sampleRate, 220.0f, settle, sampleIndex);
        expectTrue ("A2->A3 transition settles near 220 Hz (not stuck at 110)",
                    tracker.getFrequency() > 190.0f && tracker.getFrequency() < 250.0f);
    }

    std::cout << "Open A: " << openA << " Hz\n";
    std::cout << "12th fret A: " << twelfthFretA << " Hz\n";
    std::cout << "Open E: " << openE << " Hz\n";
    std::cout << "12th fret E: " << twelfthFretE << " Hz\n";
    std::cout << "A3 missing fundamental: " << a3MissingFundamental << " Hz\n";
    std::cout << "A3 strong 2nd: " << a3StrongSecond << " Hz\n";
    std::cout << "G3: " << g3 << " Hz, B3: " << b3 << " Hz, E4: " << e4 << " Hz\n";
    std::cout << testsRun << " tests run, " << testsFailed << " failed\n";
    return testsFailed == 0 ? 0 : 1;
}
