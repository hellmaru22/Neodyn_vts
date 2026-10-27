#include "PluginProcessor.h"
#include "PluginEditor.h"

// ------------------------------------------------------------------ presets
namespace
{
    struct Preset
    {
        const char* name;
        std::vector<std::pair<const char*, float>> values;
    };

    // Posiciones de wavetable: seno 0, triangulo .143, saw .286, saw brillante .429,
    // cuadrada .571, pulso .714, formante .857, digital 1.0
    const std::vector<Preset>& presets()
    {
        static const std::vector<Preset> p = {
            { "Init", {} },
            { "Supersaw Lead (EDM)", {
                { "osc1_wt", 0.286f }, { "osc2_wt", 0.286f }, { "osc2_fine", 9.0f }, { "osc2_lvl", 0.8f },
                { "uni_voices", 7.0f }, { "uni_detune", 0.55f }, { "uni_spread", 0.9f },
                { "f_cut", 7000.0f }, { "f_res", 0.15f },
                { "a_att", 0.01f }, { "a_sus", 1.0f }, { "a_rel", 0.35f },
                { "fx_chorus", 0.3f }, { "fx_dly_mix", 0.2f }, { "fx_rev_mix", 0.25f } } },
            { "Pluck Chord (Trance)", {
                { "osc1_wt", 0.286f }, { "osc2_wt", 0.571f }, { "osc2_lvl", 0.6f },
                { "uni_voices", 5.0f }, { "uni_detune", 0.35f }, { "uni_spread", 0.8f },
                { "f_cut", 1800.0f }, { "f_res", 0.2f }, { "f_env", 0.6f },
                { "a_dec", 0.35f }, { "a_sus", 0.0f }, { "a_rel", 0.2f },
                { "e_dec", 0.25f }, { "e_sus", 0.0f },
                { "fx_dly_mix", 0.3f }, { "fx_rev_mix", 0.25f } } },
            { "Folk Guitar (Avicii)", {
                { "syn_lvl", 0.0f }, { "gtr_lvl", 0.9f }, { "gtr_decay", 3.0f }, { "gtr_bright", 0.55f },
                { "gtr_strum", 22.0f }, { "fx_chorus", 0.15f }, { "fx_rev_mix", 0.2f } } },
            { "Avicii Layer (Guitar + Saw)", {
                { "syn_lvl", 0.5f }, { "gtr_lvl", 0.8f }, { "gtr_decay", 3.0f }, { "gtr_strum", 18.0f },
                { "osc1_wt", 0.286f }, { "osc2_lvl", 0.0f },
                { "uni_voices", 5.0f }, { "uni_detune", 0.3f }, { "f_cut", 3500.0f },
                { "a_att", 0.01f }, { "a_sus", 0.8f }, { "fx_rev_mix", 0.3f }, { "fx_dly_mix", 0.2f } } },
            { "Wobble Bass", {
                { "osc1_wt", 0.571f }, { "osc1_oct", -1.0f }, { "osc2_wt", 0.286f }, { "osc2_oct", -1.0f },
                { "osc2_lvl", 0.6f }, { "uni_voices", 3.0f }, { "uni_detune", 0.2f },
                { "f_cut", 500.0f }, { "f_res", 0.5f }, { "lfo_rate", 3.0f }, { "lfo_cut", 0.8f },
                { "a_sus", 1.0f }, { "fx_rev_mix", 0.0f } } },
            { "Evolving Pad", {
                { "osc1_wt", 0.7f }, { "osc2_wt", 0.4f }, { "osc2_fine", 6.0f }, { "osc2_lvl", 0.7f },
                { "uni_voices", 6.0f }, { "uni_detune", 0.5f }, { "uni_spread", 1.0f },
                { "lfo_rate", 0.3f }, { "lfo_wt", 0.8f }, { "f_cut", 4500.0f },
                { "a_att", 0.8f }, { "a_sus", 1.0f }, { "a_rel", 1.8f },
                { "fx_chorus", 0.4f }, { "fx_rev_mix", 0.5f }, { "fx_rev_size", 0.8f } } },
        };
        return p;
    }
}

// ------------------------------------------------------------------ parametros
juce::AudioProcessorValueTreeState::ParameterLayout NeoDynProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> v;

    auto F = [&](const char* id, const char* name, float lo, float hi, float def, float skew = 1.0f)
    {
        v.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, name, NormalisableRange<float>(lo, hi, 0.0f, skew), def));
    };
    auto I = [&](const char* id, const char* name, int lo, int hi, int def)
    {
        v.push_back(std::make_unique<AudioParameterInt>(ParameterID { id, 1 }, name, lo, hi, def));
    };

    F("osc1_wt", "Osc1 Wave", 0.0f, 1.0f, 0.286f);
    I("osc1_oct", "Osc1 Oct", -2, 2, 0);
    F("osc1_lvl", "Osc1 Level", 0.0f, 1.0f, 0.8f);

    F("osc2_wt", "Osc2 Wave", 0.0f, 1.0f, 0.286f);
    I("osc2_oct", "Osc2 Oct", -2, 2, 0);
    F("osc2_fine", "Osc2 Fine", -100.0f, 100.0f, 7.0f);
    F("osc2_lvl", "Osc2 Level", 0.0f, 1.0f, 0.5f);

    I("uni_voices", "Unison Voices", 1, 7, 3);
    F("uni_detune", "Unison Detune", 0.0f, 1.0f, 0.3f);
    F("uni_spread", "Unison Spread", 0.0f, 1.0f, 0.7f);

    v.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { "f_type", 1 }, "Filter Type", StringArray { "Low-pass", "Band-pass", "High-pass" }, 0));
    F("f_cut", "Cutoff", 20.0f, 20000.0f, 12000.0f, 0.3f);
    F("f_res", "Resonance", 0.0f, 1.0f, 0.1f);
    F("f_env", "Filter Env", -1.0f, 1.0f, 0.0f);

    F("a_att", "Amp A", 0.001f, 5.0f, 0.005f, 0.4f);
    F("a_dec", "Amp D", 0.005f, 5.0f, 0.3f, 0.4f);
    F("a_sus", "Amp S", 0.0f, 1.0f, 0.8f);
    F("a_rel", "Amp R", 0.005f, 8.0f, 0.25f, 0.4f);

    F("e_att", "Flt A", 0.001f, 5.0f, 0.005f, 0.4f);
    F("e_dec", "Flt D", 0.005f, 5.0f, 0.4f, 0.4f);
    F("e_sus", "Flt S", 0.0f, 1.0f, 0.0f);
    F("e_rel", "Flt R", 0.005f, 8.0f, 0.3f, 0.4f);

    F("lfo_rate", "LFO Rate", 0.05f, 20.0f, 2.0f, 0.4f);
    F("lfo_wt", "LFO > Wave", 0.0f, 1.0f, 0.0f);
    F("lfo_cut", "LFO > Cutoff", 0.0f, 1.0f, 0.0f);

    F("gtr_lvl", "Guitar Level", 0.0f, 1.0f, 0.0f);
    F("gtr_decay", "Guitar Decay", 0.2f, 8.0f, 2.5f, 0.5f);
    F("gtr_bright", "Guitar Bright", 0.0f, 1.0f, 0.5f);
    F("gtr_strum", "Strum (ms)", 0.0f, 80.0f, 0.0f);

    F("fx_chorus", "Chorus", 0.0f, 1.0f, 0.0f);
    F("fx_dly_mix", "Delay Mix", 0.0f, 1.0f, 0.0f);
    F("fx_dly_time", "Delay Time", 0.05f, 1.5f, 0.375f, 0.5f);
    F("fx_dly_fb", "Delay Fdbk", 0.0f, 0.9f, 0.35f);
    F("fx_rev_mix", "Reverb Mix", 0.0f, 1.0f, 0.15f);
    F("fx_rev_size", "Reverb Size", 0.0f, 1.0f, 0.5f);

    F("syn_lvl", "Synth Level", 0.0f, 1.0f, 0.8f);
    F("master", "Master", 0.0f, 1.0f, 0.7f);

    return { v.begin(), v.end() };
}

// ------------------------------------------------------------------ ciclo de vida
NeoDynProcessor::NeoDynProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
    synth.addSound(new FolkSound());
    for (int i = 0; i < 16; ++i)
        synth.addVoice(new FolkVoice(apvts));

    strumP = apvts.getRawParameterValue("gtr_strum");
    gtrLvlP = apvts.getRawParameterValue("gtr_lvl");
    chorusP = apvts.getRawParameterValue("fx_chorus");
    dlyMixP = apvts.getRawParameterValue("fx_dly_mix");
    dlyTimeP = apvts.getRawParameterValue("fx_dly_time");
    dlyFbP = apvts.getRawParameterValue("fx_dly_fb");
    revMixP = apvts.getRawParameterValue("fx_rev_mix");
    revSizeP = apvts.getRawParameterValue("fx_rev_size");
    masterP = apvts.getRawParameterValue("master");

    pending.reserve(64);
    scratch.reserve(64);
}

bool NeoDynProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void NeoDynProcessor::prepareToPlay(double sr, int blockSize)
{
    currentSampleRate = sr;
    synth.setCurrentPlaybackSampleRate(sr);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<FolkVoice*>(synth.getVoice(i)))
            v->prepare(sr, blockSize);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32)blockSize, 2 };
    chorus.prepare(spec);
    chorus.reset();

    delay.setMaximumDelayInSamples((int)(sr * 2.0) + 1);
    delay.prepare(spec);
    delay.reset();

    reverb.setSampleRate(sr);
    reverb.reset();

    pending.clear();
}

// ------------------------------------------------------------------ strum
// Si llegan varias notas en el mismo instante (un acorde) y el modulo de guitarra esta activo,
// las retrasa escalonadamente de grave a agudo, como un rasgueo hacia abajo.
void NeoDynProcessor::applyStrum(const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples)
{
    const float strumMs = strumP->load();
    const bool active = strumMs > 0.5f && gtrLvlP->load() > 0.01f;

    if (!active && pending.empty())
    {
        out = in;
        return;
    }

    const int strumSamples = (int)(strumMs * 0.001 * currentSampleRate);

    scratch.clear();
    if (active)
        for (const auto meta : in)
        {
            const auto m = meta.getMessage();
            if (m.isNoteOn())
                scratch.push_back({ meta.samplePosition, m });
        }

    std::sort(scratch.begin(), scratch.end(), [](const NoteOnEv& a, const NoteOnEv& b)
    {
        return a.pos != b.pos ? a.pos < b.pos : a.msg.getNoteNumber() < b.msg.getNoteNumber();
    });

    int idx = 0;
    for (size_t i = 0; i < scratch.size(); ++i)
    {
        idx = (i > 0 && scratch[i].pos == scratch[i - 1].pos) ? idx + 1 : 0;
        pending.push_back({ scratch[i].pos + idx * strumSamples, scratch[i].msg });
    }
    scratch.clear();

    for (const auto meta : in)
    {
        const auto m = meta.getMessage();
        int pos = meta.samplePosition;

        if (active && m.isNoteOn())
            continue;

        if (m.isNoteOff())
        {
            int latest = -1;
            for (const auto& p : pending)
                if (p.msg.isNoteOn() && p.msg.getNoteNumber() == m.getNoteNumber()
                    && p.msg.getChannel() == m.getChannel())
                    latest = std::max(latest, p.pos);

            if (latest >= pos)
            {
                pending.push_back({ latest + 1, m });
                continue;
            }
        }

        out.addEvent(m, pos);
    }

    for (auto it = pending.begin(); it != pending.end();)
    {
        if (it->pos < numSamples)
        {
            out.addEvent(it->msg, std::max(0, it->pos));
            it = pending.erase(it);
        }
        else
        {
            it->pos -= numSamples;
            ++it;
        }
    }
}

// ------------------------------------------------------------------ audio
void NeoDynProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const float sr = (float)currentSampleRate;

    buffer.clear();
    strumOut.clear();
    applyStrum(midi, strumOut, n);
    synth.renderNextBlock(buffer, strumOut, 0, n);

    if (buffer.getNumChannels() < 2)
        return;

    // Chorus
    const float cm = chorusP->load();
    if (cm > 0.001f)
    {
        chorus.setMix(cm * 0.6f);
        chorus.setRate(0.9f);
        chorus.setDepth(0.3f);
        chorus.setCentreDelay(8.0f);
        chorus.setFeedback(0.0f);
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        chorus.process(ctx);
    }

    // Delay
    const float dm = dlyMixP->load();
    if (dm > 0.001f)
    {
        const float fb = dlyFbP->load();
        const float dTime = juce::jlimit(1.0f, sr * 2.0f - 1.0f, dlyTimeP->load() * sr);
        delay.setDelay(dTime);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = buffer.getWritePointer(ch);
            for (int i = 0; i < n; ++i)
            {
                const float in = d[i];
                const float del = delay.popSample(ch);
                delay.pushSample(ch, in + del * fb);
                d[i] = in + del * dm;
            }
        }
    }

    // Reverb
    const float rm = revMixP->load();
    if (rm > 0.001f)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = revSizeP->load();
        rp.damping = 0.5f;
        rp.wetLevel = rm * 0.45f;
        rp.dryLevel = 1.0f - rm * 0.3f;
        rp.width = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters(rp);
        reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), n);
    }

    // Master + limitador suave
    const float g = masterP->load();
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        for (int i = 0; i < n; ++i)
            d[i] = std::tanh(d[i] * g);
    }
}

// ------------------------------------------------------------------ presets / estado
int NeoDynProcessor::getNumPrograms() { return (int)presets().size(); }

const juce::String NeoDynProcessor::getProgramName(int index)
{
    const auto& ps = presets();
    return (index >= 0 && index < (int)ps.size()) ? juce::String(ps[(size_t)index].name) : juce::String();
}

void NeoDynProcessor::setCurrentProgram(int index)
{
    const auto& ps = presets();
    if (index < 0 || index >= (int)ps.size())
        return;

    currentProgram = index;
    for (auto* p : getParameters())
        p->setValueNotifyingHost(p->getDefaultValue());

    for (const auto& kv : ps[(size_t)index].values)
        if (auto* p = apvts.getParameter(kv.first))
            p->setValueNotifyingHost(p->convertTo0to1(kv.second));
}

void NeoDynProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void NeoDynProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* NeoDynProcessor::createEditor() { return new NeoDynEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NeoDynProcessor(); }
