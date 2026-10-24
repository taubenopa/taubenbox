#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using namespace juce;

TaubenBoxProcessor::TaubenBoxProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (auto& a : layerPhase) a.store (0.f);
    for (auto& v : curveVersion) v.store (0);
    for (auto& t : tables) t.fill (1.f);

    for (int k = 0; k < tb::numLayers; ++k)
    {
        const String pre = "l" + String (k) + "_";
        auto get = [&] (const char* n) { return apvts.getRawParameterValue (pre + n); };
        raw[(size_t) k] = { get ("type"), get ("on"), get ("retrig"), get ("rate"), get ("depth"),
                            get ("mix"), get ("smooth"), get ("phase"), get ("a"), get ("b"), get ("c") };
    }
    outGainRaw = apvts.getRawParameterValue ("out_gain");
    mixRaw = apvts.getRawParameterValue ("mix");

    setCurve (0, Curve::pump(), false);
    setCurve (1, Curve::sine(), false);
    setCurve (2, Curve::sawDown(), false);
    setCurve (3, Curve::flat(), false);
}

AudioProcessorValueTreeState::ParameterLayout TaubenBoxProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = []
    {
        return AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.f)) + " %"; })
            .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue() / 100.f; });
    };

    const StringArray types = tb::typeNames();
    const int defType[] = { 0, 1, 2, 6 };

    for (int k = 0; k < tb::numLayers; ++k)
    {
        const String pre = "l" + String (k) + "_";
        const String nm = "Layer " + String (k + 1) + " ";

        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { pre + "type", 1 }, nm + "Typ", types, defType[k]));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { pre + "on", 1 }, nm + "An", k == 0));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { pre + "retrig", 1 }, nm + "Retrigger", false));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { pre + "rate", 1 }, nm + "Rate", tb::rateNames(), 4));

        auto addPct = [&] (const String& id, const String& n, float def)
        {
            layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pre + id, 1 }, nm + n,
                                                               NormalisableRange<float> (0.f, 1.f, 0.001f), def, pct()));
        };
        addPct ("depth", tb::u8 ("Tiefe"), 1.f);
        addPct ("mix", "Mix", 1.f);
        addPct ("smooth", tb::u8 ("Glättung"), 0.f);
        addPct ("phase", "Phase", 0.f);
        addPct ("a", "Regler A", 0.5f);
        addPct ("b", "Regler B", 0.5f);
        addPct ("c", "Regler C", 0.5f);
    }

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "out_gain", 1 }, "Ausgang", NormalisableRange<float> (-24.f, 12.f, 0.1f), 0.f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })
                                       .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue(); })));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Dry/Wet", NormalisableRange<float> (0.f, 1.f, 0.001f), 1.f, pct()));

    return layout;
}

void TaubenBoxProcessor::prepareToPlay (double sampleRate, int)
{
    for (auto& l : layers) l.prepare (sampleRate);
    for (auto& v : localVersion) v = -1;
}

bool TaubenBoxProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == AudioChannelSet::stereo();
}

void TaubenBoxProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);
    if (buffer.getNumChannels() < 2) return;

    // Kurventabellen übernehmen, falls geändert
    for (int k = 0; k < tb::numLayers; ++k)
    {
        const int ver = curveVersion[(size_t) k].load();
        if (ver != localVersion[(size_t) k])
        {
            SpinLock::ScopedTryLockType lock (curveLock);
            if (lock.isLocked())
            {
                layers[(size_t) k].table = tables[(size_t) k];
                localVersion[(size_t) k] = ver;
            }
        }
    }

    // Host-Zeit
    double bpm = 120.0, hostPpq = 0.0;
    bool playing = false, hasPpq = false;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) { hostPpq = *p; hasPpq = true; }
        }
    }
    const double bps = bpm / 60.0 / getSampleRate();
    const double ppq0 = (playing && hasPpq) ? hostPpq : freePpq;

    // erstes Note-On im Block
    int noteOnPos = -1;
    for (const auto meta : midi)
    {
        if (meta.getMessage().isNoteOn()) { noteOnPos = meta.samplePosition; break; }
    }

    // Parameter lesen
    std::array<tb::LayerParams, tb::numLayers> lp;
    for (int k = 0; k < tb::numLayers; ++k)
    {
        const auto& r = raw[(size_t) k];
        auto& p = lp[(size_t) k];
        p.type = (int) r.type->load();
        p.on = r.on->load() > 0.5f;
        p.retrig = r.retrig->load() > 0.5f;
        p.rate = (int) r.rate->load();
        p.depth = r.depth->load();
        p.mix = r.mix->load();
        p.smooth = r.smooth->load();
        p.phase = r.phase->load();
        p.a = r.a->load();
        p.b = r.b->load();
        p.c = r.c->load();
        layers[(size_t) k].setSmoothing (p.smooth);
    }
    const float gain = Decibels::decibelsToGain (outGainRaw->load());
    const float wetMix = mixRaw->load();

    float* chL = buffer.getWritePointer (0);
    float* chR = buffer.getWritePointer (1);
    std::array<double, tb::numLayers> lastPhase { 0.0, 0.0, 0.0, 0.0 };

    for (int i = 0; i < n; ++i)
    {
        const double ppq = ppq0 + (double) i * bps;
        if (i == noteOnPos)
            for (int k = 0; k < tb::numLayers; ++k)
                if (lp[(size_t) k].retrig) anchor[(size_t) k] = ppq;

        const float dryL = chL[i], dryR = chR[i];
        float l = dryL, r = dryR;

        for (int k = 0; k < tb::numLayers; ++k)
        {
            const auto& p = lp[(size_t) k];
            if (! p.on) continue;

            double ph = (ppq - (p.retrig ? anchor[(size_t) k] : 0.0)) / tb::rateBeats (p.rate) + (double) p.phase;
            ph -= std::floor (ph);
            lastPhase[(size_t) k] = ph;
            layers[(size_t) k].process (l, r, (float) ph, p);
        }

        chL[i] = (dryL + (l - dryL) * wetMix) * gain;
        chR[i] = (dryR + (r - dryR) * wetMix) * gain;
    }

    freePpq = ppq0 + (double) n * bps;
    for (int k = 0; k < tb::numLayers; ++k)
        layerPhase[(size_t) k].store ((float) lastPhase[(size_t) k]);
}

Curve TaubenBoxProcessor::getCurve (int layer) const
{
    const SpinLock::ScopedLockType sl (curveLock);
    return curves[(size_t) layer];
}

void TaubenBoxProcessor::setCurve (int layer, const Curve& c, bool notifyEditor)
{
    Curve copy = c;
    copy.sanitize();

    std::array<float, (size_t) Curve::tableSize + 1> tmp;
    copy.buildTable (tmp.data());

    {
        const SpinLock::ScopedLockType sl (curveLock);
        curves[(size_t) layer] = copy;
        tables[(size_t) layer] = tmp;
        curveVersion[(size_t) layer].fetch_add (1);
    }

    apvts.state.setProperty ("curve" + String (layer), copy.toString(), nullptr);
    if (notifyEditor) curveBroadcaster.sendChangeMessage();
}

void TaubenBoxProcessor::loadCurvesFromState()
{
    for (int k = 0; k < tb::numLayers; ++k)
    {
        Curve c;
        const String s = apvts.state.getProperty ("curve" + String (k)).toString();
        if (s.isEmpty() || ! c.fromString (s)) c.reset();
        setCurve (k, c, false);
    }
    curveBroadcaster.sendChangeMessage();
}

void TaubenBoxProcessor::setParam (const String& id, float plainValue)
{
    if (auto* prm = apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
}

void TaubenBoxProcessor::applyPreset (int index)
{
    const auto& presets = tb::getPresets();
    if (index < 0 || index >= (int) presets.size()) return;
    const auto& pr = presets[(size_t) index];

    for (int k = 0; k < tb::numLayers; ++k)
    {
        const auto& L = pr.layers[(size_t) k];
        const String pre = "l" + String (k) + "_";
        setParam (pre + "type", (float) L.type);
        setParam (pre + "on", L.on ? 1.f : 0.f);
        setParam (pre + "retrig", L.retrig ? 1.f : 0.f);
        setParam (pre + "rate", (float) L.rate);
        setParam (pre + "depth", L.depth);
        setParam (pre + "mix", L.mix);
        setParam (pre + "smooth", L.smooth);
        setParam (pre + "phase", L.phase);
        setParam (pre + "a", L.a);
        setParam (pre + "b", L.b);
        setParam (pre + "c", L.c);
        setCurve (k, L.curve, false);
    }
    setParam ("out_gain", pr.outGain);
    setParam ("mix", pr.mix);
    curveBroadcaster.sendChangeMessage();
}

void TaubenBoxProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void TaubenBoxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (ValueTree::fromXml (*xml));
            loadCurvesFromState();
        }
}

AudioProcessorEditor* TaubenBoxProcessor::createEditor() { return new TaubenBoxEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TaubenBoxProcessor(); }
