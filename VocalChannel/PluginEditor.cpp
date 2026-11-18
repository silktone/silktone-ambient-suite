// PluginEditor.cpp — Silktone Vocal Channel UI (Tron style).
#include "PluginEditor.h"
#include "../Shared/Presets.h"

SilktoneVocalChannelEditor::SilktoneVocalChannelEditor(SilktoneVocalChannelProcessor& p)
    : AudioProcessorEditor(p), processor_(p)
{
    setLookAndFeel(&lookAndFeel_);
    setSize(680, 420);

    titleLabel_.setText("SILKTONE  VOCAL  CHANNEL", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions(24.0f, juce::Font::bold)));
    titleLabel_.setColour(juce::Label::textColourId, tron::kText);
    titleLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel_);

    subtitleLabel_.setText("VOCAL-FIRST CHANNEL STRIP  ·  EQ / COMP / SATURATION",
                           juce::dontSendNotification);
    subtitleLabel_.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::plain)));
    subtitleLabel_.setColour(juce::Label::textColourId, tron::kCyan);
    subtitleLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(subtitleLabel_);

    // Preset bar, Waves / Native Instruments style: step arrows on either
    // side of the preset name, click the name for the full menu.
    presetPrevButton_.setButtonText("◀");
    presetPrevButton_.onClick = [this] { stepPreset(-1); };
    addAndMakeVisible(presetPrevButton_);
    presetButton_.onClick = [this] { showPresetMenu(); };
    addAndMakeVisible(presetButton_);
    presetNextButton_.setButtonText("▶");
    presetNextButton_.onClick = [this] { stepPreset(1); };
    addAndMakeVisible(presetNextButton_);
    updatePresetButton();

    auto& apvts = processor_.apvts;
    using P = SilktoneVocalChannelProcessor;
    hpFreqKnob_.init(apvts, P::kHpFreq, "High Pass", *this);
    lowKnob_.init(apvts, P::kLow, "Low", *this);
    midKnob_.init(apvts, P::kMid, "Mid", *this);
    midFreqKnob_.init(apvts, P::kMidFreq, "Mid Freq", *this);
    highKnob_.init(apvts, P::kHigh, "High", *this);
    hpToggle_.init(apvts, P::kHpOn, "HP", *this);
    compThrKnob_.init(apvts, P::kCompThr, "Threshold", *this);
    compRatioKnob_.init(apvts, P::kCompRatio, "Ratio", *this);
    compAttackKnob_.init(apvts, P::kCompAttack, "Attack", *this);
    compReleaseKnob_.init(apvts, P::kCompRelease, "Release", *this);
    compMakeupKnob_.init(apvts, P::kCompMakeup, "Makeup", *this);
    satKnob_.init(apvts, P::kSat, "Saturation", *this, tron::kOrange);
    blendKnob_.init(apvts, P::kBlend, "DRY/WET", *this, tron::kCyan);
    outputKnob_.init(apvts, P::kOutput, "OUTPUT", *this);
    tron::styleLabel(grLabel_, "COMP", 10);
    addAndMakeVisible(grLabel_);
    footerNote_.setText("Blend: dry to full processed.  Made for vocals — also great on "
                        "guitar, keys and a mix bus.",
                        juce::dontSendNotification);
    footerNote_.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::italic)));
    footerNote_.setColour(juce::Label::textColourId, tron::kDim);
    footerNote_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(footerNote_);

    startTimerHz(15);
}

void SilktoneVocalChannelEditor::updatePresetButton()
{
    presetButton_.setButtonText("Preset: " + presetName_ + " ▾");
}

void SilktoneVocalChannelEditor::stepPreset(int direction)
{
    const int count = (int)kVocalPresets.size();
    if (count == 0) return;
    // From Init, step into the list at the near end.
    if (presetIndex_ < 0) presetIndex_ = (direction > 0) ? -1 : 0;
    presetIndex_ = (presetIndex_ + direction + count) % count;
    processor_.setCurrentProgram(presetIndex_);
    presetName_ = kVocalPresets[(size_t)presetIndex_].name;
    updatePresetButton();
}

void SilktoneVocalChannelEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    for (size_t i = 0; i < kVocalPresets.size(); ++i)
        menu.addItem((int)i + 1, kVocalPresets[i].name, true,
                     presetIndex_ == (int)i);
    menu.addSeparator();
    menu.addItem(101, "Init (flat)");
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetButton_),
        [this](int result)
        {
            if (result >= 1 && result <= (int)kVocalPresets.size())
            {
                presetIndex_ = result - 1;
                processor_.setCurrentProgram(presetIndex_);
                presetName_ = kVocalPresets[(size_t)presetIndex_].name;
                updatePresetButton();
            }
            else if (result == 101)
            {
                processor_.initPatch();
                presetIndex_ = -1;
                presetName_ = "Init";
                updatePresetButton();
            }
        });
}

void SilktoneVocalChannelEditor::timerCallback()
{
    grDb_ = processor_.getCompGainReductionDb();
    repaint();
}

void SilktoneVocalChannelEditor::paint(juce::Graphics& g)
{
    g.fillAll(tron::kBg);
    g.setColour(tron::kCyan.withAlpha(0.03f));
    for (int x = 0; x < getWidth(); x += 40)
        g.drawVerticalLine(x, 0.0f, (float)getHeight());
    for (int y = 0; y < getHeight(); y += 40)
        g.drawHorizontalLine(y, 0.0f, (float)getWidth());

    g.setColour(tron::kCyan);
    g.fillRect(24, 52, getWidth() - 48, 1);

    tron::paintSection(g, { 16, 60, 648, 104 }, "EQ  ·  HIGH-PASS + 3-BAND");
    tron::paintSection(g, { 16, 176, 648, 104 }, "COMPRESSOR  ·  OPTO, GENTLE", tron::kOrange);
    tron::paintSection(g, { 16, 292, 648, 96 }, "MASTER");

    // Compressor gain-reduction meter.
    const int mx = 420, my = 336, mw = 180, mh = 10;
    g.setColour(tron::kPanelEdge);
    g.fillRect(mx, my, mw, mh);
    const float gr = juce::jlimit(0.0f, 24.0f, -grDb_);
    if (gr > 0.05f)
    {
        g.setColour(tron::kOrange);
        g.fillRect(mx, my, (int)(mw * gr / 24.0f), mh);
    }
    g.setColour(tron::kLine);
    g.drawRect(mx, my, mw, mh, 1);
}

void SilktoneVocalChannelEditor::resized()
{
    const int W = getWidth();
    titleLabel_.setBounds(24, 10, 380, 28);
    subtitleLabel_.setBounds(24, 36, 380, 14);
    presetPrevButton_.setBounds(W - 270, 14, 26, 26);
    presetButton_.setBounds(W - 238, 14, 182, 26);
    presetNextButton_.setBounds(W - 50, 14, 26, 26);

    auto place = [](std::vector<tron::Knob*> knobs, int x, int y, int pitch, int w, int h)
    {
        int cx = x;
        for (auto* k : knobs) { k->setBounds(cx, y, w, h); cx += pitch; }
    };
    place({ &hpFreqKnob_, &lowKnob_, &midKnob_, &midFreqKnob_, &highKnob_ }, 28, 78, 104, 96, 76);
    hpToggle_.button.setBounds(560, 116, 70, 24);
    place({ &compThrKnob_, &compRatioKnob_, &compAttackKnob_, &compReleaseKnob_, &compMakeupKnob_ },
          28, 194, 104, 96, 76);
    satKnob_.setBounds(28, 310, 100, 66);
    blendKnob_.setBounds(150, 310, 110, 66);
    outputKnob_.setBounds(280, 310, 90, 66);
    grLabel_.setBounds(420, 314, 180, 14);
    footerNote_.setBounds(24, 396, W - 48, 14);
}
