#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "DSP/Waveshaper.h"
#include "DSP/DynamicEQ.h"
#include "DSP/FaultDecimator.h"
#include "DSP/MidSideDivergence.h"
#include "DSP/SoftLimiter.h"

class CritAudioProcessor : public juce::AudioProcessor
{
public:
    CritAudioProcessor();
    ~CritAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

    // Metering access for UI
    float getInputLevel() const { return inputLevelSmooth; }
    float getOutputLevel() const { return outputLevelSmooth; }
    float getStressLevel() const { return stressLevelSmooth; }
    
    // Gain controls
    void setGainMatchEnabled(bool) { /* removed */ }
    bool isGainMatchEnabled() const { return false; }
    // Debug getters for RMS and compensation
    float getLastPreRms() const;
    float getLastPostRms() const;
    float getLastCompGain() const;


// Factory preset support
public:
    struct Preset {
        const char* name;
        float pressure, fault, split, momentum, resolve, mix;
    };
    static constexpr int numFactoryPresets = 12;
    static const Preset factoryPresets[numFactoryPresets];

    // Preset file I/O
    juce::File getUserPresetDirectory() const;
    bool savePresetToFile(const juce::File& file) const;
    bool loadPresetFromFile(const juce::File& file);
    void writeFactoryPresetFiles() const;

private:
    int currentProgram = 0;
    juce::AudioProcessorValueTreeState parameters;
    
    // DSP modules
    Waveshaper waveshaper;
    DynamicEQ dynamicEQ;
    FaultDecimator faultDecimator;
    MidSideDivergence midSideDivergence;
    SoftLimiter softLimiter;
    
    // Parameter smoothers with momentum-controlled ramp times
    juce::SmoothedValue<float> pressureSmooth;
    juce::SmoothedValue<float> faultSmooth;
    juce::SmoothedValue<float> splitSmooth;
    juce::SmoothedValue<float> momentumSmooth;
    juce::SmoothedValue<float> resolveSmooth;
    juce::SmoothedValue<float> mixSmooth;
    
    // Stress signal (shared across modules)
    float currentStress{0.0f};
    float stressSmooth{0.0f};
    
    // Metering (smoothed for display)
    float inputLevelSmooth{0.0f};
    float outputLevelSmooth{0.0f};
    float stressLevelSmooth{0.0f};
    float meterDecayCoeff{0.0f};
    
    // (No gain-match state) Add nothing here.
    
    // Note: output gain is provided as a parameter named "output_gain" (dB)
    
    // Sample rate for smoothing calculations
    double currentSampleRate{44100.0};

    // Oversampling for Pressure/Fault (created in prepareToPlay)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    // Pressure module state (last samples for slew limiter)
    float pressureLastL{0.0f};
    float pressureLastR{0.0f};
    
    // Audio buffer for mid/side processing
    juce::AudioBuffer<float> midSideBuffer;
    
    // Dry buffer for parallel processing (dry/wet mix)
    juce::AudioBuffer<float> dryBuffer;
    
    // Helper to update smoothing times based on momentum
    void updateSmoothingTimes(float momentum);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CritAudioProcessor)
};

