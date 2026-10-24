#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include "Dsp.h"

namespace tb
{
struct LayerPreset
{
    int type = 0;
    bool on = false;
    bool retrig = false;
    int rate = 4;
    float depth = 1.f, mix = 1.f, smooth = 0.f, phase = 0.f;
    float a = 0.5f, b = 0.5f, c = 0.5f;
    Curve curve;
};

struct Preset
{
    juce::String category, name;
    std::array<LayerPreset, numLayers> layers;
    float outGain = 0.f;
    float mix = 1.f;
};

const std::vector<Preset>& getPresets();
} // namespace tb
