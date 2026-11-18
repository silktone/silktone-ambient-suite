// PluginEditor.h — Silktone Ambient UI (Tron style).
// Sections: TONE (saturation / EQ / compressor), MODULATION + DELAY + TAPE
// ECHO, SPACE (reverb, incl. dual-space row), footer (Blend / Output / GR).
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Shared/TronLookAndFeel.h"
#include "PluginProcessor.h"

class SilktoneAmbientEditor : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit SilktoneAmbientEditor(SilktoneAmbientProcessor&);
    ~SilktoneAmbientEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void showPresetMenu();
    void stepPreset(int direction); // Waves/NI-style preset stepping (wraps)
    void updatePresetButton();      // "Preset: <name> ▾" from presetName_
    void placeKnobs(std::vector<tron::Knob*> knobs, int x, int y,
                    int pitch, int w, int h);

    SilktoneAmbientProcessor& processor_;
    tron::LookAndFeel lookAndFeel_;
    juce::String presetName_ = "Westlake Room A";
    int presetIndex_ = 0; // index into kAmbientPresets, -1 = Init/Randomized
    float grDb_ = 0.0f;

    juce::Label titleLabel_, subtitleLabel_, presetNoteLabel_;
    juce::TextButton presetButton_, presetPrevButton_, presetNextButton_, randomizeButton_;

    // TONE
    tron::Knob satKnob_, lowKnob_, midKnob_, midFreqKnob_, highKnob_, compThrKnob_, compRatioKnob_;
    tron::Toggle compToggle_;
    // MODULATION
    tron::Knob chorusRateKnob_, chorusDepthKnob_, chorusMixKnob_;
    tron::Toggle chorusToggle_;
    // DELAY
    tron::Knob delayTimeKnob_, delayFdbkKnob_, delayMixKnob_;
    tron::Toggle delayToggle_, delayTapeToggle_, delaySyncToggle_;
    juce::ComboBox delayDivBox_;
    juce::Label delayDivLabel_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> delayDivAttachment_;
    // TAPE ECHO
    tron::Knob tapeTimeKnob_, tapeFdbkKnob_, tapeMixKnob_, tapeAgeKnob_;
    tron::Toggle tapeToggle_;
    // SPACE (reverb)
    tron::Knob revSizeKnob_, revDecayKnob_, revDampKnob_, revDiffKnob_, revModKnob_,
        revShimmerKnob_, revPreDelayKnob_, revEarlyKnob_;
    tron::Toggle reverbToggle_;
    // Dual space row
    tron::Toggle dualToggle_;
    juce::ComboBox charABox_, charBBox_;
    juce::Label charALabel_, charBLabel_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> charAAttachment_, charBAttachment_;
    tron::Knob eqLowAKnob_, eqHighAKnob_, eqLowBKnob_, eqHighBKnob_,
        levelAKnob_, levelBKnob_, spaceBlendKnob_;
    // Footer
    tron::Knob blendKnob_, outputKnob_;
    juce::Label footerNote_, grLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SilktoneAmbientEditor)
};
