#include "FaultDecimator.h"

void FaultDecimator::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    maxBlockSize = samplesPerBlock;
    // seed rng
    std::random_device rd;
    rng.seed(rd());
    heldL = heldR = 0.0f;
    phasor = 0.0f;
}

void FaultDecimator::reset()
{
    heldL = heldR = 0.0f;
    phasor = 0.0f;
}

void FaultDecimator::process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& faultSmooth, float currentStress, float momentum)
{
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    // reduction: base fault + stress modulation (0..1)
    for (int s = 0; s < numSamples; ++s)
    {
        // get per-sample fault value from smoother
        float faultVal = faultSmooth.getNextValue();
        float reduction = faultVal + (currentStress * 0.5f);
        reduction = juce::jlimit(0.0f, 1.0f, reduction);

        // map reduction -> stepSize (1.0 normal, 0.01 crushed)
        float stepSize = juce::jmap(reduction, 0.0f, 1.0f, 1.0f, 0.01f);

        // apply jitter
        float jitter = jitterDist(rng);
        stepSize *= jitter;

        phasor += stepSize;
        if (phasor >= 1.0f)
        {
            // sample and hold
            if (numChannels >= 1)
                heldL = buffer.getSample(0, s);
            if (numChannels >= 2)
                heldR = buffer.getSample(1, s);
            phasor -= 1.0f;
        }

        // write held samples back
        if (numChannels >= 1)
            buffer.setSample(0, s, heldL);
        if (numChannels >= 2)
            buffer.setSample(1, s, heldR);
    }
}

void FaultDecimator::process(juce::dsp::AudioBlock<float>& block, juce::SmoothedValue<float>& faultSmooth, float currentStress, float momentum)
{
    const int numChannels = std::min((int)block.getNumChannels(), 2);
    const int numSamples = (int)block.getNumSamples();

    for (int s = 0; s < numSamples; ++s)
    {
        float faultVal = faultSmooth.getNextValue();
        float reduction = faultVal + (currentStress * 0.5f);
        reduction = juce::jlimit(0.0f, 1.0f, reduction);

        float stepSize = juce::jmap(reduction, 0.0f, 1.0f, 1.0f, 0.01f);
        float jitter = jitterDist(rng);
        stepSize *= jitter;

        phasor += stepSize;
        if (phasor >= 1.0f)
        {
            if (numChannels >= 1)
                heldL = block.getChannelPointer(0)[s];
            if (numChannels >= 2)
                heldR = block.getChannelPointer(1)[s];
            phasor -= 1.0f;
        }

        if (numChannels >= 1)
            block.getChannelPointer(0)[s] = heldL;
        if (numChannels >= 2)
            block.getChannelPointer(1)[s] = heldR;
    }
}
