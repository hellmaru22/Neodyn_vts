#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class NeoDynEditor : public juce::AudioProcessorEditor
{
public:
    explicit NeoDynEditor(NeoDynProcessor&);
    ~NeoDynEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int kCellW = 76;
    static constexpr int kCellH = 100;

    struct Knob
    {
        juce::Slider s;
        juce::Label l;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    struct Group
    {
        juce::String title;
        juce::Colour col;
        int row;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> bounds;
    };

    int addGroup(const juce::String& title, juce::Colour col, int row);
    void addKnob(int groupIndex, const char* paramId, const char* label);

    NeoDynProcessor& proc;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<Group> groups;

    juce::ComboBox presetBox, filterTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeoDynEditor)
};
