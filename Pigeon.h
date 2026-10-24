#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <vector>

namespace tb
{
inline juce::Path makeWingPath()
{
    juce::Path w;
    w.startNewSubPath (0.f, 0.f);
    w.cubicTo (-4.f, -14.f, -14.f, -30.f, -26.f, -40.f);
    w.cubicTo (-24.f, -22.f, -26.f, -10.f, -20.f, 0.f);
    w.closeSubPath();
    return w;
}

// Zeichnet eine Taube (Blick nach rechts). dir = -1 spiegelt, flap in -1..1
inline void drawPigeon (juce::Graphics& g, float x, float y, float scale, float dir, float flap, float alpha)
{
    using juce::AffineTransform;
    using juce::Colour;
    using juce::Path;

    const auto tf = AffineTransform::scale (scale * dir, scale).translated (x, y);
    auto col = [alpha] (juce::uint32 argb) { return Colour (argb).withMultipliedAlpha (alpha); };

    // hinterer Flügel
    {
        Path w = makeWingPath();
        const float ang = -0.9f + (-flap + 1.f) * 0.55f;
        w.applyTransform (AffineTransform::scale (0.9f).rotated (ang).translated (2.f, -6.f).followedBy (tf));
        g.setColour (col (0xff4d566f));
        g.fillPath (w);
    }
    // Schwanz
    {
        Path t;
        t.startNewSubPath (-16.f, -4.f);
        t.lineTo (-42.f, -8.f);
        t.lineTo (-46.f, 0.f);
        t.lineTo (-42.f, 8.f);
        t.lineTo (-16.f, 5.f);
        t.closeSubPath();
        t.applyTransform (tf);
        g.setColour (col (0xff626c88));
        g.fillPath (t);
    }
    // Körper
    {
        Path b;
        b.addEllipse (-24.f, -11.f, 48.f, 22.f);
        b.applyTransform (tf);
        g.setColour (col (0xff949db8));
        g.fillPath (b);
    }
    // Kopf
    {
        Path h;
        h.addEllipse (16.5f, -16.5f, 15.f, 15.f);
        h.applyTransform (tf);
        g.setColour (col (0xffa4adc6));
        g.fillPath (h);
    }
    // schillernder Hals
    {
        Path neck;
        neck.addEllipse (8.f, -13.f, 15.f, 14.f);
        neck.applyTransform (tf);
        float x1 = 12.f, y1 = -12.f, x2 = 20.f, y2 = 0.f;
        tf.transformPoint (x1, y1);
        tf.transformPoint (x2, y2);
        g.setGradientFill (juce::ColourGradient (col (0xff52e3c2), x1, y1, col (0xffb26bff), x2, y2, false));
        g.fillPath (neck);
    }
    // Schnabel
    {
        Path beak;
        beak.startNewSubPath (30.f, -11.f);
        beak.lineTo (38.f, -8.5f);
        beak.lineTo (30.f, -6.5f);
        beak.closeSubPath();
        beak.applyTransform (tf);
        g.setColour (col (0xffe9c8a0));
        g.fillPath (beak);
    }
    // Auge
    {
        Path eye;
        eye.addEllipse (25.f, -12.f, 3.f, 3.f);
        eye.applyTransform (tf);
        g.setColour (col (0xffffa94d));
        g.fillPath (eye);
    }
    // vorderer Flügel
    {
        Path w = makeWingPath();
        const float ang = -0.9f + (flap + 1.f) * 0.55f;
        w.applyTransform (AffineTransform::rotation (ang).translated (4.f, -6.f).followedBy (tf));
        g.setColour (col (0xff7a85a3));
        g.fillPath (w);
    }
}

// Schwarm im Hintergrund
class PigeonField : public juce::Component, private juce::Timer
{
public:
    PigeonField()
    {
        setInterceptsMouseClicks (false, false);
        setOpaque (false);
        startTimerHz (30);
    }

    void resized() override
    {
        if (! flock.empty() || getWidth() <= 0) return;
        const float w = (float) getWidth(), h = (float) getHeight();
        for (int i = 0; i < 10; ++i)
        {
            Pigeon p;
            p.dir = rng.nextBool() ? 1.f : -1.f;
            p.scale = 0.45f + rng.nextFloat() * 0.9f;
            p.x = rng.nextFloat() * w;
            p.baseY = h * (0.06f + 0.9f * rng.nextFloat());
            p.y = p.baseY;
            p.speed = (0.6f + rng.nextFloat() * 1.6f) * p.scale;
            p.phase = rng.nextFloat() * 6.28f;
            flock.push_back (p);
        }
    }

    void paint (juce::Graphics& g) override
    {
        for (const auto& p : flock)
            drawPigeon (g, p.x, p.y, p.scale, p.dir, std::sin (t * 9.f + p.phase),
                        0.14f + 0.28f * (p.scale / 1.35f));
    }

private:
    struct Pigeon
    {
        float x = 0, y = 0, baseY = 0, speed = 1, scale = 1, phase = 0, dir = 1;
    };

    void timerCallback() override
    {
        t += 1.f / 30.f;
        const float w = (float) getWidth();
        for (auto& p : flock)
        {
            p.x += p.speed * p.dir;
            if (p.dir > 0 && p.x > w + 90.f) p.x = -90.f;
            if (p.dir < 0 && p.x < -90.f) p.x = w + 90.f;
            p.y = p.baseY + std::sin (t * 1.3f + p.phase * 3.f) * 10.f;
        }
        repaint();
    }

    std::vector<Pigeon> flock;
    float t = 0.f;
    juce::Random rng;
};
} // namespace tb
