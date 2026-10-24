#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// Zeichenfläche für die Layer-Kurve
class CurveEditor : public juce::Component, private juce::Timer
{
public:
    explicit CurveEditor (TaubenBoxProcessor& p) : proc (p) { startTimerHz (30); }

    void setLayer (int l, juce::Colour c)
    {
        layer = l;
        colour = c;
        curve = proc.getCurve (l);
        repaint();
    }

    void reload() { curve = proc.getCurve (layer); repaint(); }
    void applyCurve (const Curve& c) { curve = c; commit(); repaint(); }
    void setSnap (bool s) { snap = s; }
    void setGridDiv (int d) { gridDiv = juce::jmax (1, d); repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xaa0c1020));
        g.fillRoundedRectangle (bounds, 10.f);
        g.setColour (juce::Colour (0x33ffffff));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 10.f, 1.f);

        const auto a = area();

        for (int i = 0; i <= gridDiv; ++i)
        {
            const float x = a.getX() + a.getWidth() * (float) i / (float) gridDiv;
            const bool major = (i % 4 == 0);
            g.setColour (juce::Colour (major ? 0x28ffffff : 0x12ffffff));
            g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
        }
        for (int j = 0; j <= 4; ++j)
        {
            const float y = a.getY() + a.getHeight() * (float) j / 4.f;
            g.setColour (juce::Colour (0x18ffffff));
            g.drawHorizontalLine ((int) y, a.getX(), a.getRight());
        }

        juce::Path line;
        const int steps = juce::jmax (2, (int) a.getWidth() / 2);
        for (int i = 0; i <= steps; ++i)
        {
            const float xn = (float) i / (float) steps;
            const auto p = toScreen (xn, curve.eval (xn));
            if (i == 0) line.startNewSubPath (p);
            else line.lineTo (p);
        }
        juce::Path fill (line);
        fill.lineTo (a.getRight(), a.getBottom());
        fill.lineTo (a.getX(), a.getBottom());
        fill.closeSubPath();

        g.setGradientFill (juce::ColourGradient (colour.withAlpha (0.5f), 0.f, a.getY(),
                                                 colour.withAlpha (0.04f), 0.f, a.getBottom(), false));
        g.fillPath (fill);
        g.setColour (colour);
        g.strokePath (line, juce::PathStrokeType (2.2f));

        for (const auto& pt : curve.points)
        {
            const auto p = toScreen (pt.x, pt.y);
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.fillEllipse (p.x - 5.f, p.y - 5.f, 10.f, 10.f);
            g.setColour (colour);
            g.drawEllipse (p.x - 5.f, p.y - 5.f, 10.f, 10.f, 2.f);
        }

        const float ph = proc.layerPhase[(size_t) layer].load();
        const float px = a.getX() + ph * a.getWidth();
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawLine (px, a.getY(), px, a.getBottom(), 1.5f);
        const float cy = toScreen (ph, curve.eval (ph)).y;
        g.setColour (juce::Colours::white);
        g.fillEllipse (px - 4.f, cy - 4.f, 8.f, 8.f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int hit = findPoint (e.position);
        const int last = (int) curve.points.size() - 1;

        if (e.mods.isPopupMenu())
        {
            if (hit > 0 && hit < last)
            {
                curve.points.erase (curve.points.begin() + hit);
                commit();
                repaint();
            }
            return;
        }

        if (hit >= 0) { dragIdx = hit; return; }

        const auto n = fromScreen (e.position);
        CurvePoint cp { juce::jlimit (0.002f, 0.998f, snapX (n.x)), snapY (n.y), 0.f };
        size_t idx = 0;
        while (idx < curve.points.size() && curve.points[idx].x < cp.x) ++idx;
        curve.points.insert (curve.points.begin() + (long) idx, cp);
        dragIdx = (int) idx;
        commit();
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragIdx < 0 || dragIdx >= (int) curve.points.size()) return;

        const auto n = fromScreen (e.position);
        auto& p = curve.points[(size_t) dragIdx];
        p.y = juce::jlimit (0.f, 1.f, snapY (n.y));

        const int last = (int) curve.points.size() - 1;
        if (dragIdx > 0 && dragIdx < last)
        {
            const float lo = curve.points[(size_t) dragIdx - 1].x + 0.002f;
            const float hi = curve.points[(size_t) dragIdx + 1].x - 0.002f;
            p.x = juce::jlimit (lo, juce::jmax (lo, hi), snapX (n.x));
        }
        commit();
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override { dragIdx = -1; }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const int hit = findPoint (e.position);
        const int last = (int) curve.points.size() - 1;
        if (hit > 0 && hit < last)
        {
            curve.points.erase (curve.points.begin() + hit);
            dragIdx = -1;
            commit();
            repaint();
        }
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        const auto n = fromScreen (e.position);
        for (size_t i = 0; i + 1 < curve.points.size(); ++i)
        {
            if (curve.points[i].x <= n.x && n.x < curve.points[i + 1].x)
            {
                curve.points[i].tension = juce::jlimit (-1.f, 1.f, curve.points[i].tension + wheel.deltaY * 0.6f);
                commit();
                repaint();
                break;
            }
        }
    }

private:
    void timerCallback() override { if (isShowing()) repaint(); }

    juce::Rectangle<float> area() const { return getLocalBounds().toFloat().reduced (12.f); }

    juce::Point<float> toScreen (float x, float y) const
    {
        const auto a = area();
        return { a.getX() + x * a.getWidth(), a.getBottom() - y * a.getHeight() };
    }

    juce::Point<float> fromScreen (juce::Point<float> p) const
    {
        const auto a = area();
        return { juce::jlimit (0.f, 1.f, (p.x - a.getX()) / a.getWidth()),
                 juce::jlimit (0.f, 1.f, 1.f - (p.y - a.getY()) / a.getHeight()) };
    }

    int findPoint (juce::Point<float> pos) const
    {
        for (size_t i = 0; i < curve.points.size(); ++i)
            if (toScreen (curve.points[i].x, curve.points[i].y).getDistanceFrom (pos) <= 9.f)
                return (int) i;
        return -1;
    }

    float snapX (float x) const { return snap ? std::round (x * (float) gridDiv) / (float) gridDiv : x; }
    float snapY (float y) const { return snap ? std::round (y * 8.f) / 8.f : y; }

    void commit() { proc.setCurve (layer, curve, false); }

    TaubenBoxProcessor& proc;
    Curve curve;
    int layer = 0;
    int dragIdx = -1;
    int gridDiv = 16;
    bool snap = true;
    juce::Colour colour { 0xff52e3c2 };
};
