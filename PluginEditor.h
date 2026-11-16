#pragma once
#include "PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>

// draws one fallen autumn leaf (used by the background and the falling-leaves overlay)
void drawAutumnLeaf (juce::Graphics& g, juce::Point<float> centre, float size, float rotation, juce::Colour colour);

// shared knob setup (colours + behaviour)
void styleKnob (juce::Slider& s, juce::Colour cap, juce::Colour tick, juce::Colour pointer, juce::Component* popupParent);

//==============================================================================
class EmoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    EmoLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
};

//==============================================================================
// chrome stomp switch with an LED above it
class FootSwitch : public juce::Button
{
public:
    FootSwitch();
    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
};

//==============================================================================
struct KnobDef
{
    juce::String id, label;
};

class Pedal : public juce::Component
{
public:
    Pedal (juce::AudioProcessorValueTreeState& apvts,
           const juce::String& title,
           const juce::String& subtitle,
           const juce::String& onId,
           const std::vector<KnobDef>& knobDefs,
           juce::Colour bodyColour,
           juce::Colour capColour);

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
    EmoBoardProcessor& proc;
    float level = 0.0f;
};

//==============================================================================
class FallingLeaves : public juce::Component, private juce::Timer
{
public:
    FallingLeaves();
    ~FallingLeaves() override { stopTimer(); }
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    struct Leaf
    {
        float baseX = 0, y = 0, vy = 0, amp = 0, freq = 0, phase = 0, rot = 0, rotSpeed = 0, size = 0;
        juce::Colour col;
        juce::Rectangle<int> last;
    };

    void timerCallback() override;
    juce::Point<float> posOf (const Leaf&) const;
    float rotOf (const Leaf&) const;
    juce::Rectangle<int> boundsOf (const Leaf&) const;

    std::vector<Leaf> leaves;
    float time = 0.0f;
    juce::Random rng { 99 };
};

//==============================================================================
class EmoBoardEditor : public juce::AudioProcessorEditor
{
public:
    explicit EmoBoardEditor (EmoBoardProcessor&);
    ~EmoBoardEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void renderBackground();

    EmoLookAndFeel laf;
    juce::Image background;

    std::vector<std::unique_ptr<Pedal>> pedals;

    juce::Slider inKnob, outKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inAtt, outAtt;
    LevelMeter meter;
    FallingLeaves leaves;
};
