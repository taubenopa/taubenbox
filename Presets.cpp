#include "Presets.h"

namespace tb
{
namespace
{
LayerPreset mk (int type, int rate, const Curve& curve, float depth = 1.f, float mix = 1.f,
                float a = 0.5f, float b = 0.5f, float c = 0.5f, float smooth = 0.f, bool retrig = false)
{
    LayerPreset l;
    l.type = type;
    l.on = true;
    l.rate = rate;
    l.curve = curve;
    l.depth = depth;
    l.mix = mix;
    l.a = a;
    l.b = b;
    l.c = c;
    l.smooth = smooth;
    l.retrig = retrig;
    return l;
}

Preset mkp (const char* cat, const char* name, std::initializer_list<LayerPreset> layers,
            float outGain = 0.f, float mix = 1.f)
{
    Preset p;
    p.category = u8 (cat);
    p.name = u8 (name);
    int i = 0;
    for (const auto& l : layers)
        if (i < numLayers)
            p.layers[(size_t) i++] = l;
    p.outGain = outGain;
    p.mix = mix;
    return p;
}
} // namespace

const std::vector<Preset>& getPresets()
{
    static const std::vector<Preset> presets = []
    {
        std::vector<Preset> p;

        p.push_back (mkp ("Start", "Init (Taubenschlag leer)", { mk (Volume, R1_4, Curve::flat(), 0.f) }));

        p.push_back (mkp ("Rhythmus", "Sidechain Taube", { mk (Volume, R1_4, Curve::pump()) }));
        p.push_back (mkp ("Rhythmus", "Sidechain Sanft", { mk (Volume, R1_4, Curve::pump(), 0.6f, 1.f, 0.5f, 0.5f, 0.5f, 0.25f) }));
        p.push_back (mkp ("Rhythmus", "Gurr-Gate 1/16", { mk (Volume, R1_2, Curve::steps ({ 1, 0, 1, 0, 1, 1, 0, 1 })) }));
        p.push_back (mkp ("Rhythmus", "Trap-Stotter", { mk (Volume, R1Bar, Curve::steps ({ 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0 })) }));
        p.push_back (mkp ("Rhythmus", "Triolen-Flattern", { mk (Volume, R1_2, Curve::steps ({ 1, 0.2f, 1, 0.2f, 1, 0.2f })) }));
        p.push_back (mkp ("Rhythmus", "Halbzeit-Pump", { mk (Volume, R1_2, Curve::pump(), 1.f, 1.f, 0.5f, 0.5f, 0.5f, 0.1f) }));

        p.push_back (mkp ("Filter", "Wah-Wah Federn", { mk (Filter, R1_4, Curve::sine(), 0.8f, 1.f, 0.35f, 0.6f, 0.f, 0.1f) }));
        p.push_back (mkp ("Filter", "Taubenflug-Sweep", { mk (Filter, R2Bars, Curve::sawUp(), 1.f, 1.f, 0.15f, 0.55f, 0.f) }));
        p.push_back (mkp ("Filter", "Hochpass-Riser", { mk (Filter, R4Bars, Curve::sawUp(), 1.f, 1.f, 0.1f, 0.4f, 1.f) }));
        p.push_back (mkp ("Filter", "Formant-Gurren", { mk (Filter, R1_8, Curve::triangle(), 1.f, 1.f, 0.5f, 0.85f, 0.5f, 0.05f) }));

        p.push_back (mkp ("Verzerrung", "Kropf-Puls", { mk (Drive, R1_4, Curve::sawDown(), 1.f, 1.f, 0.7f) }));
        p.push_back (mkp ("Verzerrung", "Schmutzige Taube", { mk (Drive, R1_8, Curve::square(), 1.f, 1.f, 0.9f) }));
        p.push_back (mkp ("Verzerrung", "Krümel-Schauer", { mk (Crush, R1_4, Curve::randomSteps (16, 7), 1.f, 1.f, 0.8f, 0.4f) }));
        p.push_back (mkp ("Verzerrung", "Lo-Fi Nest", { mk (Crush, R1Bar, Curve::sawDown(), 1.f, 0.7f, 0.5f, 0.3f) }));

        p.push_back (mkp ("Stereo", "Stereo Flatter", { mk (Pan, R1_4, Curve::sine()) }));
        p.push_back (mkp ("Stereo", "Autopan Schwarm", { mk (Pan, R1_8, Curve::triangle(), 0.7f) }));
        p.push_back (mkp ("Stereo", "Breit-Atem", { mk (Width, R1Bar, Curve::sine()) }));

        p.push_back (mkp ("Granular", "Schwarm Wolke", { mk (Granular, R1Bar, Curve::flat(), 0.f, 0.9f, 0.45f, 0.7f, 0.5f) }));
        p.push_back (mkp ("Granular", "Oktave Aufwärts", { mk (Granular, R1Bar, Curve::flat(), 0.f, 0.6f, 0.3f, 0.6f, 1.f) }));
        p.push_back (mkp ("Granular", "Zerrissene Flügel", { mk (Granular, R1_4, Curve::steps ({ 1, 0, 1, 1 }), 1.f, 1.f, 0.1f, 0.9f, 0.5f) }));
        p.push_back (mkp ("Granular", "Eingefroren", { mk (Granular, R2Bars, Curve::flat(), 0.f, 1.f, 0.9f, 0.2f, 0.5f) }));

        p.push_back (mkp ("Kombi", "Taubenschlag-Kombi",
                          { mk (Volume, R1_4, Curve::pump()),
                            mk (Filter, R1_8, Curve::sine(), 0.6f, 1.f, 0.5f, 0.4f, 0.f, 0.1f),
                            mk (Drive, R1_4, Curve::sawDown(), 0.5f, 1.f, 0.3f) }));
        p.push_back (mkp ("Kombi", "Dach der Stadt",
                          { mk (Width, R2Bars, Curve::sine()),
                            mk (Granular, R1Bar, Curve::flat(), 0.f, 0.5f, 0.35f, 0.6f, 0.5f),
                            mk (Volume, R1_2, Curve::steps ({ 1, 1, 0.3f, 1 }), 0.8f) }));
        p.push_back (mkp ("Kombi", "Rave-Taube",
                          { mk (Volume, R1_2, Curve::steps ({ 1, 0, 1, 0, 1, 1, 0, 1 })),
                            mk (Filter, R4Bars, Curve::sawUp(), 1.f, 1.f, 0.1f, 0.4f, 1.f),
                            mk (Crush, R1_8, Curve::square(), 0.5f, 0.6f, 0.4f, 0.3f) }));
        p.push_back (mkp ("Kombi", "Nachtschwarm",
                          { mk (Filter, R2Bars, Curve::sine(), 0.7f, 1.f, 0.4f, 0.3f, 0.f),
                            mk (Granular, R1Bar, Curve::flat(), 0.f, 0.6f, 0.5f, 0.6f, 0.5f),
                            mk (Pan, R1_4, Curve::sine(), 0.5f) }));
        return p;
    }();
    return presets;
}
} // namespace tb
