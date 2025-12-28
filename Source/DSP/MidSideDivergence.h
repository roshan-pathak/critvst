#pragma once

#include <JuceHeader.h>

/**
 * Mid/Side divergence controlled by Split parameter
 * - Independent saturation curves for left/right channels
 * - Micro timing offset between channels (≤1 ms, tempo-agnostic)
 * - Mid/Side harmonic imbalance
 * - Mono compatibility preserved up to ~0.7
 * - High values reduce correlation but avoid full phase inversion
 */
class MidSideDivergence
{
public:
    MidSideDivergence() = default;
    
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Process audio with split divergence (0-1) and momentum for timing
    void process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& splitSmooth, float momentum);
    
private:
    // Delay buffer for micro timing offset
    juce::AudioBuffer<float> timingDelayBuffer;
    int writePosition{0};
    int delayBufferSize{0};
    
    double currentSampleRate{44100.0};
    
    // simple highpass state for side channel
    float hpA{0.0f}, hpB{0.0f}, hpAlpha{0.0f};
    
    // Convert stereo to mid/side
    void convertToMidSide(juce::AudioBuffer<float>& buffer);
    
    // Convert mid/side back to stereo
    void convertToStereo(juce::AudioBuffer<float>& buffer);
    
    // Apply divergence processing with independent L/R saturation
    // Accepts a SmoothedValue so split can be sampled per-sample
    void processMidSide(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& splitSmooth);
    
    // Apply micro timing offset
    void applyTimingOffset(juce::AudioBuffer<float>& buffer, float split, float momentum);
    
    float getInterpolatedSample(const float* channelData, float readPosition);
};
