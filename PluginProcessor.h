#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "Dsp.h"

class TaubenBoxEditor;

class TaubenBoxProcessor : public juce::AudioProcessor
{
public:
    TaubenBoxProcessor();
    ~TaubenBoxProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Tauben-Box"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Kurven (Message-Thread schreibt, Audio-Thread liest eine Tabelle)
    Curve getCurve (int layer) const;
    void setCurve (int layer, const Curve& c, bool notifyEditor);
    juce::ChangeBroadcaster curveBroadcaster;

    // Presets
    void applyPreset (int index);

    // Anzeige im Editor
    std::array<std::atomic<float>, tb::numLayers> layerPhase;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const juce::String& id, float plainValue);
    void loadCurvesFromState();

    struct RawLayer
    {
        std::atomic<float> *type, *on, *retrig, *rate, *depth, *mix, *smooth, *phase, *a, *b, *c;
    };
    std::array<RawLayer, tb::numLayers> raw {};
    std::atomic<float>* outGainRaw = nullptr;
    std::atomic<float>* mixRaw = nullptr;

    std::array<tb::LayerDsp, tb::numLayers> layers;

    mutable juce::SpinLock curveLock;
    std::array<Curve, tb::numLayers> curves;
    std::array<std::array<float, (size_t) Curve::tableSize + 1>, tb::numLayers> tables;
    std::array<std::atomic<int>, tb::numLayers> curveVersion;
    std::array<int, tb::numLayers> localVersion { -1, -1, -1, -1 };

    double freePpq = 0.0;
    std::array<double, tb::numLayers> anchor { 0.0, 0.0, 0.0, 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TaubenBoxProcessor)
};
