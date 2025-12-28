#pragma once

#include <JuceHeader.h>

/**
 * Dynamic soft limiting controlled by Resolve parameter
 * Features:
 * - 1ms lookahead for cleaner limiting (catches transients before they clip)
 * - Threshold tied to Pressure (more limiting under higher stress)
 * - Harmonic smoothing / density reduction at high resolve
 * - Transient preservation logic when under heavy stress
 * - Never fully removes instability; only constrains it
 * - Acts post-instability but pre-output
 */
class SoftLimiter
{
public:
    SoftLimiter() = default;
    
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Process audio with resolve (0-1), stress from Pressure, and momentum
    // Resolve strength increases as Pressure increases
    void process(juce::AudioBuffer<float>& buffer, float resolve, float stress, float momentum);
    
    // Returns the latency in samples introduced by lookahead
    int getLatencySamples() const { return lookaheadSamples; }
    
private:
    float envelope{1.0f};
    float attackCoeff{0.0f};
    float releaseCoeff{0.0f};
    float transientDetector{0.0f};
    double currentSampleRate{44100.0};
    
    // Lookahead delay buffer (stereo)
    juce::AudioBuffer<float> lookaheadBuffer;
    int lookaheadSamples{0};
    int writePosition{0};
    int bufferSize{0};
    
    void updateCoefficients(float momentum);
    float getTransientPreservation(float input, float stress);
};
