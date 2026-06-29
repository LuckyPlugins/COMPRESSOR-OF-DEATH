#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace audio_plugin {

//==============================================================================
/**
 * Custom knob component that renders a hellish star-shaped knob
 * that glows orange/hot as the intensity increases.
 */
class HellishKnob : public juce::Component,
                    public juce::Slider::Listener
{
public:
    HellishKnob();
    ~HellishKnob() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void sliderValueChanged(juce::Slider* slider) override;
    juce::Slider& getSlider() { return knob; }

private:
    juce::Slider knob;

    // Glow animation
    float glowIntensity = 0.0f;
    float targetGlowIntensity = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HellishKnob)
};

//==============================================================================
class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    AudioPluginAudioProcessor& processorRef;

    // Parameter attachments
    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    std::unique_ptr<SliderAttachment> intensityAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;

    // UI Components
    HellishKnob intensityKnob;
    juce::Slider outputGainSlider;
    juce::Label titleLabel;
    juce::Label intensityLabel;
    juce::Label outputGainLabel;

    // Background heat animation
    juce::Image backgroundImage;
    float heatLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};

}  // namespace audio_plugin
