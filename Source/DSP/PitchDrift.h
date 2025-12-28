#pragma once

#include <JuceHeader.h>

/**
 * Signal-following micro pitch drift + HYPERPOP/RAGE DISTORTION controlled by Fault (scaled by Pressure)
 * No LFO - derived from amplitude + spectral centroid
 * Includes phase skew between partial bands and sub-millisecond time smear
 * DISTORTION: Aggressive asymmetric hard clipping with pre-gain for hyperpop/rage/trap character
 * Designed to sound "strained" and aggressive, not smooth
 */
class PitchDrift
{
public:
    PitchDrift() = default;
    
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Process audio with fault (0-1) and stress from Pressure
    // At low stress, fault has minimal audible impact
    void process(juce::AudioBuffer<float>& buffer, float fault, float stress, float momentum);
    
private:
    // Delay line for pitch shifting
    juce::AudioBuffer<float> delayBuffer;
    int writePosition{0};
    int delayBufferSize{0};
    
    // Signal-following state
    float envelopeFollower{0.0f};
    float spectralCentroid{0.0f};
    
    // Time smear randomization
    juce::Random random;
    float timeSmearAmount{0.0f};
    
    double currentSampleRate{44100.0};
    
    float getInterpolatedSample(const float* channelData, float readPosition);
    float updateEnvelopeFollower(float input, float momentum);
};
