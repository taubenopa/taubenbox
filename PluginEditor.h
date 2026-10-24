#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>
#include <vector>
#include "PluginProcessor.h"
#include "Pigeon.h"
#include "CurveEditor.h"
#include "TBLookAndFeel.h"

class TaubenBoxEditor : public juce::AudioProcessorEditor,
                        private juce::Timer,
                        private juce::ChangeListener
{
public:
    explicit TaubenBoxEditor (TaubenBoxProcessor&);
    ~TaubenBoxEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
    };

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void setupKnob (Knob& k, const juce::String& text);
    void placeKnob (Knob& k, juce::Rectangle<int> area);
    void selectLayer (int k);
    void refreshDynamic();
    void updateKnobLabels();
    void stepPreset (int delta);

    TaubenBoxProcessor& proc;
    TBLookAndFeel laf;
    tb::PigeonField pigeons;
    CurveEditor curveEditor;

    int selected = 0;
    int shownType = -1;
    juce::Rectangle<int> rightPanel;

    std::array<juce::TextButton, tb::numLayers> tabs;
    juce::ComboBox typeBox, rateBox, presetBox, gridBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::ToggleButton onBtn { "An" }, retrigBtn { "MIDI-Retrigger" }, snapBtn { "Raster" };
    std::vector<std::unique_ptr<juce::TextButton>> shapeButtons;

    Knob depthK, mixK, smoothK, phaseK, aK, bK, cK, outK, gmixK;

    // Attachments zuletzt, damit sie vor den Komponenten zerstört werden
    std::unique_ptr<CA> typeA, rateA;
    std::unique_ptr<BA> onA, retrigA;
    std::unique_ptr<SA> depthA, mixA, smoothA, phaseA, aA, bA, cA, outA, gmixA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TaubenBoxEditor)
};
