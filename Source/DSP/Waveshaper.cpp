#include "Waveshaper.h"
#include <cmath>

Waveshaper::Waveshaper()
{
    // 2x oversampling with IIR filtering - good balance of quality vs CPU
    // Uses 2 stages of IIR filtering for anti-aliasing
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        2,  // numChannels
        1,  // oversampling factor (2^1 = 2x)
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true  // use maximum quality
    );
}

Waveshaper::~Waveshaper() = default;

void Waveshaper::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    maxBlockSize = samplesPerBlock;
    
    // Prepare oversampling
    oversampling->initProcessing(static_cast<size_t>(samplesPerBlock));
    
    // Get oversampled sample rate for envelope coefficients
    double oversampledRate = sampleRate * oversampling->getOversamplingFactor();
    
    // Coefficients at original sample rate (for stress calculation)
    attackCoeff = 1.0f - std::exp(-1.0f / (0.001f * static_cast<float>(sampleRate)));
    releaseCoeff = 1.0f - std::exp(-1.0f / (0.05f * static_cast<float>(sampleRate)));
    
    // Coefficients at oversampled rate (for waveshaping envelope)
    attackCoeffOS = 1.0f - std::exp(-1.0f / (0.001f * static_cast<float>(oversampledRate)));
    releaseCoeffOS = 1.0f - std::exp(-1.0f / (0.05f * static_cast<float>(oversampledRate)));
    
    // Prepare DC blocking filters (~5Hz high-pass)
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;
    
    auto dcBlockCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 5.0);
    dcBlockerL.prepare(spec);
    dcBlockerR.prepare(spec);
    *dcBlockerL.coefficients = *dcBlockCoeffs;
    *dcBlockerR.coefficients = *dcBlockCoeffs;
    
    reset();
}

void Waveshaper::reset()
{
    levelFollower = 0.0f;
    currentStress = 0.0f;
    oversampling->reset();
    dcBlockerL.reset();
    dcBlockerR.reset();
}

float Waveshaper::applyPressureCurve(float pressure)
{
    // Nonlinear curve with emphasis above 0.6
    if (pressure < 0.6f)
        return pressure * 0.5f; // Gentler below 0.6
    else
        return 0.3f + (pressure - 0.6f) * 1.75f; // Steeper above 0.6
}

void Waveshaper::processOversampledBlock(juce::dsp::AudioBlock<float>& block, float pressure)
{
    const float curvedPressure = applyPressureCurve(pressure);
    const size_t numChannels = block.getNumChannels();
    const size_t numSamples = block.getNumSamples();
    
    for (size_t channel = 0; channel < numChannels; ++channel)
    {
        float* channelData = block.getChannelPointer(channel);
        
        for (size_t sample = 0; sample < numSamples; ++sample)
        {
            float input = channelData[sample];
            float inputLevel = std::abs(input);
            
            // Update level follower at oversampled rate
            if (inputLevel > levelFollower)
                levelFollower += attackCoeffOS * (inputLevel - levelFollower);
            else
                levelFollower += releaseCoeffOS * (inputLevel - levelFollower);
            
            // Apply waveshaping with pressure and input level
            channelData[sample] = processSample(input, curvedPressure, levelFollower);
        }
    }
}

float Waveshaper::process(juce::AudioBuffer<float>& buffer, float pressure)
{
    const float curvedPressure = applyPressureCurve(pressure);
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    // Calculate stress from input signal (before processing)
    float maxStress = 0.0f;
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const float* channelData = buffer.getReadPointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float inputLevel = std::abs(channelData[sample]);
            // Stress calculation: loud transients disproportionately increase stress
            float instantStress = curvedPressure * (inputLevel + inputLevel * inputLevel * 2.0f);
            maxStress = std::max(maxStress, instantStress);
        }
    }
    currentStress = maxStress;
    
    // Skip processing if pressure is essentially zero
    if (pressure < 0.001f)
        return currentStress;
    
    // Create audio block for oversampling
    juce::dsp::AudioBlock<float> block(buffer);
    block = block.getSubsetChannelBlock(0, static_cast<size_t>(numChannels));
    
    // Upsample
    juce::dsp::AudioBlock<float> oversampledBlock = oversampling->processSamplesUp(block);
    
    // Process at oversampled rate
    processOversampledBlock(oversampledBlock, pressure);
    
    // Downsample
    oversampling->processSamplesDown(block);
    
    // Apply DC blocking filter
    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        auto& dcBlocker = (channel == 0) ? dcBlockerL : dcBlockerR;
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            channelData[sample] = dcBlocker.processSample(channelData[sample]);
        }
    }
    
    return currentStress;
}

float Waveshaper::processSample(float input, float pressure, float inputLevel)
{
    if (pressure < 0.001f)
        return input;
    
    // Input gain scales with pressure
    float inputGain = 1.0f + pressure * 2.0f;
    float driven = input * inputGain;
    
    // Harmonic generation: odd harmonics at low pressure, odd+even at high pressure
    float oddHarmonics = std::tanh(driven * (1.0f + pressure));
    float evenHarmonics = driven * driven * std::copysign(1.0f, driven); // x^2 with sign preserved
    
    // Mix harmonic content based on pressure
    float harmonicMix = pressure * 0.3f;
    float shaped = oddHarmonics * (1.0f - harmonicMix) + 
                   (oddHarmonics * 0.7f + evenHarmonics * 0.3f) * harmonicMix;
    
    // Level-dependent response: quiet signals remain mostly unaffected
    float levelScale = inputLevel * 2.0f; // Boost effect for louder signals
    levelScale = juce::jlimit(0.0f, 1.0f, levelScale);
    
    float effectAmount = pressure * levelScale;
    return input * (1.0f - effectAmount) + shaped * effectAmount;
}
