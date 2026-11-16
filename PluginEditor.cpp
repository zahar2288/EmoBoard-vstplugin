#include "PluginEditor.h"
#include <functional>

namespace
{
    const juce::Colour kInk (0xff2f2a1c);
    const juce::Colour kPaperText (0xff3b3622);
    const juce::Colour kCream (0xffefe6bd);

    juce::Colour fadedLeaf (juce::Random& r)
    {
        static const juce::uint32 cols[] = { 0xffb5522b, 0xffc98a2b, 0xff8c3b22, 0xffa6772e, 0xff9a4a24 };
        return juce::Colour (cols[r.nextInt (5)]);
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

    void drawLeaf (juce::Graphics& g, juce::Point<float> c, float size, float rot, juce::Colour col)
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

    // a faded lemon slice, drawn with simple shapes
    void drawLemon (juce::Graphics& g, juce::Point<float> c, float r, float rot, float alpha)
    {
        g.setColour (juce::Colour (0xffe8c93a).withAlpha (alpha));
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colour (0xfff3ecbc).withAlpha (alpha));
        g.fillEllipse (c.x - r * 0.92f, c.y - r * 0.92f, r * 1.84f, r * 1.84f);

        const int n = 10;
        const float step = juce::MathConstants<float>::twoPi / (float) n;
        for (int i = 0; i < n; ++i)
        {
            juce::Path w;
            const float a0 = rot + step * (float) i + 0.07f;
            const float a1 = rot + step * (float) (i + 1) - 0.07f;
            w.addPieSegment (c.x - r * 0.84f, c.y - r * 0.84f, r * 1.68f, r * 1.68f, a0, a1, 0.1f);
            g.setColour (juce::Colour (0xfff0d65a).withAlpha (alpha));
            g.fillPath (w);
            g.setColour (juce::Colour (0xffd7b32e).withAlpha (alpha * 0.6f));
            g.strokePath (w, juce::PathStrokeType (1.0f));
        }
    }

    void drawTape (juce::Graphics& g, float x, float y, float w, float h, float angle)
    {
        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (angle, x + w * 0.5f, y + h * 0.5f));
        juce::Path p;
        p.startNewSubPath (x, y);
        const int zig = 6;
        for (int i = 1; i <= zig; ++i)
            p.lineTo (x + (i % 2 ? 2.5f : 0.0f), y + h * (float) i / (float) zig);
        p.lineTo (x + w, y + h);
        for (int i = zig - 1; i >= 0; --i)
            p.lineTo (x + w - (i % 2 ? 0.0f : 2.5f), y + h * (float) i / (float) zig);
        p.closeSubPath();
        g.setColour (juce::Colour (0xffe6dca8).withAlpha (0.82f));
        g.fillPath (p);
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        g.strokePath (p, juce::PathStrokeType (0.8f));
        g.restoreState();
    }
}

//==============================================================================
void styleKnob (juce::Slider& s, juce::Colour cap, juce::Colour tick, juce::Colour pointer)
{
    s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    s.setMouseDragSensitivity (200);
    s.setColour (juce::Slider::rotarySliderFillColourId, cap);
    s.setColour (juce::Slider::trackColourId, tick);
    s.setColour (juce::Slider::thumbColourId, pointer);
    s.setPopupDisplayEnabled (true, true, nullptr);
}

//==============================================================================
EmoLookAndFeel::EmoLookAndFeel()
{
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xf0efe6bd));
    setColour (juce::BubbleComponent::outlineColourId, kInk.withAlpha (0.6f));
    setColour (juce::TooltipWindow::textColourId, kInk);

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xfff0e6b8));
    setColour (juce::PopupMenu::textColourId, kInk);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xffb5532b));
    setColour (juce::PopupMenu::highlightedTextColourId, kCream);

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xfff0e6b8));
    setColour (juce::ComboBox::textColourId, kInk);
    setColour (juce::ComboBox::outlineColourId, kInk.withAlpha (0.7f));
    setColour (juce::ComboBox::arrowColourId, kInk);

    setColour (juce::TextButton::buttonColourId, juce::Colour (0xfff0e6b8));
    setColour (juce::TextButton::textColourOffId, kInk);
    setColour (juce::TextButton::textColourOnId, kInk);
}

juce::Image EmoLookAndFeel::knobImage (int S, juce::Colour cap, juce::Colour tick, float a0, float a1)
{
    for (auto& c : cache)
        if (c.size == S && c.cap == cap && c.tick == tick)
            return c.img;

    const float sc = 3.0f;
    juce::Image img (juce::Image::ARGB, juce::roundToInt ((float) S * sc), juce::roundToInt ((float) S * sc),
                     true, juce::SoftwareImageType());
    juce::Graphics g (img);
    g.addTransform (juce::AffineTransform::scale (sc));

    const float R = (float) S * 0.5f;
    const juce::Point<float> c (R, R);

    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 10.0f;
        const bool major = (i == 0 || i == 5 || i == 10);
        g.setColour (tick);
        g.drawLine (juce::Line<float> (polar (c, a, R * 0.85f), polar (c, a, R * (major ? 0.99f : 0.94f))),
                    major ? 1.6f : 1.1f);
    }

    const float kr = R * 0.72f;
    {
        juce::Path sp;
        sp.addEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.6f), juce::jmax (2, (int) (R * 0.22f)),
                          { 0, juce::jmax (1, (int) (R * 0.09f)) }).drawForPath (g, sp);
    }

    juce::ColourGradient side (cap.darker (0.75f), c.x - kr, c.y, cap.darker (0.75f), c.x + kr, c.y, false);
    side.addColour (0.28, cap.brighter (0.2f));
    side.addColour (0.55, cap.darker (0.2f));
    g.setGradientFill (side);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (int i = 0; i < 40; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 40.0f;
        g.drawLine (juce::Line<float> (polar (c, a, kr * 0.9f), polar (c, a, kr)), 1.0f);
    }

    const float tr = kr * 0.8f;
    juce::ColourGradient top (cap.brighter (0.55f), c.x - tr * 0.4f, c.y - tr * 0.5f,
                              cap.darker (0.45f), c.x + tr * 0.6f, c.y + tr * 0.8f, true);
    g.setGradientFill (top);
    g.fillEllipse (c.x - tr, c.y - tr, tr * 2.0f, tr * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (c.x - tr, c.y - tr, tr * 2.0f, tr * 2.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (c.x - tr + 1.0f, c.y - tr + 1.0f, tr * 2.0f - 2.0f, tr * 2.0f - 2.0f, 1.0f);

    cache.push_back ({ S, cap, tick, img });
    return img;
}

void EmoLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float a0, float a1, juce::Slider& s)
{
    const int S = juce::jmin (w, h);
    if (S <= 4) return;

    const auto cap = s.findColour (juce::Slider::rotarySliderFillColourId);
    const auto tick = s.findColour (juce::Slider::trackColourId);
    const auto pointer = s.findColour (juce::Slider::thumbColourId);

    const float ox = (float) x + (float) (w - S) * 0.5f;
    const float oy = (float) y + (float) (h - S) * 0.5f;

    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (knobImage (S, cap, tick, a0, a1), juce::Rectangle<float> (ox, oy, (float) S, (float) S));

    const float R = (float) S * 0.5f;
    const juce::Point<float> c (ox + R, oy + R);
    const float tr = R * 0.72f * 0.8f;
    const float angle = a0 + pos * (a1 - a0);

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

    const juce::Point<float> sc (cx, 52.0f);
    juce::Path hex;
    for (int i = 0; i < 6; ++i)
    {
        const float a = juce::MathConstants<float>::pi / 3.0f * (float) i + juce::MathConstants<float>::pi / 6.0f;
        const juce::Point<float> p (sc.x + std::cos (a) * 31.0f, sc.y + std::sin (a) * 31.0f);
        if (i == 0) hex.startNewSubPath (p); else hex.lineTo (p);
    }
    hex.closeSubPath();

    juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 6, { 0, 3 }).drawForPath (g, hex);
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
    textCol = bodyCol.getPerceivedBrightness() > 0.55f ? juce::Colour (0xff1e1a16) : juce::Colour (0xffeee4bf);
    const auto pointer = capCol.getPerceivedBrightness() > 0.5f ? juce::Colour (0xff1a1512) : juce::Colour (0xfff1e6cc);
    labelFont = makeFont ("Futura", 10.5f, juce::Font::bold, 0.1f);

    for (auto& d : defs)
    {
        auto k = std::make_unique<Knob>();
        styleKnob (k->slider, capCol, textCol.withAlpha (0.7f), pointer);
        k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, d.id, k->slider);
        addAndMakeVisible (k->slider);
        knobLabels.push_back (d.label);
        knobs.push_back (std::move (k));
    }

    addAndMakeVisible (foot);
    footAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, onId, foot);

    setSize (250, 316);   // fixed design size - this triggers resized() -> renderBody() once
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
    if (body.isNull())
        renderBody();
}

void Pedal::renderBody()
{
    const int W = getWidth(), H = getHeight();
    if (W <= 0 || H <= 0) return;

    const float sc = 2.5f;
    body = juce::Image (juce::Image::ARGB, juce::roundToInt ((float) W * sc), juce::roundToInt ((float) H * sc),
                        true, juce::SoftwareImageType());

    const auto b = juce::Rectangle<float> (6.0f, 6.0f, (float) W - 12.0f, (float) H - 12.0f);
    juce::Path shape;
    shape.addRoundedRectangle (b, 14.0f);

    {
        juce::Graphics g (body);
        g.addTransform (juce::AffineTransform::scale (sc));
        juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 14, { 0, 6 }).drawForPath (g, shape);
        juce::ColourGradient grad (bodyColour.brighter (0.15f), 0.0f, b.getY(),
                                   bodyColour.darker (0.45f), 0.0f, b.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (shape);
    }

    {   // powder-coat grain
        juce::Image::BitmapData bd (body, juce::Image::BitmapData::readWrite);
        juce::Random rng (1234 + title.hashCode());
        for (int yy = 0; yy < bd.height; ++yy)
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

    {
        juce::Graphics g (body);
        g.addTransform (juce::AffineTransform::scale (sc));
        juce::Random rng (title.hashCode() ^ 0x5bd1e995);

        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.12f), 0.0f, b.getY(),
                                    juce::Colours::transparentWhite, 0.0f, b.getY() + b.getHeight() * 0.45f, false);
        g.setGradientFill (sheen);
        g.fillPath (shape);

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawRoundedRectangle (b, 14.0f, 1.6f);
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawRoundedRectangle (b.reduced (2.0f), 12.5f, 1.2f);

        for (int i = 0; i < 14; ++i)
        {
            const float x1 = b.getX() + 10.0f + rng.nextFloat() * (b.getWidth() - 20.0f);
            const float y1 = b.getY() + 10.0f + rng.nextFloat() * (b.getHeight() - 20.0f);
            const float a = rng.nextFloat() * juce::MathConstants<float>::twoPi;
            const float len = 8.0f + rng.nextFloat() * 30.0f;
            g.setColour (juce::Colours::white.withAlpha (0.04f + rng.nextFloat() * 0.05f));
            g.drawLine (x1, y1, x1 + std::cos (a) * len, y1 + std::sin (a) * len, 0.7f);
        }

        for (int i = 0; i < 28; ++i)
        {
            const int side = rng.nextInt (4);
            float px = 0.0f, py = 0.0f;
            if (side == 0)      { px = b.getX() + 8.0f + rng.nextFloat() * (b.getWidth() - 16.0f); py = b.getY() + rng.nextFloat() * 5.0f; }
            else if (side == 1) { px = b.getX() + 8.0f + rng.nextFloat() * (b.getWidth() - 16.0f); py = b.getBottom() - rng.nextFloat() * 5.0f; }
            else if (side == 2) { px = b.getX() + rng.nextFloat() * 5.0f; py = b.getY() + 8.0f + rng.nextFloat() * (b.getHeight() - 16.0f); }
            else                { px = b.getRight() - rng.nextFloat() * 5.0f; py = b.getY() + 8.0f + rng.nextFloat() * (b.getHeight() - 16.0f); }
            const float sz = 1.0f + rng.nextFloat() * 2.0f;
            g.setColour (juce::Colour (0xffd9d2c0).withAlpha (0.3f + rng.nextFloat() * 0.2f));
            g.fillEllipse (px, py, sz * 1.4f, sz);
        }

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

        const auto plate = juce::Rectangle<float> (14.0f, 172.0f, (float) W - 28.0f, 30.0f);
        g.setFont (makeFont ("Futura", 24.0f, juce::Font::bold, 0.14f));
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawFittedText (title, plate.translated (0.0f, 1.5f).toNearestInt(), juce::Justification::centred, 1, 0.75f);
        g.setColour (textCol.withAlpha (0.93f));
        g.drawFittedText (title, plate.toNearestInt(), juce::Justification::centred, 1, 0.75f);

        g.setFont (makeFont ("Georgia", 12.5f, juce::Font::italic, 0.05f));
        g.setColour (textCol.withAlpha (0.6f));
        g.drawText (subtitle, juce::Rectangle<float> (14.0f, 201.0f, (float) W - 28.0f, 16.0f), juce::Justification::centred, false);
    }
}

void Pedal::paint (juce::Graphics& g)
{
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (body, getLocalBounds().toFloat());

    g.setFont (labelFont);
    for (size_t i = 0; i < labelAreas.size() && i < knobLabels.size(); ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.drawText (knobLabels[i], labelAreas[i].translated (0.0f, 1.0f), juce::Justification::centred, false);
        g.setColour (textCol.withAlpha (0.9f));
        g.drawText (knobLabels[i], labelAreas[i], juce::Justification::centred, false);
    }
}

//==============================================================================
LevelMeter::LevelMeter (EmoBoardProcessor& p) : proc (p)
{
    startTimerHz (20);
}

int LevelMeter::litSegments() const
{
    const float db = juce::Decibels::gainToDecibels (level, -60.0f);
    int n = 0;
    for (int i = 0; i < 16; ++i)
        if (db >= -48.0f + 3.0f * (float) i) ++n;
    return n;
}

void LevelMeter::timerCallback()
{
    level = juce::jmax (proc.outPeak.load(), level * 0.85f);
    const int lit = litSegments();
    if (lit != lastLit)          // only repaint when something visibly changes
    {
        lastLit = lit;
        repaint();
    }
}

void LevelMeter::paint (juce::Graphics& g)
{
    const int segs = 16;
    const int lit = litSegments();
    const float w = (float) getWidth() / (float) segs;
    const float h = (float) getHeight();

    for (int i = 0; i < segs; ++i)
    {
        const juce::Colour base = i < 9 ? juce::Colour (0xffa3ae45) : (i < 13 ? juce::Colour (0xffe0aa30) : juce::Colour (0xffd2482e));
        const auto r = juce::Rectangle<float> ((float) i * w + 1.5f, 3.0f, w - 3.0f, h - 6.0f);
        g.setColour (i < lit ? base : base.darker (0.85f).withAlpha (0.8f));
        g.fillRoundedRectangle (r, 2.0f);
    }
}

//==============================================================================
Board::Board (EmoBoardProcessor& p) : proc (p), meter (p)
{
    auto& a = p.apvts;
    using KD = std::vector<KnobDef>;

    pedals.push_back (std::make_unique<Pedal> (a, "HUSH", "noise gate", "gate_on",
        KD { { "gate_thr", "THRESH" }, { "gate_rel", "RELEASE" } },
        juce::Colour (0xff6c7a89), juce::Colour (0xffe8dcae)));
    pedals.push_back (std::make_unique<Pedal> (a, "SQUEEZE", "compressor", "comp_on",
        KD { { "comp_amt", "SUSTAIN" }, { "comp_atk", "ATTACK" }, { "comp_blend", "BLEND" } },
        juce::Colour (0xffd9b84a), juce::Colour (0xff2d2a22)));
    pedals.push_back (std::make_unique<Pedal> (a, "RUST", "overdrive / fuzz", "drv_on",
        KD { { "drv_mode", "MODE" }, { "drv_gain", "GAIN" }, { "drv_tone", "TONE" }, { "drv_level", "LEVEL" } },
        juce::Colour (0xffb4553a), juce::Colour (0xffeadfb4)));
    pedals.push_back (std::make_unique<Pedal> (a, "FLICKER", "tremolo", "trm_on",
        KD { { "trm_rate", "RATE" }, { "trm_depth", "DEPTH" }, { "trm_shape", "SHAPE" } },
        juce::Colour (0xffd9a79a), juce::Colour (0xff3b2b28)));
    pedals.push_back (std::make_unique<Pedal> (a, "DRIFT", "chorus", "cho_on",
        KD { { "cho_rate", "RATE" }, { "cho_depth", "DEPTH" }, { "cho_mix", "MIX" } },
        juce::Colour (0xff86a07a), juce::Colour (0xff2b2d22)));
    pedals.push_back (std::make_unique<Pedal> (a, "ECHO HOLLOW", "tape delay", "dly_on",
        KD { { "dly_time", "TIME" }, { "dly_fb", "REPEATS" }, { "dly_mix", "MIX" }, { "dly_wob", "WOBBLE" } },
        juce::Colour (0xff3f5f5a), juce::Colour (0xffe8dcae)));
    pedals.push_back (std::make_unique<Pedal> (a, "NOVEMBER", "reverb", "rev_on",
        KD { { "rev_size", "SIZE" }, { "rev_damp", "DAMP" }, { "rev_pre", "PRE-DLY" }, { "rev_mix", "MIX" } },
        juce::Colour (0xffe8e0c0), juce::Colour (0xff4a3a2a)));
    pedals.push_back (std::make_unique<Pedal> (a, "CASSETTE", "tape saturation", "tape_on",
        KD { { "tape_wow", "WOW" }, { "tape_dark", "DARK" }, { "tape_warm", "WARMTH" } },
        juce::Colour (0xff8c6a46), juce::Colour (0xffeadfb4)));

    for (auto& pd : pedals)
        addAndMakeVisible (*pd);

    const auto cap = juce::Colour (0xff3a3224);
    const auto ptr = juce::Colour (0xfff1e6cc);
    styleKnob (inKnob, cap, kInk.withAlpha (0.65f), ptr);
    styleKnob (outKnob, cap, kInk.withAlpha (0.65f), ptr);
    inAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "in_gain", inKnob);
    outAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "out_vol", outKnob);
    addAndMakeVisible (inKnob);
    addAndMakeVisible (outKnob);
    addAndMakeVisible (meter);

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (juce::jlimit (0, proc.getNumPrograms() - 1, proc.getCurrentProgram()),
                                    juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedItemIndex()); };
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (1); };
    addAndMakeVisible (presetBox);
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    setSize (kDesignW, kDesignH);
}

void Board::stepPreset (int delta)
{
    const int n = proc.getNumPrograms();
    if (n <= 0) return;
    const int cur = juce::jmax (0, presetBox.getSelectedItemIndex());
    presetBox.setSelectedItemIndex ((cur + delta + n) % n, juce::sendNotification);
}

void Board::resized()
{
    for (size_t i = 0; i < pedals.size(); ++i)
    {
        const int col = (int) i % 4, row = (int) i / 4;
        pedals[i]->setTopLeftPosition (26 + col * 266, 132 + row * 332);
    }

    inKnob.setBounds (704, 20, 64, 64);
    outKnob.setBounds (784, 20, 64, 64);
    meter.setBounds (872, 40, 170, 24);

    prevBtn.setBounds (430, 52, 26, 26);
    presetBox.setBounds (460, 52, 186, 26);
    nextBtn.setBounds (650, 52, 26, 26);

    if (background.isNull())
        renderBackground();
}

void Board::renderBackground()
{
    const float sc = 2.0f;
    const float fW = (float) kDesignW, fH = (float) kDesignH;
    background = juce::Image (juce::Image::ARGB, juce::roundToInt (fW * sc), juce::roundToInt (fH * sc),
                              true, juce::SoftwareImageType());
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (sc));
    juce::Random rng (20241116);

    // ---- sun-bleached sour-yellow paper ----
    {
        juce::ColourGradient paper (juce::Colour (0xffe9df9c), 0.0f, 0.0f, juce::Colour (0xffc9c18a), 0.0f, fH, false);
        paper.addColour (0.5, juce::Colour (0xffdcd28f));
        g.setGradientFill (paper);
        g.fillAll();
    }

    // ---- faded halftone dots (pink, bottom right to top) ----
    for (int gy = 0; gy < 46; ++gy)
        for (int gx = 0; gx < 66; ++gx)
        {
            const float x = (float) gx * 18.0f + (gy % 2 ? 9.0f : 0.0f);
            const float y = (float) gy * 18.0f;
            const float t = juce::jlimit (0.0f, 1.0f, (x / fW) * 0.6f + (y / fH) * 0.5f - 0.35f);
            const float r = 5.0f * t;
            if (r > 0.6f)
            {
                g.setColour (juce::Colour (0xffd98f86).withAlpha (0.35f));
                g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f);
            }
        }

    // ---- big faded lemon slices peeking in ----
    drawLemon (g, { 1010.0f, 60.0f }, 120.0f, 0.3f, 0.45f);
    drawLemon (g, { 70.0f, 112.0f }, 70.0f, 1.0f, 0.4f);
    drawLemon (g, { 560.0f, 40.0f }, 44.0f, 0.6f, 0.35f);

    // ---- low-sun watercolour blotches ----
    for (int i = 0; i < 7; ++i)
    {
        const float cx = rng.nextFloat() * fW, cy = rng.nextFloat() * fH;
        const float r = 60.0f + rng.nextFloat() * 120.0f;
        juce::ColourGradient bl ((i % 2 ? juce::Colour (0xffa9b567) : juce::Colour (0xffd9a05a)).withAlpha (0.16f), cx, cy,
                                 juce::Colours::transparentBlack, cx + r, cy, true);
        g.setGradientFill (bl);
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    // ---- felt pedalboard cloth ----
    const auto cloth = juce::Rectangle<float> (12.0f, 122.0f, fW - 24.0f, fH - 134.0f);
    {
        juce::Path cp;
        cp.addRoundedRectangle (cloth, 14.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.45f), 16, { 0, 7 }).drawForPath (g, cp);

        juce::ColourGradient felt (juce::Colour (0xff5f817a), 0.0f, cloth.getY(), juce::Colour (0xff48665f), 0.0f, cloth.getBottom(), false);
        g.setGradientFill (felt);
        g.fillPath (cp);

        g.setColour (juce::Colours::black.withAlpha (0.05f));
        for (int i = 0; i < 6000; ++i)
            g.fillRect (cloth.getX() + rng.nextFloat() * cloth.getWidth(), cloth.getY() + rng.nextFloat() * cloth.getHeight(), 1.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        for (int i = 0; i < 6000; ++i)
            g.fillRect (cloth.getX() + rng.nextFloat() * cloth.getWidth(), cloth.getY() + rng.nextFloat() * cloth.getHeight(), 1.0f, 1.0f);

        // stitched edge
        juce::Path stitch;
        stitch.addRoundedRectangle (cloth.reduced (7.0f), 10.0f);
        juce::Path dashed;
        const float dashes[] = { 6.0f, 5.0f };
        juce::PathStrokeType (1.4f).createDashedStroke (dashed, stitch, dashes, 2);
        g.setColour (juce::Colour (0xffe6dca8).withAlpha (0.55f));
        g.fillPath (dashed);

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawRoundedRectangle (cloth, 14.0f, 1.4f);
    }

    // ---- patch cables ----
    auto cable = [&] (const juce::Path& p)
    {
        g.setColour (juce::Colour (0xff0b0a08));
        g.strokePath (p, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white.withAlpha (0.12f));
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
    {
        juce::Path pin;  pin.startNewSubPath (12.0f, jy1 + 6.0f);  pin.quadraticTo (18.0f, jy1 + 16.0f, 28.0f, jy1);
        cable (pin);  plug (24.0f, jy1);
        juce::Path pout; pout.startNewSubPath (26.0f + 3.0f * 266.0f + 250.0f - 2.0f, jy2);
        pout.quadraticTo (fW - 22.0f, jy2 + 16.0f, fW - 12.0f, jy2 + 6.0f);
        cable (pout);
        juce::Path loop;
        loop.startNewSubPath (26.0f + 3.0f * 266.0f + 250.0f - 2.0f, jy1);
        loop.cubicTo (fW + 40.0f, jy1 + 20.0f, -40.0f, jy2 - 20.0f, 28.0f, jy2);
        cable (loop);
        plug (24.0f, jy2);
    }

    // ---- a few fallen leaves on the cloth ----
    for (int i = 0; i < 16; ++i)
    {
        const juce::Point<float> lp (cloth.getX() + 10.0f + rng.nextFloat() * (cloth.getWidth() - 20.0f),
                                     cloth.getY() + 10.0f + rng.nextFloat() * (cloth.getHeight() - 20.0f));
        drawLeaf (g, lp, 7.0f + rng.nextFloat() * 7.0f, rng.nextFloat() * 6.28f, fadedLeaf (rng).withAlpha (0.85f));
    }

    // ---- master card ----
    {
        const auto card = juce::Rectangle<float> (690.0f, 14.0f, 384.0f, 98.0f);
        juce::Path pp;
        pp.addRoundedRectangle (card, 6.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.4f), 10, { 0, 4 }).drawForPath (g, pp);
        g.setColour (juce::Colour (0xfff3ecc6));
        g.fillPath (pp);
        g.setColour (kInk.withAlpha (0.35f));
        g.drawRoundedRectangle (card, 6.0f, 1.0f);

        g.setColour (juce::Colour (0xff0e0d0b));
        g.fillRoundedRectangle (866.0f, 36.0f, 182.0f, 32.0f, 4.0f);

        drawTape (g, 672.0f, 6.0f, 56.0f, 18.0f, -0.6f);
        drawTape (g, 1040.0f, 98.0f, 56.0f, 18.0f, -0.5f);
    }

    // ---- title card + preset label ----
    drawTape (g, 14.0f, 10.0f, 56.0f, 18.0f, -0.25f);
    drawTape (g, 640.0f, 96.0f, 40.0f, 16.0f, 0.4f);
}

void Board::paint (juce::Graphics& g)
{
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (background, getLocalBounds().toFloat());

    g.setFont (makeFont ("Georgia", 50.0f, juce::Font::bold | juce::Font::italic));
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawText ("emoboard", 37, 17, 380, 60, juce::Justification::centredLeft, false);
    g.setColour (kPaperText);
    g.drawText ("emoboard", 34, 14, 380, 60, juce::Justification::centredLeft, false);

    g.setFont (makeFont ("Georgia", 15.0f, juce::Font::italic, 0.03f));
    g.setColour (kPaperText.withAlpha (0.75f));
    g.drawText ("songs for leaving early, still sour", 38, 74, 380, 20, juce::Justification::centredLeft, false);

    g.setFont (makeFont ("Futura", 9.5f, juce::Font::bold, 0.12f));
    g.setColour (kPaperText.withAlpha (0.55f));
    g.drawText ("HUSH / SQUEEZE / RUST / FLICKER / DRIFT / ECHO HOLLOW / NOVEMBER / CASSETTE",
                38, 100, 620, 14, juce::Justification::centredLeft, false);

    g.setFont (makeFont ("Futura", 10.0f, juce::Font::bold, 0.14f));
    g.setColour (kPaperText.withAlpha (0.8f));
    g.drawText ("PRESET", 430, 36, 120, 14, juce::Justification::centredLeft, false);

    g.setColour (kInk.withAlpha (0.85f));
    g.drawText ("INPUT", inKnob.getX(), inKnob.getBottom() - 2, inKnob.getWidth(), 12, juce::Justification::centred, false);
    g.drawText ("OUTPUT", outKnob.getX(), outKnob.getBottom() - 2, outKnob.getWidth(), 12, juce::Justification::centred, false);
    g.drawText ("LEVEL", meter.getX(), meter.getBottom() + 6, meter.getWidth(), 12, juce::Justification::centred, false);
}

//==============================================================================
EmoBoardEditor::EmoBoardEditor (EmoBoardProcessor& p)
    : AudioProcessorEditor (&p), board (p)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (board);

    setResizable (true, true);
    setResizeLimits (660, 480, 1650, 1200);
    getConstrainer()->setFixedAspectRatio ((double) kDesignW / (double) kDesignH);
    setSize (kDesignW, kDesignH);
}

EmoBoardEditor::~EmoBoardEditor()
{
    setLookAndFeel (nullptr);
}

void EmoBoardEditor::resized()
{
    const float s = (float) getWidth() / (float) kDesignW;
    board.setBounds (0, 0, kDesignW, kDesignH);
    board.setTransform (juce::AffineTransform::scale (s));
}
