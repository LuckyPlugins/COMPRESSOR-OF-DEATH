#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

namespace audio_plugin {

class AudioPluginAudioProcessor : public juce::AudioProcessor {
public:
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midiMessages) override;

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

    // Public access to APVTS for the editor
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Parameter IDs
    static constexpr auto paramIntensity = "intensity";
    static constexpr auto paramOutputGain = "outputGain";

private:
    juce::AudioProcessorValueTreeState apvts;

    // Raw parameter pointers for real-time access
    std::atomic<float>* intensityParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;

    // DSP state
    double currentSampleRate = 44100.0;
    float previousIntensity = 0.0f;
    float previousOutputGain = 1.0f;

    // Envelope follower for sidechain-style compression
    float envelope = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // Saturation / waveshaper
    juce::dsp::WaveShaper<float> waveShaper;

    // DC blocker
    float dcBlockerStateL = 0.0f;
    float dcBlockerStateR = 0.0f;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};

}  // namespace audio_plugin
