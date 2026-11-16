#pragma once
#include "PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>

constexpr int kDesignW = 1100;
constexpr int kDesignH = 800;

struct KnobDef
{
    juce::String id, label;
};

void styleKnob (juce::Slider& s, juce::Colour cap, juce::Colour tick, juce::Colour pointer);

//==============================================================================
// Knobs are rendered once into cached images; only the pointer is drawn live.
class EmoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    EmoLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

private:
    struct Cached
    {
        int size;
        juce::Colour cap, tick;
        juce::Image img;
    };
    std::vector<Cached> cache;
    juce::Image knobImage (int size, juce::Colour cap, juce::Colour tick, float a0, float a1);
};

//==============================================================================
class FootSwitch : public juce::Button
{
public:
    FootSwitch();
    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
};

//==============================================================================
class Pedal : public juce::Component
{
public:
    Pedal (juce::AudioProcessorValueTreeState& apvts,
           const juce::String& title, const juce::String& subtitle, const juce::String& onId,
           const std::vector<KnobDef>& knobDefs, juce::Colour bodyColour, juce::Colour capColour);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void renderBody();

    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    juce::String title, subtitle;
    juce::Colour bodyColour, textCol;
    juce::Font labelFont { juce::FontOptions (10.5f) };
    juce::Image body;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<juce::String> knobLabels;
    std::vector<juce::Rectangle<float>> labelAreas;

    FootSwitch foot;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> footAtt;
};

//==============================================================================
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter (EmoBoardProcessor& p);
    ~LevelMeter() override { stopTimer(); }
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    int litSegments() const;

    EmoBoardProcessor& proc;
    float level = 0.0f;
    int lastLit = -1;
};

//==============================================================================
// Everything lives in this fixed-size (design) component; the editor scales it.
class Board : public juce::Component
{
public:
    explicit Board (EmoBoardProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void renderBackground();
    void stepPreset (int delta);

    EmoBoardProcessor& proc;
    juce::Image background;
    std::vector<std::unique_ptr<Pedal>> pedals;

    juce::Slider inKnob, outKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inAtt, outAtt;
    LevelMeter meter;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
};

//==============================================================================
class EmoBoardEditor : public juce::AudioProcessorEditor
{
public:
    explicit EmoBoardEditor (EmoBoardProcessor&);
    ~EmoBoardEditor() override;
    void resized() override;

private:
    EmoLookAndFeel laf;
    Board board;
};
