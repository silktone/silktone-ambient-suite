// PluginProcessor.h — Silktone Ambient (AU/VST3/Standalone)
// Centerpiece ambient multi-effects: saturation, 3-band EQ, compressor,
// modulation (chorus), stereo delay, tape echo, lush modulated FDN reverb
// (Hall / vintage Plate, dual-space mode with per-side EQ), blend, output.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Shared/AmbientDSP.h"

class SilktoneAmbientProcessor : public juce::AudioProcessor
{
public:
    SilktoneAmbientProcessor();
    ~SilktoneAmbientProcessor() override = default;

    // Parameter IDs (stable — used by saved state / automation).
    static constexpr const char* kSat        = "saturation";
    static constexpr const char* kLow        = "lowEq";
    static constexpr const char* kMid        = "midEq";
    static constexpr const char* kMidFreq    = "midFreq";
    static constexpr const char* kHigh       = "highEq";
    static constexpr const char* kCompOn     = "compOn";
    static constexpr const char* kCompThr    = "compThreshold";
    static constexpr const char* kCompRatio  = "compRatio";
    static constexpr const char* kChorusOn   = "chorusOn";
    static constexpr const char* kChorusRate = "chorusRate";
    static constexpr const char* kChorusDepth= "chorusDepth";
    static constexpr const char* kChorusMix  = "chorusMix";
    static constexpr const char* kDelayOn    = "delayOn";
    static constexpr const char* kDelayTime  = "delayTime";
    static constexpr const char* kDelayFdbk  = "delayFeedback";
    static constexpr const char* kDelayMix   = "delayMix";
    static constexpr const char* kDelayTape  = "delayTape";
    static constexpr const char* kDelaySync  = "delaySync";
    static constexpr const char* kDelayDiv   = "delayDivision";
    static constexpr const char* kTapeOn     = "tapeOn";
    static constexpr const char* kTapeTime   = "tapeTime";
    static constexpr const char* kTapeFdbk   = "tapeFeedback";
    static constexpr const char* kTapeMix    = "tapeMix";
    static constexpr const char* kTapeAge    = "tapeAge";
    static constexpr const char* kReverbOn   = "reverbOn";
    static constexpr const char* kRevSize    = "revSize";
    static constexpr const char* kRevDecay   = "revDecay";
    static constexpr const char* kRevDamp    = "revDamping";
    static constexpr const char* kRevDiff    = "revDiffusion";
    static constexpr const char* kRevMod     = "revMovement";
    static constexpr const char* kRevShimmer = "revShimmer";
    static constexpr const char* kRevPreDelay= "revPreDelay";
    static constexpr const char* kRevEarly   = "revEarly";
    static constexpr const char* kRevDual    = "revDual";
    static constexpr const char* kRevCharA   = "revCharA";
    static constexpr const char* kRevCharB   = "revCharB";
    static constexpr const char* kRevEqLowA  = "revEqLowA";
    static constexpr const char* kRevEqHighA = "revEqHighA";
    static constexpr const char* kRevEqLowB  = "revEqLowB";
    static constexpr const char* kRevEqHighB = "revEqHighB";
    static constexpr const char* kRevLevelA  = "revLevelA";
    static constexpr const char* kRevLevelB  = "revLevelB";
    static constexpr const char* kRevSpaceBlend = "revSpaceBlend";
    static constexpr const char* kBlend      = "blend";
    static constexpr const char* kOutput     = "outputDb";

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Silktone Ambient"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // One tap for a new sound: randomises the musical parameters within
    // sane ranges (never silence, never blow-up).
    void randomize();
    // Reset every parameter to its default (Init patch).
    void initPatch();
    float getCompGainReductionDb() const { return dsp_.getCompGainReductionDb(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void applyPreset(int index);
    float syncedDelayMs() const;

    silktone::AmbientDSP dsp_;
    int currentProgram_ = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SilktoneAmbientProcessor)
};
