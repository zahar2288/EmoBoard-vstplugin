#include "PluginEditor.h"
#include <functional>

namespace
{
    const juce::Colour kCream (0xffe9dcc0);

    float displayScale()
    {
        if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
            return juce::jlimit (1.0f, 2.0f, (float) d->scale);
        return 2.0f;
    }

    juce::Colour autumnColour (juce::Random& r)
    {
        static const juce::uint32 cols[] = { 0xffb5522b, 0xffc98a2b, 0xff8c3b22, 0xff6e2a2a,
                                             0xffa6772e, 0xff7a5a2e, 0xff9a4a24 };
        return juce::Colour (cols[r.nextInt (7)]);
    }

    juce::Font makeFont (const char* name, float size, int style, float kerning = 0.0f)
    {
        juce::Font f (juce::FontOptions (name, size, style));
        if (kerning != 0.0f)
            f.setExtraKerningFactor (kerning);
        return f;
    }

    juce::uint8 clamp8 (int v) { return (juce::uint8) juce::jlimit (0, 255, v); }

    juce::Point<float> polar (juce::Point<float> c, float angle, float radius)
    {
        return { c.x + std::sin (angle) * radius, c.y - std::cos (angle) * radius };
    }
}

//==============================================================================
void drawAutumnLeaf (juce::Graphics& g, juce::Point<float> c, float size, float rot, juce::Colour col)
{
    juce::Path leaf;
    leaf.startNewSubPath (0.0f, -1.0f);
    leaf.cubicTo (0.95f, -0.55f, 0.85f, 0.55f, 0.0f, 1.0f);
    leaf.cubicTo (-0.85f, 0.55f, -0.95f, -0.55f, 0.0f, -1.0f);
    leaf.closeSubPath();

    const auto t = juce::AffineTransform::scale (size * 0.55f, size).rotated (rot).translated (c.x, c.y);
    leaf.applyTransform (t);
    g.setColour (col);
    g.fillPath (leaf);

    juce::Path vein;
    vein.startNewSubPath (0.0f, -0.85f);
    vein.lineTo (0.0f, 1.35f);
    vein.applyTransform (t);
    g.setColour (col.darker (0.55f).withAlpha (0.8f));
    g.strokePath (vein, juce::PathStrokeType (juce::jmax (0.6f, size * 0.07f)));
}

void styleKnob (juce::Slider& s, juce::Colour cap, juce::Colour tick, juce::Colour pointer, juce::Component* popupParent)
{
    s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    s.setMouseDragSensitivity (200);
    s.setColour (juce::Slider::rotarySliderFillColourId, cap);
    s.setColour (juce::Slider::trackColourId, tick);
    s.setColour (juce::Slider::thumbColourId, pointer);
    s.setPopupDisplayEnabled (true, true, popupParent);
}

//==============================================================================
EmoLookAndFeel::EmoLookAndFeel()
{
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xee1a1411));
    setColour (juce::BubbleComponent::outlineColourId, kCream.withAlpha (0.35f));
    setColour (juce::TooltipWindow::textColourId, kCream);
}

void EmoLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float a0, float a1, juce::Slider& s)
{
    const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float R = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const auto c = area.getCentre();
    const float angle = a0 + pos * (a1 - a0);
    const auto cap = s.findColour (juce::Slider::rotarySliderFillColourId);
    const auto tick = s.findColour (juce::Slider::trackColourId);
    const auto pointer = s.findColour (juce::Slider::thumbColourId);

    // printed scale around the knob
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 10.0f;
        const bool major = (i == 0 || i == 5 || i == 10);
        g.setColour (tick);
        g.drawLine (juce::Line<float> (polar (c, a, R * 0.85f), polar (c, a, R * (major ? 1.0f : 0.94f))),
                    major ? 1.6f : 1.1f);
    }

    const float kr = R * 0.72f;

    // drop shadow
    {
        juce::Path sp;
        sp.addEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.7f), (int) (R * 0.3f) + 2,
                          { 0, (int) (R * 0.12f) + 1 }).drawForPath (g, sp);
    }

    // cylindrical skirt
    juce::ColourGradient side (cap.darker (0.75f), c.x - kr, c.y, cap.darker (0.75f), c.x + kr, c.y, false);
    side.addColour (0.28, cap.brighter (0.2f));
    side.addColour (0.55, cap.darker (0.2f));
    g.setGradientFill (side);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);

    // knurling
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (int i = 0; i < 40; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 40.0f;
        g.drawLine (juce::Line<float> (polar (c, a, kr * 0.9f), polar (c, a, kr)), 1.0f);
    }

    // top face
    const float tr = kr * 0.8f;
    juce::ColourGradient top (cap.brighter (0.55f), c.x - tr * 0.4f, c.y - tr * 0.5f,
                              cap.darker (0.45f), c.x + tr * 0.6f, c.y + tr * 0.8f, true);
    g.setGradientFill (top);
    g.fillEllipse (c.x - tr, c.y - tr, tr * 2.0f, tr * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (c.x - tr, c.y - tr, tr * 2.0f, tr * 2.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (c.x - tr + 1.0f, c.y - tr + 1.0f, tr * 2.0f - 2.0f, tr * 2.0f - 2.0f, 1.0f);

    // pointer
    g.setColour (pointer);
    g.drawLine (juce::Line<float> (polar (c, angle, tr * 0.2f), polar (c, angle, tr * 0.92f)),
                juce::jmax (2.0f, R * 0.07f));
}

//==============================================================================
FootSwitch::FootSwitch() : juce::Button ("footswitch")
{
    setClickingTogglesState (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void FootSwitch::paintButton (juce::Graphics& g, bool, bool down)
{
    const auto b = getLocalBounds().toFloat();
    const float cx = b.getCentreX();
    const bool on = getToggleState();

    // ---- LED ----
    const juce::Point<float> led (cx, 10.0f);
    if (on)
    {
        juce::ColourGradient halo (juce::Colour (0xffff5a2a).withAlpha (0.5f), led,
                                   juce::Colours::transparentBlack, { led.x + 26.0f, led.y }, true);
        g.setGradientFill (halo);
        g.fillEllipse (led.x - 26.0f, led.y - 26.0f, 52.0f, 52.0f);
    }
    g.setColour (juce::Colour (0xff0e0e0f));
    g.fillEllipse (led.x - 8.0f, led.y - 8.0f, 16.0f, 16.0f);
    g.setColour (juce::Colour (0xff8d8f94));
    g.drawEllipse (led.x - 8.0f, led.y - 8.0f, 16.0f, 16.0f, 1.4f);

    juce::ColourGradient lens (on ? juce::Colour (0xffffb08a) : juce::Colour (0xff6a2018), led.x - 1.5f, led.y - 2.0f,
                               on ? juce::Colour (0xffd2301a) : juce::Colour (0xff2a0a08), led.x + 5.0f, led.y + 5.0f, true);
    g.setGradientFill (lens);
    g.fillEllipse (led.x - 5.5f, led.y - 5.5f, 11.0f, 11.0f);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.7f : 0.25f));
    g.fillEllipse (led.x - 2.8f, led.y - 3.6f, 3.0f, 2.2f);

    // ---- stomp switch ----
    const juce::Point<float> sc (cx, 52.0f);
    juce::Path hex;
    for (int i = 0; i < 6; ++i)
    {
        const float a = juce::MathConstants<float>::pi / 3.0f * (float) i + juce::MathConstants<float>::pi / 6.0f;
        const juce::Point<float> p (sc.x + std::cos (a) * 31.0f, sc.y + std::sin (a) * 31.0f);
        if (i == 0) hex.startNewSubPath (p); else hex.lineTo (p);
    }
    hex.closeSubPath();

    juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 8, { 0, 4 }).drawForPath (g, hex);
    juce::ColourGradient nut (juce::Colour (0xff9a9ca1), sc.x - 26.0f, sc.y - 26.0f,
                              juce::Colour (0xff3b3c40), sc.x + 26.0f, sc.y + 26.0f, false);
    g.setGradientFill (nut);
    g.fillPath (hex);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.strokePath (hex, juce::PathStrokeType (1.2f));

    g.setColour (juce::Colour (0xff1d1e20));
    g.fillEllipse (sc.x - 23.0f, sc.y - 23.0f, 46.0f, 46.0f);

    const float off = down ? 1.5f : 0.0f;
    const float cr = down ? 19.0f : 20.0f;
    juce::ColourGradient cap (juce::Colour (0xfff4f4f2), sc.x - 8.0f, sc.y - 10.0f + off,
                              juce::Colour (0xff6a6c70), sc.x + 12.0f, sc.y + 14.0f + off, true);
    g.setGradientFill (cap);
    g.fillEllipse (sc.x - cr, sc.y - cr + off, cr * 2.0f, cr * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (sc.x - cr, sc.y - cr + off, cr * 2.0f, cr * 2.0f, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawEllipse (sc.x - cr * 0.6f, sc.y - cr * 0.6f + off, cr * 1.2f, cr * 1.2f, 0.8f);
}

//==============================================================================
Pedal::Pedal (juce::AudioProcessorValueTreeState& apvts,
              const juce::String& t, const juce::String& sub, const juce::String& onId,
              const std::vector<KnobDef>& defs, juce::Colour bodyCol, juce::Colour capCol)
    : title (t), subtitle (sub), bodyColour (bodyCol)
{
    textCol = bodyCol.getPerceivedBrightness() > 0.55f ? juce::Colour (0xff1e1a16) : juce::Colour (0xffe9dcc0);
    const auto pointer = capCol.getPerceivedBrightness() > 0.5f ? juce::Colour (0xff1a1512) : juce::Colour (0xfff1e6cc);
    labelFont = makeFont ("Futura", 10.5f, juce::Font::bold, 0.1f);

    for (auto& d : defs)
    {
        auto k = std::make_unique<Knob>();
        styleKnob (k->slider, capCol, textCol.withAlpha (0.7f), pointer, this);
        k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, d.id, k->slider);
        addAndMakeVisible (k->slider);
        knobLabels.push_back (d.label);
        knobs.push_back (std::move (k));
    }

    addAndMakeVisible (foot);
    footAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, onId, foot);
}

void Pedal::resized()
{
    const int W = getWidth();
    const int n = (int) knobs.size();
    if (W <= 0 || n == 0) return;

    const int cols = n <= 3 ? n : 2;
    const int rows = (n + cols - 1) / cols;
    const float zx = 20.0f, zy = 22.0f, zw = (float) W - 40.0f, zh = 146.0f;
    const float cw = zw / (float) cols, ch = zh / (float) rows;

    labelAreas.clear();
    for (int i = 0; i < n; ++i)
    {
        const int r = i / cols, c = i % cols;
        const int inRow = (r == rows - 1) ? n - r * cols : cols;
        const float x0 = zx + (zw - (float) inRow * cw) * 0.5f + (float) c * cw;
        const float y0 = zy + (float) r * ch;
        const float D = juce::jmin (cw - 2.0f, ch - 13.0f, 104.0f);
        const float kx = x0 + (cw - D) * 0.5f;
        const float ky = y0 + (ch - 13.0f - D) * 0.5f;
        knobs[(size_t) i]->slider.setBounds (juce::Rectangle<float> (kx, ky, D, D).toNearestInt());
        labelAreas.push_back (juce::Rectangle<float> (x0, ky + D - 1.0f, cw, 12.0f));
    }

    foot.setBounds (W / 2 - 42, 218, 84, 84);
    renderBody();
}

void Pedal::renderBody()
{
    const int W = getWidth(), H = getHeight();
    if (W <= 0 || H <= 0) return;

    const float sc = displayScale();
    body = juce::Image (juce::Image::ARGB, juce::roundToInt ((float) W * sc), juce::roundToInt ((float) H * sc),
                        true, juce::SoftwareImageType());

    const auto b = juce::Rectangle<float> (6.0f, 6.0f, (float) W - 12.0f, (float) H - 12.0f);
    juce::Path shape;
    shape.addRoundedRectangle (b, 14.0f);

    // ---- pass 1: shadow + base gradient ----
    {
        juce::Graphics g (body);
        g.addTransform (juce::AffineTransform::scale (sc));
        juce::DropShadow (juce::Colours::black.withAlpha (0.65f), 16, { 0, 7 }).drawForPath (g, shape);
        juce::ColourGradient grad (bodyColour.brighter (0.18f), 0.0f, b.getY(),
                                   bodyColour.darker (0.5f), 0.0f, b.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (shape);
    }

    // ---- pass 2: powder-coat grain on fully opaque pixels ----
    {
        juce::Image::BitmapData bd (body, juce::Image::BitmapData::readWrite);
        juce::Random rng (1234 + title.hashCode());
        for (int yy = 0; yy < bd.height; ++yy)
        {
            for (int xx = 0; xx < bd.width; ++xx)
            {
                auto* px = (juce::PixelARGB*) bd.getPixelPointer (xx, yy);
                if (px->getAlpha() == 0xff)
                {
                    const int nz = rng.nextInt (15) - 7;
                    px->setARGB (0xff, clamp8 (px->getRed() + nz), clamp8 (px->getGreen() + nz), clamp8 (px->getBlue() + nz));
                }
            }
        }
    }

    // ---- pass 3: details ----
    {
        juce::Graphics g (body);
        g.addTransform (juce::AffineTransform::scale (sc));
        juce::Random rng (title.hashCode() ^ 0x5bd1e995);

        // glossy sheen
        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.13f), 0.0f, b.getY(),
                                    juce::Colours::transparentWhite, 0.0f, b.getY() + b.getHeight() * 0.45f, false);
        g.setGradientFill (sheen);
        g.fillPath (shape);

        // bevel
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawRoundedRectangle (b, 14.0f, 1.6f);
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawRoundedRectangle (b.reduced (2.0f), 12.5f, 1.2f);

        // scratches
        for (int i = 0; i < 14; ++i)
        {
            const float x1 = b.getX() + 10.0f + rng.nextFloat() * (b.getWidth() - 20.0f);
            const float y1 = b.getY() + 10.0f + rng.nextFloat() * (b.getHeight() - 20.0f);
            const float a = rng.nextFloat() * juce::MathConstants<float>::twoPi;
            const float len = 8.0f + rng.nextFloat() * 30.0f;
            g.setColour (juce::Colours::white.withAlpha (0.04f + rng.nextFloat() * 0.05f));
            g.drawLine (x1, y1, x1 + std::cos (a) * len, y1 + std::sin (a) * len, 0.7f);
        }

        // chipped paint: bare metal showing on the edges
        for (int i = 0; i < 28; ++i)
        {
            const int side = rng.nextInt (4);
            float px = 0.0f, py = 0.0f;
            if (side == 0)      { px = b.getX() + 8.0f + rng.nextFloat() * (b.getWidth() - 16.0f); py = b.getY() + rng.nextFloat() * 5.0f; }
            else if (side == 1) { px = b.getX() + 8.0f + rng.nextFloat() * (b.getWidth() - 16.0f); py = b.getBottom() - rng.nextFloat() * 5.0f; }
            else if (side == 2) { px = b.getX() + rng.nextFloat() * 5.0f; py = b.getY() + 8.0f + rng.nextFloat() * (b.getHeight() - 16.0f); }
            else                { px = b.getRight() - rng.nextFloat() * 5.0f; py = b.getY() + 8.0f + rng.nextFloat() * (b.getHeight() - 16.0f); }
            const float sz = 1.0f + rng.nextFloat() * 2.0f;
            g.setColour (juce::Colour (0xffb9b2a6).withAlpha (0.28f + rng.nextFloat() * 0.2f));
            g.fillEllipse (px, py, sz * 1.4f, sz);
        }

        // screws
        auto screw = [&] (float cx, float cy)
        {
            juce::ColourGradient sg (juce::Colour (0xffb0b2b6), cx - 3.0f, cy - 3.0f, juce::Colour (0xff3a3b3f), cx + 4.0f, cy + 4.0f, true);
            g.setGradientFill (sg);
            g.fillEllipse (cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.drawEllipse (cx - 5.0f, cy - 5.0f, 10.0f, 10.0f, 1.0f);
            const float a = rng.nextFloat() * juce::MathConstants<float>::pi;
            g.drawLine (cx - std::cos (a) * 3.6f, cy - std::sin (a) * 3.6f, cx + std::cos (a) * 3.6f, cy + std::sin (a) * 3.6f, 1.3f);
        };
        screw (b.getX() + 17.0f, b.getBottom() - 17.0f);
        screw (b.getRight() - 17.0f, b.getBottom() - 17.0f);

        // side jacks (input left / output right)
        auto jack = [&] (bool left)
        {
            const juce::Rectangle<float> r (left ? 0.0f : (float) W - 10.0f, 59.0f, 10.0f, 22.0f);
            juce::ColourGradient jg (juce::Colour (0xff8a8c91), r.getX(), r.getY(), juce::Colour (0xff25262a), r.getX(), r.getBottom(), false);
            g.setGradientFill (jg);
            g.fillRoundedRectangle (r, 2.5f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawRoundedRectangle (r, 2.5f, 1.0f);
            g.setColour (juce::Colour (0xff0e0e10));
            g.fillEllipse (r.getCentreX() - 2.0f, r.getCentreY() - 2.0f, 4.0f, 4.0f);
        };
        jack (true);
        jack (false);

        // name plate
        const auto plate = juce::Rectangle<float> (14.0f, 172.0f, (float) W - 28.0f, 30.0f);
        g.setFont (makeFont ("Futura", 24.0f, juce::Font::bold, 0.14f));
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawFittedText (title, plate.translated (0.0f, 1.5f).toNearestInt(), juce::Justification::centred, 1, 0.75f);
        g.setColour (textCol.withAlpha (0.93f));
        g.drawFittedText (title, plate.toNearestInt(), juce::Justification::centred, 1, 0.75f);

        g.setFont (makeFont ("Georgia", 12.5f, juce::Font::italic, 0.05f));
        g.setColour (textCol.withAlpha (0.6f));
        g.drawText (subtitle, juce::Rectangle<float> (14.0f, 201.0f, (float) W - 28.0f, 16.0f), juce::Justification::centred, false);
    }

    repaint();
}

void Pedal::paint (juce::Graphics& g)
{
    g.drawImage (body, getLocalBounds().toFloat());

    g.setFont (labelFont);
    for (size_t i = 0; i < labelAreas.size() && i < knobLabels.size(); ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawText (knobLabels[i], labelAreas[i].translated (0.0f, 1.0f), juce::Justification::centred, false);
        g.setColour (textCol.withAlpha (0.88f));
        g.drawText (knobLabels[i], labelAreas[i], juce::Justification::centred, false);
    }
}

//==============================================================================
LevelMeter::LevelMeter (EmoBoardProcessor& p) : proc (p)
{
    startTimerHz (24);
}

void LevelMeter::timerCallback()
{
    level = juce::jmax (proc.outPeak.load(), level * 0.88f);
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    const int segs = 16;
    const float db = juce::Decibels::gainToDecibels (level, -60.0f);
    const float w = (float) getWidth() / (float) segs;
    const float h = (float) getHeight();

    for (int i = 0; i < segs; ++i)
    {
        const float thr = -48.0f + 3.0f * (float) i;
        const bool lit = db >= thr;
        const juce::Colour base = i < 9 ? juce::Colour (0xff8f9a3c) : (i < 13 ? juce::Colour (0xffd09a2c) : juce::Colour (0xffc2412a));
        const auto r = juce::Rectangle<float> ((float) i * w + 1.5f, 3.0f, w - 3.0f, h - 6.0f);

        if (lit)
        {
            g.setColour (base.withAlpha (0.25f));
            g.fillRoundedRectangle (r.expanded (2.0f), 3.0f);
            g.setColour (base);
        }
        else
        {
            g.setColour (base.darker (0.85f).withAlpha (0.8f));
        }
        g.fillRoundedRectangle (r, 2.0f);
    }
}

//==============================================================================
FallingLeaves::FallingLeaves()
{
    setInterceptsMouseClicks (false, false);
    setOpaque (false);
    startTimerHz (30);
}

void FallingLeaves::resized()
{
    if (!leaves.empty() || getWidth() <= 0) return;

    for (int i = 0; i < 22; ++i)
    {
        Leaf l;
        l.baseX = rng.nextFloat() * (float) getWidth();
        l.y = rng.nextFloat() * (float) getHeight();
        l.vy = 0.5f + rng.nextFloat() * 0.9f;
        l.amp = 18.0f + rng.nextFloat() * 28.0f;
        l.freq = 0.5f + rng.nextFloat() * 0.9f;
        l.phase = rng.nextFloat() * 6.28f;
        l.rot = rng.nextFloat() * 6.28f;
        l.rotSpeed = (rng.nextFloat() - 0.5f) * 0.8f;
        l.size = 8.0f + rng.nextFloat() * 9.0f;
        l.col = autumnColour (rng);
        l.last = boundsOf (l);
        leaves.push_back (l);
    }
}

juce::Point<float> FallingLeaves::posOf (const Leaf& l) const
{
    return { l.baseX + std::sin (time * l.freq + l.phase) * l.amp, l.y };
}

float FallingLeaves::rotOf (const Leaf& l) const
{
    return l.rot + time * l.rotSpeed + std::sin (time * l.freq + l.phase) * 0.5f;
}

juce::Rectangle<int> FallingLeaves::boundsOf (const Leaf& l) const
{
    const auto p = posOf (l);
    const float r = l.size * 1.4f;
    return juce::Rectangle<float> (p.x - r, p.y - r, r * 2.0f, r * 2.0f).toNearestInt().expanded (2);
}

void FallingLeaves::timerCallback()
{
    time += 1.0f / 30.0f;
    const float W = (float) getWidth(), H = (float) getHeight();
    for (auto& l : leaves)
    {
        repaint (l.last);
        l.y += l.vy;
        if (l.y > H + 30.0f)
        {
            l.y = -30.0f;
            l.baseX = rng.nextFloat() * W;
        }
        l.last = boundsOf (l);
        repaint (l.last);
    }
}

void FallingLeaves::paint (juce::Graphics& g)
{
    const auto clip = g.getClipBounds();

    // dark vignette in the corners
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        juce::ColourGradient vg (juce::Colours::transparentBlack, c,
                                 juce::Colours::black.withAlpha (0.5f), { 0.0f, 0.0f }, true);
        vg.addColour (0.55, juce::Colours::transparentBlack);
        g.setGradientFill (vg);
        g.fillAll();
    }

    for (auto& l : leaves)
        if (clip.intersects (l.last))
            drawAutumnLeaf (g, posOf (l), l.size, rotOf (l), l.col.withAlpha (0.85f));
}

//==============================================================================
EmoBoardEditor::EmoBoardEditor (EmoBoardProcessor& p)
    : AudioProcessorEditor (&p), meter (p)
{
    setLookAndFeel (&laf);

    auto& a = p.apvts;
    using KD = std::vector<KnobDef>;

    pedals.push_back (std::make_unique<Pedal> (a, "HUSH", "noise gate", "gate_on",
        KD { { "gate_thr", "THRESH" }, { "gate_rel", "RELEASE" } },
        juce::Colour (0xff34343a), juce::Colour (0xffb4602a)));

    pedals.push_back (std::make_unique<Pedal> (a, "SQUEEZE", "compressor", "comp_on",
        KD { { "comp_amt", "SUSTAIN" }, { "comp_atk", "ATTACK" }, { "comp_blend", "BLEND" } },
        juce::Colour (0xff3f5a72), juce::Colour (0xffe4d8bb)));

    pedals.push_back (std::make_unique<Pedal> (a, "RUST", "overdrive / fuzz", "drv_on",
        KD { { "drv_mode", "MODE" }, { "drv_gain", "GAIN" }, { "drv_tone", "TONE" }, { "drv_level", "LEVEL" } },
        juce::Colour (0xff9a4a26), juce::Colour (0xff1c1a19)));

    pedals.push_back (std::make_unique<Pedal> (a, "FLICKER", "tremolo", "trm_on",
        KD { { "trm_rate", "RATE" }, { "trm_depth", "DEPTH" }, { "trm_shape", "SHAPE" } },
        juce::Colour (0xffb08a2e), juce::Colour (0xff2a1f17)));

    pedals.push_back (std::make_unique<Pedal> (a, "DRIFT", "chorus", "cho_on",
        KD { { "cho_rate", "RATE" }, { "cho_depth", "DEPTH" }, { "cho_mix", "MIX" } },
        juce::Colour (0xff4f6247), juce::Colour (0xffe4d8bb)));

    pedals.push_back (std::make_unique<Pedal> (a, "ECHO HOLLOW", "tape delay", "dly_on",
        KD { { "dly_time", "TIME" }, { "dly_fb", "REPEATS" }, { "dly_mix", "MIX" }, { "dly_wob", "WOBBLE" } },
        juce::Colour (0xff6a2a32), juce::Colour (0xffe4d8bb)));

    pedals.push_back (std::make_unique<Pedal> (a, "NOVEMBER", "reverb", "rev_on",
        KD { { "rev_size", "SIZE" }, { "rev_damp", "DAMP" }, { "rev_pre", "PRE-DLY" }, { "rev_mix", "MIX" } },
        juce::Colour (0xffcfc5ad), juce::Colour (0xff2a2623)));

    pedals.push_back (std::make_unique<Pedal> (a, "CASSETTE", "tape saturation", "tape_on",
        KD { { "tape_wow", "WOW" }, { "tape_dark", "DARK" }, { "tape_warm", "WARMTH" } },
        juce::Colour (0xff5a4130), juce::Colour (0xffc79a3a)));

    for (auto& pd : pedals)
        addAndMakeVisible (*pd);

    const auto cap = juce::Colour (0xffd9ccaa);
    const auto ink = juce::Colour (0xff1a1512);
    styleKnob (inKnob, cap, kCream.withAlpha (0.65f), ink, this);
    styleKnob (outKnob, cap, kCream.withAlpha (0.65f), ink, this);
    inAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "in_gain", inKnob);
    outAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "out_vol", outKnob);
    addAndMakeVisible (inKnob);
    addAndMakeVisible (outKnob);
    addAndMakeVisible (meter);
    addAndMakeVisible (leaves);

    setSize (1100, 800);
}

EmoBoardEditor::~EmoBoardEditor()
{
    setLookAndFeel (nullptr);
}

void EmoBoardEditor::resized()
{
    for (size_t i = 0; i < pedals.size(); ++i)
    {
        const int col = (int) i % 4, row = (int) i / 4;
        pedals[i]->setBounds (26 + col * 266, 132 + row * 332, 250, 316);
    }

    inKnob.setBounds (704, 20, 64, 64);
    outKnob.setBounds (784, 20, 64, 64);
    meter.setBounds (872, 40, 170, 24);
    leaves.setBounds (getLocalBounds());

    renderBackground();
}

void EmoBoardEditor::renderBackground()
{
    const int W = getWidth(), H = getHeight();
    if (W <= 0 || H <= 0) return;

    const float sc = displayScale();
    background = juce::Image (juce::Image::ARGB, juce::roundToInt ((float) W * sc), juce::roundToInt ((float) H * sc),
                              true, juce::SoftwareImageType());
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (sc));
    juce::Random rng (20241115);

    const float fW = (float) W, fH = (float) H;
    const float boardTop = 124.0f;

    // ---- dusk sky ----
    {
        juce::ColourGradient sky (juce::Colour (0xff0f1319), 0.0f, 0.0f, juce::Colour (0xff1b1511), 0.0f, fH, false);
        sky.addColour (0.12, juce::Colour (0xff262a31));
        sky.addColour (0.17, juce::Colour (0xff4a362c));
        g.setGradientFill (sky);
        g.fillAll();
    }

    // ---- warm lamp glow, top left ----
    {
        juce::ColourGradient glow (juce::Colour (0xffd0732a).withAlpha (0.20f), 110.0f, 30.0f,
                                   juce::Colours::transparentBlack, 400.0f, 30.0f, true);
        g.setGradientFill (glow);
        g.fillRect (0.0f, 0.0f, 520.0f, boardTop + 40.0f);
    }

    // ---- pale moon ----
    {
        const juce::Point<float> moon (fW * 0.60f, 48.0f);
        juce::ColourGradient halo (juce::Colour (0xffe8e0c8).withAlpha (0.30f), moon,
                                   juce::Colours::transparentBlack, { moon.x + 110.0f, moon.y }, true);
        g.setGradientFill (halo);
        g.fillEllipse (moon.x - 110.0f, moon.y - 110.0f, 220.0f, 220.0f);
        g.setColour (juce::Colour (0xffe4dcc6).withAlpha (0.92f));
        g.fillEllipse (moon.x - 26.0f, moon.y - 26.0f, 52.0f, 52.0f);
        g.setColour (juce::Colour (0xff8a8472).withAlpha (0.18f));
        g.fillEllipse (moon.x - 12.0f, moon.y - 10.0f, 14.0f, 14.0f);
        g.fillEllipse (moon.x + 6.0f, moon.y + 8.0f, 9.0f, 9.0f);
        g.fillEllipse (moon.x + 8.0f, moon.y - 14.0f, 7.0f, 7.0f);
    }

    // ---- thin clouds ----
    for (int i = 0; i < 6; ++i)
    {
        const float cx = rng.nextFloat() * fW;
        const float cy = 14.0f + rng.nextFloat() * 80.0f;
        g.setColour (juce::Colour (0xff9099a6).withAlpha (0.06f));
        g.fillEllipse (cx - 170.0f, cy, 340.0f + rng.nextFloat() * 120.0f, 10.0f + rng.nextFloat() * 12.0f);
    }

    // ---- faint rain ----
    g.setColour (juce::Colour (0xffb9c2cf).withAlpha (0.07f));
    for (int i = 0; i < 90; ++i)
    {
        const float rx = rng.nextFloat() * fW;
        const float ry = rng.nextFloat() * boardTop;
        g.drawLine (rx, ry, rx - 3.0f, ry + 11.0f, 0.8f);
    }

    // ---- bare trees ----
    std::function<void (juce::Point<float>, float, float, int)> branch;
    branch = [&] (juce::Point<float> p, float ang, float len, int depth)
    {
        if (depth <= 0 || len < 3.0f) return;
        const auto e = polar (p, ang, len);
        g.setColour (juce::Colour (0xff07090c));
        g.drawLine (juce::Line<float> (p, e), juce::jmax (0.8f, (float) depth * 0.9f));
        const int kids = (depth > 2 && rng.nextInt (4) == 0) ? 3 : 2;
        for (int i = 0; i < kids; ++i)
        {
            const float da = (rng.nextFloat() - 0.5f) * 1.1f + (i == 0 ? -0.28f : (i == 1 ? 0.28f : 0.0f));
            branch (e, ang + da, len * (0.68f + rng.nextFloat() * 0.14f), depth - 1);
        }
    };
    branch ({ 70.0f, boardTop + 6.0f }, 0.0f, 38.0f, 8);
    branch ({ fW - 90.0f, boardTop + 6.0f }, 0.1f, 34.0f, 8);

    // ---- wooden pedalboard ----
    const auto board = juce::Rectangle<float> (8.0f, boardTop - 4.0f, fW - 16.0f, fH - boardTop - 4.0f);
    {
        juce::Path bp;
        bp.addRoundedRectangle (board, 10.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 18, { 0, 8 }).drawForPath (g, bp);

        juce::ColourGradient wood (juce::Colour (0xff2c211b), 0.0f, board.getY(), juce::Colour (0xff1a120e), 0.0f, board.getBottom(), false);
        g.setGradientFill (wood);
        g.fillPath (bp);

        // grain
        for (int i = 0; i < 90; ++i)
        {
            const float y0 = board.getY() + rng.nextFloat() * board.getHeight();
            const float ph = rng.nextFloat() * 6.28f;
            const float amp = 1.0f + rng.nextFloat() * 4.0f;
            juce::Path gr;
            gr.startNewSubPath (board.getX(), y0);
            for (float x = board.getX(); x <= board.getRight(); x += 40.0f)
                gr.lineTo (x, y0 + std::sin (x * 0.012f + ph) * amp);
            g.setColour ((i % 2 == 0 ? juce::Colours::black : juce::Colours::white).withAlpha (0.04f + rng.nextFloat() * 0.05f));
            g.strokePath (gr, juce::PathStrokeType (0.6f + rng.nextFloat() * 0.8f));
        }
        for (int i = 0; i < 4000; ++i)
        {
            g.setColour (juce::Colours::white.withAlpha (rng.nextFloat() * 0.05f));
            g.fillRect (board.getX() + rng.nextFloat() * board.getWidth(), board.getY() + rng.nextFloat() * board.getHeight(), 1.0f, 1.0f);
        }

        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (board.reduced (1.0f), 10.0f, 1.2f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawRoundedRectangle (board, 10.0f, 1.6f);

        // corner screws
        const float sx[] = { board.getX() + 10.0f, board.getRight() - 10.0f };
        const float sy[] = { board.getY() + 10.0f, board.getBottom() - 10.0f };
        for (float xx : sx)
            for (float yy : sy)
            {
                g.setColour (juce::Colour (0xff6b6a68));
                g.fillEllipse (xx - 3.5f, yy - 3.5f, 7.0f, 7.0f);
                g.setColour (juce::Colours::black.withAlpha (0.6f));
                g.drawEllipse (xx - 3.5f, yy - 3.5f, 7.0f, 7.0f, 1.0f);
                g.drawLine (xx - 2.5f, yy - 1.0f, xx + 2.5f, yy + 1.0f, 1.0f);
            }
    }

    // ---- velcro rails (visible in the gaps between the rows) ----
    for (float ry : { 449.0f, 782.0f })
    {
        g.setColour (juce::Colour (0xff17181b));
        g.fillRect (board.getX() + 10.0f, ry, board.getWidth() - 20.0f, 13.0f);
        for (float x = board.getX() + 10.0f; x < board.getRight() - 10.0f; x += 2.0f)
        {
            g.setColour (juce::Colours::white.withAlpha (0.04f + rng.nextFloat() * 0.12f));
            g.drawLine (x, ry + 1.0f, x, ry + 12.0f, 0.7f);
        }
    }

    // ---- patch cables ----
    auto cable = [&] (const juce::Path& p)
    {
        g.setColour (juce::Colour (0xff050506));
        g.strokePath (p, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.strokePath (p, juce::PathStrokeType (1.4f), juce::AffineTransform::translation (-0.8f, -0.8f));
    };
    auto plug = [&] (float cx, float cy)
    {
        g.setColour (juce::Colour (0xff9a9ca0));
        g.fillRoundedRectangle (cx - 6.0f, cy - 5.0f, 12.0f, 10.0f, 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawRoundedRectangle (cx - 6.0f, cy - 5.0f, 12.0f, 10.0f, 2.0f, 1.0f);
    };

    const float jy1 = 132.0f + 70.0f, jy2 = 464.0f + 70.0f;
    for (int row = 0; row < 2; ++row)
    {
        const float jy = row == 0 ? jy1 : jy2;
        for (int c = 0; c < 3; ++c)
        {
            const float xr = 26.0f + (float) c * 266.0f + 250.0f;
            juce::Path p;
            p.startNewSubPath (xr - 2.0f, jy);
            p.quadraticTo (xr + 8.0f, jy + 14.0f, xr + 18.0f, jy);
            cable (p);
            plug (xr + 2.0f, jy);
            plug (xr + 14.0f, jy);
        }
    }
    {   // input from the guitar (top-left) and output to the amp (bottom-right)
        juce::Path pin;  pin.startNewSubPath (0.0f, jy1 + 6.0f);  pin.quadraticTo (14.0f, jy1 + 16.0f, 28.0f, jy1);
        cable (pin);  plug (24.0f, jy1);
        juce::Path pout; pout.startNewSubPath (26.0f + 3.0f * 266.0f + 250.0f - 2.0f, jy2);
        pout.quadraticTo (fW - 14.0f, jy2 + 16.0f, fW, jy2 + 6.0f);
        cable (pout);
        // loop that carries the signal from row 1 to row 2
        juce::Path loop;
        loop.startNewSubPath (26.0f + 3.0f * 266.0f + 250.0f - 2.0f, jy1);
        loop.cubicTo (fW + 60.0f, jy1 + 20.0f, -60.0f, jy2 - 20.0f, 28.0f, jy2);
        cable (loop);
        plug (24.0f, jy2);
    }

    // ---- fallen leaves lying on the board ----
    for (int i = 0; i < 34; ++i)
    {
        const juce::Point<float> lp (rng.nextFloat() * fW, boardTop + rng.nextFloat() * (fH - boardTop));
        drawAutumnLeaf (g, lp, 8.0f + rng.nextFloat() * 9.0f, rng.nextFloat() * 6.28f, autumnColour (rng).withAlpha (0.9f));
    }

    // ---- master plate (input / output / meter) ----
    {
        const auto plate = juce::Rectangle<float> (690.0f, 14.0f, 384.0f, 98.0f);
        juce::Path pp;
        pp.addRoundedRectangle (plate, 8.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 12, { 0, 5 }).drawForPath (g, pp);
        juce::ColourGradient pg (juce::Colour (0xff2e2d30), 0.0f, plate.getY(), juce::Colour (0xff17171a), 0.0f, plate.getBottom(), false);
        g.setGradientFill (pg);
        g.fillPath (pp);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawRoundedRectangle (plate.reduced (1.5f), 7.0f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawRoundedRectangle (plate, 8.0f, 1.4f);

        for (float sx : { plate.getX() + 10.0f, plate.getRight() - 10.0f })
            for (float sy : { plate.getY() + 10.0f, plate.getBottom() - 10.0f })
            {
                g.setColour (juce::Colour (0xff6b6a68));
                g.fillEllipse (sx - 3.5f, sy - 3.5f, 7.0f, 7.0f);
                g.setColour (juce::Colours::black.withAlpha (0.6f));
                g.drawEllipse (sx - 3.5f, sy - 3.5f, 7.0f, 7.0f, 1.0f);
            }

        // recessed meter window
        g.setColour (juce::Colour (0xff0a0a0b));
        g.fillRoundedRectangle (866.0f, 36.0f, 182.0f, 32.0f, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (866.0f, 36.0f, 182.0f, 32.0f, 4.0f, 1.0f);
    }

    // ---- low fog hugging the top of the board ----
    {
        juce::ColourGradient fog (juce::Colours::transparentWhite, 0.0f, boardTop - 70.0f,
                                  juce::Colours::transparentWhite, 0.0f, boardTop + 24.0f, false);
        fog.addColour (0.7, juce::Colour (0xffb5aca0).withAlpha (0.07f));
        g.setGradientFill (fog);
        g.fillRect (0.0f, boardTop - 70.0f, fW, 94.0f);
    }
}

void EmoBoardEditor::paint (juce::Graphics& g)
{
    g.drawImage (background, getLocalBounds().toFloat());

    // title
    g.setFont (makeFont ("Georgia", 50.0f, juce::Font::bold | juce::Font::italic));
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("emoboard", 36, 16, 420, 60, juce::Justification::centredLeft, false);
    g.setColour (kCream.withAlpha (0.96f));
    g.drawText ("emoboard", 34, 14, 420, 60, juce::Justification::centredLeft, false);

    g.setFont (makeFont ("Georgia", 15.0f, juce::Font::italic, 0.03f));
    g.setColour (kCream.withAlpha (0.62f));
    g.drawText ("a pedalboard for songs about leaving", 38, 72, 420, 20, juce::Justification::centredLeft, false);

    g.setFont (makeFont ("Futura", 9.5f, juce::Font::bold, 0.12f));
    g.setColour (kCream.withAlpha (0.38f));
    g.drawText ("HUSH  /  SQUEEZE  /  RUST  /  FLICKER  /  DRIFT  /  ECHO HOLLOW  /  NOVEMBER  /  CASSETTE",
                38, 98, 640, 14, juce::Justification::centredLeft, false);

    // master labels
    g.setFont (makeFont ("Futura", 10.5f, juce::Font::bold, 0.12f));
    g.setColour (kCream.withAlpha (0.75f));
    g.drawText ("INPUT", inKnob.getX(), inKnob.getBottom() - 2, inKnob.getWidth(), 12, juce::Justification::centred, false);
    g.drawText ("OUTPUT", outKnob.getX(), outKnob.getBottom() - 2, outKnob.getWidth(), 12, juce::Justification::centred, false);
    g.drawText ("LEVEL", meter.getX(), meter.getBottom() + 6, meter.getWidth(), 12, juce::Justification::centred, false);
}
