#include <functional>
#include "PluginEditor.h"
#include "Presets.h"

namespace
{
const juce::Colour layerColours[tb::numLayers] = {
    juce::Colour (0xff52e3c2), juce::Colour (0xffb26bff), juce::Colour (0xffffb347), juce::Colour (0xff5cb8ff)
};
const int gridValues[] = { 4, 8, 16, 32 };
} // namespace

TaubenBoxEditor::TaubenBoxEditor (TaubenBoxProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), curveEditor (p)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (pigeons);
    addAndMakeVisible (curveEditor);

    for (int k = 0; k < tb::numLayers; ++k)
    {
        auto& t = tabs[(size_t) k];
        t.setClickingTogglesState (true);
        t.setRadioGroupId (101);
        t.setColour (juce::TextButton::buttonOnColourId, layerColours[k]);
        t.onClick = [this, k] { selectLayer (k); };
        addAndMakeVisible (t);
    }

    typeBox.addItemList (tb::typeNames(), 1);
    rateBox.addItemList (tb::rateNames(), 1);
    addAndMakeVisible (typeBox);
    addAndMakeVisible (rateBox);
    addAndMakeVisible (onBtn);
    addAndMakeVisible (retrigBtn);

    setupKnob (depthK, tb::u8 ("Tiefe"));
    setupKnob (mixK, "Mix");
    setupKnob (smoothK, tb::u8 ("Glättung"));
    setupKnob (phaseK, "Phase");
    setupKnob (aK, "A");
    setupKnob (bK, "B");
    setupKnob (cK, "C");
    setupKnob (outK, "Ausgang");
    setupKnob (gmixK, "Dry/Wet");
    outK.slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffffb347));
    gmixK.slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffffb347));

    // Kurvenformen
    struct Shape { const char* name; std::function<Curve()> make; };
    const std::vector<Shape> shapes = {
        { "Säge ab", [] { return Curve::sawDown(); } },
        { "Säge auf", [] { return Curve::sawUp(); } },
        { "Sinus", [] { return Curve::sine(); } },
        { "Dreieck", [] { return Curve::triangle(); } },
        { "Rechteck", [] { return Curve::square(); } },
        { "Zufall", [] { return Curve::randomSteps (16, juce::Random::getSystemRandom().nextInt()); } },
        { "Pump", [] { return Curve::pump(); } },
        { "Flach", [] { return Curve::flat(); } }
    };
    for (const auto& s : shapes)
    {
        auto b = std::make_unique<juce::TextButton> (tb::u8 (s.name));
        auto make = s.make;
        b->onClick = [this, make] { curveEditor.applyCurve (make()); };
        addAndMakeVisible (*b);
        shapeButtons.push_back (std::move (b));
    }

    snapBtn.setToggleState (true, juce::dontSendNotification);
    snapBtn.onClick = [this] { curveEditor.setSnap (snapBtn.getToggleState()); };
    addAndMakeVisible (snapBtn);

    gridBox.addItem ("4 Schritte", 1);
    gridBox.addItem ("8 Schritte", 2);
    gridBox.addItem ("16 Schritte", 3);
    gridBox.addItem ("32 Schritte", 4);
    gridBox.setSelectedId (3, juce::dontSendNotification);
    gridBox.onChange = [this]
    {
        const int id = gridBox.getSelectedId();
        if (id >= 1 && id <= 4) curveEditor.setGridDiv (gridValues[id - 1]);
    };
    addAndMakeVisible (gridBox);

    // Presets
    {
        const auto& presets = tb::getPresets();
        juce::String lastCat;
        for (size_t i = 0; i < presets.size(); ++i)
        {
            if (presets[i].category != lastCat)
            {
                lastCat = presets[i].category;
                if (i > 0) presetBox.addSeparator();
                presetBox.addSectionHeading (lastCat);
            }
            presetBox.addItem (presets[i].name, (int) i + 1);
        }
        presetBox.setTextWhenNothingSelected (tb::u8 ("Taubenschlag – Preset wählen"));
        presetBox.onChange = [this]
        {
            const int id = presetBox.getSelectedId();
            if (id > 0) proc.applyPreset (id - 1);
        };
    }
    addAndMakeVisible (presetBox);
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (1); };
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    // Globale Regler
    outA = std::make_unique<SA> (proc.apvts, "out_gain", outK.slider);
    gmixA = std::make_unique<SA> (proc.apvts, "mix", gmixK.slider);

    proc.curveBroadcaster.addChangeListener (this);

    setSize (1000, 660);
    selectLayer (0);
    startTimerHz (10);
}

TaubenBoxEditor::~TaubenBoxEditor()
{
    proc.curveBroadcaster.removeChangeListener (this);
    stopTimer();
    setLookAndFeel (nullptr);
}

void TaubenBoxEditor::setupKnob (Knob& k, const juce::String& text)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    k.slider.setColour (juce::Slider::rotarySliderFillColourId, layerColours[0]);
    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.label);
}

void TaubenBoxEditor::placeKnob (Knob& k, juce::Rectangle<int> area)
{
    k.label.setBounds (area.removeFromTop (16));
    k.slider.setBounds (area);
}

void TaubenBoxEditor::selectLayer (int k)
{
    selected = juce::jlimit (0, tb::numLayers - 1, k);
    const juce::String pre = "l" + juce::String (selected) + "_";
    auto& st = proc.apvts;

    typeA.reset(); rateA.reset(); onA.reset(); retrigA.reset();
    depthA.reset(); mixA.reset(); smoothA.reset(); phaseA.reset(); aA.reset(); bA.reset(); cA.reset();

    typeA = std::make_unique<CA> (st, pre + "type", typeBox);
    rateA = std::make_unique<CA> (st, pre + "rate", rateBox);
    onA = std::make_unique<BA> (st, pre + "on", onBtn);
    retrigA = std::make_unique<BA> (st, pre + "retrig", retrigBtn);
    depthA = std::make_unique<SA> (st, pre + "depth", depthK.slider);
    mixA = std::make_unique<SA> (st, pre + "mix", mixK.slider);
    smoothA = std::make_unique<SA> (st, pre + "smooth", smoothK.slider);
    phaseA = std::make_unique<SA> (st, pre + "phase", phaseK.slider);
    aA = std::make_unique<SA> (st, pre + "a", aK.slider);
    bA = std::make_unique<SA> (st, pre + "b", bK.slider);
    cA = std::make_unique<SA> (st, pre + "c", cK.slider);

    const auto col = layerColours[selected];
    for (auto* kn : { &depthK, &mixK, &smoothK, &phaseK, &aK, &bK, &cK })
        kn->slider.setColour (juce::Slider::rotarySliderFillColourId, col);

    curveEditor.setLayer (selected, col);
    for (int i = 0; i < tb::numLayers; ++i)
        tabs[(size_t) i].setToggleState (i == selected, juce::dontSendNotification);

    shownType = -1;
    refreshDynamic();
    repaint();
}

void TaubenBoxEditor::refreshDynamic()
{
    for (int k = 0; k < tb::numLayers; ++k)
    {
        const juce::String pre = "l" + juce::String (k) + "_";
        const int type = (int) proc.apvts.getRawParameterValue (pre + "type")->load();
        const bool on = proc.apvts.getRawParameterValue (pre + "on")->load() > 0.5f;
        tabs[(size_t) k].setButtonText (juce::String (k + 1) + "  " + tb::u8 (on ? "● " : "○ ") + tb::shortTypeName (type));
    }

    const int t = (int) proc.apvts.getRawParameterValue ("l" + juce::String (selected) + "_type")->load();
    if (t != shownType)
    {
        shownType = t;
        updateKnobLabels();
    }
}

void TaubenBoxEditor::updateKnobLabels()
{
    juce::String a, b, c;
    switch (shownType)
    {
        case tb::Filter:   a = "Cutoff"; b = "Resonanz"; c = "Modus LP-BP-HP"; break;
        case tb::Drive:    a = "Drive"; break;
        case tb::Crush:    a = "Tiefe"; b = "Downsample"; break;
        case tb::Granular: a = tb::u8 ("Korngröße"); b = "Dichte"; c = tb::u8 ("Tonhöhe"); break;
        default: break;
    }
    auto apply = [] (Knob& k, const juce::String& text)
    {
        k.label.setText (text, juce::dontSendNotification);
        k.label.setVisible (text.isNotEmpty());
        k.slider.setVisible (text.isNotEmpty());
    };
    apply (aK, a);
    apply (bK, b);
    apply (cK, c);
}

void TaubenBoxEditor::stepPreset (int delta)
{
    const int count = (int) tb::getPresets().size();
    int id = presetBox.getSelectedId();
    id = id <= 0 ? (delta > 0 ? 1 : count) : id + delta;
    if (id < 1) id = count;
    if (id > count) id = 1;
    presetBox.setSelectedId (id, juce::sendNotificationSync);
}

void TaubenBoxEditor::timerCallback() { refreshDynamic(); }

void TaubenBoxEditor::changeListenerCallback (juce::ChangeBroadcaster*) { curveEditor.reload(); }

void TaubenBoxEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0d1020), 0.f, 0.f, juce::Colour (0xff222c52), 0.f, h, false));
    g.fillAll();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0x3352e3c2), w * 0.2f, h, juce::Colour (0x0052e3c2), w * 0.2f, h * 0.45f, true));
    g.fillRect (getLocalBounds());
    g.setGradientFill (juce::ColourGradient (juce::Colour (0x28b26bff), w * 0.9f, h * 0.1f, juce::Colour (0x00b26bff), w * 0.9f, h * 0.5f, true));
    g.fillRect (getLocalBounds());

    // Kopfzeile
    g.setColour (juce::Colour (0x55000000));
    g.fillRect (0, 0, getWidth(), 64);
    tb::drawPigeon (g, 52.f, 40.f, 0.85f, 1.f, 0.35f, 1.f);

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (30.f, juce::Font::bold)));
    g.drawText ("TAUBEN-BOX", 98, 8, 320, 34, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff52e3c2));
    g.setFont (13.f);
    g.drawText ("by taubenopa", 100, 38, 300, 18, juce::Justification::centredLeft);

    // rechtes Panel
    g.setColour (juce::Colour (0x88101528));
    g.fillRoundedRectangle (rightPanel.toFloat(), 10.f);
    g.setColour (juce::Colour (0x33ffffff));
    g.drawRoundedRectangle (rightPanel.toFloat().reduced (0.5f), 10.f, 1.f);

    g.setColour (juce::Colour (0x99d5dcf2));
    g.setFont (12.f);
    g.drawText (tb::u8 ("Klick: Punkt setzen · Ziehen: verschieben · Rechtsklick / Doppelklick: löschen · Mausrad: Kurvenform"),
                16, getHeight() - 26, getWidth() - 32, 18, juce::Justification::centredLeft);
}

void TaubenBoxEditor::resized()
{
    auto r = getLocalBounds();
    pigeons.setBounds (r);

    auto header = r.removeFromTop (64);
    auto pb = header.removeFromRight (440).reduced (8, 18);
    prevBtn.setBounds (pb.removeFromLeft (34));
    nextBtn.setBounds (pb.removeFromRight (34));
    presetBox.setBounds (pb.reduced (6, 0));

    r.reduce (16, 6);
    auto tabRow = r.removeFromTop (38);
    const int tw = (tabRow.getWidth() - 3 * 8) / 4;
    for (int k = 0; k < tb::numLayers; ++k)
    {
        tabs[(size_t) k].setBounds (tabRow.removeFromLeft (tw));
        tabRow.removeFromLeft (8);
    }
    r.removeFromTop (8);

    auto left = r.removeFromLeft (650);
    r.removeFromLeft (16);
    rightPanel = r.removeFromTop (440);

    curveEditor.setBounds (left.removeFromTop (360));
    left.removeFromTop (8);
    auto shapes = left.removeFromTop (30);
    const int sw = shapes.getWidth() / juce::jmax (1, (int) shapeButtons.size());
    for (auto& b : shapeButtons) b->setBounds (shapes.removeFromLeft (sw).reduced (2, 0));
    left.removeFromTop (8);
    auto gridRow = left.removeFromTop (28);
    snapBtn.setBounds (gridRow.removeFromLeft (110));
    gridBox.setBounds (gridRow.removeFromLeft (150));

    auto rp = rightPanel.reduced (10, 10);
    typeBox.setBounds (rp.removeFromTop (28));
    rp.removeFromTop (6);
    rateBox.setBounds (rp.removeFromTop (28));
    rp.removeFromTop (6);
    auto tog = rp.removeFromTop (26);
    onBtn.setBounds (tog.removeFromLeft (70));
    retrigBtn.setBounds (tog);
    rp.removeFromTop (8);

    auto row1 = rp.removeFromTop (96);
    const int w4 = row1.getWidth() / 4;
    placeKnob (depthK, row1.removeFromLeft (w4));
    placeKnob (mixK, row1.removeFromLeft (w4));
    placeKnob (smoothK, row1.removeFromLeft (w4));
    placeKnob (phaseK, row1);
    rp.removeFromTop (6);

    auto row2 = rp.removeFromTop (96);
    const int w3 = row2.getWidth() / 3;
    placeKnob (aK, row2.removeFromLeft (w3));
    placeKnob (bK, row2.removeFromLeft (w3));
    placeKnob (cK, row2);
    rp.removeFromTop (6);

    auto row3 = rp.removeFromTop (96);
    placeKnob (outK, row3.removeFromLeft (row3.getWidth() / 2));
    placeKnob (gmixK, row3);
}
