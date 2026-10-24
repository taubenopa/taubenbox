#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>
#include <vector>
#include "Curve.h"

namespace tb
{
constexpr int numLayers = 4;

enum LayerType { Volume = 0, Filter, Drive, Crush, Pan, Width, Granular, numTypes };

enum Rate { R4Bars = 0, R2Bars, R1Bar, R1_2, R1_4, R1_8, R1_16, R1_32, R1_4T, R1_8T, R1_16T };

inline juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

inline juce::StringArray typeNames()
{
    juce::StringArray n;
    n.add (u8 ("Flügelschlag (Volume)"));
    n.add (u8 ("Federn (Filter)"));
    n.add (u8 ("Kropf (Drive)"));
    n.add (u8 ("Krümel (Crush)"));
    n.add (u8 ("Schwingen (Pan)"));
    n.add (u8 ("Spreizen (Width)"));
    n.add (u8 ("Schwarm (Granular)"));
    return n;
}

inline juce::String shortTypeName (int t)
{
    static const char* s[] = { "Flügelschlag", "Federn", "Kropf", "Krümel", "Schwingen", "Spreizen", "Schwarm" };
    return u8 (s[juce::jlimit (0, (int) numTypes - 1, t)]);
}

inline juce::StringArray rateNames()
{
    return { "4 Takte", "2 Takte", "1 Takt", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4 T", "1/8 T", "1/16 T" };
}

// Länge eines Zyklus in Vierteln
inline double rateBeats (int i)
{
    static const double b[] = { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };
    return b[juce::jlimit (0, 10, i)];
}

struct LayerParams
{
    int type = 0;
    bool on = false;
    bool retrig = false;
    int rate = 4;
    float depth = 1.f, mix = 1.f, smooth = 0.f, phase = 0.f;
    float a = 0.5f, b = 0.5f, c = 0.5f;
};

// Zustandsvariablen-Filter (TPT)
struct SVF
{
    float s1 = 0.f, s2 = 0.f;
    void reset() { s1 = s2 = 0.f; }
    inline void process (float x, float g, float R, float& lp, float& bp, float& hp)
    {
        hp = (x - (2.f * R + g) * s1 - s2) / (1.f + 2.f * R * g + g * g);
        bp = g * hp + s1;
        s1 = g * hp + bp;
        lp = g * bp + s2;
        s2 = g * bp + lp;
    }
};

// Einfacher Granulator mit 2 Sekunden Puffer
class Granulator
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        size = (int) (sr * 2.0);
        bufL.assign ((size_t) size, 0.f);
        bufR.assign ((size_t) size, 0.f);
        writePos = 0;
        spawnCounter = 0.f;
        for (auto& g : grains) g.active = false;
    }

    void process (float& l, float& r, float sizeMs, float density, float pitchSemi, float amt)
    {
        if (size <= 0) return;

        bufL[(size_t) writePos] = l;
        bufR[(size_t) writePos] = r;

        spawnCounter -= 1.f;
        if (spawnCounter <= 0.f)
        {
            spawn (sizeMs, pitchSemi);
            const float interval = (float) sampleRate / juce::jmax (0.5f, density);
            spawnCounter += interval * (0.75f + 0.5f * rng.nextFloat());
        }

        float oL = 0.f, oR = 0.f;
        for (auto& g : grains)
        {
            if (! g.active) continue;

            const float w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * g.age / g.length);
            int i0 = (int) g.readPos;
            const float fr = g.readPos - (float) i0;
            if (i0 >= size) i0 -= size;
            int i1 = i0 + 1;
            if (i1 >= size) i1 = 0;

            oL += (bufL[(size_t) i0] + (bufL[(size_t) i1] - bufL[(size_t) i0]) * fr) * w * g.gL;
            oR += (bufR[(size_t) i0] + (bufR[(size_t) i1] - bufR[(size_t) i0]) * fr) * w * g.gR;

            g.readPos += g.ratio;
            if (g.readPos >= (float) size) g.readPos -= (float) size;
            g.age += 1.f;
            if (g.age >= g.length) g.active = false;
        }

        const float overlap = density * sizeMs * 0.001f;
        const float norm = 1.f / std::sqrt (juce::jmax (1.f, overlap * 0.375f));
        l = l * (1.f - amt) + oL * norm * amt;
        r = r * (1.f - amt) + oR * norm * amt;

        if (++writePos >= size) writePos = 0;
    }

private:
    struct Grain
    {
        bool active = false;
        float readPos = 0.f, ratio = 1.f, age = 0.f, length = 1.f, gL = 1.f, gR = 1.f;
    };

    void spawn (float sizeMs, float pitchSemi)
    {
        for (auto& g : grains)
        {
            if (g.active) continue;

            const float len = juce::jlimit (64.f, (float) size * 0.4f, sizeMs * 0.001f * (float) sampleRate);
            const float ratio = std::pow (2.f, pitchSemi / 12.f);
            const float offset = len * juce::jmax (1.f, ratio) + 8.f + rng.nextFloat() * 0.02f * (float) sampleRate;
            float start = (float) writePos - offset;
            while (start < 0.f) start += (float) size;

            const float theta = juce::MathConstants<float>::pi * 0.25f + (rng.nextFloat() - 0.5f) * 0.5f;
            g.readPos = start;
            g.ratio = ratio;
            g.age = 0.f;
            g.length = len;
            g.gL = std::cos (theta) * 1.4142f;
            g.gR = std::sin (theta) * 1.4142f;
            g.active = true;
            return;
        }
    }

    double sampleRate = 44100.0;
    int size = 0, writePos = 0;
    float spawnCounter = 0.f;
    std::vector<float> bufL, bufR;
    std::array<Grain, 32> grains;
    juce::Random rng;
};

// Ein Layer: liest die Kurve und wendet den gewählten Effekt an
class LayerDsp
{
public:
    std::array<float, (size_t) Curve::tableSize + 1> table;

    LayerDsp() { table.fill (1.f); }

    void prepare (double sr)
    {
        sampleRate = sr;
        grain.prepare (sr);
        svfL.reset();
        svfR.reset();
        smoothed = 1.f;
        holdCounter = 0.f;
        heldL = heldR = 0.f;
    }

    void setSmoothing (float smooth)
    {
        const float tau = smooth * smooth * 0.1f; // bis 100 ms
        smoothCoef = tau < 1.0e-4f ? 0.f : std::exp (-1.f / (tau * (float) sampleRate));
    }

    void process (float& l, float& r, float phase, const LayerParams& p)
    {
        const float pos = phase * (float) Curve::tableSize;
        const int i = juce::jlimit (0, Curve::tableSize - 1, (int) pos);
        const float fr = pos - (float) i;
        const float raw = table[(size_t) i] + (table[(size_t) i + 1] - table[(size_t) i]) * fr;
        smoothed = smoothCoef * smoothed + (1.f - smoothCoef) * raw;

        const float c = smoothed;
        const float d = p.depth;
        float wl = l, wr = r;

        switch (p.type)
        {
            case Volume:
            {
                const float g = 1.f - d * (1.f - c);
                wl = l * g;
                wr = r * g;
                break;
            }
            case Filter:
            {
                const float fcBase = 40.f * std::pow (450.f, p.a);
                const float oct = (c - 0.5f) * 2.f * d * 5.f;
                const float fc = juce::jlimit (20.f, 0.45f * (float) sampleRate, fcBase * std::pow (2.f, oct));
                const float g = std::tan (juce::MathConstants<float>::pi * fc / (float) sampleRate);
                const float Q = 0.5f + p.b * p.b * 11.5f;
                const float R = 1.f / (2.f * Q);

                float lpL, bpL, hpL, lpR, bpR, hpR;
                svfL.process (l, g, R, lpL, bpL, hpL);
                svfR.process (r, g, R, lpR, bpR, hpR);
                bpL *= 2.f * R;
                bpR *= 2.f * R;

                const float m = p.c;
                auto morph = [m] (float lp, float bp, float hp)
                {
                    return m < 0.5f ? lp + (bp - lp) * (m * 2.f) : bp + (hp - bp) * ((m - 0.5f) * 2.f);
                };
                wl = morph (lpL, bpL, hpL);
                wr = morph (lpR, bpR, hpR);
                break;
            }
            case Drive:
            {
                const float amount = d * c;
                const float drv = 1.f + amount * p.a * 40.f;
                const float k = juce::jlimit (0.f, 1.f, amount * 6.f);
                const float comp = 1.f / std::tanh (0.6f * drv + 0.4f);
                wl = l + (std::tanh (l * drv) * comp - l) * k;
                wr = r + (std::tanh (r * drv) * comp - r) * k;
                break;
            }
            case Crush:
            {
                const float amount = d * c;
                const float bits = 16.f - amount * p.a * 14.f;
                const float levels = std::pow (2.f, bits - 1.f);
                const float factor = 1.f + amount * p.b * 40.f;
                holdCounter += 1.f;
                if (holdCounter >= factor)
                {
                    holdCounter -= factor;
                    heldL = std::round (l * levels) / levels;
                    heldR = std::round (r * levels) / levels;
                }
                wl = heldL;
                wr = heldR;
                break;
            }
            case Pan:
            {
                const float pan = (c * 2.f - 1.f) * d;
                const float theta = (pan + 1.f) * juce::MathConstants<float>::pi * 0.25f;
                wl = l * std::cos (theta) * 1.4142f;
                wr = r * std::sin (theta) * 1.4142f;
                break;
            }
            case Width:
            {
                const float mid = 0.5f * (l + r);
                const float side = 0.5f * (l - r);
                const float w = (1.f - d) + d * c * 2.f;
                wl = mid + side * w;
                wr = mid - side * w;
                break;
            }
            case Granular:
            {
                const float amt = p.mix * (1.f - d + d * c);
                const float sizeMs = 10.f + p.a * p.a * 390.f;
                const float density = 1.f + p.b * p.b * 59.f;
                const float pitch = (p.c - 0.5f) * 24.f;
                grain.process (l, r, sizeMs, density, pitch, amt);
                return;
            }
            default: break;
        }

        l = l + (wl - l) * p.mix;
        r = r + (wr - r) * p.mix;
    }

private:
    double sampleRate = 44100.0;
    float smoothed = 1.f, smoothCoef = 0.f;
    float holdCounter = 0.f, heldL = 0.f, heldR = 0.f;
    SVF svfL, svfR;
    Granulator grain;
};
} // namespace tb
