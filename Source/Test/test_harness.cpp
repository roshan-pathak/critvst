#include <JuceHeader.h>
#include "../DSP/FaultDecimator.h"
#include "../DSP/MidSideDivergence.h"
#include "../DSP/SoftLimiter.h"

int main()
{
    juce::ConsoleApplication app;

    const double sampleRate = 48000.0;
    const int blockSize = 512;

    FaultDecimator fault;
    MidSideDivergence midSide;
    SoftLimiter soft;

    fault.prepare(sampleRate, blockSize);
    midSide.prepare(sampleRate, blockSize);
    soft.prepare(sampleRate, blockSize);

    juce::SmoothedValue<float> pressureSmooth, faultSmooth, splitSmooth, momentumSmooth, resolveSmooth;
    pressureSmooth.reset(sampleRate, 0.02);
    faultSmooth.reset(sampleRate, 0.02);
    splitSmooth.reset(sampleRate, 0.02);
    momentumSmooth.reset(sampleRate, 0.05);
    resolveSmooth.reset(sampleRate, 0.02);

    pressureSmooth.setCurrentAndTargetValue(0.7f);
    faultSmooth.setCurrentAndTargetValue(0.5f);
    splitSmooth.setCurrentAndTargetValue(0.4f);
    momentumSmooth.setCurrentAndTargetValue(0.5f);
    resolveSmooth.setCurrentAndTargetValue(0.3f);

    const int numBlocks = 64;
    juce::AudioBuffer<float> buffer(2, blockSize);

    double phase = 0.0;
    const double freq = 440.0;

    float lastL = 0.0f, lastR = 0.0f;

    for (int b = 0; b < numBlocks; ++b)
    {
        buffer.clear();
        for (int s = 0; s < blockSize; ++s)
        {
            float amp = 0.25f;
            if ((b % 16) == 0 && s < 8) amp = 1.0f;
            float v = amp * std::sin(phase);
            phase += 2.0 * juce::MathConstants<double>::pi * freq / sampleRate;
            if (phase > juce::MathConstants<double>::twoPi) phase -= juce::MathConstants<double>::twoPi;
            buffer.setSample(0, s, v);
            buffer.setSample(1, s, v);
        }

        // Copy input
        juce::AudioBuffer<float> inCopy(buffer);

        // Pressure: per-sample slew limiter + hard clip
        for (int ch = 0; ch < 2; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            float& last = (ch == 0) ? lastL : lastR;
            for (int i = 0; i < blockSize; ++i)
            {
                float pres = pressureSmooth.getNextValue();
                float inv = 1.0f - pres;
                float maxSlew = 0.001f * std::pow(1000.0f, juce::jlimit(0.0f, 1.0f, inv));
                float inSample = data[i];
                float delta = inSample - last;
                if (delta > maxSlew) delta = maxSlew;
                else if (delta < -maxSlew) delta = -maxSlew;
                float out = last + delta;
                if (pres > 0.5f) out = juce::jlimit(-1.0f, 1.0f, out);
                data[i] = out;
                last = out;
            }
        }

        // compute instantStress
        double diff = 0.0;
        for (int ch = 0; ch < 2; ++ch)
            for (int s = 0; s < blockSize; ++s)
                diff += std::abs(buffer.getSample(ch, s) - inCopy.getSample(ch, s));
        float instantStress = (float)(diff / (blockSize * 2));

        // Fault (decimator)
        fault.process(buffer, faultSmooth, instantStress, momentumSmooth.getCurrentValue());

        // Split
        midSide.process(buffer, splitSmooth, momentumSmooth.getCurrentValue());

        // Resolve (soft limiter/upward compression)
        soft.process(buffer, resolveSmooth.getCurrentValue(), instantStress, momentumSmooth.getCurrentValue());

        double inRms = inCopy.getRMSLevel(0, 0, blockSize);
        double outRms = buffer.getRMSLevel(0, 0, blockSize);

        // compute avg abs diff
        double avgDiff = 0.0;
        for (int ch = 0; ch < 2; ++ch)
            for (int s = 0; s < blockSize; ++s)
                avgDiff += std::abs(buffer.getSample(ch, s) - inCopy.getSample(ch, s));
        avgDiff /= (double)(blockSize * 2);

        std::cout << "Block " << b << ": inRMS=" << inRms << " outRMS=" << outRms << " avgAbsDiff=" << avgDiff << " stress=" << instantStress << std::endl;
    }

    return 0;
}
