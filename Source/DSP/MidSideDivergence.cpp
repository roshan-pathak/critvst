#include "MidSideDivergence.h"
#include <cmath>

void MidSideDivergence::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    
    // Allocate delay buffer for side-channel short delays (50ms max)
    delayBufferSize = static_cast<int>(sampleRate * 0.050) + 1;
    timingDelayBuffer.setSize(2, delayBufferSize);
    timingDelayBuffer.clear();

    // Prepare simple 1-pole highpass state for side channel (200Hz)
    hpA = 0.0f;
    hpB = 0.0f;
    float rc = 1.0f / (2.0f * float(M_PI) * 200.0f);
    float dt = 1.0f / static_cast<float>(sampleRate);
    hpAlpha = rc / (rc + dt);
}

void MidSideDivergence::reset()
{
    timingDelayBuffer.clear();
    writePosition = 0;
    hpA = hpB = 0.0f;
}

void MidSideDivergence::process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& splitSmooth, float momentum)
{
    if (buffer.getNumChannels() < 2)
        return;
    // Convert to mid/side
    convertToMidSide(buffer);

    // Delay the side channel with short delay lines (0.1ms..15ms) and slight feedback
    const int numSamples = buffer.getNumSamples();
    float* mid = buffer.getWritePointer(0);
    float* side = buffer.getWritePointer(1);

    // per-sample processing using sample-accurate split
    float minMs = 0.1f;
    float maxMs = 25.0f; // extend max delay for more dramatic metallic/haas effects

    for (int i = 0; i < numSamples; ++i)
    {
        float split = splitSmooth.getNextValue();
        // map split to delay time
        float delayMs = minMs + split * (maxMs - minMs);
        float delaySamples = delayMs * 0.001f * static_cast<float>(currentSampleRate);
        // feedback amount scaled by split (0..0.6)
        float feedback = split * 0.6f;
        // write current side sample into delay buffer
        timingDelayBuffer.setSample(1, writePosition, side[i]);
        timingDelayBuffer.setSample(0, writePosition, mid[i]);

        // read delayed side
        float readPos = static_cast<float>(writePosition) - delaySamples;
        while (readPos < 0.0f) readPos += static_cast<float>(delayBufferSize);

        float delayed = getInterpolatedSample(timingDelayBuffer.getReadPointer(1), readPos);

        // apply feedback (read-modify-write)
        float fbSample = delayed * feedback;
        // add feedback into the buffer (simple approach)
        float bufVal = timingDelayBuffer.getSample(1, writePosition);
        timingDelayBuffer.setSample(1, writePosition, bufVal + fbSample);

        // High-pass side to avoid low-frequency phase cancellation (simple one-pole)
        float hpOut = hpAlpha * (hpA + delayed - hpB);
        hpA = delayed;
        hpB = hpOut;

        side[i] = hpOut;

        writePosition = (writePosition + 1) % delayBufferSize;
    }

    // Apply divergence processing with independent saturation (adds metallic character)
    processMidSide(buffer, splitSmooth);

    // Convert back to stereo
    convertToStereo(buffer);
}

void MidSideDivergence::applyTimingOffset(juce::AudioBuffer<float>& buffer, float split, float momentum)
{
    // Not used in new implementation (handled inside process)
}

void MidSideDivergence::convertToMidSide(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    
    for (int i = 0; i < numSamples; ++i)
    {
        float l = left[i];
        float r = right[i];
        
        // M = (L + R) / 2, S = (L - R) / 2
        left[i] = (l + r) * 0.5f;  // Mid
        right[i] = (l - r) * 0.5f; // Side
    }
}

void MidSideDivergence::convertToStereo(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    float* mid = buffer.getWritePointer(0);
    float* side = buffer.getWritePointer(1);
    
    for (int i = 0; i < numSamples; ++i)
    {
        float m = mid[i];
        float s = side[i];
        
        // L = M + S, R = M - S
        mid[i] = m + s;   // Left
        side[i] = m - s;  // Right
    }
}

void MidSideDivergence::processMidSide(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& splitSmooth)
{
    const int numSamples = buffer.getNumSamples();
    float* mid = buffer.getWritePointer(0);
    float* side = buffer.getWritePointer(1);
    
    // Give the side channel metallic resonance by applying a stronger nonlinearity
    for (int i = 0; i < numSamples; ++i)
    {
        float split = splitSmooth.getNextValue();
        // per-sample derived gains
        float midCompression = 1.0f / (1.0f + split * 0.25f);
        float sideDrive = 1.0f + split * 2.0f;

        // Mid: gentle compression to maintain mono stability
        float m = mid[i] * midCompression;
        mid[i] = std::tanh(m * 1.1f);

        // Side: drive and metallic shaping
        float s = side[i] * sideDrive;
        s = std::tanh(s * 2.0f);
        side[i] = s;
    }
}

float MidSideDivergence::getInterpolatedSample(const float* channelData, float readPosition)
{
    int index1 = static_cast<int>(readPosition);
    int index2 = (index1 + 1) % delayBufferSize;
    float frac = readPosition - static_cast<float>(index1);
    
    return channelData[index1] * (1.0f - frac) + channelData[index2] * frac;
}
