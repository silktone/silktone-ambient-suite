// AmbientDSP.h — the Silktone Ambient multi-effects chain.
// Signal flow:
//   in -> Saturation -> 3-band EQ -> Compressor -> Chorus (modulation)
//      -> Stereo Delay -> Tape Echo (reuses TapeEchoDSP) -> FDN Reverb
//      -> Blend (dry/wet) -> Output trim
// Pure C++ (no JUCE). One instance handles stereo (two per-channel
// instances of the mono blocks, one stereo instance of the stereo blocks).
#pragma once

#include <algorithm>
#include <cmath>
#include "DSPBlocks.h"
#include "FDNReverb.h"
#include "TapeEchoDSP.h"

namespace silktone
{

struct AmbientParams
{
    // Tone
    float satDrive = 0.15f;     // 0..1
    float lowDb = 0.0f, midDb = 0.0f, highDb = 0.0f, midFreqHz = 1000.0f;
    bool compOn = true;
    float compThresholdDb = -18.0f, compRatio = 2.0f;
    float compAttackMs = 12.0f, compReleaseMs = 180.0f, compMakeupDb = 0.0f;
    // Modulation
    bool chorusOn = true;
    float chorusRateHz = 0.8f, chorusDepth = 0.4f, chorusMix = 0.3f;
    // Stereo delay
    bool delayOn = true;
    float delayTimeMs = 320.0f, delayFeedback = 0.35f, delayMix = 0.3f;
    bool delayTape = true;
    // Tape echo (subset of the tape echo's own parameters)
    bool tapeOn = false;
    float tapeTimeMs = 320.0f, tapeFeedback = 0.4f; // 0..1 sustain
    float tapeMix = 0.25f, tapeAge = 0.35f, tapeDrive = 0.28f;
    // Reverb
    bool reverbOn = true;
    FDNReverbParams reverb;
    // Master
    float blend = 0.5f;   // 0 = dry, 1 = full wet chain
    float outputDb = 0.0f;
};

class AmbientDSP
{
public:
    void prepare(double sampleRate);
    void reset();
    void setParams(const AmbientParams& p);
    const AmbientParams& getParams() const { return params_; }

    // Stereo in/out; in may alias out.
    void process(float* const* in, float* const* out, int numSamples);
    float getCompGainReductionDb() const { return compL_.getGainReductionDb(); }

private:
    double sr_ = 44100.0;
    AmbientParams params_;

    ThreeBandEQ eqL_, eqR_;
    OptoCompressor compL_, compR_;
    Chorus chorusL_, chorusR_;
    StereoDelay delay_;
    TapeEchoDSP tape_;
    FDNReverb reverb_;

    float blendSmooth_ = 0.5f;
    float outputSmooth_ = 1.0f;
};

} // namespace silktone
