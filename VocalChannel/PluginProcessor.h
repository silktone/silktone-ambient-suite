// PluginProcessor.h — Silktone Vocal Channel (AU/VST3/Standalone)
// Vocal-first channel strip: high-pass, 3-band EQ, opto compressor,
// saturation, blend, output. Full dry/wet blend, 10 factory presets.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Shared/VocalChannelDSP.h"

class SilktoneVocalChannelProcessor : public juce::AudioProcessor
{
public:
    SilktoneVocalChannelProcessor();
    ~SilktoneVocalChannelProcessor() override = default;

    static constexpr const char* kHpOn       = "hpOn";
    static constexpr const char* kHpFreq     = "hpFreq";
    static constexpr const char* kLow        = "lowEq";
    static constexpr const char* kMid        = "midEq";
    static constexpr const char* kMidFreq    = "midFreq";
    static constexpr const char* kHigh       = "highEq";
    static constexpr const char* kCompThr    = "compThreshold";
    static constexpr const char* kCompRatio  = "compRatio";
    static constexpr const char* kCompAttack = "compAttack";
    static constexpr const char* kCompRelease= "compRelease";
    static constexpr const char* kCompMakeup = "compMakeup";
    static constexpr const char* kSat        = "saturation";
    static constexpr const char* kBlend      = "blend";
    static constexpr const char* kOutput     = "outputDb";

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Silktone Vocal Channel"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void initPatch();
    float getCompGainReductionDb() const { return dsp_.getCompGainReductionDb(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void applyPreset(int index);

    silktone::VocalChannelDSP dsp_;
    int currentProgram_ = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SilktoneVocalChannelProcessor)
};
