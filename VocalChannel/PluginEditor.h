// PluginEditor.h — Silktone Vocal Channel UI (Tron style).
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Shared/TronLookAndFeel.h"
#include "PluginProcessor.h"

class SilktoneVocalChannelEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit SilktoneVocalChannelEditor(SilktoneVocalChannelProcessor&);
    ~SilktoneVocalChannelEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void showPresetMenu();
    void stepPreset(int direction); // Waves/NI-style preset stepping (wraps)
    void updatePresetButton();      // "Preset: <name> ▾" from presetName_

    SilktoneVocalChannelProcessor& processor_;
    tron::LookAndFeel lookAndFeel_;
    juce::String presetName_ = "Airy Vocal";
    int presetIndex_ = 0; // index into kVocalPresets, -1 = Init
    float grDb_ = 0.0f;

    juce::Label titleLabel_, subtitleLabel_;
    juce::TextButton presetButton_, presetPrevButton_, presetNextButton_;

    tron::Knob hpFreqKnob_, lowKnob_, midKnob_, midFreqKnob_, highKnob_;
    tron::Toggle hpToggle_;
    tron::Knob compThrKnob_, compRatioKnob_, compAttackKnob_, compReleaseKnob_, compMakeupKnob_;
    tron::Knob satKnob_, blendKnob_, outputKnob_;
    juce::Label grLabel_, footerNote_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SilktoneVocalChannelEditor)
};
