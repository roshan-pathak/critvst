#pragma once

#include <JuceHeader.h>

/**
 * Level-dependent waveshaper controlled by Pressure
 * Features 2x oversampling to reduce aliasing from harmonic generation
 * Quiet input remains mostly unaffected
 * Loud transients disproportionately increase stress
 * Generates odd harmonics → odd+even as pressure increases
 * Includes DC blocking filter post-waveshaping
 */
class Waveshaper
{
public:
    Waveshaper();
    ~Waveshaper();
    
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Process audio with pressure (0-1, nonlinear emphasis above 0.6)
    // Returns current stress signal for other modules
    float process(juce::AudioBuffer<float>& buffer, float pressure);
    
    float getCurrentStress() const { return currentStress; }
    
private:
    // Apply nonlinear curve to pressure
    float applyPressureCurve(float pressure);
    
    // Level-dependent harmonic generation
    float processSample(float input, float pressure, float inputLevel);
    
    // Process oversampled block
    void processOversampledBlock(juce::dsp::AudioBlock<float>& block, float pressure);
    
    // Level follower for dynamic response
    float levelFollower{0.0f};
    float currentStress{0.0f};
    float attackCoeff{0.0f};
    float releaseCoeff{0.0f};
    float attackCoeffOS{0.0f};  // Oversampled attack coefficient
    float releaseCoeffOS{0.0f}; // Oversampled release coefficient
    
    // 2x oversampling with IIR filtering (good balance of quality vs CPU)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    
    // DC blocking filter (high-pass at ~5Hz)
    juce::dsp::IIR::Filter<float> dcBlockerL;
    juce::dsp::IIR::Filter<float> dcBlockerR;
    
    double currentSampleRate{44100.0};
    int maxBlockSize{512};
};
