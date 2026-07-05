#include <cmath>
#include <cstdlib>
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

    float readInputSample (const float* ch0, const float* ch1, int index)
    {
        float sample = ch0[index];
        if (ch1 != nullptr && std::abs (ch1[index]) > std::abs (sample))
            sample = ch1[index];
        return sample;
    }

    float measurePeak (const float* ch0, const float* ch1, int numSamples)
    {
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            peak = std::fmax (peak, std::abs (readInputSample (ch0, ch1, i)));
        return peak;
    }

    float dbToLinear (float db)
    {
        return std::pow (10.0f, db / 20.0f);
    }
}

int main()
{
    {
        const float ch0[] { 0.0f, 0.0f, 0.0f };
        const float ch1[] { 0.0f, 0.25f, 0.0f };
        const float peak = measurePeak (ch0, ch1, 3);
        expectTrue ("Uses louder input channel for peak", peak > 0.24f);
    }

    {
        const float ch0[] { 0.0f, 0.0f, 0.0f };
        const float peak = measurePeak (ch0, nullptr, 2);
        expectTrue ("Silent buffer reports zero peak", peak <= 1.0e-6f);
    }

    {
        const float ch0[] { 0.2f, 0.2f };
        const float ch1[] { 0.0f, 0.0f };
        const float peak = measurePeak (ch0, ch1, 2);
        expectTrue ("Mono guitar on channel 1 is detected", peak > 0.19f);
    }

    {
        expectTrue ("Default gate threshold accepts instrument-level signal",
                    0.05f >= dbToLinear (-58.0f));
    }

    std::cout << testsRun << " tests run, " << testsFailed << " failed\n";
    return testsFailed == 0 ? 0 : 1;
}
