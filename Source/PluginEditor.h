#pragma once
#include "PluginProcessor.h"

namespace ui
{
const juce::Colour bg       { 0xff111317 };
const juce::Colour panel    { 0xff1a1d23 };
const juce::Colour panelHi  { 0xff222630 };
const juce::Colour edge     { 0xff2c313b };
const juce::Colour accent   { 0xffff9a3c };
const juce::Colour accent2  { 0xff4cc9f0 };
const juce::Colour text     { 0xffdde1e8 };
const juce::Colour dim      { 0xff7d8593 };
}

//==============================================================================
class SatLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SatLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;
};

//==============================================================================
// Reglage du signal de test de l'analyseur (partage entre les 4 slots)
struct TestTone
{
    std::atomic<float> freq { 1000.0f }, levelDb { -12.0f };
};

//==============================================================================
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& text);
    void resized() override;
    void setText (const juce::String& t) { label.setText (t, juce::dontSendNotification); }

    juce::Slider slider;
    juce::Label label;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
// Courbe de transfert dynamique (x -> y, boucle d'hysteresis visible) + spectre d'un sinus de test
class AnalyzerComponent : public juce::Component, private juce::Timer
{
public:
    AnalyzerComponent (SatLabProcessor& p, int slotIndex, TestTone& t);
    void paint (juce::Graphics&) override;
    void resized() override {}
    void refresh();   // recalcule immediatement (utilise aussi pour les captures d'ecran)
private:
    void timerCallback() override;
    void compute (const satlab::SlotParams& sp);
    static std::vector<float> signature (const satlab::SlotParams& sp, float f, float l);

    SatLabProcessor& proc;
    int slot;
    TestTone& tone;
    std::vector<float> lastSig;

    static constexpr int N = 32768;
    float fs = 192000.0f, f0 = 1000.0f, amp = 0.25f;
    std::vector<float> xs, ys;          // 2 periodes
    std::vector<float> specDb;          // N/2 bins
    float harmDb[9] {};                 // H2..H9 par rapport au fondamental
    float thdPct = 0.0f;
    float yScale = 1.0f;
    bool  valid = false;
};

//==============================================================================
class SlotPanel : public juce::Component
{
public:
    SlotPanel (SatLabProcessor& p, int slotIndex, TestTone& t);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshAnalyzer() { analyzer.refresh(); }

private:
    struct Section { juce::Rectangle<int> r; juce::String title; };
    void onTypeChanged (int type);
    void showTypeMenu();
    void resetCharacterDefaults (int type);
    void place (juce::Rectangle<int> r, std::initializer_list<int> params);

    SatLabProcessor& proc;
    int slot;
    int currentType = 1;

    juce::TextButton typeButton;
    std::array<std::unique_ptr<Knob>, satlab::P_Count> knobs;
    juce::ComboBox stereoBox;
    juce::ToggleButton autoGain { "Auto Gain" };
    AnalyzerComponent analyzer;
    std::vector<Section> sections;
    juce::Rectangle<int> hintRect;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stereoAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoAtt;
    std::unique_ptr<juce::ParameterAttachment> typeAtt;
};

//==============================================================================
class LevelMeter : public juce::Component
{
public:
    explicit LevelMeter (std::atomic<float>& s) : src (s) {}
    void paint (juce::Graphics&) override;
private:
    std::atomic<float>& src;
};

//==============================================================================
class SatLabEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SatLabEditor (SatLabProcessor&);
    ~SatLabEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void snapshotRefresh (int slot) { selectSlot (slot); for (auto& p : panels) p->refreshAnalyzer(); }

private:
    void timerCallback() override;
    void selectSlot (int s);

    SatLabProcessor& proc;
    SatLookAndFeel laf;
    TestTone tone;

    juce::ComboBox presetBox, osBox, routingBox;
    juce::ToggleButton safety { "Safety Clip" };
    std::unique_ptr<Knob> inKnob, outKnob, mixKnob;
    LevelMeter inMeter, outMeter;
    std::array<std::unique_ptr<SlotPanel>, satlab::kNumSlots> panels;
    std::array<juce::TextButton, satlab::kNumSlots> tabs;
    std::array<juce::ToggleButton, satlab::kNumSlots> onButtons;
    std::array<std::unique_ptr<juce::ParameterAttachment>, satlab::kNumSlots> tabTypeAtt;
    juce::Slider testFreq, testLevel;
    juce::Label testFreqLabel, testLevelLabel;
    int selected = 0;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osAtt, routingAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> safetyAtt;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, satlab::kNumSlots> onAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SatLabEditor)
};
