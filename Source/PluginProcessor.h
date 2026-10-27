#pragma once
#include <JuceHeader.h>
#include "FolkVoice.h"

class NeoDynProcessor : public juce::AudioProcessor
{
public:
    NeoDynProcessor();
    ~NeoDynProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "NeoDyn"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void applyStrum(const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples);

    struct Pending { int pos; juce::MidiMessage msg; };
    struct NoteOnEv { int pos; juce::MidiMessage msg; };

    juce::Synthesiser synth;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float> delay;
    juce::Reverb reverb;

    std::vector<Pending> pending;
    std::vector<NoteOnEv> scratch;
    juce::MidiBuffer strumOut;

    std::atomic<float> *strumP = nullptr, *gtrLvlP = nullptr, *chorusP = nullptr,
                       *dlyMixP = nullptr, *dlyTimeP = nullptr, *dlyFbP = nullptr,
                       *revMixP = nullptr, *revSizeP = nullptr, *masterP = nullptr;

    double currentSampleRate = 44100.0;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeoDynProcessor)
};
