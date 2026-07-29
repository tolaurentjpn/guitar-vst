#include "OctaveNet.h"
#include "OctaveNetWeights.h"

namespace OctaveNet
{
    namespace
    {
        float relu (float x) noexcept
        {
            return x > 0.0f ? x : 0.0f;
        }

        void dense (const float* input, int inDim,
                    const float* weights, const float* bias, int outDim,
                    float* output, bool applyRelu) noexcept
        {
            for (int o = 0; o < outDim; ++o)
            {
                float sum = bias[o];
                const float* row = weights + o * inDim;
                for (int i = 0; i < inDim; ++i)
                    sum += row[i] * input[i];

                output[o] = applyRelu ? relu (sum) : sum;
            }
        }
    }

    void evaluate (const float features[kNumFeatures], float& scoreLogit, float& voicedLogit) noexcept
    {
        float h1[kHidden1];
        float h2[kHidden2];
        float out[kNumOutputs];

        dense (features, kNumFeatures,
               OctaveNetWeights::w1, OctaveNetWeights::b1, kHidden1,
               h1, true);
        dense (h1, kHidden1,
               OctaveNetWeights::w2, OctaveNetWeights::b2, kHidden2,
               h2, true);
        dense (h2, kHidden2,
               OctaveNetWeights::w3, OctaveNetWeights::b3, kNumOutputs,
               out, false);

        scoreLogit = out[0];
        voicedLogit = out[1];
    }

    Result selectBest (const float features[kMaxCandidates][kNumFeatures],
                       int numCandidates,
                       float voicedThreshold) noexcept
    {
        Result result;
        if (numCandidates <= 0)
            return result;

        numCandidates = numCandidates > kMaxCandidates ? kMaxCandidates : numCandidates;

        for (int c = 0; c < numCandidates; ++c)
        {
            float scoreLogit = 0.0f;
            float voicedLogit = 0.0f;
            evaluate (features[c], scoreLogit, voicedLogit);

            if (scoreLogit > result.bestScore)
            {
                result.bestScore = scoreLogit;
                result.bestCandidateIndex = c;
                result.voicedProbability = sigmoid (voicedLogit);
            }
        }

        result.voiced = result.bestCandidateIndex >= 0
                     && result.voicedProbability >= voicedThreshold;
        return result;
    }
}
