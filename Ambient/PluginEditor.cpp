// PluginEditor.cpp — Silktone Ambient UI (Tron style).
#include "PluginEditor.h"
#include "../Shared/Presets.h"

namespace
{
constexpr int kW = 920;
constexpr int kH = 640;
}

SilktoneAmbientEditor::SilktoneAmbientEditor(SilktoneAmbientProcessor& p)
    : AudioProcessorEditor(p), processor_(p)
{
    setLookAndFeel(&lookAndFeel_);
    setSize(kW, kH);

    titleLabel_.setText("SILKTONE  AMBIENT", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions(26.0f, juce::Font::bold)));
    titleLabel_.setColour(juce::Label::textColourId, tron::kText);
    titleLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel_);

    subtitleLabel_.setText("AMBIENT MULTI-EFFECTS  ·  LEXICON / VALHALLA STYLE",
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

    randomizeButton_.setButtonText("RANDOMIZE");
    randomizeButton_.setColour(juce::TextButton::textColourOffId, tron::kOrange);
    randomizeButton_.onClick = [this]
    {
        processor_.randomize();
        presetIndex_ = -1;
        presetName_ = "Randomized";
        updatePresetButton();
    };
    addAndMakeVisible(randomizeButton_);

    // Preset note: what was recorded in the current room, shown as a
    // caption under the preset bar (Joel's ask — e.g. Bowie's "Heroes"
    // at Hansa). Set by updatePresetButton().
    presetNoteLabel_.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::italic)));
    presetNoteLabel_.setColour(juce::Label::textColourId, tron::kDim);
    presetNoteLabel_.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(presetNoteLabel_);

    auto& apvts = processor_.apvts;
    using P = SilktoneAmbientProcessor;
    // TONE
    satKnob_.init(apvts, P::kSat, "Saturation", *this, tron::kOrange);
    lowKnob_.init(apvts, P::kLow, "Low", *this);
    midKnob_.init(apvts, P::kMid, "Mid", *this);
    midFreqKnob_.init(apvts, P::kMidFreq, "Mid Freq", *this);
    highKnob_.init(apvts, P::kHigh, "High", *this);
    compThrKnob_.init(apvts, P::kCompThr, "Comp Thr", *this);
    compRatioKnob_.init(apvts, P::kCompRatio, "Comp Ratio", *this);
    compToggle_.init(apvts, P::kCompOn, "COMP", *this);
    // MODULATION
    chorusRateKnob_.init(apvts, P::kChorusRate, "Rate", *this);
    chorusDepthKnob_.init(apvts, P::kChorusDepth, "Depth", *this);
    chorusMixKnob_.init(apvts, P::kChorusMix, "Mix", *this);
    chorusToggle_.init(apvts, P::kChorusOn, "CHORUS", *this);
    // DELAY
    delayTimeKnob_.init(apvts, P::kDelayTime, "Time", *this);
    delayFdbkKnob_.init(apvts, P::kDelayFdbk, "Feedback", *this);
    delayMixKnob_.init(apvts, P::kDelayMix, "Mix", *this);
    delayToggle_.init(apvts, P::kDelayOn, "DELAY", *this);
    delayTapeToggle_.init(apvts, P::kDelayTape, "TAPE", *this);
    delaySyncToggle_.init(apvts, P::kDelaySync, "SYNC", *this);
    delayDivBox_.addItem("1/4", 1);
    delayDivBox_.addItem("Dotted 1/8", 2);
    delayDivBox_.addItem("1/8", 3);
    delayDivBox_.addItem("1/4 Triplet", 4);
    addAndMakeVisible(delayDivBox_);
    delayDivAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, P::kDelayDiv, delayDivBox_);
    tron::styleLabel(delayDivLabel_, "Division", 10);
    addAndMakeVisible(delayDivLabel_);
    // TAPE ECHO
    tapeTimeKnob_.init(apvts, P::kTapeTime, "Time", *this);
    tapeFdbkKnob_.init(apvts, P::kTapeFdbk, "Feedback", *this);
    tapeMixKnob_.init(apvts, P::kTapeMix, "Mix", *this);
    tapeAgeKnob_.init(apvts, P::kTapeAge, "Tape Age", *this);
    tapeToggle_.init(apvts, P::kTapeOn, "TAPE ECHO", *this);
    // SPACE
    revSizeKnob_.init(apvts, P::kRevSize, "Size", *this);
    revDecayKnob_.init(apvts, P::kRevDecay, "Decay", *this);
    revDampKnob_.init(apvts, P::kRevDamp, "Damping", *this);
    revDiffKnob_.init(apvts, P::kRevDiff, "Diffusion", *this);
    revModKnob_.init(apvts, P::kRevMod, "Movement", *this);
    revShimmerKnob_.init(apvts, P::kRevShimmer, "Shimmer", *this, tron::kOrange);
    revPreDelayKnob_.init(apvts, P::kRevPreDelay, "Pre-Delay", *this);
    revEarlyKnob_.init(apvts, P::kRevEarly, "Early", *this);
    reverbToggle_.init(apvts, P::kReverbOn, "REVERB", *this);
    // Dual space row
    dualToggle_.init(apvts, P::kRevDual, "DUAL SPACE", *this, tron::kOrange);
    charABox_.addItem("Hall", 1);
    charABox_.addItem("Plate", 2);
    charABox_.addItem("Room", 3);
    charABox_.addItem("Chamber", 4);
    addAndMakeVisible(charABox_);
    charAAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, P::kRevCharA, charABox_);
    tron::styleLabel(charALabel_, "Space A", 10);
    addAndMakeVisible(charALabel_);
    charBBox_.addItem("Hall", 1);
    charBBox_.addItem("Plate", 2);
    charBBox_.addItem("Room", 3);
    charBBox_.addItem("Chamber", 4);
    addAndMakeVisible(charBBox_);
    charBAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, P::kRevCharB, charBBox_);
    tron::styleLabel(charBLabel_, "Space B", 10);
    addAndMakeVisible(charBLabel_);
    eqLowAKnob_.init(apvts, P::kRevEqLowA, "A Low", *this);
    eqHighAKnob_.init(apvts, P::kRevEqHighA, "A High", *this);
    eqLowBKnob_.init(apvts, P::kRevEqLowB, "B Low", *this);
    eqHighBKnob_.init(apvts, P::kRevEqHighB, "B High", *this);
    levelAKnob_.init(apvts, P::kRevLevelA, "A Level", *this);
    levelBKnob_.init(apvts, P::kRevLevelB, "B Level", *this);
    spaceBlendKnob_.init(apvts, P::kRevSpaceBlend, "Space Blend", *this, tron::kOrange);
    // Footer — full dry/wet blend (0 = dry, 100 = full wet) + output.
    blendKnob_.init(apvts, P::kBlend, "DRY/WET", *this, tron::kCyan);
    outputKnob_.init(apvts, P::kOutput, "OUTPUT", *this);
    tron::styleLabel(grLabel_, "COMP", 10);
    addAndMakeVisible(grLabel_);
    footerNote_.setText("Blend: dry to full wet.  Randomize: one tap, a new sound.  "
                        "Dual Space: two different reverbs, one per side, each with its own EQ.",
                        juce::dontSendNotification);
    footerNote_.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::italic)));
    footerNote_.setColour(juce::Label::textColourId, tron::kDim);
    footerNote_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(footerNote_);

    startTimerHz(15);
}

void SilktoneAmbientEditor::placeKnobs(std::vector<tron::Knob*> knobs, int x, int y,
                                       int pitch, int w, int h)
{
    int cx = x;
    for (auto* k : knobs)
    {
        k->setBounds(cx, y, w, h);
        cx += pitch;
    }
}

void SilktoneAmbientEditor::updatePresetButton()
{
    presetButton_.setButtonText("Preset: " + presetName_ + " ▾");
    // The note names what was recorded in this room; blank for Init /
    // Randomized, which are not rooms.
    presetNoteLabel_.setText(
        (presetIndex_ >= 0 && presetIndex_ < (int)kAmbientPresets.size())
            ? kAmbientPresets[(size_t)presetIndex_].note
            : "",
        juce::dontSendNotification);
}

void SilktoneAmbientEditor::stepPreset(int direction)
{
    const int count = (int)kAmbientPresets.size();
    if (count == 0) return;
    // From Init/Randomized, step into the list at the near end.
    if (presetIndex_ < 0) presetIndex_ = (direction > 0) ? -1 : 0;
    presetIndex_ = (presetIndex_ + direction + count) % count;
    processor_.setCurrentProgram(presetIndex_);
    presetName_ = kAmbientPresets[(size_t)presetIndex_].name;
    updatePresetButton();
}

void SilktoneAmbientEditor::showPresetMenu()
{
    // Waves / Native Instruments style: preset names grouped under
    // section headers (West Coast Verbs, Soul, ...), with Randomize
    // and Init at the bottom. The tick marks the current preset.
    juce::PopupMenu menu;
    for (size_t i = 0; i < kAmbientPresets.size(); ++i)
    {
        const auto& pr = kAmbientPresets[i];
        if (pr.section != nullptr)
            menu.addSectionHeader(pr.section);
        menu.addItem((int)i + 1, pr.name, true, presetIndex_ == (int)i);
    }
    menu.addSeparator();
    menu.addItem(100, "Randomize");
    menu.addItem(101, "Init (flat)");
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetButton_),
        [this](int result)
        {
            if (result >= 1 && result <= (int)kAmbientPresets.size())
            {
                presetIndex_ = result - 1;
                processor_.setCurrentProgram(presetIndex_);
                presetName_ = kAmbientPresets[(size_t)presetIndex_].name;
                updatePresetButton();
            }
            else if (result == 100)
            {
                processor_.randomize();
                presetIndex_ = -1;
                presetName_ = "Randomized";
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

void SilktoneAmbientEditor::timerCallback()
{
    grDb_ = processor_.getCompGainReductionDb();
    repaint();
}

void SilktoneAmbientEditor::paint(juce::Graphics& g)
{
    g.fillAll(tron::kBg);
    // Faint grid — Tron circuit feel, kept very subtle.
    g.setColour(tron::kCyan.withAlpha(0.03f));
    for (int x = 0; x < getWidth(); x += 40)
        g.drawVerticalLine(x, 0.0f, (float)getHeight());
    for (int y = 0; y < getHeight(); y += 40)
        g.drawHorizontalLine(y, 0.0f, (float)getWidth());

    // Header rule.
    g.setColour(tron::kCyan);
    g.fillRect(24, 52, getWidth() - 48, 1);

    tron::paintSection(g, { 16, 64, 888, 104 }, "TONE  ·  SATURATION / EQ / COMPRESSOR");
    tron::paintSection(g, { 16, 180, 888, 128 }, "MODULATION  ·  STEREO DELAY  ·  TAPE ECHO");
    tron::paintSection(g, { 16, 320, 888, 196 }, "SPACE  ·  LUSH REVERB", tron::kOrange);
    tron::paintSection(g, { 16, 528, 888, 96 }, "MASTER");

    // Sub-group labels inside the middle band.
    g.setColour(tron::kDim);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("MODULATION", 28, 196, 200, 12, juce::Justification::centredLeft);
    g.drawText("STEREO DELAY", 300, 196, 200, 12, juce::Justification::centredLeft);
    g.drawText("TAPE ECHO", 620, 196, 200, 12, juce::Justification::centredLeft);
    g.drawText("DUAL SPACE — TWO REVERBS, ONE PER SIDE", 28, 436, 500, 12,
               juce::Justification::centredLeft);

    // Compressor gain-reduction meter (footer).
    const int mx = 560, my = 566, mw = 180, mh = 10;
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

void SilktoneAmbientEditor::resized()
{
    const int W = getWidth();
    titleLabel_.setBounds(24, 10, 420, 28);
    subtitleLabel_.setBounds(24, 36, 420, 14);
    presetPrevButton_.setBounds(W - 452, 14, 26, 26);
    presetButton_.setBounds(W - 420, 14, 190, 26);
    presetNextButton_.setBounds(W - 224, 14, 26, 26);
    randomizeButton_.setBounds(W - 192, 14, 168, 26);
    presetNoteLabel_.setBounds(W - 452, 42, 428, 14);

    // TONE band: 7 knobs + COMP toggle.
    placeKnobs({ &satKnob_, &lowKnob_, &midKnob_, &midFreqKnob_, &highKnob_,
                 &compThrKnob_, &compRatioKnob_ }, 28, 82, 100, 92, 76);
    compToggle_.button.setBounds(740, 116, 90, 24);

    // MODULATION / DELAY / TAPE band: knobs row + toggle row.
    placeKnobs({ &chorusRateKnob_, &chorusDepthKnob_, &chorusMixKnob_ }, 28, 212, 84, 76, 72);
    chorusToggle_.button.setBounds(28, 288, 90, 22);
    placeKnobs({ &delayTimeKnob_, &delayFdbkKnob_, &delayMixKnob_ }, 300, 212, 84, 76, 72);
    delayToggle_.button.setBounds(300, 288, 70, 22);
    delayTapeToggle_.button.setBounds(376, 288, 70, 22);
    delaySyncToggle_.button.setBounds(452, 288, 60, 22);
    delayDivLabel_.setBounds(520, 288, 60, 22);
    delayDivBox_.setBounds(580, 288, 110, 22);
    placeKnobs({ &tapeTimeKnob_, &tapeFdbkKnob_, &tapeMixKnob_, &tapeAgeKnob_ }, 620, 212, 68, 60, 72);
    tapeToggle_.button.setBounds(620, 288, 100, 22);

    // SPACE band: 8 knobs row 1, dual row 2.
    placeKnobs({ &revSizeKnob_, &revDecayKnob_, &revDampKnob_, &revDiffKnob_, &revModKnob_,
                 &revShimmerKnob_, &revPreDelayKnob_, &revEarlyKnob_ }, 28, 338, 104, 96, 76);
    reverbToggle_.button.setBounds(800, 338, 80, 24);
    dualToggle_.button.setBounds(28, 470, 110, 24);
    charALabel_.setBounds(150, 452, 80, 14);
    charABox_.setBounds(150, 468, 80, 24);
    charBLabel_.setBounds(242, 452, 80, 14);
    charBBox_.setBounds(242, 468, 80, 24);
    placeKnobs({ &eqLowAKnob_, &eqHighAKnob_, &eqLowBKnob_, &eqHighBKnob_,
                 &levelAKnob_, &levelBKnob_, &spaceBlendKnob_ }, 340, 450, 76, 68, 62);

    // Footer: Blend + Output + GR meter.
    blendKnob_.setBounds(28, 544, 110, 70);
    outputKnob_.setBounds(160, 544, 90, 70);
    grLabel_.setBounds(560, 546, 180, 14);
    footerNote_.setBounds(24, 616, W - 48, 14);
}
