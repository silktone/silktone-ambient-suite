// PluginProcessor.cpp — Silktone Ambient
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../Shared/Presets.h"
#include <cmath>
#include <random>

SilktoneAmbientProcessor::SilktoneAmbientProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SilktoneAmbient", createParameterLayout())
{
    // Open on the first factory preset (Westlake Room A) so the plugin
    // starts on a real researched space, not the generic defaults.
    applyPreset(0);
}

juce::AudioProcessorValueTreeState::ParameterLayout
SilktoneAmbientProcessor::createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    auto f = [&](const char* id, const char* name, float lo, float hi, float def, float skew = 0.0f)
    {
        if (skew > 0.0f)
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                id, name, Range(lo, hi, 0.01f, skew), def));
        else
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                id, name, Range(lo, hi, 0.01f), def));
    };
    auto b = [&](const char* id, const char* name, bool def)
    {
        params.push_back(std::make_unique<juce::AudioParameterBool>(id, name, def));
    };
    auto c = [&](const char* id, const char* name, juce::StringArray choices, int def)
    {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(id, name, choices, def));
    };

    // Tone
    f(kSat, "Saturation", 0.0f, 100.0f, 15.0f);
    f(kLow, "Low EQ", -12.0f, 12.0f, 0.0f);
    f(kMid, "Mid EQ", -12.0f, 12.0f, 0.0f);
    f(kMidFreq, "Mid Freq", 200.0f, 8000.0f, 1000.0f, 0.35f);
    f(kHigh, "High EQ", -12.0f, 12.0f, 0.0f);
    b(kCompOn, "Comp", true);
    f(kCompThr, "Comp Threshold", -40.0f, 0.0f, -18.0f);
    f(kCompRatio, "Comp Ratio", 1.0f, 8.0f, 2.0f);
    // Modulation
    b(kChorusOn, "Chorus", true);
    f(kChorusRate, "Chorus Rate", 0.05f, 5.0f, 0.8f, 0.4f);
    f(kChorusDepth, "Chorus Depth", 0.0f, 100.0f, 40.0f);
    f(kChorusMix, "Chorus Mix", 0.0f, 100.0f, 30.0f);
    // Stereo delay
    b(kDelayOn, "Delay", true);
    f(kDelayTime, "Delay Time", 10.0f, 1000.0f, 320.0f, 0.4f);
    f(kDelayFdbk, "Delay Feedback", 0.0f, 95.0f, 35.0f);
    f(kDelayMix, "Delay Mix", 0.0f, 100.0f, 30.0f);
    b(kDelayTape, "Tape Character", true);
    b(kDelaySync, "Delay Sync", false);
    c(kDelayDiv, "Delay Division", { "1/4", "Dotted 1/8", "1/8", "1/4 Triplet" }, 0);
    // Tape echo
    b(kTapeOn, "Tape Echo", false);
    f(kTapeTime, "Tape Time", 80.0f, 750.0f, 320.0f, 0.4f);
    f(kTapeFdbk, "Tape Feedback", 1.0f, 9.0f, 4.0f);
    f(kTapeMix, "Tape Mix", 1.0f, 9.0f, 4.5f);
    f(kTapeAge, "Tape Age", 0.0f, 10.0f, 3.5f);
    // Reverb
    b(kReverbOn, "Reverb", true);
    f(kRevSize, "Space Size", 0.0f, 100.0f, 55.0f);
    f(kRevDecay, "Decay", 0.0f, 100.0f, 55.0f);
    f(kRevDamp, "Damping", 0.0f, 100.0f, 50.0f);
    f(kRevDiff, "Diffusion", 0.0f, 100.0f, 70.0f);
    f(kRevMod, "Movement", 0.0f, 100.0f, 50.0f);
    f(kRevShimmer, "Shimmer", 0.0f, 100.0f, 0.0f);
    f(kRevPreDelay, "Pre-Delay", 0.0f, 200.0f, 20.0f);
    f(kRevEarly, "Early Reflections", 0.0f, 100.0f, 50.0f);
    b(kRevDual, "Dual Space", false);
    c(kRevCharA, "Space A", { "Hall", "Plate", "Room", "Chamber" }, 0);
    c(kRevCharB, "Space B", { "Hall", "Plate", "Room", "Chamber" }, 0);
    f(kRevEqLowA, "Space A Low EQ", -12.0f, 12.0f, 0.0f);
    f(kRevEqHighA, "Space A High EQ", -12.0f, 12.0f, 0.0f);
    f(kRevEqLowB, "Space B Low EQ", -12.0f, 12.0f, 0.0f);
    f(kRevEqHighB, "Space B High EQ", -12.0f, 12.0f, 0.0f);
    f(kRevLevelA, "Space A Level", 0.0f, 100.0f, 100.0f);
    f(kRevLevelB, "Space B Level", 0.0f, 100.0f, 100.0f);
    f(kRevSpaceBlend, "Space Blend", 0.0f, 50.0f, 25.0f);
    // Master
    f(kBlend, "Blend", 0.0f, 100.0f, 50.0f);
    f(kOutput, "Output", -12.0f, 12.0f, 0.0f);
    return { params.begin(), params.end() };
}

void SilktoneAmbientProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    dsp_.prepare(sampleRate);
    dsp_.reset();
}

void SilktoneAmbientProcessor::releaseResources() { dsp_.reset(); }

float SilktoneAmbientProcessor::syncedDelayMs() const
{
    auto* playHead = getPlayHead();
    if (playHead == nullptr) return -1.0f;
    auto pos = playHead->getPosition();
    if (!pos.hasValue()) return -1.0f;
    auto bpm = pos->getBpm();
    if (!bpm.hasValue() || *bpm <= 0.0) return -1.0f;
    const float beatMs = 60000.0f / (float)*bpm;
    const int div = (int)apvts.getRawParameterValue(kDelayDiv)->load();
    switch (div)
    {
        case 1: return beatMs * 0.75f;
        case 2: return beatMs * 0.5f;
        case 3: return beatMs * (2.0f / 3.0f);
        default: return beatMs;
    }
}

void SilktoneAmbientProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    auto getF = [&](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    auto getB = [&](const char* id) { return getF(id) > 0.5f; };

    silktone::AmbientParams p;
    p.satDrive = getF(kSat) / 100.0f;
    p.lowDb = getF(kLow);
    p.midDb = getF(kMid);
    p.midFreqHz = getF(kMidFreq);
    p.highDb = getF(kHigh);
    p.compOn = getB(kCompOn);
    p.compThresholdDb = getF(kCompThr);
    p.compRatio = getF(kCompRatio);
    p.compAttackMs = 12.0f;
    p.compReleaseMs = 180.0f;
    p.compMakeupDb = 0.0f;

    p.chorusOn = getB(kChorusOn);
    p.chorusRateHz = getF(kChorusRate);
    p.chorusDepth = getF(kChorusDepth) / 100.0f;
    p.chorusMix = getF(kChorusMix) / 100.0f;

    p.delayOn = getB(kDelayOn);
    float delayMs = getF(kDelayTime);
    if (getB(kDelaySync))
    {
        const float synced = syncedDelayMs();
        if (synced > 0.0f) delayMs = juce::jlimit(10.0f, 1000.0f, synced);
    }
    p.delayTimeMs = delayMs;
    p.delayFeedback = getF(kDelayFdbk) / 100.0f;
    p.delayMix = getF(kDelayMix) / 100.0f;
    p.delayTape = getB(kDelayTape);

    p.tapeOn = getB(kTapeOn);
    p.tapeTimeMs = getF(kTapeTime);
    p.tapeFeedback = ((getF(kTapeFdbk) - 1.0f) / 8.0f) * 1.02f;
    p.tapeMix = ((getF(kTapeMix) - 1.0f) / 8.0f);
    p.tapeAge = getF(kTapeAge) / 10.0f;
    p.tapeDrive = 0.28f;

    p.reverbOn = getB(kReverbOn);
    p.reverb.size = getF(kRevSize) / 100.0f;
    p.reverb.decay = getF(kRevDecay) / 100.0f;
    p.reverb.damping = getF(kRevDamp) / 100.0f;
    p.reverb.diffusion = getF(kRevDiff) / 100.0f;
    p.reverb.modulation = getF(kRevMod) / 100.0f;
    p.reverb.modRate = 0.3f;
    p.reverb.preDelayMs = getF(kRevPreDelay);
    p.reverb.shimmer = getF(kRevShimmer) / 100.0f;
    p.reverb.earlyLevel = getF(kRevEarly) / 100.0f;
    p.reverb.wetLevel = 1.0f;
    p.reverb.dual = getB(kRevDual);
    p.reverb.characterA = (float)apvts.getRawParameterValue(kRevCharA)->load();
    p.reverb.characterB = (float)apvts.getRawParameterValue(kRevCharB)->load();
    p.reverb.eqLowA = getF(kRevEqLowA);
    p.reverb.eqHighA = getF(kRevEqHighA);
    p.reverb.eqLowB = getF(kRevEqLowB);
    p.reverb.eqHighB = getF(kRevEqHighB);
    p.reverb.levelA = getF(kRevLevelA) / 100.0f;
    p.reverb.levelB = getF(kRevLevelB) / 100.0f;
    p.reverb.spaceBlend = getF(kRevSpaceBlend) / 100.0f;

    p.blend = getF(kBlend) / 100.0f;
    p.outputDb = getF(kOutput);
    dsp_.setParams(p);

    if (numChannels == 1)
    {
        // Mono buffer (auval's 1-channel test, a mono Logic track): the
        // DSP chain is stereo, so upmix the single channel to stereo
        // scratch, run the chain, and take the left channel back out.
        monoInL_.assign((size_t)numSamples, 0.0f);
        monoInR_.assign((size_t)numSamples, 0.0f);
        monoOutL_.assign((size_t)numSamples, 0.0f);
        monoOutR_.assign((size_t)numSamples, 0.0f);
        const float* src = buffer.getReadPointer(0);
        for (int i = 0; i < numSamples; ++i)
        {
            monoInL_[(size_t)i] = src[i];
            monoInR_[(size_t)i] = src[i];
        }
        float* inPtrs[2] = { monoInL_.data(), monoInR_.data() };
        float* outPtrs[2] = { monoOutL_.data(), monoOutR_.data() };
        dsp_.process(inPtrs, outPtrs, numSamples);
        float* dst = buffer.getWritePointer(0);
        for (int i = 0; i < numSamples; ++i)
            dst[i] = monoOutL_[(size_t)i];
        return;
    }

    std::vector<float*> ptrs((size_t)numChannels);
    for (int ch = 0; ch < numChannels; ++ch)
        ptrs[(size_t)ch] = buffer.getWritePointer(ch);
    dsp_.process(ptrs.data(), ptrs.data(), numSamples);
}

juce::AudioProcessorEditor* SilktoneAmbientProcessor::createEditor()
{
    return new SilktoneAmbientEditor(*this);
}

// --- Factory presets ---
int SilktoneAmbientProcessor::getNumPrograms() { return (int)kAmbientPresets.size(); }
int SilktoneAmbientProcessor::getCurrentProgram()
{
    return juce::jlimit(0, getNumPrograms() - 1, currentProgram_);
}

void SilktoneAmbientProcessor::applyPreset(int index)
{
    if (index < 0 || index >= (int)kAmbientPresets.size()) return;
    const auto& pr = kAmbientPresets[(size_t)index];
    auto setFloat = [this](const char* id, float v)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(v));
    };
    auto setBool = [this](const char* id, bool v)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(v ? 1.0f : 0.0f);
    };
    auto setChoice = [this](const char* id, int v)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1((float)v));
    };
    setFloat(kSat, pr.sat);
    setFloat(kLow, pr.lowDb);
    setFloat(kMid, pr.midDb);
    setFloat(kMidFreq, pr.midFreqHz);
    setFloat(kHigh, pr.highDb);
    setBool(kCompOn, pr.compOn);
    setFloat(kCompThr, pr.compThrDb);
    setFloat(kCompRatio, pr.compRatio);
    setBool(kChorusOn, pr.chorusOn);
    setFloat(kChorusRate, pr.chorusRateHz);
    setFloat(kChorusDepth, pr.chorusDepth);
    setFloat(kChorusMix, pr.chorusMix);
    setBool(kDelayOn, pr.delayOn);
    setFloat(kDelayTime, pr.delayTimeMs);
    setFloat(kDelayFdbk, pr.delayFdbk);
    setFloat(kDelayMix, pr.delayMix);
    setBool(kDelayTape, pr.delayTape);
    setBool(kTapeOn, pr.tapeOn);
    setFloat(kTapeTime, pr.tapeTimeMs);
    setFloat(kTapeFdbk, pr.tapeFdbk);
    setFloat(kTapeMix, pr.tapeMix);
    setFloat(kTapeAge, pr.tapeAge);
    setBool(kReverbOn, pr.reverbOn);
    setFloat(kRevSize, pr.revSize);
    setFloat(kRevDecay, pr.revDecay);
    setFloat(kRevDamp, pr.revDamp);
    setFloat(kRevDiff, pr.revDiff);
    setFloat(kRevMod, pr.revMod);
    setFloat(kRevShimmer, pr.revShimmer);
    setFloat(kRevPreDelay, pr.revPreDelayMs);
    setFloat(kRevEarly, pr.revEarly);
    setBool(kRevDual, pr.revDual);
    setChoice(kRevCharA, (int)pr.revCharA);
    setChoice(kRevCharB, (int)pr.revCharB);
    setFloat(kRevEqLowA, pr.revEqLowA);
    setFloat(kRevEqHighA, pr.revEqHighA);
    setFloat(kRevEqLowB, pr.revEqLowB);
    setFloat(kRevEqHighB, pr.revEqHighB);
    setFloat(kRevLevelA, pr.revLevelA * 100.0f);
    setFloat(kRevLevelB, pr.revLevelB * 100.0f);
    setFloat(kRevSpaceBlend, pr.revSpaceBlend * 100.0f);
    setFloat(kBlend, pr.blend);
    setFloat(kOutput, pr.outputDb);
    currentProgram_ = index;
}

void SilktoneAmbientProcessor::setCurrentProgram(int index)
{
    applyPreset(index);
    currentProgram_ = juce::jlimit(0, getNumPrograms() - 1, index);
}

const juce::String SilktoneAmbientProcessor::getProgramName(int index)
{
    if (index < 0 || index >= (int)kAmbientPresets.size()) return "Unknown";
    return kAmbientPresets[(size_t)index].name;
}

void SilktoneAmbientProcessor::randomize()
{
    std::random_device rd;
    std::mt19937 rng(rd());
    auto uniform = [&](float lo, float hi)
    {
        return std::uniform_real_distribution<float>(lo, hi)(rng);
    };
    auto pick = [&](std::initializer_list<float> values)
    {
        std::vector<float> v(values);
        return v[(size_t)std::uniform_int_distribution<size_t>(0, v.size() - 1)(rng)];
    };
    auto setFloat = [this](const char* id, float v)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(v));
    };
    auto setBool = [this](const char* id, bool v)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(v ? 1.0f : 0.0f);
    };
    auto setChoice = [this](const char* id, int v)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1((float)v));
    };

    // Tone: gentle, never extreme.
    setFloat(kSat, uniform(0.0f, 35.0f));
    setFloat(kLow, uniform(-4.0f, 4.0f));
    setFloat(kMid, uniform(-3.0f, 3.0f));
    setFloat(kMidFreq, pick({ 400.0f, 800.0f, 1200.0f, 2000.0f, 3000.0f }));
    setFloat(kHigh, uniform(-2.0f, 5.0f));
    setBool(kCompOn, true);
    setFloat(kCompThr, uniform(-24.0f, -10.0f));
    setFloat(kCompRatio, uniform(1.5f, 3.0f));
    // Modulation.
    setBool(kChorusOn, uniform(0.0f, 1.0f) > 0.25f);
    setFloat(kChorusRate, uniform(0.2f, 2.5f));
    setFloat(kChorusDepth, uniform(20.0f, 80.0f));
    setFloat(kChorusMix, uniform(10.0f, 50.0f));
    // Stereo delay: musical times.
    setBool(kDelayOn, uniform(0.0f, 1.0f) > 0.3f);
    setFloat(kDelayTime, pick({ 130.0f, 190.0f, 250.0f, 320.0f, 380.0f, 470.0f, 580.0f }));
    setFloat(kDelayFdbk, uniform(20.0f, 60.0f));
    setFloat(kDelayMix, uniform(15.0f, 50.0f));
    setBool(kDelayTape, uniform(0.0f, 1.0f) > 0.4f);
    // Tape echo: on about half the time.
    const bool tapeOn = uniform(0.0f, 1.0f) > 0.5f;
    setBool(kTapeOn, tapeOn);
    setFloat(kTapeTime, pick({ 140.0f, 220.0f, 300.0f, 380.0f, 460.0f }));
    setFloat(kTapeFdbk, uniform(2.5f, 6.0f));
    setFloat(kTapeMix, uniform(2.5f, 6.5f));
    setFloat(kTapeAge, uniform(2.0f, 7.0f));
    // Reverb: the heart of the random sound.
    setBool(kReverbOn, true);
    setFloat(kRevSize, uniform(30.0f, 90.0f));
    setFloat(kRevDecay, uniform(35.0f, 90.0f));
    setFloat(kRevDamp, uniform(20.0f, 80.0f));
    setFloat(kRevDiff, uniform(50.0f, 100.0f));
    setFloat(kRevMod, uniform(20.0f, 90.0f));
    setFloat(kRevShimmer, uniform(0.0f, 1.0f) > 0.6f ? uniform(20.0f, 60.0f) : 0.0f);
    setFloat(kRevPreDelay, pick({ 0.0f, 15.0f, 30.0f, 55.0f, 80.0f }));
    setFloat(kRevEarly, uniform(20.0f, 80.0f));
    // Dual space: on ~40% of the time, with contrasting types.
    const bool dual = uniform(0.0f, 1.0f) > 0.6f;
    setBool(kRevDual, dual);
    setChoice(kRevCharA, std::uniform_int_distribution<int>(0, 3)(rng));
    setChoice(kRevCharB, std::uniform_int_distribution<int>(0, 3)(rng));
    setFloat(kRevEqLowA, uniform(-3.0f, 3.0f));
    setFloat(kRevEqHighA, uniform(-3.0f, 3.0f));
    setFloat(kRevEqLowB, uniform(-3.0f, 3.0f));
    setFloat(kRevEqHighB, uniform(-3.0f, 3.0f));
    setFloat(kRevLevelA, uniform(70.0f, 100.0f));
    setFloat(kRevLevelB, uniform(70.0f, 100.0f));
    setFloat(kRevSpaceBlend, uniform(0.0f, 40.0f));
    // Master.
    setFloat(kBlend, uniform(30.0f, 80.0f));
    setFloat(kOutput, 0.0f);
    currentProgram_ = -1; // not a factory preset any more
}

void SilktoneAmbientProcessor::initPatch()
{
    // JUCE 8 removed AudioProcessorValueTreeState::getParameters();
    // the processor's own getParameters() returns the same list.
    for (auto* param : getParameters())
        param->setValueNotifyingHost(param->getDefaultValue());
    currentProgram_ = -1;
}

void SilktoneAmbientProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void SilktoneAmbientProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SilktoneAmbientProcessor();
}
