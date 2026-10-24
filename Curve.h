#pragma once
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <vector>

struct CurvePoint
{
    float x = 0.f, y = 0.f, tension = 0.f;
};

// Eine zeichenbare Kurve (0..1 in x und y). "tension" formt das Segment ab diesem Punkt.
class Curve
{
public:
    static constexpr int tableSize = 1024;

    std::vector<CurvePoint> points;

    Curve() { reset(); }

    void reset() { points = { { 0.f, 1.f, 0.f }, { 1.f, 1.f, 0.f } }; }

    void sanitize()
    {
        for (auto& p : points)
        {
            p.x = juce::jlimit (0.f, 1.f, p.x);
            p.y = juce::jlimit (0.f, 1.f, p.y);
            p.tension = juce::jlimit (-1.f, 1.f, p.tension);
        }
        std::stable_sort (points.begin(), points.end(),
                          [] (const CurvePoint& a, const CurvePoint& b) { return a.x < b.x; });
        if (points.size() < 2) { reset(); return; }
        points.front().x = 0.f;
        points.back().x = 1.f;
    }

    float eval (float x) const
    {
        if (points.empty()) return 1.f;
        x = juce::jlimit (0.f, 1.f, x);
        if (x <= points.front().x) return points.front().y;

        for (size_t i = 0; i + 1 < points.size(); ++i)
        {
            const auto& a = points[i];
            const auto& b = points[i + 1];
            if (x <= b.x)
            {
                const float w = b.x - a.x;
                if (w <= 1.0e-6f) return b.y;
                float t = (x - a.x) / w;
                t = std::pow (t, std::pow (3.f, a.tension));
                return a.y + (b.y - a.y) * t;
            }
        }
        return points.back().y;
    }

    // dest braucht tableSize + 1 Einträge
    void buildTable (float* dest) const
    {
        for (int i = 0; i <= tableSize; ++i)
            dest[i] = eval ((float) i / (float) tableSize);
    }

    juce::String toString() const
    {
        juce::StringArray parts;
        for (auto& p : points)
            parts.add (juce::String (p.x, 4) + "," + juce::String (p.y, 4) + "," + juce::String (p.tension, 3));
        return parts.joinIntoString (";");
    }

    bool fromString (const juce::String& s)
    {
        std::vector<CurvePoint> pts;
        for (auto& part : juce::StringArray::fromTokens (s, ";", ""))
        {
            auto v = juce::StringArray::fromTokens (part, ",", "");
            if (v.size() >= 2)
                pts.push_back ({ v[0].getFloatValue(), v[1].getFloatValue(), v.size() > 2 ? v[2].getFloatValue() : 0.f });
        }
        if (pts.size() < 2) return false;
        points = pts;
        sanitize();
        return true;
    }

    // ---- Formen ----
    static Curve flat() { return Curve(); }

    static Curve sawDown() { Curve c; c.points = { { 0.f, 1.f, 0.f }, { 1.f, 0.f, 0.f } }; return c; }
    static Curve sawUp()   { Curve c; c.points = { { 0.f, 0.f, 0.f }, { 1.f, 1.f, 0.f } }; return c; }

    static Curve triangle() { Curve c; c.points = { { 0.f, 0.f, 0.f }, { 0.5f, 1.f, 0.f }, { 1.f, 0.f, 0.f } }; return c; }

    static Curve sine()
    {
        Curve c;
        c.points.clear();
        const int n = 24;
        for (int i = 0; i <= n; ++i)
        {
            const float x = (float) i / (float) n;
            c.points.push_back ({ x, 0.5f + 0.5f * std::cos (juce::MathConstants<float>::twoPi * x), 0.f });
        }
        return c;
    }

    // Ducking wie ein Sidechain-Kompressor: unten starten, schnell zurückkommen
    static Curve pump()
    {
        Curve c;
        c.points = { { 0.f, 0.f, -0.55f }, { 0.7f, 1.f, 0.f }, { 1.f, 1.f, 0.f } };
        return c;
    }

    static Curve steps (const std::vector<float>& v)
    {
        Curve c;
        c.points.clear();
        const int n = (int) v.size();
        for (int i = 0; i < n; ++i)
        {
            const float x0 = (float) i / (float) n;
            const float x1 = (float) (i + 1) / (float) n;
            c.points.push_back ({ x0, v[(size_t) i], 0.f });
            c.points.push_back ({ i == n - 1 ? 1.f : x1 - 0.0005f, v[(size_t) i], 0.f });
        }
        if (c.points.size() < 2) c.reset();
        return c;
    }

    static Curve square (int n = 2)
    {
        std::vector<float> v;
        for (int i = 0; i < n; ++i) v.push_back (i % 2 == 0 ? 1.f : 0.f);
        return steps (v);
    }

    static Curve randomSteps (int n, int seed)
    {
        juce::Random rnd (seed);
        std::vector<float> v;
        for (int i = 0; i < n; ++i) v.push_back ((float) rnd.nextInt (4) / 3.f);
        return steps (v);
    }
};
