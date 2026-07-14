#pragma once

#include <JuceHeader.h>
#include <vector>

// Probabilistic YIN (pYIN) — Mauch & Dixon, ICASSP 2014.
// Stage 1: threshold-distribution pitch candidates.
// Stage 2: HMM + Viterbi pitch-path decoding (librosa-compatible parameters).
class PyinDecoder
{
public:
    static constexpr int nThresholds = 100;
    static constexpr float boltzmannParameter = 2.0f;
    static constexpr float noTroughProb = 0.01f;
    static constexpr float switchProb = 0.01f;
    static constexpr float maxTransitionRate = 35.92f; // semitones / second
    static constexpr float pitchResolution = 0.1f;     // semitones per bin
    static constexpr int viterbiFrames = 16;

    void prepare (double sampleRate, float fmin, float fmax, int frameLength, int hopSize);
    void reset();

    // Analyse one frame; returns voiced f0 in Hz (0 if unvoiced) and marginal voicing probability.
    void processFrame (const float* samples, int numSamples, float& outFrequency, float& outVoicedProb);

    float getLastBestCandidateFrequency() const noexcept { return lastBestCandidateFreq; }
    float getLastBestCandidateProbability() const noexcept { return lastBestCandidateProb; }

private:
    void computeCmnd (const float* data, int numSamples);
    float parabolicShift (int tauIndex) const;
    float scoreHarmonicClarity (int tau) const;
    int findLocalMinimumTau (int centreTau) const;
    void computeFrameObservations (const std::vector<float>& parabolicShifts);
    void runViterbi();
    void buildTransitionMatrix();
    static float regularizedBetaInc (float x, float alpha, float beta);
    static float boltzmannPmf (int position, int numItems, float lambda);

    double sampleRate = 44100.0;
    float fmin = 70.0f;
    float fmax = 1200.0f;
    int frameLength = 2048;
    int hopSize = 64;
    int minPeriod = 2;
    int maxPeriod = 512;
    int numPeriods = 0;
    int nPitchBins = 0;
    int nBinsPerSemitone = 10;
    int transitionWidth = 1;
    int numStates = 0;

    std::vector<float> thresholds;
    std::vector<float> betaProbs;
    std::vector<float> cmnd;
    std::vector<float> transitionMatrix;
    std::vector<float> observationBuffer;
    std::vector<float> viterbiCost;
    std::vector<int> viterbiPath;
    float lastBestCandidateFreq = 0.0f;
    float lastBestCandidateProb = 0.0f;

    struct FrameCandidate { float freq; float prob; int bin; };
    std::vector<FrameCandidate> frameCandidates;

    std::vector<float> pitchBinFreqs;

    int framesFilled = 0;
};
