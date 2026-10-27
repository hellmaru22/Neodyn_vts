#include "PluginEditor.h"

int NeoDynEditor::addGroup(const juce::String& title, juce::Colour col, int row)
{
    groups.push_back({ title, col, row, {}, {} });
    return (int)groups.size() - 1;
}

void NeoDynEditor::addKnob(int gi, const char* paramId, const char* label)
{
    auto k = std::make_unique<Knob>();
    k->s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k->s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, kCellW - 6, 14);
    k->s.setColour(juce::Slider::rotarySliderFillColourId, groups[(size_t)gi].col);
    k->s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    k->l.setText(label, juce::dontSendNotification);
    k->l.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(k->s);
    addAndMakeVisible(k->l);

    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, paramId, k->s);

    groups[(size_t)gi].knobs.push_back(k.get());
    knobs.push_back(std::move(k));
}

NeoDynEditor::NeoDynEditor(NeoDynProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    groups.reserve(12);

    const auto cyan   = juce::Colour(0xff4fc3f7);
    const auto purple = juce::Colour(0xffba68c8);
    const auto orange = juce::Colour(0xffffb74d);
    const auto green  = juce::Colour(0xff81c784);
    const auto pink   = juce::Colour(0xfff06292);
    const auto grey   = juce::Colour(0xffb0bec5);

    int g = addGroup("OSC 1", cyan, 0);
    addKnob(g, "osc1_wt", "Wave");  addKnob(g, "osc1_oct", "Octave");  addKnob(g, "osc1_lvl", "Level");

    g = addGroup("OSC 2", cyan, 0);
    addKnob(g, "osc2_wt", "Wave");  addKnob(g, "osc2_oct", "Octave");
    addKnob(g, "osc2_fine", "Fine"); addKnob(g, "osc2_lvl", "Level");

    g = addGroup("UNISON", purple, 0);
    addKnob(g, "uni_voices", "Voices"); addKnob(g, "uni_detune", "Detune"); addKnob(g, "uni_spread", "Spread");

    g = addGroup("FILTER", orange, 1);
    addKnob(g, "f_cut", "Cutoff"); addKnob(g, "f_res", "Reso"); addKnob(g, "f_env", "Env Amt");

    g = addGroup("AMP ENV", green, 1);
    addKnob(g, "a_att", "A"); addKnob(g, "a_dec", "D"); addKnob(g, "a_sus", "S"); addKnob(g, "a_rel", "R");

    g = addGroup("FILTER ENV", green, 1);
    addKnob(g, "e_att", "A"); addKnob(g, "e_dec", "D"); addKnob(g, "e_sus", "S"); addKnob(g, "e_rel", "R");

    g = addGroup("LFO", purple, 2);
    addKnob(g, "lfo_rate", "Rate"); addKnob(g, "lfo_wt", "> Wave"); addKnob(g, "lfo_cut", "> Cutoff");

    g = addGroup("ACOUSTIC GUITAR", orange, 2);
    addKnob(g, "gtr_lvl", "Level"); addKnob(g, "gtr_decay", "Decay");
    addKnob(g, "gtr_bright", "Bright"); addKnob(g, "gtr_strum", "Strum ms");

    g = addGroup("MASTER", grey, 2);
    addKnob(g, "syn_lvl", "Synth"); addKnob(g, "master", "Master");

    g = addGroup("FX", pink, 3);
    addKnob(g, "fx_chorus", "Chorus");
    addKnob(g, "fx_dly_mix", "Dly Mix"); addKnob(g, "fx_dly_time", "Dly Time"); addKnob(g, "fx_dly_fb", "Dly Fdbk");
    addKnob(g, "fx_rev_mix", "Rev Mix"); addKnob(g, "fx_rev_size", "Rev Size");

    // Presets
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem(proc.getProgramName(i), i + 1);
    presetBox.setSelectedItemIndex(proc.getCurrentProgram(), juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram(presetBox.getSelectedItemIndex()); };
    addAndMakeVisible(presetBox);

    // Tipo de filtro
    filterTypeBox.addItem("Low-pass", 1);
    filterTypeBox.addItem("Band-pass", 2);
    filterTypeBox.addItem("High-pass", 3);
    addAndMakeVisible(filterTypeBox);
    filterTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "f_type", filterTypeBox);

    setSize(920, 640);
}

void NeoDynEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff14161c));

    g.setColour(juce::Colour(0xff4fc3f7));
    g.setFont(22.0f);
    g.drawText("NeoDyn", getWidth() - 200, 8, 190, 32, juce::Justification::centredRight);

    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    g.drawText("Preset", 10, 4, 60, 12, juce::Justification::centredLeft);
    g.drawText("Filter type", 250, 4, 100, 12, juce::Justification::centredLeft);

    for (const auto& gr : groups)
    {
        g.setColour(juce::Colour(0xff1f232c));
        g.fillRoundedRectangle(gr.bounds.toFloat(), 8.0f);
        g.setColour(gr.col);
        g.setFont(12.0f);
        g.drawText(gr.title, gr.bounds.getX() + 8, gr.bounds.getY() + 4,
                   gr.bounds.getWidth() - 16, 16, juce::Justification::centredLeft);
    }
}

void NeoDynEditor::resized()
{
    presetBox.setBounds(10, 16, 230, 26);
    filterTypeBox.setBounds(250, 16, 140, 26);

    int x[4] = { 10, 10, 10, 10 };
    for (auto& gr : groups)
    {
        const int y = 50 + gr.row * 146;
        const int w = (int)gr.knobs.size() * kCellW + 12;
        gr.bounds = { x[gr.row], y, w, 140 };

        int kx = gr.bounds.getX() + 6;
        for (auto* k : gr.knobs)
        {
            k->l.setBounds(kx, y + 22, kCellW, 14);
            k->s.setBounds(kx, y + 36, kCellW, kCellH - 6);
            kx += kCellW;
        }
        x[gr.row] += w + 8;
    }
}
