#pragma once

#include <JuceHeader.h>

/**
 * Dynamic EQ tilt controlled by Pressure
 * Features:
 * - Low-end tightening (high-pass filter that increases with pressure)
 * - Low shelf cut for mud reduction under stress
 * - High-mid presence boost (2-4kHz)
 * - High shelf for air/brightness
 * All filter amounts scale with pressure for level-dependent tonal shaping
 */
class DynamicEQ
{
public:
    DynamicEQ() = default;
    
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Process audio with pressure (0-1) for tilt control
    void process(juce::AudioBuffer<float>& buffer, float pressure);
    
private:
    // Per-channel filter arrays (stereo)
    juce::dsp::IIR::Filter<float> lowCutFilter[2];      // High-pass for low-end tightening
    juce::dsp::IIR::Filter<float> lowShelfFilter[2];    // Low shelf for mud reduction
    juce::dsp::IIR::Filter<float> midBoostFilter[2];    // Presence peak (2-4kHz)
    juce::dsp::IIR::Filter<float> highShelfFilter[2];   // High shelf for air
    
    double currentSampleRate{44100.0};
    float lastPressure{-1.0f}; // Track for efficient coefficient updates
    
    void updateFilters(float pressure);
};
