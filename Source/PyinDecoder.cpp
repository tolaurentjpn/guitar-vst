#include "PyinDecoder.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace
{
    constexpr float kLogFloor = 1.0e-12f;

    float safeLog (float x) noexcept
    {
        return std::log (juce::jmax (x, kLogFloor));
    }

    bool isLocalMinimum (const std::vector<float>& values, int index)
    {
        if (index <= 0 || index >= static_cast<int> (values.size()) - 1)
            return false;

        return values[static_cast<size_t> (index)] <= values[static_cast<size_t> (index - 1)]
            && values[static_cast<size_t> (index)] <= values[static_cast<size_t> (index + 1)];
    }
}

void PyinDecoder::prepare (double newSampleRate, float newFmin, float newFmax, int newFrameLength, int newHopSize)
{
    sampleRate = newSampleRate;
    fmin = newFmin;
    fmax = newFmax;
    frameLength = newFrameLength;
    hopSize = newHopSize;

    minPeriod = juce::jmax (2, static_cast<int> (std::floor (sampleRate / static_cast<double> (fmax))));
    maxPeriod = juce::jmin (newFrameLength / 2 - 1,
                            static_cast<int> (std::ceil (sampleRate / static_cast<double> (fmin))));
    numPeriods = juce::jmax (0, maxPeriod - minPeriod + 1);

    nBinsPerSemitone = static_cast<int> (std::ceil (1.0f / pitchResolution));
    nPitchBins = static_cast<int> (std::floor (12.0f * static_cast<float> (nBinsPerSemitone)
                                               * std::log2 (fmax / fmin)))
               + 1;
    nPitchBins = juce::jmax (1, nPitchBins);
    numStates = 2 * nPitchBins;

    const int maxSemitonesPerFrame = juce::roundToInt (maxTransitionRate * 12.0f
                                                       * static_cast<float> (hopSize)
                                                       / static_cast<float> (sampleRate));
    transitionWidth = juce::jmax (1, maxSemitonesPerFrame) * nBinsPerSemitone + 1;

    thresholds.resize (static_cast<size_t> (nThresholds + 1));
    betaProbs.resize (static_cast<size_t> (nThresholds));

    for (int i = 0; i <= nThresholds; ++i)
        thresholds[static_cast<size_t> (i)] = static_cast<float> (i) / static_cast<float> (nThresholds);

    for (int i = 0; i < nThresholds; ++i)
    {
        const float upper = regularizedBetaInc (thresholds[static_cast<size_t> (i + 1)], 2.0f, 18.0f);
        const float lower = regularizedBetaInc (thresholds[static_cast<size_t> (i)], 2.0f, 18.0f);
        betaProbs[static_cast<size_t> (i)] = juce::jmax (0.0f, upper - lower);
    }

    pitchBinFreqs.resize (static_cast<size_t> (nPitchBins));
    for (int bin = 0; bin < nPitchBins; ++bin)
        pitchBinFreqs[static_cast<size_t> (bin)] = fmin * std::pow (2.0f,
            static_cast<float> (bin) / (12.0f * static_cast<float> (nBinsPerSemitone)));

    cmnd.assign (static_cast<size_t> (maxPeriod + 1), 1.0f);
    buildTransitionMatrix();

    observationBuffer.assign (static_cast<size_t> (numStates * viterbiFrames), 0.0f);
    viterbiCost.assign (static_cast<size_t> (numStates), 0.0f);
    viterbiPath.assign (static_cast<size_t> (viterbiFrames), 0);

    reset();
}

void PyinDecoder::reset()
{
    std::fill (observationBuffer.begin(), observationBuffer.end(), 0.0f);
    framesFilled = 0;
    lastBestCandidateFreq = 0.0f;
    lastBestCandidateProb = 0.0f;
    frameCandidates.clear();

    const float initLogProb = safeLog (1.0f / static_cast<float> (numStates));
    std::fill (viterbiCost.begin(), viterbiCost.end(), initLogProb);
}

void PyinDecoder::buildTransitionMatrix()
{
    transitionMatrix.assign (static_cast<size_t> (numStates * numStates), 0.0f);

    std::vector<float> voicedTransition (static_cast<size_t> (nPitchBins * nPitchBins), 0.0f);
    for (int from = 0; from < nPitchBins; ++from)
    {
        float rowSum = 0.0f;
        for (int to = 0; to < nPitchBins; ++to)
        {
            const int distance = std::abs (to - from);
            if (distance <= transitionWidth)
            {
                const float weight = 1.0f - static_cast<float> (distance) / static_cast<float> (transitionWidth + 1);
                voicedTransition[static_cast<size_t> (from * nPitchBins + to)] = weight;
                rowSum += weight;
            }
        }

        if (rowSum > 0.0f)
        {
            for (int to = 0; to < nPitchBins; ++to)
                voicedTransition[static_cast<size_t> (from * nPitchBins + to)] /= rowSum;
        }
        else
        {
            voicedTransition[static_cast<size_t> (from * nPitchBins + from)] = 1.0f;
        }
    }

    const float stayVoiced = 1.0f - switchProb;
    const float stayUnvoiced = 1.0f - switchProb;

    for (int fromVoice = 0; fromVoice < 2; ++fromVoice)
    {
        for (int fromBin = 0; fromBin < nPitchBins; ++fromBin)
        {
            const int fromState = fromVoice * nPitchBins + fromBin;

            for (int toVoice = 0; toVoice < 2; ++toVoice)
            {
                for (int toBin = 0; toBin < nPitchBins; ++toBin)
                {
                    const int toState = toVoice * nPitchBins + toBin;
                    float prob = 0.0f;

                    if (fromVoice == 0 && toVoice == 0)
                        prob = stayVoiced * voicedTransition[static_cast<size_t> (fromBin * nPitchBins + toBin)];
                    else if (fromVoice == 0 && toVoice == 1)
                        prob = switchProb / static_cast<float> (nPitchBins);
                    else if (fromVoice == 1 && toVoice == 0)
                        prob = switchProb * voicedTransition[static_cast<size_t> (fromBin * nPitchBins + toBin)];
                    else
                        prob = stayUnvoiced / static_cast<float> (nPitchBins);

                    transitionMatrix[static_cast<size_t> (fromState * numStates + toState)] = safeLog (prob);
                }
            }
        }
    }
}

void PyinDecoder::computeCmnd (const float* data, int numSamples)
{
    const int halfSize = numSamples / 2;
    if (halfSize < 2 || maxPeriod < minPeriod)
    {
        std::fill (cmnd.begin(), cmnd.end(), 1.0f);
        return;
    }

    const int effectiveMaxTau = juce::jmin (maxPeriod, halfSize - 1);
    cmnd[0] = 1.0f;

    float runningSum = 0.0f;
    for (int tau = 1; tau <= effectiveMaxTau; ++tau)
    {
        float sum = 0.0f;
        for (int i = 0; i < halfSize; ++i)
        {
            const float delta = data[i] - data[i + tau];
            sum += delta * delta;
        }

        runningSum += sum;
        cmnd[static_cast<size_t> (tau)] = runningSum > 0.0f
            ? sum * static_cast<float> (tau) / runningSum
            : 1.0f;
    }
}

float PyinDecoder::parabolicShift (int tauIndex) const
{
    const int tau = minPeriod + tauIndex;
    if (tau <= 0 || tau >= static_cast<int> (cmnd.size()) - 1)
        return 0.0f;

    const float s0 = cmnd[static_cast<size_t> (tau - 1)];
    const float s1 = cmnd[static_cast<size_t> (tau)];
    const float s2 = cmnd[static_cast<size_t> (tau + 1)];
    const float denom = s0 - 2.0f * s1 + s2;

    if (std::abs (denom) <= 1.0e-6f)
        return 0.0f;

    return 0.5f * (s0 - s2) / denom;
}

int PyinDecoder::findLocalMinimumTau (int centreTau) const
{
    centreTau = juce::jlimit (minPeriod, maxPeriod, centreTau);
    int localMinTau = centreTau;

    for (int tau = juce::jmax (minPeriod, centreTau - 3); tau <= juce::jmin (maxPeriod, centreTau + 3); ++tau)
    {
        if (tau > 0 && tau < static_cast<int> (cmnd.size()) - 1
            && cmnd[static_cast<size_t> (tau)] <= cmnd[static_cast<size_t> (localMinTau)]
            && cmnd[static_cast<size_t> (tau)] <= cmnd[static_cast<size_t> (tau - 1)]
            && cmnd[static_cast<size_t> (tau)] <= cmnd[static_cast<size_t> (tau + 1)])
        {
            localMinTau = tau;
        }
    }

    return localMinTau;
}

float PyinDecoder::scoreHarmonicClarity (int tau) const
{
    if (tau < minPeriod || tau > maxPeriod)
        return 0.0f;

    const int fundTau = findLocalMinimumTau (tau);
    const float fundYin = cmnd[static_cast<size_t> (fundTau)];

    if (fundTau / 2 >= minPeriod)
    {
        const int halfTau = findLocalMinimumTau (fundTau / 2);
        if (cmnd[static_cast<size_t> (halfTau)] < fundYin * 0.88f)
            return 0.0f;
    }

    if (fundTau * 2 <= maxPeriod)
    {
        const int doubleTau = findLocalMinimumTau (fundTau * 2);
        if (cmnd[static_cast<size_t> (doubleTau)] < fundYin * 0.88f)
            return 0.0f;
    }

    float score = 1.0f - juce::jlimit (0.0f, 0.99f, fundYin);

    for (int harmonic = 2; harmonic <= 5; ++harmonic)
    {
        const int harmonicTau = fundTau / harmonic;
        if (harmonicTau < minPeriod)
            break;

        const int localTau = findLocalMinimumTau (harmonicTau);
        const float yin = cmnd[static_cast<size_t> (localTau)];
        score *= (1.0f - juce::jlimit (0.0f, 0.99f, yin));
    }

    return score;
}

void PyinDecoder::computeFrameObservations (const std::vector<float>& parabolicShifts)
{
    std::vector<float> yinProbs (static_cast<size_t> (numPeriods), 0.0f);
    std::vector<int> troughIndices;
    troughIndices.reserve (static_cast<size_t> (numPeriods));

    for (int i = 0; i < numPeriods; ++i)
    {
        if (isLocalMinimum (cmnd, minPeriod + i))
            troughIndices.push_back (i);
    }

    if (! troughIndices.empty())
    {
        const int numTroughs = static_cast<int> (troughIndices.size());
        std::vector<float> troughHeights (static_cast<size_t> (numTroughs));
        for (int t = 0; t < numTroughs; ++t)
            troughHeights[static_cast<size_t> (t)] = cmnd[static_cast<size_t> (minPeriod + troughIndices[static_cast<size_t> (t)])];

        std::vector<float> probs (static_cast<size_t> (numTroughs), 0.0f);

        for (int thresholdIndex = 0; thresholdIndex < nThresholds; ++thresholdIndex)
        {
            const float threshold = thresholds[static_cast<size_t> (thresholdIndex + 1)];
            int position = 0;
            int troughsBelow = 0;

            for (int t = 0; t < numTroughs; ++t)
            {
                if (troughHeights[static_cast<size_t> (t)] < threshold)
                {
                    probs[static_cast<size_t> (t)] += boltzmannPmf (position, numTroughs, boltzmannParameter)
                                                      * betaProbs[static_cast<size_t> (thresholdIndex)];
                    ++position;
                    ++troughsBelow;
                }
            }

            if (troughsBelow == 0)
            {
                int globalMinIndex = 0;
                float globalMinHeight = troughHeights[0];
                for (int t = 1; t < numTroughs; ++t)
                {
                    if (troughHeights[static_cast<size_t> (t)] < globalMinHeight)
                    {
                        globalMinHeight = troughHeights[static_cast<size_t> (t)];
                        globalMinIndex = t;
                    }
                }

                int thresholdsBelowMin = 0;
                for (int ti = 0; ti < nThresholds; ++ti)
                {
                    if (globalMinHeight < thresholds[static_cast<size_t> (ti + 1)])
                        ++thresholdsBelowMin;
                }

                float betaSum = 0.0f;
                for (int ti = 0; ti < thresholdsBelowMin; ++ti)
                    betaSum += betaProbs[static_cast<size_t> (ti)];

                probs[static_cast<size_t> (globalMinIndex)] += noTroughProb * betaSum;
            }
        }

        for (int t = 0; t < numTroughs; ++t)
            yinProbs[static_cast<size_t> (troughIndices[static_cast<size_t> (t)])] = probs[static_cast<size_t> (t)];
    }

    int maxPeriodIndex = -1;
    float maxPeriodProb = 0.0f;
    for (int periodIndex = 0; periodIndex < numPeriods; ++periodIndex)
    {
        if (yinProbs[static_cast<size_t> (periodIndex)] > maxPeriodProb)
        {
            maxPeriodProb = yinProbs[static_cast<size_t> (periodIndex)];
            maxPeriodIndex = periodIndex;
        }
    }

    if (maxPeriodIndex >= 0)
    {
        const int tau = minPeriod + maxPeriodIndex;
        const int doubleTau = tau * 2;
        if (doubleTau <= maxPeriod)
        {
            const int doubleIndex = doubleTau - minPeriod;
            if (doubleIndex >= 0 && doubleIndex < numPeriods)
            {
                const float harmFreq = static_cast<float> (sampleRate / static_cast<double> (tau));
                const float fundFreq = static_cast<float> (sampleRate / static_cast<double> (doubleTau));
                const float ratio = harmFreq / fundFreq;

                const float fundYin = cmnd[static_cast<size_t> (findLocalMinimumTau (doubleTau))];
                const float harmYin = cmnd[static_cast<size_t> (findLocalMinimumTau (tau))];

                // Guitar timbres below ~300 Hz carry the fundamental reliably; above that,
                // missing-fundamental tones often lock an octave high on the 2nd harmonic.
                if (harmFreq > 300.0f
                    && ratio > 1.85f && ratio < 2.15f
                    && fundYin < harmYin * 0.88f
                    && yinProbs[static_cast<size_t> (doubleIndex)] <= 0.0f
                    && scoreHarmonicClarity (tau) <= 0.0f)
                {
                    yinProbs[static_cast<size_t> (doubleIndex)] = yinProbs[static_cast<size_t> (maxPeriodIndex)] * 0.85f;
                    yinProbs[static_cast<size_t> (maxPeriodIndex)] *= 0.35f;
                }
            }
        }
    }

    std::vector<float> frameObs (static_cast<size_t> (numStates), 0.0f);
    float voicedSum = 0.0f;
    float bestCandidateFreq = 0.0f;
    float bestCandidateProb = 0.0f;

    frameCandidates.clear();
    frameCandidates.reserve (static_cast<size_t> (numPeriods));

    for (int periodIndex = 0; periodIndex < numPeriods; ++periodIndex)
    {
        const float prob = yinProbs[static_cast<size_t> (periodIndex)];
        if (prob <= 0.0f)
            continue;

        const float period = static_cast<float> (minPeriod + periodIndex) + parabolicShifts[static_cast<size_t> (periodIndex)];
        if (period <= 0.0f)
            continue;

        const float f0 = static_cast<float> (sampleRate / static_cast<double> (period));
        const float binExact = 12.0f * static_cast<float> (nBinsPerSemitone) * std::log2 (f0 / fmin);
        const int binIndex = juce::jlimit (0, nPitchBins - 1, static_cast<int> (std::round (binExact)));

        frameObs[static_cast<size_t> (binIndex)] += prob;
        voicedSum += prob;
        frameCandidates.push_back ({ f0, prob, binIndex });

        if (prob > bestCandidateProb)
        {
            bestCandidateProb = prob;
            bestCandidateFreq = f0;
        }
    }

    voicedSum = juce::jlimit (0.0f, 1.0f, voicedSum);
    const float unvoicedProb = juce::jmax (0.0f, 1.0f - voicedSum);
    const float unvoicedEach = unvoicedProb / static_cast<float> (nPitchBins);

    for (int bin = 0; bin < nPitchBins; ++bin)
    {
        frameObs[static_cast<size_t> (bin)] = juce::jmax (frameObs[static_cast<size_t> (bin)], kLogFloor);
        frameObs[static_cast<size_t> (nPitchBins + bin)] = juce::jmax (unvoicedEach, kLogFloor);
    }

    if (framesFilled >= viterbiFrames)
    {
        for (int state = 0; state < numStates; ++state)
        {
            for (int frame = 1; frame < viterbiFrames; ++frame)
                observationBuffer[static_cast<size_t> (state * viterbiFrames + frame - 1)]
                    = observationBuffer[static_cast<size_t> (state * viterbiFrames + frame)];
        }
    }
    else
    {
        ++framesFilled;
    }

    const int column = juce::jmin (framesFilled, viterbiFrames) - 1;
    for (int state = 0; state < numStates; ++state)
        observationBuffer[static_cast<size_t> (state * viterbiFrames + column)] = frameObs[static_cast<size_t> (state)];

    lastBestCandidateFreq = bestCandidateFreq;
    lastBestCandidateProb = bestCandidateProb;
}

void PyinDecoder::runViterbi()
{
    if (framesFilled <= 0)
        return;

    const float initLogProb = safeLog (1.0f / static_cast<float> (numStates));
    std::vector<float> previousCost (static_cast<size_t> (numStates), initLogProb);
    std::vector<std::vector<int>> backpointers (static_cast<size_t> (framesFilled));

    for (int frame = 0; frame < framesFilled; ++frame)
    {
        const int column = frame;
        std::vector<float> currentCost (static_cast<size_t> (numStates),
                                        -std::numeric_limits<float>::infinity());
        backpointers[static_cast<size_t> (frame)].assign (static_cast<size_t> (numStates), 0);

        for (int toState = 0; toState < numStates; ++toState)
        {
            const float obs = observationBuffer[static_cast<size_t> (toState * viterbiFrames + column)];
            float bestCost = -std::numeric_limits<float>::infinity();
            int bestPrev = 0;

            for (int fromState = 0; fromState < numStates; ++fromState)
            {
                const float cost = previousCost[static_cast<size_t> (fromState)]
                                 + transitionMatrix[static_cast<size_t> (fromState * numStates + toState)]
                                 + safeLog (obs);
                if (cost > bestCost)
                {
                    bestCost = cost;
                    bestPrev = fromState;
                }
            }

            currentCost[static_cast<size_t> (toState)] = bestCost;
            backpointers[static_cast<size_t> (frame)][static_cast<size_t> (toState)] = bestPrev;
        }

        previousCost.swap (currentCost);
    }

    int state = 0;
    float bestFinalCost = previousCost[0];
    for (int s = 1; s < numStates; ++s)
    {
        if (previousCost[static_cast<size_t> (s)] > bestFinalCost)
        {
            bestFinalCost = previousCost[static_cast<size_t> (s)];
            state = s;
        }
    }

    for (int frame = framesFilled - 1; frame >= 0; --frame)
    {
        viterbiPath[static_cast<size_t> (frame)] = state;
        state = backpointers[static_cast<size_t> (frame)][static_cast<size_t> (state)];
    }
}

void PyinDecoder::processFrame (const float* samples, int numSamples, float& outFrequency, float& outVoicedProb)
{
    outFrequency = 0.0f;
    outVoicedProb = 0.0f;

    if (numSamples < 4 || numPeriods <= 0 || numStates <= 0)
        return;

    computeCmnd (samples, numSamples);

    std::vector<float> parabolicShifts (static_cast<size_t> (numPeriods), 0.0f);
    for (int i = 0; i < numPeriods; ++i)
        parabolicShifts[static_cast<size_t> (i)] = parabolicShift (i);

    computeFrameObservations (parabolicShifts);
    runViterbi();

    const int lastFrame = framesFilled - 1;
    if (lastFrame < 0)
        return;

    const int decodedState = viterbiPath[static_cast<size_t> (lastFrame)];
    const bool voiced = decodedState < nPitchBins;
    const int binIndex = voiced ? decodedState : decodedState - nPitchBins;

    float voicedSum = 0.0f;
    const int column = lastFrame;
    for (int bin = 0; bin < nPitchBins; ++bin)
        voicedSum += observationBuffer[static_cast<size_t> (bin * viterbiFrames + column)];

    outVoicedProb = juce::jlimit (0.0f, 1.0f, voicedSum);

    if (voiced)
    {
        float outputFreq = pitchBinFreqs[static_cast<size_t> (binIndex)];

        for (const auto& candidate : frameCandidates)
        {
            if (candidate.bin != binIndex)
                continue;

            if (candidate.prob >= lastBestCandidateProb * 0.94f)
                outputFreq = candidate.freq;
        }

        if (outputFreq <= 0.0f && lastBestCandidateFreq > 0.0f)
            outputFreq = lastBestCandidateFreq;

        outFrequency = outputFreq;
    }
}

float PyinDecoder::boltzmannPmf (int position, int numItems, float lambda)
{
    if (numItems <= 0 || position < 0 || position >= numItems)
        return 0.0f;

    float normalizer = 0.0f;
    for (int i = 0; i < numItems; ++i)
        normalizer += std::exp (-lambda * static_cast<float> (i));

    if (normalizer <= 0.0f)
        return 0.0f;

    return std::exp (-lambda * static_cast<float> (position)) / normalizer;
}

float PyinDecoder::regularizedBetaInc (float x, float alpha, float beta)
{
    if (x <= 0.0f)
        return 0.0f;
    if (x >= 1.0f)
        return 1.0f;

    const float logBeta = std::lgamma (alpha) + std::lgamma (beta) - std::lgamma (alpha + beta);
    const float front = std::exp (std::log (x) * alpha + std::log (1.0f - x) * beta - logBeta) / alpha;

    float term = 1.0f;
    float sum = 1.0f;
    for (int n = 1; n <= 200; ++n)
    {
        term *= x * (alpha + static_cast<float> (n) - 1.0f)
              / (alpha + beta + static_cast<float> (n) - 1.0f);
        sum += term;
        if (term <= 1.0e-8f * sum)
            break;
    }

    return juce::jlimit (0.0f, 1.0f, front * sum);
}
