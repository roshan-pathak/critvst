#include "SoftLimiter.h"
#include <cmath>

void SoftLimiter::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentSampleRate = sampleRate;
    // No lookahead required for upward compression; reset state
    lookaheadSamples = 0;
    bufferSize = 0;
    lookaheadBuffer.setSize(0, 0);
    writePosition = 0;

    // default coefficients
    attackCoeff = 1.0f - std::exp(-1.0f / (0.001f * static_cast<float>(sampleRate)));
    releaseCoeff = 1.0f - std::exp(-1.0f / (0.2f * static_cast<float>(sampleRate)));
    reset();
}

void SoftLimiter::reset()
{
    envelope = 1.0f;
    transientDetector = 0.0f;
    writePosition = 0;
}

void SoftLimiter::process(juce::AudioBuffer<float>& buffer, float resolve, float stress, float momentum)
{
    if (resolve < 0.001f)
        return;

    // Effective resolve scaled by stress
    float effectiveResolve = resolve * (0.3f + stress * 0.7f);

    // coefficients governed by momentum for envelope follower
    updateCoefficients(momentum);
    float attackCoeffLocal = attackCoeff;
    float releaseCoeffLocal = releaseCoeff;

    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    // simple first-order crossover at 2kHz
    float crossover = 2000.0f;
    float wc = 2.0f * static_cast<float>(M_PI) * crossover;
    float T = 1.0f / static_cast<float>(currentSampleRate);
    float alpha = wc * T / (1.0f + wc * T);

    // per-channel one-pole lowpass for low band, high = input - low
    std::vector<float> lowState(numChannels, 0.0f);
    // per-channel envelope states used by upward compressor (driven by momentum coeffs)
    std::vector<float> envelopeState(numChannels, 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        float lowL = 0.0f, lowR = 0.0f;
        float inL = buffer.getSample(0, i);
        float inR = (numChannels > 1) ? buffer.getSample(1, i) : inL;

        // Lowpass (simple)
        lowState[0] = lowState[0] + alpha * (inL - lowState[0]);
        lowL = lowState[0];
        if (numChannels > 1)
        {
            lowState[1] = lowState[1] + alpha * (inR - lowState[1]);
            lowR = lowState[1];
        }

        float highL = inL - lowL;
        float highR = inR - lowR;

        // Upward compression: boost quiet parts of high band
        float thresh = 0.02f; // threshold for quiet parts

        // update envelope follower per-channel (attack/release governed by momentum)
        float magL = std::abs(highL);
        envelopeState[0] += (magL - envelopeState[0]) * ((magL > envelopeState[0]) ? attackCoeffLocal : releaseCoeffLocal);
        float magR = std::abs(highR);
        if (numChannels > 1)
            envelopeState[1] += (magR - envelopeState[1]) * ((magR > envelopeState[1]) ? attackCoeffLocal : releaseCoeffLocal);

        auto upward = [&](float x, float env)->float {
            float ax = env;
            if (ax < thresh)
            {
                float gain = 1.0f + (thresh - ax) / thresh * effectiveResolve * 12.0f; // more aggressive upward boost
                return x * gain;
            }
            return x;
        };

        float procHighL = upward(highL, envelopeState[0]);
        float procHighR = upward(highR, envelopeState[1]);

        // Recombine
        float outL = lowL + procHighL;
        float outR = lowR + procHighR;

        // Final hard clip at -0.1 dB (~0.988)
        float ceiling = 0.988f;
        outL = juce::jlimit(-ceiling, ceiling, outL);
        outR = juce::jlimit(-ceiling, ceiling, outR);

        buffer.setSample(0, i, outL);
        if (numChannels > 1)
            buffer.setSample(1, i, outR);
    }
}

void SoftLimiter::updateCoefficients(float momentum)
{
    // derive simple attack/release coeffs from momentum (ms ranges)
    float attackMs = juce::jmap(momentum, 0.0f, 50.0f);
    float releaseMs = juce::jmap(momentum, 10.0f, 1000.0f);
    attackCoeff = 1.0f - std::exp(-1.0f / (attackMs * 0.001f * static_cast<float>(currentSampleRate) + 1e-9f));
    releaseCoeff = 1.0f - std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(currentSampleRate) + 1e-9f));
}

float SoftLimiter::getTransientPreservation(float input, float)
{
    // Detect transients for preservation logic
    float delta = input - transientDetector;
    transientDetector = input;
    
    // Transient strength: sharp increases indicate transients
    float transientStrength = juce::jlimit(0.0f, 1.0f, delta * 10.0f);
    
    return transientStrength;
}
