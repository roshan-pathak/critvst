#include "DynamicEQ.h"

void DynamicEQ::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    lastPressure = -1.0f; // Force coefficient update on first process
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;
    
    for (int i = 0; i < 2; ++i)
    {
        lowCutFilter[i].prepare(spec);
        lowShelfFilter[i].prepare(spec);
        midBoostFilter[i].prepare(spec);
        highShelfFilter[i].prepare(spec);
    }
    
    reset();
}

void DynamicEQ::reset()
{
    for (int i = 0; i < 2; ++i)
    {
        lowCutFilter[i].reset();
        lowShelfFilter[i].reset();
        midBoostFilter[i].reset();
        highShelfFilter[i].reset();
    }
    lastPressure = -1.0f;
}

void DynamicEQ::process(juce::AudioBuffer<float>& buffer, float pressure)
{
    if (pressure < 0.001f)
        return;
    
    // Only update coefficients if pressure changed significantly (efficiency)
    if (std::abs(pressure - lastPressure) > 0.01f)
    {
        updateFilters(pressure);
        lastPressure = pressure;
    }
    
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Process through filter chain: HP -> Low Shelf -> Mid Peak -> High Shelf
            float processed = lowCutFilter[channel].processSample(channelData[sample]);
            processed = lowShelfFilter[channel].processSample(processed);
            processed = midBoostFilter[channel].processSample(processed);
            processed = highShelfFilter[channel].processSample(processed);
            channelData[sample] = processed;
        }
    }
}

void DynamicEQ::updateFilters(float pressure)
{
    // 1. Low-end tightening: high-pass filter that increases with pressure
    // Subtle at low pressure, more aggressive at high pressure
    float lowCutFreq = 30.0f + pressure * pressure * 100.0f; // 30Hz to 130Hz (nonlinear)
    auto lowCutCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
        currentSampleRate, lowCutFreq, 0.707f
    );
    
    // 2. Low shelf cut: reduces mud in 150-300Hz region under stress
    float lowShelfFreq = 200.0f;
    float lowShelfGain = 1.0f - pressure * 0.15f; // Up to -1.5dB cut
    auto lowShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        currentSampleRate, lowShelfFreq, 0.707f, lowShelfGain
    );
    
    // 3. High-mid presence boost: 2-4kHz region for clarity and bite (CRUNCH zone)
    // Frequency sweeps slightly with pressure for dynamic character
    // More resonant (higher Q) and more gain for aggressive mid-frequency crunchiness
    float midFreq = 2500.0f + pressure * 1200.0f; // 2.5kHz to 3.7kHz
    float midQ = 2.0f + pressure * 1.0f; // More resonant: 2.0 to 3.0 (narrower, peakier)
    float midGain = 1.0f + pressure * pressure * 1.2f; // Up to +8dB boost (nonlinear for aggression)
    auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        currentSampleRate, midFreq, midQ, midGain
    );
    
    // 4. High shelf for air and brightness
    float highShelfFreq = 8000.0f;
    float highShelfGain = 1.0f + pressure * 0.2f; // Up to +1.5dB at high frequencies
    auto highShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        currentSampleRate, highShelfFreq, 0.707f, highShelfGain
    );
    
    for (int i = 0; i < 2; ++i)
    {
        *lowCutFilter[i].coefficients = *lowCutCoeffs;
        *lowShelfFilter[i].coefficients = *lowShelfCoeffs;
        *midBoostFilter[i].coefficients = *midCoeffs;
        *highShelfFilter[i].coefficients = *highShelfCoeffs;
    }
}
