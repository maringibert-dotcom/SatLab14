#include "PluginEditor.h"
#include "Presets.h"

using namespace satlab;

//==============================================================================
// Look & feel
//==============================================================================
SatLookAndFeel::SatLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, ui::bg);
    setColour (juce::Slider::textBoxTextColourId, ui::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, ui::accent.withAlpha (0.4f));
    setColour (juce::Label::textColourId, ui::dim);
    setColour (juce::ComboBox::backgroundColourId, ui::panelHi);
    setColour (juce::ComboBox::textColourId, ui::text);
    setColour (juce::ComboBox::outlineColourId, ui::edge);
    setColour (juce::ComboBox::arrowColourId, ui::accent);
    setColour (juce::PopupMenu::backgroundColourId, ui::panel);
    setColour (juce::PopupMenu::textColourId, ui::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::accent.withAlpha (0.35f));
    setColour (juce::PopupMenu::highlightedTextColourId, ui::text);
    setColour (juce::PopupMenu::headerTextColourId, ui::accent);
    setColour (juce::TextButton::buttonColourId, ui::panelHi);
    setColour (juce::TextButton::buttonOnColourId, ui::accent.withAlpha (0.28f));
    setColour (juce::TextButton::textColourOffId, ui::text);
    setColour (juce::TextButton::textColourOnId, ui::text);
}

juce::Font SatLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return juce::Font (juce::FontOptions (14.0f)); }
juce::Font SatLookAndFeel::getComboBoxFont (juce::ComboBox&)         { return juce::Font (juce::FontOptions (14.0f)); }
juce::Font SatLookAndFeel::getLabelFont (juce::Label&)               { return juce::Font (juce::FontOptions (12.0f)); }

void SatLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float a0, float a1, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();

    g.setColour (ui::panelHi);
    g.fillEllipse (c.x - r * 0.74f, c.y - r * 0.74f, r * 1.48f, r * 1.48f);
    g.setColour (ui::edge);
    g.drawEllipse (c.x - r * 0.74f, c.y - r * 0.74f, r * 1.48f, r * 1.48f, 1.0f);

    const float rad = r - 2.0f;
    juce::Path track;
    track.addCentredArc (c.x, c.y, rad, rad, 0.0f, a0, a1, true);
    g.setColour (ui::edge.brighter (0.2f));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    const float ang = a0 + pos * (a1 - a0);
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float from = bipolar ? a0 + (a1 - a0) * (float) s.valueToProportionOfLength (0.0) : a0;
    juce::Path val;
    val.addCentredArc (c.x, c.y, rad, rad, 0.0f, std::min (from, ang), std::max (from, ang), true);
    g.setColour (s.isEnabled() ? ui::accent : ui::dim);
    g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -r * 0.68f, 3.0f, r * 0.32f, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    g.setColour (ui::text);
    g.fillPath (pointer);
}

void SatLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    g.setColour (on ? ui::accent.withAlpha (0.22f) : (hl ? ui::panelHi.brighter (0.1f) : ui::panelHi));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? ui::accent : ui::edge);
    g.drawRoundedRectangle (r, 5.0f, 1.2f);
    const juce::Rectangle<float> led (r.getX() + 8.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f);
    g.setColour (on ? ui::accent : ui::dim.withAlpha (0.5f));
    g.fillEllipse (led);
    g.setColour (on ? ui::text : ui::dim);
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (24.0f), juce::Justification::centredLeft);
}

void SatLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    auto fill = on ? ui::accent.withAlpha (0.25f) : ui::panelHi;
    if (hl) fill = fill.brighter (0.08f);
    if (down) fill = fill.brighter (0.12f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (on ? ui::accent : ui::edge);
    g.drawRoundedRectangle (r, 6.0f, on ? 1.5f : 1.0f);
}

//==============================================================================
// Knob
//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setInterceptsMouseClicks (false, false);
    label.setColour (juce::Label::textColourId, ui::dim);
    label.setFont (juce::Font (juce::FontOptions (10.5f)));
    addAndMakeVisible (label);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setColour (juce::Slider::textBoxTextColourId, ui::text);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxHighlightColourId, ui::accent.withAlpha (0.4f));
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 15);
    slider.setMouseDragSensitivity (260);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, slider);
    if (auto* p = apvts.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    slider.setBounds (r);
}

//==============================================================================
// Analyseur
//==============================================================================
AnalyzerComponent::AnalyzerComponent (SatLabProcessor& p, int slotIndex, TestTone& t)
    : proc (p), slot (slotIndex), tone (t)
{
    startTimerHz (10);
}

std::vector<float> AnalyzerComponent::signature (const SlotParams& s, float f, float l)
{
    std::vector<float> v { (float) s.type, (float) s.stereoMode, s.rangeLo, s.rangeHi, s.driveDb, s.bias, s.asym,
                           s.focusFreq, s.focusGainDb, s.focusQ, s.deemph, s.sag, s.sagTimeMs,
                           s.s[0], s.s[1], s.s[2], s.s[3], s.warmth, s.warmFreq, s.harsh, s.harshFreq,
                           s.lsFreq, s.lsGainDb, s.midFreq, s.midGainDb, s.midQ, s.hsFreq, s.hsGainDb, f, l };
    return v;
}

void AnalyzerComponent::timerCallback()
{
    if (! isShowing()) return;
    const auto sp = readSlotParams (proc.params, slot);
    const auto sig = signature (sp, tone.freq.load(), tone.levelDb.load());
    if (sig != lastSig) { lastSig = sig; compute (sp); repaint(); }
}

void AnalyzerComponent::refresh()
{
    const auto sp = readSlotParams (proc.params, slot);
    lastSig = signature (sp, tone.freq.load(), tone.levelDb.load());
    compute (sp);
    repaint();
}

void AnalyzerComponent::compute (const SlotParams& spIn)
{
    SlotParams sp = spIn;
    sp.on = true; sp.mix = 1.0f; sp.outDb = 0.0f; sp.autoGain = false;

    const int os = 4;
    fs = 192000.0f;
    const int kbin = std::max (2, (int) std::lround (tone.freq.load() * (float) N / fs));
    f0 = (float) kbin * fs / (float) N;
    amp = dbToLin (tone.levelDb.load());

    auto s = std::make_unique<SatSlot>();
    s->prepare (fs, os);
    s->setParams (sp);

    const int warm = 16384, total = warm + N;
    std::vector<float> L ((size_t) total), R ((size_t) total);
    for (int i = 0; i < total; ++i)
        L[(size_t) i] = R[(size_t) i] = amp * (float) std::sin (2.0 * juce::MathConstants<double>::pi * (double) kbin * (double) i / (double) N);
    for (int pos = 0; pos < total; pos += 1024)
    {
        const int n = std::min (1024, total - pos);
        s->process (L.data() + pos, R.data() + pos, n);
    }
    const auto& out = (sp.stereoMode == 4) ? R : L;

    // courbe de transfert : 2 periodes
    const int count = std::min (N, (int) (2.0f * (float) N / (float) kbin));
    xs.resize ((size_t) count); ys.resize ((size_t) count);
    float peak = 1.0f;
    for (int i = 0; i < count; ++i)
    {
        const int idx = total - count + i;
        xs[(size_t) i] = amp * (float) std::sin (2.0 * juce::MathConstants<double>::pi * (double) kbin * (double) idx / (double) N);
        ys[(size_t) i] = out[(size_t) idx];
        peak = std::max (peak, std::abs (ys[(size_t) i]));
    }
    yScale = peak * 1.05f;

    // spectre (sinus aligne sur un bin : pas besoin de fenetre)
    juce::dsp::FFT fft (15);
    std::vector<float> data ((size_t) N * 2, 0.0f);
    for (int i = 0; i < N; ++i) data[(size_t) i] = out[(size_t) (warm + i)];
    fft.performFrequencyOnlyForwardTransform (data.data());
    specDb.resize ((size_t) N / 2);
    for (int k = 0; k < N / 2; ++k)
        specDb[(size_t) k] = 20.0f * std::log10 (2.0f * data[(size_t) k] / (float) N + 1e-9f);

    const float fund = 2.0f * data[(size_t) kbin] / (float) N + 1e-9f;
    double sumSq = 0.0;
    for (int h = 2; h * f0 < 20000.0f && h * kbin < N / 2; ++h)
    {
        const float a = 2.0f * data[(size_t) (h * kbin)] / (float) N;
        sumSq += (double) a * a;
        if (h <= 9) harmDb[h - 2] = 20.0f * std::log10 (a / fund + 1e-9f);
    }
    thdPct = 100.0f * (float) std::sqrt (sumSq) / fund;
    valid = true;
}

void AnalyzerComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (ui::panel);
    g.fillRoundedRectangle (b, 8.0f);
    g.setColour (ui::edge);
    g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);
    if (! valid) return;

    auto area = b.reduced (10.0f);
    auto tf = area.removeFromTop (area.getHeight() * 0.46f);
    area.removeFromTop (14.0f);
    auto txt = area.removeFromBottom (36.0f);
    auto sp = area;

    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.setColour (ui::dim);
    g.drawText ("TRANSFER (dynamic)", tf.removeFromTop (12.0f), juce::Justification::left);
    tf.removeFromTop (2.0f);

    // ---- transfert
    {
        g.setColour (ui::edge);
        g.drawRect (tf, 1.0f);
        g.drawLine (tf.getX(), tf.getCentreY(), tf.getRight(), tf.getCentreY(), 0.6f);
        g.drawLine (tf.getCentreX(), tf.getY(), tf.getCentreX(), tf.getBottom(), 0.6f);

        const auto sparams = readSlotParams (proc.params, slot);
        const float gLin = dbToLin (sparams.driveDb);
        auto mapPt = [&] (float x, float y)
        {
            return juce::Point<float> (tf.getCentreX() + (x / amp) * tf.getWidth() * 0.5f,
                                       tf.getCentreY() - (y / yScale) * tf.getHeight() * 0.5f);
        };
        // ligne de reference : gain lineaire = drive
        g.saveState();
        g.reduceClipRegion (tf.toNearestInt());
        g.setColour (ui::dim.withAlpha (0.55f));
        g.drawLine (juce::Line<float> (mapPt (-amp, -amp * gLin), mapPt (amp, amp * gLin)), 1.0f);
        juce::Path p;
        for (size_t i = 0; i < xs.size(); ++i)
        {
            const auto pt = mapPt (xs[i], ys[i]);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        g.setColour (ui::accent);
        g.strokePath (p, juce::PathStrokeType (1.8f));
        g.restoreState();
    }

    // ---- spectre
    {
        g.setColour (ui::dim);
        g.drawText ("SPECTRUM  (test tone " + juce::String ((int) std::round (f0)) + " Hz, " + juce::String (tone.levelDb.load(), 0) + " dBFS)",
                    sp.removeFromTop (12.0f), juce::Justification::left);
        sp.removeFromTop (2.0f);
        const float dbMin = -110.0f, dbMax = 6.0f;
        auto yOf = [&] (float db) { return sp.getBottom() - (juce::jlimit (dbMin, dbMax, db) - dbMin) / (dbMax - dbMin) * sp.getHeight(); };
        g.setColour (ui::edge);
        g.drawRect (sp, 1.0f);
        for (float db : { -20.0f, -40.0f, -60.0f, -80.0f, -100.0f })
            g.drawLine (sp.getX(), yOf (db), sp.getRight(), yOf (db), 0.5f);
        for (float f : { 100.0f, 1000.0f, 10000.0f })
        {
            const float x = sp.getX() + std::log10 (f / 20.0f) / 3.0f * sp.getWidth();
            g.drawLine (x, sp.getY(), x, sp.getBottom(), 0.5f);
            g.setColour (ui::dim);
            g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), (int) x + 2, (int) sp.getBottom() - 13, 30, 12, juce::Justification::left);
            g.setColour (ui::edge);
        }

        const float binHz = fs / (float) N;
        const int W = (int) sp.getWidth();
        juce::Path line;
        for (int i = 0; i < W; ++i)
        {
            const float fLo = 20.0f * std::pow (1000.0f, (float) i / (float) W);
            const float fHi = 20.0f * std::pow (1000.0f, (float) (i + 1) / (float) W);
            int kLo = (int) std::floor (fLo / binHz), kHi = (int) std::ceil (fHi / binHz);
            kLo = juce::jlimit (1, N / 2 - 1, kLo); kHi = juce::jlimit (kLo, N / 2 - 1, kHi);
            float m = -200.0f;
            for (int k = kLo; k <= kHi; ++k) m = std::max (m, specDb[(size_t) k]);
            const float x = sp.getX() + (float) i, y = yOf (m);
            if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);
        }
        juce::Path fill (line);
        fill.lineTo (sp.getRight(), sp.getBottom()); fill.lineTo (sp.getX(), sp.getBottom()); fill.closeSubPath();
        g.setColour (ui::accent.withAlpha (0.18f));
        g.fillPath (fill);
        g.setColour (ui::accent);
        g.strokePath (line, juce::PathStrokeType (1.3f));

        // reperes des harmoniques (paires = cyan, impaires = orange)
        for (int h = 1; h <= 16 && h * f0 < 20000.0f; ++h)
        {
            const float x = sp.getX() + std::log10 (h * f0 / 20.0f) / 3.0f * sp.getWidth();
            g.setColour ((h % 2 == 0 ? ui::accent2 : ui::accent).withAlpha (0.9f));
            g.fillRect (x - 1.0f, sp.getY() + 1.0f, 2.0f, 4.0f);
            if (h <= 6) { g.setFont (juce::Font (juce::FontOptions (10.0f))); g.drawText (juce::String (h), (int) x - 6, (int) sp.getY() + 5, 12, 11, juce::Justification::centred); }
        }
    }

    // ---- lecture numerique
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.setColour (ui::text);
    juce::String l1 = "THD " + juce::String (thdPct, thdPct < 10 ? 2 : 1) + " %";
    juce::String l2;
    for (int h = 2; h <= 7; ++h) l2 << "H" << h << " " << (harmDb[h - 2] < -99.0f ? juce::String ("<-99") : juce::String ((int) std::round (harmDb[h - 2]))) << "   ";
    g.drawText (l1, txt.removeFromTop (17.0f), juce::Justification::left);
    g.setColour (ui::dim);
    g.drawText (l2, txt, juce::Justification::left);
}

//==============================================================================
// Panneau d'un slot
//==============================================================================
static juce::String knobTitle (int p)
{
    switch (p)
    {
        case P_Drive: return "DRIVE";      case P_Bias: return "BIAS";       case P_Asym: return "ASYMMETRY";
        case P_Sag: return "SAG";          case P_SagT: return "SAG TIME";
        case P_FFreq: return "FREQ";       case P_FGain: return "GAIN";      case P_FQ: return "Q";       case P_Deemph: return "DE-EMPH";
        case P_Warm: return "WARMTH";      case P_WarmF: return "WARM FREQ"; case P_Harsh: return "HARSHNESS"; case P_HarshF: return "HARSH FREQ";
        case P_LsF: return "LOW FREQ";     case P_LsG: return "LOW GAIN";    case P_MidF: return "MID FREQ"; case P_MidG: return "MID GAIN";
        case P_MidQ: return "MID Q";       case P_HsF: return "HIGH FREQ";   case P_HsG: return "HIGH GAIN";
        case P_Lo: return "FROM";          case P_Hi: return "TO";
        case P_Mix: return "MIX";          case P_Out: return "LEVEL";
        default: return "";
    }
}

SlotPanel::SlotPanel (SatLabProcessor& p, int slotIndex, TestTone& t)
    : proc (p), slot (slotIndex), analyzer (p, slotIndex, t)
{
    auto& apvts = proc.apvts;
    for (int i = 0; i < P_Count; ++i)
        if (slotDef (i).kind == Kind::Float)
        {
            knobs[(size_t) i] = std::make_unique<Knob> (apvts, slotId (slot, i), knobTitle (i));
            addAndMakeVisible (*knobs[(size_t) i]);
        }

    addAndMakeVisible (typeButton);
    typeButton.onClick = [this] { showTypeMenu(); };

    stereoBox.addItemList (stereoModeNames(), 1);
    addAndMakeVisible (stereoBox);
    stereoAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, slotId (slot, P_Stereo), stereoBox);

    addAndMakeVisible (autoGain);
    autoAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, slotId (slot, P_Auto), autoGain);

    addAndMakeVisible (analyzer);

    typeAtt = std::make_unique<juce::ParameterAttachment> (*apvts.getParameter (slotId (slot, P_Type)),
                                                            [this] (float v) { onTypeChanged ((int) std::round (v)); });
    typeAtt->sendInitialUpdate();
}

void SlotPanel::onTypeChanged (int type)
{
    currentType = juce::jlimit (0, (int) NumSatTypes - 1, type);
    const auto& info = getTypeInfo (currentType);
    typeButton.setButtonText (info.name);
    for (int i = 0; i < 4; ++i)
    {
        auto& k = knobs[(size_t) (P_S1 + i)];
        if (info.label[i] != nullptr) { k->setText (juce::String (info.label[i]).toUpperCase()); k->setVisible (true); }
        else k->setVisible (false);
    }
    repaint();
}

void SlotPanel::showTypeMenu()
{
    juce::PopupMenu m;
    auto add = [&] (const char* heading, int first, int last)
    {
        m.addSectionHeader (heading);
        for (int t = first; t <= last; ++t) m.addItem (t + 1, getTypeInfo (t).name, true, t == currentType);
    };
    add ("Basic", Clean, Clean);
    add ("Waveshapers", Tanh, Exponential);
    add ("Analog gear", Triode, TapeClassic);
    add ("Pedals & circuits", DiodeClipper, ClassB);
    add ("Digital & creative", Bitcrusher, Jfet);
    add ("Full chains", Console, TubeAmp);

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (typeButton).withMinimumWidth (typeButton.getWidth()),
                     [this] (int r)
                     {
                         if (r <= 0) return;
                         const int t = r - 1;
                         if (auto* prm = proc.apvts.getParameter (slotId (slot, P_Type)))
                         {
                             prm->beginChangeGesture();
                             prm->setValueNotifyingHost (prm->convertTo0to1 ((float) t));
                             prm->endChangeGesture();
                         }
                         resetCharacterDefaults (t);
                     });
}

void SlotPanel::resetCharacterDefaults (int type)
{
    const auto& info = getTypeInfo (type);
    for (int i = 0; i < 4; ++i)
        if (auto* prm = proc.apvts.getParameter (slotId (slot, P_S1 + i)))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (info.def[i]));
            prm->endChangeGesture();
        }
}

void SlotPanel::place (juce::Rectangle<int> r, std::initializer_list<int> params)
{
    int x = r.getX() + 12;
    for (int p : params)
    {
        knobs[(size_t) p]->setBounds (x, r.getY() + 24, 68, 96);
        x += 68;
    }
}

void SlotPanel::resized()
{
    auto r = getLocalBounds();
    auto left = r.removeFromLeft (360);
    r.removeFromLeft (12);

    typeButton.setBounds (left.getX(), left.getY() + 16, left.getWidth(), 30);
    hintRect = juce::Rectangle<int> (left.getX() + 2, left.getY() + 50, left.getWidth() - 4, 42);
    analyzer.setBounds (left.withTrimmedTop (98));

    sections.clear();
    const int x0 = r.getX(), kw = 68, secH = 134, gap = 8;
    auto secWidth = [&] (int n) { return n * kw + 24; };
    auto add = [&] (int x, int y, int n, const juce::String& title) { sections.push_back ({ { x, y, secWidth (n), secH }, title }); return sections.back().r; };

    int y = 0;
    auto s1 = add (x0, y, 5, "DRIVE  /  BIAS  /  DYNAMICS");
    place (s1, { P_Drive, P_Bias, P_Asym, P_Sag, P_SagT });
    auto s2 = add (s1.getRight() + gap, y, 4, "CHARACTER");
    place (s2, { P_S1, P_S2, P_S3, P_S4 });

    y += secH + gap;
    auto s3 = add (x0, y, 4, "FOCUS  (PRE-EMPHASIS)");
    place (s3, { P_FFreq, P_FGain, P_FQ, P_Deemph });
    auto s4 = add (s3.getRight() + gap, y, 4, "TIMBRE  (WARMTH / HARSHNESS)");
    place (s4, { P_Warm, P_WarmF, P_Harsh, P_HarshF });
    auto s5 = add (s4.getRight() + gap, y, 2, "BAND");
    place (s5, { P_Lo, P_Hi });

    y += secH + gap;
    auto s6 = add (x0, y, 7, "TONE EQ  (POST)");
    place (s6, { P_LsF, P_LsG, P_MidF, P_MidG, P_MidQ, P_HsF, P_HsG });
    auto s7 = add (s6.getRight() + gap, y, 2, "OUTPUT");
    place (s7, { P_Mix, P_Out });
    juce::Rectangle<int> s8 { s7.getRight() + gap, y, r.getRight() - (s7.getRight() + gap), secH };
    sections.push_back ({ s8, "STEREO" });
    stereoBox.setBounds (s8.getX() + 10, s8.getY() + 32, s8.getWidth() - 20, 28);
    autoGain.setBounds (s8.getX() + 10, s8.getY() + 74, s8.getWidth() - 20, 28);
}

void SlotPanel::paint (juce::Graphics& g)
{
    for (auto& s : sections)
    {
        auto rf = s.r.toFloat();
        g.setColour (ui::panel);
        g.fillRoundedRectangle (rf, 8.0f);
        g.setColour (ui::edge);
        g.drawRoundedRectangle (rf.reduced (0.5f), 8.0f, 1.0f);
        g.setColour (ui::accent.withAlpha (0.85f));
        g.setFont (juce::Font (juce::FontOptions (11.0f)));
        juce::String title = s.title;
        if (title == "CHARACTER") title += "   -   " + juce::String (getTypeInfo (currentType).name).toUpperCase();
        g.drawText (title, s.r.getX() + 12, s.r.getY() + 6, s.r.getWidth() - 20, 14, juce::Justification::left);
        if (s.title == "CHARACTER" && getTypeInfo (currentType).label[0] == nullptr)
        {
            g.setColour (ui::dim);
            g.drawFittedText ("This circuit has no extra controls.\nShape it with Drive, Bias, Asymmetry, Focus, Warmth and Harshness.",
                              s.r.reduced (16, 36), juce::Justification::centred, 3);
        }
    }
    g.setColour (ui::dim);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("SATURATION TYPE", 2, 0, 200, 14, juce::Justification::left);
    g.setColour (ui::text.withAlpha (0.8f));
    g.setFont (juce::Font (juce::FontOptions (12.5f)));
    g.drawFittedText (getTypeInfo (currentType).hint, hintRect, juce::Justification::topLeft, 3);
}

//==============================================================================
void LevelMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (ui::panel);
    g.fillRoundedRectangle (b, 3.0f);
    const float db = juce::Decibels::gainToDecibels (src.load(), -60.0f);
    const float f = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f);
    auto bar = b.reduced (1.5f);
    bar = bar.withTrimmedTop (bar.getHeight() * (1.0f - f));
    g.setColour (db > 0.0f ? juce::Colours::red : (db > -9.0f ? ui::accent : juce::Colour (0xff5bd68a)));
    g.fillRoundedRectangle (bar, 2.0f);
}

//==============================================================================
// Editeur
//==============================================================================
SatLabEditor::SatLabEditor (SatLabProcessor& p)
    : AudioProcessorEditor (&p), proc (p), inMeter (p.inLevel), outMeter (p.outLevel)
{
    setLookAndFeel (&laf);

    inKnob  = std::make_unique<Knob> (proc.apvts, "in",  "INPUT");
    outKnob = std::make_unique<Knob> (proc.apvts, "out", "OUTPUT");
    mixKnob = std::make_unique<Knob> (proc.apvts, "mix", "DRY / WET");
    for (auto* k : { inKnob.get(), outKnob.get(), mixKnob.get() }) addAndMakeVisible (*k);

    auto& presets = getPresets();
    for (size_t i = 0; i < presets.size(); ++i) presetBox.addItem (presets[i].name, (int) i + 1);
    presetBox.setSelectedItemIndex (juce::jlimit (0, (int) presets.size() - 1, proc.getCurrentProgram()), juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    osBox.addItemList (osNames(), 1);       addAndMakeVisible (osBox);
    routingBox.addItemList (routingNames(), 1); addAndMakeVisible (routingBox);
    osAtt      = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "os", osBox);
    routingAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "routing", routingBox);
    addAndMakeVisible (safety);
    safetyAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "safety", safety);
    addAndMakeVisible (inMeter); addAndMakeVisible (outMeter);

    for (int s = 0; s < kNumSlots; ++s)
    {
        panels[(size_t) s] = std::make_unique<SlotPanel> (proc, s, tone);
        addChildComponent (*panels[(size_t) s]);

        tabs[(size_t) s].setClickingTogglesState (false);
        tabs[(size_t) s].onClick = [this, s] { selectSlot (s); };
        addAndMakeVisible (tabs[(size_t) s]);

        onButtons[(size_t) s].setButtonText ("ON");
        addAndMakeVisible (onButtons[(size_t) s]);
        onAtt[(size_t) s] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, slotId (s, P_On), onButtons[(size_t) s]);

        tabTypeAtt[(size_t) s] = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (slotId (s, P_Type)),
            [this, s] (float v) { tabs[(size_t) s].setButtonText ("SLOT " + juce::String (s + 1) + "   " + getTypeInfo ((int) std::round (v)).name); });
        tabTypeAtt[(size_t) s]->sendInitialUpdate();
    }

    auto setupSlider = [this] (juce::Slider& sl, juce::Label& lb, const juce::String& text)
    {
        sl.setSliderStyle (juce::Slider::LinearHorizontal);
        sl.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 20);
        sl.setColour (juce::Slider::trackColourId, ui::accent);
        sl.setColour (juce::Slider::thumbColourId, ui::text);
        sl.setColour (juce::Slider::backgroundColourId, ui::edge);
        addAndMakeVisible (sl);
        lb.setText (text, juce::dontSendNotification);
        lb.setColour (juce::Label::textColourId, ui::dim);
        addAndMakeVisible (lb);
    };
    setupSlider (testFreq, testFreqLabel, "TEST TONE FREQ");
    setupSlider (testLevel, testLevelLabel, "LEVEL");
    testFreq.setRange (40.0, 10000.0, 1.0);
    testFreq.setSkewFactorFromMidPoint (800.0);
    testFreq.setTextValueSuffix (" Hz");
    testFreq.setValue (1000.0, juce::dontSendNotification);
    testFreq.onValueChange = [this] { tone.freq = (float) testFreq.getValue(); };
    testLevel.setRange (-40.0, 0.0, 0.5);
    testLevel.setTextValueSuffix (" dBFS");
    testLevel.setValue (-12.0, juce::dontSendNotification);
    testLevel.onValueChange = [this] { tone.levelDb = (float) testLevel.getValue(); };

    selectSlot (0);
    setSize (1180, 708);      // en dernier : declenche resized() une fois tous les composants crees
    startTimerHz (30);
}

SatLabEditor::~SatLabEditor() { setLookAndFeel (nullptr); }

void SatLabEditor::selectSlot (int s)
{
    selected = s;
    for (int i = 0; i < kNumSlots; ++i)
    {
        panels[(size_t) i]->setVisible (i == s);
        tabs[(size_t) i].setToggleState (i == s, juce::dontSendNotification);
    }
}

void SatLabEditor::timerCallback()
{
    inMeter.repaint(); outMeter.repaint();
}

void SatLabEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);
    g.setColour (ui::accent);
    g.setFont (juce::Font (juce::FontOptions (30.0f).withStyle ("Bold")));
    g.drawText ("SatLab", 18, 12, 200, 36, juce::Justification::left);
    g.setColour (ui::dim);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("saturation laboratory  -  25 circuits, 4 slots", 20, 46, 300, 16, juce::Justification::left);

    g.setColour (ui::dim);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("PRESET", presetBox.getX(), presetBox.getY() - 15, 100, 13, juce::Justification::left);
    g.drawText ("OVERSAMPLING", osBox.getX(), osBox.getY() - 15, 100, 13, juce::Justification::left);
    g.drawText ("ROUTING", routingBox.getX(), routingBox.getY() - 15, 100, 13, juce::Justification::left);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawText ("IN", inMeter.getX() - 7, inMeter.getBottom() + 2, 24, 12, juce::Justification::centred);
    g.drawText ("OUT", outMeter.getX() - 7, outMeter.getBottom() + 2, 24, 12, juce::Justification::centred);
}

void SatLabEditor::resized()
{
    auto r = getLocalBounds().reduced (12);
    auto header = r.removeFromTop (92);
    r.removeFromTop (6);
    auto tabRow = r.removeFromTop (40);
    r.removeFromTop (8);
    auto footer = r.removeFromBottom (32);

    // en-tete
    auto hr = header;
    outMeter.setBounds (hr.removeFromRight (10).withTrimmedTop (8).withTrimmedBottom (22));
    hr.removeFromRight (16);
    inMeter.setBounds (hr.removeFromRight (10).withTrimmedTop (8).withTrimmedBottom (22));
    hr.removeFromRight (14);
    safety.setBounds (hr.removeFromRight (120).withSizeKeepingCentre (120, 30).translated (0, 14));
    hr.removeFromRight (10);
    routingBox.setBounds (hr.removeFromRight (100).withSizeKeepingCentre (100, 28).translated (0, 14));
    hr.removeFromRight (10);
    osBox.setBounds (hr.removeFromRight (90).withSizeKeepingCentre (90, 28).translated (0, 14));
    hr.removeFromRight (16);
    mixKnob->setBounds (hr.removeFromRight (74));
    outKnob->setBounds (hr.removeFromRight (74));
    inKnob->setBounds (hr.removeFromRight (74));
    presetBox.setBounds (320, header.getY() + 38, 230, 28);

    // onglets
    const int tw = (tabRow.getWidth() - 3 * 8) / 4;
    for (int i = 0; i < kNumSlots; ++i)
    {
        auto t = tabRow.removeFromLeft (tw);
        tabRow.removeFromLeft (8);
        onButtons[(size_t) i].setBounds (t.removeFromLeft (62).reduced (0, 4));
        t.removeFromLeft (4);
        tabs[(size_t) i].setBounds (t);
    }

    for (auto& p : panels) p->setBounds (r);

    // pied
    auto f = footer;
    testFreqLabel.setBounds (f.removeFromLeft (120));
    testFreq.setBounds (f.removeFromLeft (330));
    f.removeFromLeft (20);
    testLevelLabel.setBounds (f.removeFromLeft (50));
    testLevel.setBounds (f.removeFromLeft (300));
}
