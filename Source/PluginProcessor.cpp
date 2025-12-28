// Factory preset definitions (must be after class definition)


#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Pressure: Primary drive and stress input (0-1, nonlinear curve, emphasis above 0.6)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "pressure",
        "Pressure",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.0f,
        "Pressure"));
    
    // Fault: Introduces instability only under stress (0-1, scaled by Pressure internally)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fault",
        "Fault",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.0f,
        "Fault"));
    
    // Split: Stereo divergence under load (0-1)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "split",
        "Split",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.0f,
        "Split"));
    
    // Momentum: System reaction speed and recovery behavior (0-1)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "momentum",
        "Momentum",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f,
        "Momentum"));
    
    // Resolve: Dynamic recovery and stabilization (0-1)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "resolve",
        "Resolve",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.0f,
        "Resolve"));
    
    // Mix: Dry/Wet blend for parallel processing (0-1, default 100% wet)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix",
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        1.0f,
        "Mix"));

    // Input gain: pre-processing gain in decibels (-60 dB .. +24 dB)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "input_gain",
        "Input Gain",
        juce::NormalisableRange<float>(-60.0f, 24.0f, 0.1f),
        0.0f,
        "dB"));

    // Output gain: post-processing gain in decibels (-60 dB .. +24 dB)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "output_gain",
        "Output Gain",
        juce::NormalisableRange<float>(-60.0f, 24.0f, 0.1f),
        0.0f,
        "dB"));
    
    return layout;
}

CritAudioProcessor::CritAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(*this, nullptr, juce::Identifier("CritParameters"), createParameterLayout())
{
    // Initialize smoothers with safe default state
    // These will be properly configured in prepareToPlay
    pressureSmooth.setCurrentAndTargetValue(0.0f);
    faultSmooth.setCurrentAndTargetValue(0.0f);
    splitSmooth.setCurrentAndTargetValue(0.0f);
    momentumSmooth.setCurrentAndTargetValue(0.5f);
    resolveSmooth.setCurrentAndTargetValue(0.0f);
    mixSmooth.setCurrentAndTargetValue(1.0f);

    // Ensure factory preset files exist in the user preset directory
    writeFactoryPresetFiles();
}

CritAudioProcessor::~CritAudioProcessor()
{
}

const juce::String CritAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CritAudioProcessor::acceptsMidi() const
{
    return false;
}

bool CritAudioProcessor::producesMidi() const
{
    return false;
}

bool CritAudioProcessor::isMidiEffect() const
{
    return false;
}

double CritAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}


int CritAudioProcessor::getNumPrograms()
{
    return numFactoryPresets;
}


int CritAudioProcessor::getCurrentProgram()
{
    return currentProgram;
}


void CritAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= numFactoryPresets)
        return;
    currentProgram = index;
    const auto& preset = factoryPresets[index];
    parameters.getParameter("pressure")->setValueNotifyingHost(preset.pressure);
    parameters.getParameter("fault")->setValueNotifyingHost(preset.fault);
    parameters.getParameter("split")->setValueNotifyingHost(preset.split);
    parameters.getParameter("momentum")->setValueNotifyingHost(preset.momentum);
    parameters.getParameter("resolve")->setValueNotifyingHost(preset.resolve);
    parameters.getParameter("mix")->setValueNotifyingHost(preset.mix);
}


const juce::String CritAudioProcessor::getProgramName(int index)
{
    if (index < 0 || index >= numFactoryPresets)
        return {};
    return factoryPresets[index].name;
}


void CritAudioProcessor::changeProgramName(int, const juce::String&)
{
    // Not user-editable
}

void CritAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Prepare all DSP modules
    waveshaper.prepare(sampleRate, samplesPerBlock);
    dynamicEQ.prepare(sampleRate, samplesPerBlock);
    faultDecimator.prepare(sampleRate, samplesPerBlock);
    midSideDivergence.prepare(sampleRate, samplesPerBlock);
    softLimiter.prepare(sampleRate, samplesPerBlock);
    
    // Report latency from lookahead limiter
    setLatencySamples(softLimiter.getLatencySamples());
    
    // Prepare parameter smoothers with default momentum (0.5)
    // Will be updated per-block based on actual momentum value
    float defaultSmoothTime = 0.02f; // 20ms default
    pressureSmooth.reset(sampleRate, defaultSmoothTime);
    faultSmooth.reset(sampleRate, defaultSmoothTime);
    splitSmooth.reset(sampleRate, defaultSmoothTime);
    momentumSmooth.reset(sampleRate, 0.05f); // Momentum itself smooths slower
    resolveSmooth.reset(sampleRate, defaultSmoothTime);
    mixSmooth.reset(sampleRate, 0.02f); // Mix has fixed smoothing
    
    // Meter decay: ~300ms to fall to near zero
    meterDecayCoeff = 1.0f - std::exp(-1.0f / (0.3f * static_cast<float>(sampleRate)));
    
    // Initialize stress
    currentStress = 0.0f;
    stressSmooth = 0.0f;

    // no gain-match smoother
    
    // Initialize mid/side buffer
    midSideBuffer.setSize(2, samplesPerBlock);
    
    // Initialize dry buffer for parallel processing
    dryBuffer.setSize(2, samplesPerBlock);

    // Create oversampler for pressure/fault modules (4x, stereo)
    // second argument is exponent (2^2 = 4x)
    oversampler.reset(new juce::dsp::Oversampling<float>(/*numChannels=*/2, /*factorPow=*/2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true));
    oversampler->initProcessing(static_cast<size_t>(samplesPerBlock));
}

void CritAudioProcessor::releaseResources()
{
    waveshaper.reset();
    dynamicEQ.reset();
    faultDecimator.reset();
    midSideDivergence.reset();
    softLimiter.reset();
}

bool CritAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Support mono and stereo, but process as stereo internally (mono-safe)
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;

    return true;
}

void CritAudioProcessor::updateSmoothingTimes(float /*unused*/)
{
    // Keep parameter smoothing times fixed. Momentum no longer controls parameter smoothing.
    const float defaultSmoothTimeSec = 0.02f; // 20ms
    pressureSmooth.reset(currentSampleRate, defaultSmoothTimeSec);
    faultSmooth.reset(currentSampleRate, defaultSmoothTimeSec);
    splitSmooth.reset(currentSampleRate, defaultSmoothTimeSec);
    resolveSmooth.reset(currentSampleRate, defaultSmoothTimeSec);
}

void CritAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Safety check: if prepareToPlay hasn't been called yet or sample rate is invalid
    if (currentSampleRate <= 0.0 || numSamples <= 0)
    {
        buffer.clear();
        return;
    }
    
    // Clear any unused output channels
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);
    
    // Handle mono input by duplicating to stereo for processing
    bool isMono = (totalNumInputChannels == 1);
    if (isMono && buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
    }
    
    // Apply input gain (pre-processing) and calculate input level for metering
    auto inputGainParam = parameters.getRawParameterValue("input_gain");
    float inputGainDb = inputGainParam ? inputGainParam->load() : 0.0f;
    float inputGain = juce::Decibels::decibelsToGain(inputGainDb);

    float inputPeak = 0.0f;
    for (int ch = 0; ch < std::min(totalNumInputChannels, 2); ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int s = 0; s < numSamples; ++s)
            data[s] *= inputGain;

        inputPeak = std::max(inputPeak, buffer.getMagnitude(ch, 0, numSamples));
    }
    // Smooth meter with fast attack, exponential decay (sample-rate correct)
    if (inputPeak > inputLevelSmooth)
        inputLevelSmooth = inputPeak;
    else
    {
        float timeConst = 0.3f; // 300ms decay to near zero
        float decayFactor = std::exp(- (float)numSamples / (timeConst * static_cast<float>(currentSampleRate)));
        inputLevelSmooth = inputPeak * (1.0f - decayFactor) + inputLevelSmooth * decayFactor;
    }
    
    // Get parameter values with nullptr safety
    auto pressureParam = parameters.getRawParameterValue("pressure");
    auto faultParam = parameters.getRawParameterValue("fault");
    auto splitParam = parameters.getRawParameterValue("split");
    auto momentumParam = parameters.getRawParameterValue("momentum");
    auto resolveParam = parameters.getRawParameterValue("resolve");
    auto mixParam = parameters.getRawParameterValue("mix");
    
    if (!pressureParam || !faultParam || !splitParam || !momentumParam || !resolveParam || !mixParam)
    {
        // Parameters not ready yet - pass through
        return;
    }
    
    float pressure = pressureParam->load();
    float fault = faultParam->load();
    float split = splitParam->load();
    float momentum = momentumParam->load();
    float resolve = resolveParam->load();
    float mix = mixParam->load();
    
    // Do not use momentum to drive parameter smoothing anymore.
    // Momentum now controls the stress envelope follower attack/release only.
    float currentMomentum = momentumSmooth.getCurrentValue();
    
    // Update parameter smoothers - target values
    pressureSmooth.setTargetValue(pressure);
    faultSmooth.setTargetValue(fault);
    splitSmooth.setTargetValue(split);
    momentumSmooth.setTargetValue(momentum);
    resolveSmooth.setTargetValue(resolve);
    mixSmooth.setTargetValue(mix);
    
    // For Pressure/Fault/Split we will use sample-accurate getNextValue() inside processing loops.
    // Get smoothed values for other controls for this block
    float smoothedMomentum = momentumSmooth.skip(numSamples);
    float smoothedResolve = resolveSmooth.skip(numSamples);
    float smoothedMix = mixSmooth.skip(numSamples);
    
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    
    // Store dry signal for parallel processing (before DSP chain)
    for (int ch = 0; ch < numChannels; ++ch)
    {
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }
    
    // Apply DSP chain in blocks
    // 1. Pressure -> Slew Rate Limiter + Hard Clipper (oversampled)
    // Prepare audio blocks for oversampling
    juce::dsp::AudioBlock<float> inBlock (buffer);
    auto upBlock = oversampler ? oversampler->processSamplesUp(inBlock) : inBlock;

    const int upNumSamples = (int) upBlock.getNumSamples();
    const int upNumChannels = (int) upBlock.getNumChannels();

    // Process per-sample on oversampled block for sample-accurate modulation
    for (int ch = 0; ch < upNumChannels; ++ch)
    {
        float* data = upBlock.getChannelPointer(ch);
        float last = (ch == 0) ? pressureLastL : pressureLastR;

        for (int i = 0; i < upNumSamples; ++i)
        {
            // Sample-accurate pressure value
            float pres = pressureSmooth.getNextValue();

            // Map pressure to maxSlew (log mapping): pres==1 -> very small slew (slow), pres==0 -> high slew
            float inv = 1.0f - pres;
            // maxSlew range ~0.0005 .. 1.0 (linear sample units at oversampled rate)
            // make mapping steeper for more aggressive slew limiting at high pressure
            float maxSlew = 0.0005f * std::pow(20000.0f, juce::jlimit(0.0f, 1.0f, inv));

            float inSample = data[i];
            float delta = inSample - last;
            if (delta > maxSlew) delta = maxSlew;
            else if (delta < -maxSlew) delta = -maxSlew;

            float out = last + delta;

            // Hard clip at +/-1.0 if pressure > 0.5 to create square-like edges
            if (pres > 0.5f)
                out = juce::jlimit(-1.0f, 1.0f, out);

            data[i] = out;
            last = out;
        }

        if (ch == 0) pressureLastL = last; else pressureLastR = last;
    }

    // Downsample back to original rate if oversampled
    if (oversampler)
        oversampler->processSamplesDown(inBlock);

    // Compute instantStress as average absolute difference between pre-DSP dry and post-pressure buffer
    float diffSum = 0.0f;
    const int numCh = std::min(buffer.getNumChannels(), 2);
    for (int ch = 0; ch < numCh; ++ch)
    {
        const float* pre = dryBuffer.getReadPointer(ch);
        const float* post = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            diffSum += std::abs(post[i] - pre[i]);
    }
    float instantStress = diffSum / (float)(numSamples * numCh + 1);
    
    // Smooth the stress signal for other modules using time-constant attack/release
    // Map momentum to attack/release times per spec:
    // attackMs = map(momentum, 0.0, 50.0)  (0ms..50ms)
    // releaseMs = map(momentum, 10.0, 1000.0) (10ms..1000ms)
    float attackMs = juce::jmap(smoothedMomentum, 0.0f, 50.0f);
    float releaseMs = juce::jmap(smoothedMomentum, 10.0f, 1000.0f);
    float attackTimeSec = attackMs * 0.001f;
    float releaseTimeSec = releaseMs * 0.001f;

    // Compute per-block smoothing alpha from time constants: alpha = 1 - exp(-N / (timeSec*sr))
    float attackAlpha = 1.0f - std::exp(- (float)numSamples / (attackTimeSec * (float)currentSampleRate));
    float releaseAlpha = 1.0f - std::exp(- (float)numSamples / (releaseTimeSec * (float)currentSampleRate));

    if (instantStress > stressSmooth)
        stressSmooth += attackAlpha * (instantStress - stressSmooth);
    else
        stressSmooth += releaseAlpha * (instantStress - stressSmooth);
    currentStress = stressSmooth;
    
    // Update stress meter
    if (currentStress > stressLevelSmooth)
        stressLevelSmooth = currentStress;
    else
    {
        float timeConst = 0.3f;
        float decayFactor = std::exp(- (float)numSamples / (timeConst * static_cast<float>(currentSampleRate)));
        stressLevelSmooth = currentStress * (1.0f - decayFactor) + stressLevelSmooth * decayFactor;
    }
    
    // Dynamic EQ follows pressure (use raw parameter as block-level control)
    dynamicEQ.process(buffer, pressure);

    // 2. Fault -> Dynamic sample-rate reduction (decimator) scaled by stress and momentum
    // Run FaultDecimator on an oversampled block to avoid aliasing from hard hold steps
    if (oversampler)
    {
        juce::dsp::AudioBlock<float> faultBlock(buffer);
        auto upFaultBlock = oversampler->processSamplesUp(faultBlock);
        faultDecimator.process(upFaultBlock, faultSmooth, currentStress, smoothedMomentum);
        oversampler->processSamplesDown(faultBlock);
    }
    else
    {
        faultDecimator.process(buffer, faultSmooth, currentStress, smoothedMomentum);
    }
    
    // 3. Split -> Mid/Side Divergence (affected by Momentum)
    // Skip if mono
    if (numChannels >= 2)
    {
        midSideDivergence.process(buffer, splitSmooth, smoothedMomentum);
    }
    
    // 4. Resolve -> Soft Limiter (strength increases with Pressure, affected by Momentum)
    softLimiter.process(buffer, smoothedResolve, currentStress, smoothedMomentum);
    
    // Apply dry/wet mix for parallel processing using equal-power crossfade
    if (smoothedMix < 0.999f)
    {
        float wetGain = std::sqrt(smoothedMix);
        float dryGain = std::sqrt(1.0f - smoothedMix);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* wetData = buffer.getWritePointer(ch);
            const float* dryData = dryBuffer.getReadPointer(ch);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                wetData[sample] = dryData[sample] * dryGain + wetData[sample] * wetGain;
            }
        }
    }
    
    // If input was mono, mix back to mono for mono-safe output
    if (isMono)
    {
        // Average the stereo processing back to mono (only if buffer has 2 channels)
        if (buffer.getNumChannels() >= 2)
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                float monoSample = (buffer.getSample(0, sample) + buffer.getSample(1, sample)) * 0.5f;
                buffer.setSample(0, sample, monoSample);
                if (totalNumOutputChannels >= 2)
                    buffer.setSample(1, sample, monoSample);
            }
        }
        // If buffer is truly mono (1 channel), it's already mono - no mixing needed
    }
    
    // Calculate output level for metering (after processing)
    float outputPeak = 0.0f;
    for (int ch = 0; ch < std::min(totalNumOutputChannels, 2); ++ch)
    {
        outputPeak = std::max(outputPeak, buffer.getMagnitude(ch, 0, numSamples));
    }
    // Apply output gain parameter (post-processing)
    {
        auto outParam = parameters.getRawParameterValue("output_gain");
        float outDb = outParam ? outParam->load() : 0.0f;
        float outGain = juce::Decibels::decibelsToGain(outDb);
        if (outGain != 1.0f)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer(ch);
                for (int s = 0; s < numSamples; ++s)
                    data[s] *= outGain;
            }
            // recompute outputPeak after applying output gain
            outputPeak = 0.0f;
            for (int ch = 0; ch < std::min(totalNumOutputChannels, 2); ++ch)
                outputPeak = std::max(outputPeak, buffer.getMagnitude(ch, 0, numSamples));
        }
    }

    // Smooth meter
    if (outputPeak > outputLevelSmooth)
        outputLevelSmooth = outputPeak;
    else
    {
        float timeConst = 0.3f;
        float decayFactor = std::exp(- (float)numSamples / (timeConst * static_cast<float>(currentSampleRate)));
        outputLevelSmooth = outputPeak * (1.0f - decayFactor) + outputLevelSmooth * decayFactor;
    }
}

bool CritAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* CritAudioProcessor::createEditor()
{
    return new CritAudioProcessorEditor(*this);
}

void CritAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save parameter state
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CritAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore parameter state
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(parameters.state.getType()))
        {
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

// This creates the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CritAudioProcessor();
}

// Factory preset definitions (must be after class definition)
const CritAudioProcessor::Preset CritAudioProcessor::factoryPresets[CritAudioProcessor::numFactoryPresets] = {
    // Drums
    {"Crunch Kit",      0.7f, 0.4f, 0.25f, 0.6f, 0.35f, 1.0f},
    {"Slam Room",       0.95f,0.75f,0.5f,  0.85f,0.18f, 1.0f},
    {"Snare Collapse",  0.85f,0.9f, 0.15f, 0.4f, 0.25f, 1.0f},
    // Synths
    {"Shimmer Lead",    0.5f, 0.15f,0.6f,  0.7f, 0.5f, 1.0f},
    {"Bitcrush Dream",  0.6f, 0.85f,0.35f, 0.35f,0.28f, 1.0f},
    {"Digital Bloom",   0.4f, 0.3f, 0.7f,  0.9f, 0.6f, 1.0f},
    // Vocals
    {"Pop Squeeze",     0.4f, 0.08f,0.18f, 0.5f, 0.7f, 1.0f},
    {"Glitch Diva",     0.82f,0.6f, 0.7f,  0.25f,0.22f, 1.0f},
    {"Vocal Grit",      0.6f, 0.4f, 0.25f, 0.65f,0.45f, 1.0f},
    // Basses
    {"Pluck Destroyer", 0.85f,0.5f, 0.2f,  0.9f, 0.1f, 1.0f},
    {"Rage Bass",       1.0f, 0.92f,0.45f, 0.75f,0.2f, 1.0f},
    {"Sub Melt",        0.95f,0.3f, 0.05f, 0.55f,0.12f, 1.0f},
};

// Preset file format: simple key=value lines. e.g.
// name=Crunch Kit
// pressure=0.7
// fault=0.4
// split=0.25
// momentum=0.6
// resolve=0.35
// mix=1.0

juce::File CritAudioProcessor::getUserPresetDirectory() const
{
    juce::File appSupport = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    juce::File dir = appSupport.getChildFile("crit").getChildFile("presets");
    if (!dir.exists()) dir.createDirectory();
    return dir;
}

bool CritAudioProcessor::savePresetToFile(const juce::File& file) const
{
    juce::StringArray lines;
    // write current parameter values
    auto getVal = [&](const char* id)->float { auto v = parameters.getRawParameterValue(id); return v ? v->load() : 0.0f; };
    lines.add("name=" + file.getFileNameWithoutExtension());
    lines.add("pressure=" + juce::String(getVal("pressure"), 6));
    lines.add("fault=" + juce::String(getVal("fault"), 6));
    lines.add("split=" + juce::String(getVal("split"), 6));
    lines.add("momentum=" + juce::String(getVal("momentum"), 6));
    lines.add("resolve=" + juce::String(getVal("resolve"), 6));
    lines.add("mix=" + juce::String(getVal("mix"), 6));

    juce::String txt = lines.joinIntoString("\n");
    return file.replaceWithText(txt);
}

bool CritAudioProcessor::loadPresetFromFile(const juce::File& file)
{
    if (!file.existsAsFile()) return false;
    juce::String txt = file.loadFileAsString();
    auto lines = juce::StringArray::fromLines(txt);
    juce::HashMap<juce::String, juce::String> kv;
    for (auto& l : lines)
    {
        auto parts = juce::StringArray::fromTokens(l, "=", "");
        if (parts.size() == 2)
            kv.set(parts[0].trim(), parts[1].trim());
    }

    auto setParam = [&](const char* id, const juce::String& key){ if (kv.contains(key)) parameters.getParameter(id)->setValueNotifyingHost(kv[key].getFloatValue()); };
    setParam("pressure", "pressure");
    setParam("fault", "fault");
    setParam("split", "split");
    setParam("momentum", "momentum");
    setParam("resolve", "resolve");
    setParam("mix", "mix");

    return true;
}

void CritAudioProcessor::writeFactoryPresetFiles() const
{
    auto dir = getUserPresetDirectory();
    for (int i = 0; i < numFactoryPresets; ++i)
    {
        auto& p = factoryPresets[i];
        juce::String name(p.name);
        // sanitize file name
        juce::String fn = name.retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-").replace(" ", "_");
        juce::File f = dir.getChildFile(juce::String::formatted("%02d_%s.crit", i+1, fn.toRawUTF8()));
        if (!f.existsAsFile())
        {
            juce::StringArray lines;
            lines.add("name=" + name);
            lines.add("pressure=" + juce::String(p.pressure, 6));
            lines.add("fault=" + juce::String(p.fault, 6));
            lines.add("split=" + juce::String(p.split, 6));
            lines.add("momentum=" + juce::String(p.momentum, 6));
            lines.add("resolve=" + juce::String(p.resolve, 6));
            lines.add("mix=" + juce::String(p.mix, 6));
            juce::String txt = lines.joinIntoString("\n");
            f.replaceWithText(txt);
        }
    }
}

// (Removed gain-match debug getters)
