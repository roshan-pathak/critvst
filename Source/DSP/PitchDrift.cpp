#include "PitchDrift.h"
#include <cmath>

void PitchDrift::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    
    // Allocate delay buffer for pitch shifting (50ms max delay for sub-ms smear)
    delayBufferSize = static_cast<int>(sampleRate * 0.05);
    delayBuffer.setSize(2, delayBufferSize);
    delayBuffer.clear();
    
    // Seed random for time smear
    random.setSeedRandomly();
    
    reset();
}

void PitchDrift::reset()
{
    delayBuffer.clear();
    writePosition = 0;
    envelopeFollower = 0.0f;
    spectralCentroid = 0.0f;
    timeSmearAmount = 0.0f;
}

void PitchDrift::process(juce::AudioBuffer<float>& buffer, float fault, float stress, float momentum)
{
    // Fault effect is multiplied by current Pressure value (stress)
    float effectiveIntensity = fault * stress;
    
    if (effectiveIntensity < 0.001f)
        return;
    
    const int numChannels = std::min(buffer.getNumChannels(), delayBuffer.getNumChannels());
    const int numSamples = buffer.getNumSamples();
    
    // Momentum affects speed of drift onset and correction
    float driftSpeed = 0.001f + momentum * 0.01f;
    
    // Generate random values less frequently for efficiency (every 16 samples)
    int randomUpdateCounter = 0;
    float nextRandomValue = random.nextFloat();
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Calculate signal-following modulation from amplitude
        float sumAbs = 0.0f;
        for (int channel = 0; channel < numChannels; ++channel)
        {
            sumAbs += std::abs(buffer.getSample(channel, sample));
        }
        
        float currentLevel = sumAbs / static_cast<float>(numChannels);
        envelopeFollower = updateEnvelopeFollower(currentLevel, momentum);
        
        // Derive pitch drift from envelope follower (no LFO)
        // Creates a signal-following micro pitch shift
        float driftModulation = (envelopeFollower - 0.5f) * 2.0f; // -1 to 1
        
        // Update random value every 16 samples for efficiency
        if (randomUpdateCounter == 0)
        {
            nextRandomValue = random.nextFloat();
        }
        randomUpdateCounter = (randomUpdateCounter + 1) % 16;
        
        // Sub-millisecond time smear with bounded randomization
        float smearTarget = nextRandomValue * 0.001f * effectiveIntensity;
        timeSmearAmount += (smearTarget - timeSmearAmount) * driftSpeed;
        
        // Calculate delay time in samples: micro pitch drift + time smear
        float sampleRateF = static_cast<float>(currentSampleRate);
        float pitchDriftSamples = driftModulation * effectiveIntensity * 0.001f * sampleRateF; // ±1ms
        float smearSamples = timeSmearAmount * sampleRateF; // Up to 1ms
        float totalDelaySamples = 5.0f + pitchDriftSamples + smearSamples;
        totalDelaySamples = juce::jlimit(1.0f, static_cast<float>(delayBufferSize) - 10.0f, totalDelaySamples);
        
        for (int channel = 0; channel < numChannels; ++channel)
        {
            // Write to delay buffer
            delayBuffer.setSample(channel, writePosition, buffer.getSample(channel, sample));
            
            // Phase skew between channels for partial bands
            float phaseSkew = (channel == 1) ? effectiveIntensity * 0.0005f * sampleRateF : 0.0f;
            
            // Read from delay buffer with modulation and phase skew
            float readPosition = static_cast<float>(writePosition) - totalDelaySamples - phaseSkew;
            if (readPosition < 0.0f)
                readPosition += static_cast<float>(delayBufferSize);
            
            float delayedSample = getInterpolatedSample(delayBuffer.getReadPointer(channel), readPosition);
            
            // Mix dry and wet - subtle to sound "strained" not detuned
            float dry = buffer.getSample(channel, sample);
            float wet = delayedSample;
            float mix = effectiveIntensity * 0.2f; // Keep it subtle
            
            float processedSample = dry * (1.0f - mix) + wet * mix;
            
            // HYPERPOP/RAGE DISTORTION: Aggressive digital clipping with asymmetry
            // Scales with fault intensity - gets more extreme as fault increases
            if (effectiveIntensity > 0.01f)
            {
                // Pre-gain to push into distortion harder
                float distortionGain = 1.0f + effectiveIntensity * 8.0f; // Up to 9x gain
                processedSample *= distortionGain;
                
                // Asymmetric hard clipping (different thresholds for positive/negative)
                // Creates harsher, more digital character
                float clipThreshold = 1.0f - effectiveIntensity * 0.6f; // Lower threshold at higher fault
                float positiveClip = clipThreshold;
                float negativeClip = clipThreshold * 0.7f; // Asymmetric for aggression
                
                if (processedSample > positiveClip)
                    processedSample = positiveClip + (processedSample - positiveClip) * 0.1f; // Hard clip with slight softness
                else if (processedSample < -negativeClip)
                    processedSample = -negativeClip + (processedSample + negativeClip) * 0.1f;
                
                // Post-gain makeup to compensate
                processedSample *= 0.5f / distortionGain;
            }
            
            buffer.setSample(channel, sample, processedSample);
        }
        
        writePosition = (writePosition + 1) % delayBufferSize;
    }
}

float PitchDrift::updateEnvelopeFollower(float input, float momentum)
{
    // Momentum controls reaction speed
    float speed = 0.01f + momentum * 0.1f;
    float target = input;
    envelopeFollower += (target - envelopeFollower) * speed;
    return envelopeFollower;
}

float PitchDrift::getInterpolatedSample(const float* channelData, float readPosition)
{
    // Hermite interpolation for smoother pitch shifting (reduces artifacts)
    int index1 = static_cast<int>(readPosition);
    int index0 = (index1 - 1 + delayBufferSize) % delayBufferSize;
    int index2 = (index1 + 1) % delayBufferSize;
    int index3 = (index1 + 2) % delayBufferSize;
    float frac = readPosition - static_cast<float>(index1);
    
    float y0 = channelData[index0];
    float y1 = channelData[index1];
    float y2 = channelData[index2];
    float y3 = channelData[index3];
    
    // Hermite polynomial coefficients
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}
