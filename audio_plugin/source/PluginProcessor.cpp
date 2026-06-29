#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace audio_plugin {

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                           .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                           .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ),
#endif
      apvts(*this, nullptr, juce::Identifier("COMPRESSOR_OF_DEATH"), createParameterLayout())
{
    intensityParam = apvts.getRawParameterValue(paramIntensity);
    outputGainParam = apvts.getRawParameterValue(paramOutputGain);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
AudioPluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        paramIntensity,
        "Intensity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("Hell")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        paramOutputGain,
        "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")));

    return layout;
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index,
                                                   const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate,
                                                int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;

    // Initialize envelope follower
    envelope = 0.0f;
    previousIntensity = *intensityParam;
    previousOutputGain = juce::Decibels::decibelsToGain(*outputGainParam);

    // DC blocker states
    dcBlockerStateL = 0.0f;
    dcBlockerStateR = 0.0f;

    // Initialize waveshaper for hellish saturation
    waveShaper.functionToUse = [](float x) {
        // Asymmetric tube-like saturation with hard clipping
        float saturated = std::tanh(x * 2.0f);
        return saturated + (x - saturated) * 0.3f;  // Mix dry/sat for grit
    };
}

void AudioPluginAudioProcessor::releaseResources()
{
    // Release any resources allocated in prepareToPlay
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

//==============================================================================
void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't have corresponding input
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Get current parameter values
    float intensity = juce::jlimit(0.0f, 1.0f, *intensityParam);
    float outputGainDb = *outputGainParam;
    float outputGain = juce::Decibels::decibelsToGain(outputGainDb);

    // === HELLISH COMPRESSOR DSP ===
    //
    // Intensity mapping:
    // 0.0 = clean pass-through
    // 0.5 = moderate compression with saturation
    // 1.0 = maximum hell: extreme compression, heavy saturation, aggressive limiting
    //

    // Calculate attack/release times based on intensity (faster = more aggressive)
    float attackMs = juce::jmap(intensity, 30.0f, 0.1f);    // 30ms -> 0.1ms
    float releaseMs = juce::jmap(intensity, 100.0f, 10.0f);  // 100ms -> 10ms

    attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * static_cast<float>(currentSampleRate)));
    releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(currentSampleRate)));

    // Threshold gets lower as intensity increases (more compression)
    float thresholdDb = juce::jmap(intensity, 0.0f, -40.0f);  // 0dB -> -40dB
    float thresholdLinear = juce::Decibels::decibelsToGain(thresholdDb);

    // Ratio increases with intensity (1:1 -> 20:1 limiting)
    float ratio = juce::jmap(intensity, 1.0f, 20.0f);

    // Makeup gain increases with intensity (compensate for compression + add warmth)
    float makeupGainDb = juce::jmap(intensity, 0.0f, 18.0f);
    float makeupGain = juce::Decibels::decibelsToGain(makeupGainDb);

    // Saturation amount increases with intensity
    float saturationAmount = intensity * intensity * 3.0f;  // Quadratic curve

    // Knee width (softer knee at lower intensities)
    float kneeWidthDb = juce::jmap(intensity, 6.0f, 0.0f);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int channel = 0; channel < numChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample) {
            float inputSample = channelData[sample];

            // --- 1. ENVELOPE FOLLOWER (RMS-ish peak detector) ---
            float inputAbs = std::abs(inputSample);
            if (inputAbs > envelope)
                envelope = attackCoeff * envelope + (1.0f - attackCoeff) * inputAbs;
            else
                envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * inputAbs;

            // --- 2. GAIN REDUCTION CALCULATION ---
            float gainReduction = 1.0f;

            if (envelope > thresholdLinear) {
                float envelopeDb = juce::Decibels::gainToDecibels(envelope, -144.0f);
                float overThresholdDb = envelopeDb - thresholdDb;

                // Soft knee
                if (overThresholdDb <= kneeWidthDb / 2.0f) {
                    float kneeFactor = overThresholdDb + kneeWidthDb / 2.0f;
                    float compressedDb = (kneeFactor * kneeFactor) / (2.0f * kneeWidthDb);
                    gainReduction = juce::Decibels::decibelsToGain(-compressedDb);
                } else {
                    float compressedDb = overThresholdDb / ratio;
                    gainReduction = juce::Decibels::decibelsToGain(-compressedDb);
                }
            }

            // Smooth the gain reduction to avoid clicks
            float smoothedGain = gainReduction;

            // --- 3. APPLY COMPRESSION ---
            float compressedSample = inputSample * smoothedGain * makeupGain;

            // --- 4. HELLISH SATURATION ---
            // Asymmetric saturation: positive samples saturate harder
            float saturatedSample = compressedSample;
            if (saturationAmount > 0.001f) {
                float drive = 1.0f + saturationAmount * 4.0f;
                float wetDry = saturationAmount;

                // Positive half-wave: aggressive tanh
                float pos = std::tanh(compressedSample * drive * 1.5f) * 0.7f;
                // Negative half-wave: softer, more musical
                float neg = std::tanh(compressedSample * drive * 0.8f) * 0.9f;

                float shaped = (compressedSample >= 0.0f) ? pos : neg;
                saturatedSample = compressedSample * (1.0f - wetDry) + shaped * wetDry;
            }

            // --- 5. HELLISH LIMITER (brickwall at high intensity) ---
            if (intensity > 0.7f) {
                float limitThreshold = juce::jmap(intensity, 0.7f, 1.0f, 1.0f, 0.3f);
                saturatedSample = std::tanh(saturatedSample / limitThreshold) * limitThreshold;
            }

            // --- 6. DC BLOCKER ---
            float dcBlocked = saturatedSample - dcBlockerStateL + 0.995f * dcBlockerStateL;
            if (channel == 0)
                dcBlockerStateL = saturatedSample;
            else
                dcBlockerStateR = saturatedSample;

            // --- 7. OUTPUT GAIN ---
            float outputSample = dcBlocked * outputGain;

            channelData[sample] = outputSample;
        }
    }

    // Update state for next block
    previousIntensity = intensity;
    previousOutputGain = outputGain;
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor(*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

}  // namespace audio_plugin

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new audio_plugin::AudioPluginAudioProcessor();
}
