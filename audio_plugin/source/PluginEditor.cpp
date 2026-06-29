#include "PluginEditor.h"

namespace audio_plugin {

//==============================================================================
// HellishKnob Implementation
//==============================================================================
HellishKnob::HellishKnob()
{
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    knob.setRange(0.0, 1.0, 0.001);
    knob.setValue(0.0);
    knob.addListener(this);
    addAndMakeVisible(knob);
}

HellishKnob::~HellishKnob() = default;

void HellishKnob::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &knob) {
        targetGlowIntensity = static_cast<float>(knob.getValue());
    }
}

void HellishKnob::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto centre = bounds.getCentre();
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.42f;

    // Animate glow intensity
    glowIntensity += (targetGlowIntensity - glowIntensity) * 0.15f;

    // === BACKGROUND GLOW ===
    // The glow gets more intense and shifts from dark to bright orange
    if (glowIntensity > 0.01f) {
        float glowRadius = radius * (1.0f + glowIntensity * 0.6f);
        int numGlowRings = static_cast<int>(juce::jmap(glowIntensity, 1.0f, 8.0f));

        for (int i = numGlowRings; i >= 0; --i) {
            float t = static_cast<float>(i) / static_cast<float>(numGlowRings);
            float ringRadius = glowRadius * (0.5f + t * 0.5f);
            float alpha = (1.0f - t) * glowIntensity * 0.4f;

            // Color shifts from dark red to bright orange to white-hot
            juce::Colour glowColor;
            if (glowIntensity < 0.5f) {
                glowColor = juce::Colour::fromHSV(
                    juce::jmap(glowIntensity, 0.0f, 0.5f, 0.08f, 0.06f),  // Hue: brownish -> orange
                    0.9f,
                    juce::jmap(glowIntensity, 0.0f, 0.5f, 0.2f, 0.7f),
                    alpha);
            } else {
                glowColor = juce::Colour::fromHSV(
                    juce::jmap(glowIntensity, 0.5f, 1.0f, 0.06f, 0.02f),  // Hue: orange -> yellow-white
                    juce::jmap(glowIntensity, 0.5f, 1.0f, 0.9f, 0.3f),    // Saturation drops at max
                    juce::jmap(glowIntensity, 0.5f, 1.0f, 0.7f, 1.0f),    // Value increases
                    alpha);
            }

            g.setColour(glowColor);
            g.fillEllipse(centre.x - ringRadius, centre.y - ringRadius,
                          ringRadius * 2.0f, ringRadius * 2.0f);
        }
    }

    // === OUTER RING (Scalloped edge like the image) ===
    float outerRadius = radius * 1.08f;
    int numScallops = 16;

    // Ring color shifts from dark charcoal to glowing orange
    juce::Colour ringColor;
    if (glowIntensity < 0.3f) {
        ringColor = juce::Colour(30, 30, 30).interpolatedWith(
            juce::Colour(80, 40, 20), glowIntensity / 0.3f);
    } else if (glowIntensity < 0.7f) {
        ringColor = juce::Colour(80, 40, 20).interpolatedWith(
            juce::Colour(200, 100, 20), (glowIntensity - 0.3f) / 0.4f);
    } else {
        ringColor = juce::Colour(200, 100, 20).interpolatedWith(
            juce::Colour(255, 180, 40), (glowIntensity - 0.7f) / 0.3f);
    }

    juce::Path outerRing;
    for (int i = 0; i <= numScallops * 2; ++i) {
        float angle = juce::MathConstants<float>::twoPi * i / (numScallops * 2);
        float r = (i % 2 == 0) ? outerRadius : outerRadius * 0.94f;
        float x = centre.x + std::cos(angle) * r;
        float y = centre.y + std::sin(angle) * r;
        if (i == 0)
            outerRing.startNewSubPath(x, y);
        else
            outerRing.lineTo(x, y);
    }
    outerRing.closeSubPath();

    g.setColour(ringColor);
    g.fillPath(outerRing);

    // Inner shadow on ring
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.strokePath(outerRing, juce::PathStrokeType(2.0f));

    // === INNER FACE (The dark background of the knob) ===
    float innerRadius = radius * 0.88f;
    juce::Colour innerColor;
    if (glowIntensity < 0.4f) {
        innerColor = juce::Colour(15, 15, 15).interpolatedWith(
            juce::Colour(40, 20, 5), glowIntensity / 0.4f);
    } else {
        innerColor = juce::Colour(40, 20, 5).interpolatedWith(
            juce::Colour(80, 35, 5), (glowIntensity - 0.4f) / 0.6f);
    }

    g.setColour(innerColor);
    g.fillEllipse(centre.x - innerRadius, centre.y - innerRadius,
                  innerRadius * 2.0f, innerRadius * 2.0f);

    // Inner face highlight
    g.setColour(juce::Colours::white.withAlpha(0.03f + glowIntensity * 0.08f));
    g.fillEllipse(centre.x - innerRadius * 0.7f, centre.y - innerRadius * 0.7f,
                  innerRadius * 1.4f, innerRadius * 1.4f);

    // === STAR SHAPE (The indicator that glows orange) ===
    float starRadius = radius * 0.55f;
    int numPoints = 5;
    float rotation = juce::MathConstants<float>::pi * 0.5f;  // Point up

    // Star color: starts dark yellow, goes to bright orange, then white-hot
    juce::Colour starColor;
    if (glowIntensity < 0.2f) {
        starColor = juce::Colour(60, 55, 10).interpolatedWith(
            juce::Colour(180, 140, 10), glowIntensity / 0.2f);
    } else if (glowIntensity < 0.6f) {
        starColor = juce::Colour(180, 140, 10).interpolatedWith(
            juce::Colour(255, 140, 0), (glowIntensity - 0.2f) / 0.4f);
    } else if (glowIntensity < 0.85f) {
        starColor = juce::Colour(255, 140, 0).interpolatedWith(
            juce::Colour(255, 200, 50), (glowIntensity - 0.6f) / 0.25f);
    } else {
        starColor = juce::Colour(255, 200, 50).interpolatedWith(
            juce::Colour(255, 255, 220), (glowIntensity - 0.85f) / 0.15f);
    }

    juce::Path star;
    for (int i = 0; i < numPoints * 2; ++i) {
        float angle = rotation + juce::MathConstants<float>::twoPi * i / (numPoints * 2);
        float r = (i % 2 == 0) ? starRadius : starRadius * 0.4f;
        float x = centre.x + std::cos(angle) * r;
        float y = centre.y + std::sin(angle) * r;
        if (i == 0)
            star.startNewSubPath(x, y);
        else
            star.lineTo(x, y);
    }
    star.closeSubPath();

    // Fill star with glow
    g.setColour(starColor);
    g.fillPath(star);

    // Star inner glow (brighter center)
    if (glowIntensity > 0.3f) {
        g.setColour(starColor.brighter(0.3f).withAlpha(glowIntensity * 0.5f));
        juce::Path innerStar;
        float innerStarRadius = starRadius * 0.5f;
        for (int i = 0; i < numPoints * 2; ++i) {
            float angle = rotation + juce::MathConstants<float>::twoPi * i / (numPoints * 2);
            float r = (i % 2 == 0) ? innerStarRadius : innerStarRadius * 0.4f;
            float x = centre.x + std::cos(angle) * r;
            float y = centre.y + std::sin(angle) * r;
            if (i == 0)
                innerStar.startNewSubPath(x, y);
            else
                innerStar.lineTo(x, y);
        }
        innerStar.closeSubPath();
        g.fillPath(innerStar);
    }

    // Star outline
    g.setColour(starColor.darker(0.3f).withAlpha(0.8f));
    g.strokePath(star, juce::PathStrokeType(1.0f));

    // === INDICATOR LINE (The lever/arm on the knob) ===
    // The line rotates with the knob value
    float knobValue = static_cast<float>(knob.getValue());
    float lineAngle = juce::MathConstants<float>::pi * 1.25f
                    - knobValue * juce::MathConstants<float>::pi * 1.5f;
    float lineStart = radius * 0.15f;
    float lineEnd = radius * 0.82f;

    float startX = centre.x + std::cos(lineAngle) * lineStart;
    float startY = centre.y + std::sin(lineAngle) * lineStart;
    float endX = centre.x + std::cos(lineAngle) * lineEnd;
    float endY = centre.y + std::sin(lineAngle) * lineEnd;

    // Line color: metallic silver that glows orange at high intensity
    juce::Colour lineColor = juce::Colour(200, 200, 210).interpolatedWith(
        juce::Colour(255, 180, 60), glowIntensity * glowIntensity);

    g.setColour(lineColor);
    g.drawLine(startX, startY, endX, endY, 3.5f);

    // Line highlight
    g.setColour(juce::Colours::white.withAlpha(0.4f + glowIntensity * 0.4f));
    g.drawLine(startX, startY, endX, endY, 1.5f);

    // Line shadow
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawLine(startX + 1.0f, startY + 1.0f, endX + 1.0f, endY + 1.0f, 3.5f);

    // === HEAT PARTICLES (floating embers at high intensity) ===
    if (glowIntensity > 0.5f) {
        juce::Random random;
        random.setSeed(static_cast<juce::int64>(juce::Time::getMillisecondCounter()));

        int numParticles = static_cast<int>(juce::jmap(glowIntensity, 0.5f, 1.0f, 0.0f, 12.0f));
        for (int i = 0; i < numParticles; ++i) {
            float px = centre.x + random.nextFloat() * radius * 2.2f - radius * 1.1f;
            float py = centre.y + random.nextFloat() * radius * 2.2f - radius * 1.1f;
            float pSize = random.nextFloat() * 2.0f + 0.5f;
            float pAlpha = random.nextFloat() * glowIntensity * 0.6f;

            juce::Colour pColor = juce::Colour(255, 120 + random.nextInt(80), random.nextInt(40));
            g.setColour(pColor.withAlpha(pAlpha));
            g.fillEllipse(px - pSize / 2.0f, py - pSize / 2.0f, pSize, pSize);
        }
    }
}

void HellishKnob::resized()
{
    knob.setBounds(getLocalBounds());
}

//==============================================================================
// AudioPluginAudioProcessorEditor Implementation
//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processorRef(p)
{
    // Set plugin window size
    setSize(400, 500);

    // === TITLE LABEL ===
    titleLabel.setText("COMPRESSOR OF DEATH", juce::dontSendNotification);
    titleLabel.setFont(juce::Font("Impact", 28.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(220, 220, 220));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // === INTENSITY KNOB (The Hellish Star Knob) ===
    intensityKnob.getSlider().setRange(0.0, 1.0, 0.001);
    intensityKnob.getSlider().setValue(0.0);
    addAndMakeVisible(intensityKnob);

    intensityAttachment.reset(new SliderAttachment(
        processorRef.getAPVTS(),
        AudioPluginAudioProcessor::paramIntensity,
        intensityKnob.getSlider()));

    // === INTENSITY LABEL ===
    intensityLabel.setText("INTENSITY", juce::dontSendNotification);
    intensityLabel.setFont(juce::Font("Arial", 14.0f, juce::Font::bold));
    intensityLabel.setColour(juce::Label::textColourId, juce::Colour(180, 180, 180));
    intensityLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(intensityLabel);

    // === OUTPUT GAIN SLIDER ===
    outputGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    outputGainSlider.setRange(-24.0, 12.0, 0.1);
    outputGainSlider.setValue(0.0);
    outputGainSlider.setColour(juce::Slider::thumbColourId, juce::Colour(200, 100, 20));
    outputGainSlider.setColour(juce::Slider::trackColourId, juce::Colour(60, 30, 10));
    outputGainSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(30, 30, 30));
    outputGainSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(200, 200, 200));
    outputGainSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(20, 20, 20));
    addAndMakeVisible(outputGainSlider);

    outputGainAttachment.reset(new SliderAttachment(
        processorRef.getAPVTS(),
        AudioPluginAudioProcessor::paramOutputGain,
        outputGainSlider));

    // === OUTPUT GAIN LABEL ===
    outputGainLabel.setText("OUTPUT", juce::dontSendNotification);
    outputGainLabel.setFont(juce::Font("Arial", 12.0f, juce::Font::bold));
    outputGainLabel.setColour(juce::Label::textColourId, juce::Colour(150, 150, 150));
    outputGainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputGainLabel);

    // Start timer for animation updates
    startTimerHz(30);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer();
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Get current intensity for background heat
    float intensity = static_cast<float>(intensityKnob.getSlider().getValue());
    heatLevel += (intensity - heatLevel) * 0.1f;

    // === BACKGROUND ===
    // Dark background that subtly warms up with intensity
    juce::Colour bgColor;
    if (heatLevel < 0.3f) {
        bgColor = juce::Colour(12, 12, 12).interpolatedWith(
            juce::Colour(25, 12, 5), heatLevel / 0.3f);
    } else if (heatLevel < 0.7f) {
        bgColor = juce::Colour(25, 12, 5).interpolatedWith(
            juce::Colour(45, 20, 8), (heatLevel - 0.3f) / 0.4f);
    } else {
        bgColor = juce::Colour(45, 20, 8).interpolatedWith(
            juce::Colour(60, 25, 10), (heatLevel - 0.7f) / 0.3f);
    }

    g.fillAll(bgColor);

    // === HEAT VIGNETTE ===
    // Subtle radial gradient that intensifies with heat
    if (heatLevel > 0.1f) {
        auto bounds = getLocalBounds().toFloat();
        auto centre = bounds.getCentre();
        float maxDist = std::sqrt(bounds.getWidth() * bounds.getWidth()
                                  + bounds.getHeight() * bounds.getHeight()) * 0.5f;

        juce::Colour vignetteColor;
        if (heatLevel < 0.5f) {
            vignetteColor = juce::Colour(80, 30, 5).withAlpha(heatLevel * 0.15f);
        } else {
            vignetteColor = juce::Colour(180, 70, 10).withAlpha(heatLevel * 0.2f);
        }

        juce::ColourGradient gradient(
            vignetteColor, centre.x, centre.y,
            juce::Colours::transparentBlack, centre.x, centre.y + maxDist * 0.7f,
            true);
        g.setGradientFill(gradient);
        g.fillRect(bounds);
    }

    // === BOTTOM BORDER GLOW ===
    if (heatLevel > 0.2f) {
        float borderAlpha = heatLevel * 0.3f;
        juce::Colour borderColor = juce::Colour(200, 80, 10).withAlpha(borderAlpha);
        g.setColour(borderColor);
        g.fillRect(0, getHeight() - 3, getWidth(), 3);
    }

    // === DECORATIVE LINES ===
    g.setColour(juce::Colour(60, 60, 60).withAlpha(0.5f));
    g.drawLine(20, 70, getWidth() - 20, 70, 1.0f);
    g.drawLine(20, getHeight() - 80, getWidth() - 20, getHeight() - 80, 1.0f);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Title at top
    titleLabel.setBounds(0, 15, getWidth(), 45);

    // Intensity knob (center, large)
    int knobSize = 220;
    intensityKnob.setBounds(
        (getWidth() - knobSize) / 2,
        75,
        knobSize,
        knobSize);

    // Intensity label below knob
    intensityLabel.setBounds(
        (getWidth() - 150) / 2,
        75 + knobSize + 5,
        150,
        25);

    // Output gain slider (right side)
    outputGainSlider.setBounds(getWidth() - 70, 90, 50, 180);
    outputGainLabel.setBounds(getWidth() - 80, 275, 70, 20);
}

// FIX: timerCallback implementation with correct member name
void AudioPluginAudioProcessorEditor::timerCallback()
{
    // Trigger repaint for animation
    intensityKnob.repaint();
    repaint();
}

}  // namespace audio_plugin
