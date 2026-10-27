#pragma once
#include <JuceHeader.h>
#include "Wavetable.h"

struct FolkSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class FolkVoice : public juce::SynthesiserVoice
{
public:
    explicit FolkVoice(juce::AudioProcessorValueTreeState& s) : apvts(s), table(Wavetable::get())
    {
        auto g = [&](const char* id) { return apvts.getRawParameterValue(id); };
        osc1Wt = g("osc1_wt");   osc1Oct = g("osc1_oct");   osc1Lvl = g("osc1_lvl");
        osc2Wt = g("osc2_wt");   osc2Oct = g("osc2_oct");   osc2Fine = g("osc2_fine"); osc2Lvl = g("osc2_lvl");
        uniVoices = g("uni_voices"); uniDetune = g("uni_detune"); uniSpread = g("uni_spread");
        fType = g("f_type"); fCut = g("f_cut"); fRes = g("f_res"); fEnv = g("f_env");
        aAtt = g("a_att"); aDec = g("a_dec"); aSus = g("a_sus"); aRel = g("a_rel");
        eAtt = g("e_att"); eDec = g("e_dec"); eSus = g("e_sus"); eRel = g("e_rel");
        lfoRate = g("lfo_rate"); lfoWt = g("lfo_wt"); lfoCut = g("lfo_cut");
        gtrLvl = g("gtr_lvl"); gtrDecay = g("gtr_decay"); gtrBright = g("gtr_bright");
        synLvl = g("syn_lvl");

        pluckBuf.assign((size_t)kPluckSize, 0.0f);
    }

    void prepare(double sr, int blockSize)
    {
        sampleRate = sr;
        amp.setSampleRate(sr);
        flt.setSampleRate(sr);
        juce::dsp::ProcessSpec spec { sr, (juce::uint32)blockSize, 2 };
        filter.prepare(spec);
        filter.reset();
    }

    bool canPlaySound(juce::SynthesiserSound* s) override
    {
        return dynamic_cast<FolkSound*>(s) != nullptr;
    }

    void startNote(int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override
    {
        noteFreq = (float)juce::MidiMessage::getMidiNoteInHertz(midiNote);
        vel = velocity;
        bend = ((float)pitchWheel - 8192.0f) / 8192.0f * 2.0f;

        amp.setParameters(juce::ADSR::Parameters(aAtt->load(), aDec->load(), aSus->load(), aRel->load()));
        flt.setParameters(juce::ADSR::Parameters(eAtt->load(), eDec->load(), eSus->load(), eRel->load()));
        amp.noteOn();
        flt.noteOn();

        for (int o = 0; o < 2; ++o)
            for (int k = 0; k < 7; ++k)
                phase[o][k] = rng.nextFloat();

        lfoPhase = 0.0f;
        cutCounter = 0;
        filter.reset();

        pluckLeft = 0;
        pluckRel = false;
        if (gtrLvl->load() > 0.001f)
            startPluck();
    }

    void stopNote(float, bool allowTailOff) override
    {
        amp.noteOff();
        flt.noteOff();

        if (pluckLeft > 0)
        {
            pluckRel = true;
            pluckLeft = std::min(pluckLeft, (int)(0.15 * sampleRate));
        }

        if (!allowTailOff)
        {
            amp.reset();
            pluckLeft = 0;
            clearCurrentNote();
        }
    }

    void pitchWheelMoved(int v) override { bend = ((float)v - 8192.0f) / 8192.0f * 2.0f; }
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (!isVoiceActive())
            return;

        const float sr = (float)sampleRate;

        // ---- parametros del bloque ----
        const int nUni = juce::jlimit(1, 7, (int)std::lround(uniVoices->load()));
        const float det = uniDetune->load();
        const float spread = uniSpread->load();

        const float wtBase[2] = { osc1Wt->load(), osc2Wt->load() };
        const float lvl[2] = { osc1Lvl->load(), osc2Lvl->load() };
        const float semis[2] = { osc1Oct->load() * 12.0f + bend,
                                 osc2Oct->load() * 12.0f + bend + osc2Fine->load() / 100.0f };

        float ratio[7], gl[7], gr[7];
        const float norm = 1.0f / std::sqrt((float)nUni);
        for (int k = 0; k < nUni; ++k)
        {
            const float rel = nUni > 1 ? ((float)k / (float)(nUni - 1)) * 2.0f - 1.0f : 0.0f;
            ratio[k] = std::exp2(rel * det * 70.0f / 1200.0f);
            const float pan = rel * spread;
            const float ang = (pan * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi;
            gl[k] = std::cos(ang) * 1.4142f * norm;
            gr[k] = std::sin(ang) * 1.4142f * norm;
        }

        float inc[2][7];
        int mip[2];
        for (int o = 0; o < 2; ++o)
        {
            const float base = noteFreq * std::exp2(semis[o] / 12.0f);
            for (int k = 0; k < nUni; ++k)
                inc[o][k] = base * ratio[k] / sr;
            mip[o] = Wavetable::mipFor(base * ratio[nUni - 1], sr);
        }

        const int ft = (int)std::lround(fType->load());
        filter.setType(ft == 0 ? juce::dsp::StateVariableTPTFilterType::lowpass
                     : ft == 1 ? juce::dsp::StateVariableTPTFilterType::bandpass
                               : juce::dsp::StateVariableTPTFilterType::highpass);
        filter.setResonance(0.7f + fRes->load() * 9.0f);

        const float baseCut = fCut->load();
        const float envAmt = fEnv->load();
        const float lfoR = lfoRate->load();
        const float lfoW = lfoWt->load();
        const float lfoC = lfoCut->load();
        const float synGain = synLvl->load() * vel * 0.3f;
        const float gtrGain = gtrLvl->load() * vel * 0.8f;
        const float relMul = std::exp(-1.0f / (0.04f * sr));

        for (int i = 0; i < numSamples; ++i)
        {
            if (!amp.isActive() && pluckLeft <= 0)
            {
                clearCurrentNote();
                break;
            }

            const float ae = amp.getNextSample();
            const float fe = flt.getNextSample();

            const float lfo = std::sin(juce::MathConstants<float>::twoPi * lfoPhase);
            lfoPhase += lfoR / sr;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

            float outL = 0.0f, outR = 0.0f;

            // ---- sintetizador ----
            if (synGain > 0.0f && ae > 0.0f)
            {
                float sL = 0.0f, sR = 0.0f;
                for (int o = 0; o < 2; ++o)
                {
                    if (lvl[o] <= 0.0001f) continue;
                    const float wt = juce::jlimit(0.0f, 1.0f, wtBase[o] + lfo * lfoW * 0.5f);
                    for (int k = 0; k < nUni; ++k)
                    {
                        const float s = table.read(wt, mip[o], phase[o][k]);
                        phase[o][k] += inc[o][k];
                        if (phase[o][k] >= 1.0f) phase[o][k] -= 1.0f;
                        sL += s * lvl[o] * gl[k];
                        sR += s * lvl[o] * gr[k];
                    }
                }

                if ((cutCounter++ & 7) == 0)
                {
                    float fc = baseCut * std::exp2(envAmt * fe * 5.0f + lfoC * lfo * 2.0f);
                    fc = juce::jlimit(20.0f, std::min(20000.0f, 0.45f * sr), fc);
                    filter.setCutoffFrequency(fc);
                }

                outL = filter.processSample(0, sL) * ae * synGain;
                outR = filter.processSample(1, sR) * ae * synGain;
            }

            // ---- guitarra (Karplus-Strong) ----
            if (pluckLeft > 0)
            {
                const float p = pluckTick() * pluckEnv * gtrGain;
                if (pluckRel) pluckEnv *= relMul;
                --pluckLeft;
                outL += p * pluckGL;
                outR += p * pluckGR;
            }

            out.addSample(0, startSample + i, outL);
            if (out.getNumChannels() > 1)
                out.addSample(1, startSample + i, outR);
        }
    }

private:
    static constexpr int kPluckSize = 16384;
    static constexpr int kPluckMask = kPluckSize - 1;

    void startPluck()
    {
        const float bright = gtrBright->load();
        const float decay = std::max(0.05f, gtrDecay->load());

        pluckA = 0.12f + 0.88f * bright;
        pluckD = (float)(sampleRate / noteFreq) - (1.0f - pluckA) / pluckA;
        pluckD = juce::jlimit(2.0f, (float)(kPluckSize - 8), pluckD);
        pluckFb = std::pow(10.0f, -3.0f * (1.0f / noteFreq) / decay);

        std::fill(pluckBuf.begin(), pluckBuf.end(), 0.0f);
        const int n = (int)pluckD + 2;
        float lp = 0.0f, mean = 0.0f, peak = 0.0f;
        const float exA = 0.25f + 0.75f * bright;
        for (int i = 0; i < n; ++i)
        {
            const float nz = rng.nextFloat() * 2.0f - 1.0f;
            lp += exA * (nz - lp);
            pluckBuf[(size_t)i] = lp;
            mean += lp;
        }
        mean /= (float)n;
        for (int i = 0; i < n; ++i)
        {
            pluckBuf[(size_t)i] -= mean;
            peak = std::max(peak, std::abs(pluckBuf[(size_t)i]));
        }
        if (peak > 0.0f)
            for (int i = 0; i < n; ++i)
                pluckBuf[(size_t)i] /= peak;

        pluckWp = n;
        pluckLp = 0.0f;
        pluckEnv = 1.0f;
        pluckLeft = (int)(decay * (float)sampleRate * 1.1f);

        const float pan = (rng.nextFloat() - 0.5f) * 0.4f;
        pluckGL = 0.7071f * (1.0f - pan);
        pluckGR = 0.7071f * (1.0f + pan);
    }

    float pluckTick()
    {
        const float rp = (float)pluckWp - pluckD;
        const int i0 = (int)std::floor(rp);
        const float fr = rp - (float)i0;
        const float s = pluckBuf[(size_t)(i0 & kPluckMask)] * (1.0f - fr)
                      + pluckBuf[(size_t)((i0 + 1) & kPluckMask)] * fr;
        pluckLp += pluckA * (s - pluckLp);
        pluckBuf[(size_t)(pluckWp & kPluckMask)] = pluckLp * pluckFb;
        pluckWp = (pluckWp + 1) & kPluckMask;
        return s;
    }

    juce::AudioProcessorValueTreeState& apvts;
    const Wavetable& table;
    juce::Random rng;

    std::atomic<float> *osc1Wt = nullptr, *osc1Oct = nullptr, *osc1Lvl = nullptr,
                       *osc2Wt = nullptr, *osc2Oct = nullptr, *osc2Fine = nullptr, *osc2Lvl = nullptr,
                       *uniVoices = nullptr, *uniDetune = nullptr, *uniSpread = nullptr,
                       *fType = nullptr, *fCut = nullptr, *fRes = nullptr, *fEnv = nullptr,
                       *aAtt = nullptr, *aDec = nullptr, *aSus = nullptr, *aRel = nullptr,
                       *eAtt = nullptr, *eDec = nullptr, *eSus = nullptr, *eRel = nullptr,
                       *lfoRate = nullptr, *lfoWt = nullptr, *lfoCut = nullptr,
                       *gtrLvl = nullptr, *gtrDecay = nullptr, *gtrBright = nullptr,
                       *synLvl = nullptr;

    double sampleRate = 44100.0;
    float noteFreq = 440.0f, vel = 1.0f, bend = 0.0f;
    float phase[2][7] {};
    float lfoPhase = 0.0f;
    int cutCounter = 0;

    juce::ADSR amp, flt;
    juce::dsp::StateVariableTPTFilter<float> filter;

    // guitarra
    std::vector<float> pluckBuf;
    int pluckWp = 0, pluckLeft = 0;
    float pluckD = 100.0f, pluckA = 0.5f, pluckFb = 0.99f, pluckLp = 0.0f, pluckEnv = 1.0f;
    float pluckGL = 0.7071f, pluckGR = 0.7071f;
    bool pluckRel = false;
};
