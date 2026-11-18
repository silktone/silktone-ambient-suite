// PluginProcessor.cpp — Silktone Vocal Channel
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../Shared/Presets.h"

SilktoneVocalChannelProcessor::SilktoneVocalChannelProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SilktoneVocalChannel", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
SilktoneVocalChannelProcessor::createParameterLayout()
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

    b(kHpOn, "High Pass", true);
    f(kHpFreq, "High Pass Freq", 20.0f, 400.0f, 80.0f, 0.4f);
    f(kLow, "Low EQ", -12.0f, 12.0f, 0.0f);
    f(kMid, "Mid EQ", -12.0f, 12.0f, 0.0f);
    f(kMidFreq, "Mid Freq", 200.0f, 8000.0f, 2500.0f, 0.35f);
    f(kHigh, "High EQ", -12.0f, 12.0f, 0.0f);
    f(kCompThr, "Comp Threshold", -40.0f, 0.0f, -20.0f);
    f(kCompRatio, "Comp Ratio", 1.0f, 10.0f, 3.0f);
    f(kCompAttack, "Comp Attack", 1.0f, 100.0f, 8.0f, 0.4f);
    f(kCompRelease, "Comp Release", 50.0f, 1000.0f, 200.0f, 0.4f);
    f(kCompMakeup, "Comp Makeup", 0.0f, 12.0f, 4.0f);
    f(kSat, "Saturation", 0.0f, 100.0f, 10.0f);
    f(kBlend, "Blend", 0.0f, 100.0f, 100.0f);
    f(kOutput, "Output", -12.0f, 12.0f, 0.0f);
    return { params.begin(), params.end() };
}

void SilktoneVocalChannelProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    dsp_.prepare(sampleRate);
    dsp_.reset();
}

void SilktoneVocalChannelProcessor::releaseResources() { dsp_.reset(); }

void SilktoneVocalChannelProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    auto getF = [&](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    auto getB = [&](const char* id) { return getF(id) > 0.5f; };

    silktone::VocalChannelParams p;
    p.hpOn = getB(kHpOn);
    p.hpFreqHz = getF(kHpFreq);
    p.lowDb = getF(kLow);
    p.midDb = getF(kMid);
    p.midFreqHz = getF(kMidFreq);
    p.highDb = getF(kHigh);
    p.compThresholdDb = getF(kCompThr);
    p.compRatio = getF(kCompRatio);
    p.compAttackMs = getF(kCompAttack);
    p.compReleaseMs = getF(kCompRelease);
    p.compMakeupDb = getF(kCompMakeup);
    p.satDrive = getF(kSat) / 100.0f;
    p.blend = getF(kBlend) / 100.0f;
    p.outputDb = getF(kOutput);
    dsp_.setParams(p);

    std::vector<float*> ptrs((size_t)numChannels);
    for (int ch = 0; ch < numChannels; ++ch)
        ptrs[(size_t)ch] = buffer.getWritePointer(ch);
    dsp_.process(ptrs.data(), ptrs.data(), numSamples);
}

juce::AudioProcessorEditor* SilktoneVocalChannelProcessor::createEditor()
{
    return new SilktoneVocalChannelEditor(*this);
}

int SilktoneVocalChannelProcessor::getNumPrograms() { return (int)kVocalPresets.size(); }
int SilktoneVocalChannelProcessor::getCurrentProgram()
{
    return juce::jlimit(0, getNumPrograms() - 1, currentProgram_);
}

void SilktoneVocalChannelProcessor::applyPreset(int index)
{
    if (index < 0 || index >= (int)kVocalPresets.size()) return;
    const auto& pr = kVocalPresets[(size_t)index];
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
    setBool(kHpOn, pr.hpOn);
    setFloat(kHpFreq, pr.hpFreqHz);
    setFloat(kLow, pr.lowDb);
    setFloat(kMid, pr.midDb);
    setFloat(kMidFreq, pr.midFreqHz);
    setFloat(kHigh, pr.highDb);
    setFloat(kCompThr, pr.compThrDb);
    setFloat(kCompRatio, pr.compRatio);
    setFloat(kCompAttack, pr.compAttackMs);
    setFloat(kCompRelease, pr.compReleaseMs);
    setFloat(kCompMakeup, pr.compMakeupDb);
    setFloat(kSat, pr.sat);
    setFloat(kBlend, pr.blend);
    setFloat(kOutput, pr.outputDb);
    currentProgram_ = index;
}

void SilktoneVocalChannelProcessor::setCurrentProgram(int index)
{
    applyPreset(index);
    currentProgram_ = juce::jlimit(0, getNumPrograms() - 1, index);
}

const juce::String SilktoneVocalChannelProcessor::getProgramName(int index)
{
    if (index < 0 || index >= (int)kVocalPresets.size()) return "Unknown";
    return kVocalPresets[(size_t)index].name;
}

void SilktoneVocalChannelProcessor::initPatch()
{
    // JUCE 8 removed AudioProcessorValueTreeState::getParameters();
    // the processor's own getParameters() returns the same list.
    for (auto* param : getParameters())
        param->setValueNotifyingHost(param->getDefaultValue());
    currentProgram_ = -1;
}

void SilktoneVocalChannelProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void SilktoneVocalChannelProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SilktoneVocalChannelProcessor();
}
