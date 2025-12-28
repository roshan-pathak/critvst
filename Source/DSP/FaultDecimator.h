#pragma once

#include <JuceHeader.h>
#include <random>

/**
 * FaultDecimator: Sample & Hold decimator reactive to stress and fault parameter.
 * Produces dynamic sample-rate reduction with jitter for broken-digital artifacts.
 */
class FaultDecimator
{
public:
    FaultDecimator() = default;
    ~FaultDecimator() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // Process in-place buffer using per-block stress value (0..1)
    void process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& faultSmooth, float currentStress, float momentum);
    // Overload that accepts a dsp::AudioBlock (for oversampled processing)
    void process(juce::dsp::AudioBlock<float>& block, juce::SmoothedValue<float>& faultSmooth, float currentStress, float momentum);

private:
    double currentSampleRate{44100.0};
    int maxBlockSize{512};

    // state
    float phasor{0.0f};
    float heldL{0.0f}, heldR{0.0f};

    // jitter/randomness
    std::mt19937 rng;
    std::uniform_real_distribution<float> jitterDist{0.90f, 1.10f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FaultDecimator)
};
