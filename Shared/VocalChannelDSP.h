// VocalChannelDSP.h — the Silktone Vocal Channel strip.
// Signal flow:
//   in -> High-pass -> 3-band EQ -> Opto Compressor -> Saturation
//      -> Blend (dry/wet) -> Output trim
// Vocal-first: gentle compressor, air on top, warmth underneath — but it
// works on anything (guitar, keys, bass, a whole mix bus in a pinch).
// Pure C++ (no JUCE).
#pragma once

#include <algorithm>
#include <cmath>
#include "DSPBlocks.h"

namespace silktone
{

struct VocalChannelParams
{
    bool hpOn = true;
    float hpFreqHz = 80.0f; // 20..400
    float lowDb = 0.0f, midDb = 0.0f, highDb = 0.0f, midFreqHz = 2500.0f;
    float compThresholdDb = -20.0f, compRatio = 3.0f;
    float compAttackMs = 8.0f, compReleaseMs = 200.0f, compMakeupDb = 4.0f;
    float satDrive = 0.10f; // 0..1
    float blend = 1.0f;     // 0 = dry, 1 = full processed
    float outputDb = 0.0f;
};

class VocalChannelDSP
{
public:
    void prepare(double sampleRate);
    void reset();
    void setParams(const VocalChannelParams& p);
    const VocalChannelParams& getParams() const { return params_; }

    // Stereo in/out; in may alias out.
    void process(float* const* in, float* const* out, int numSamples);
    float getCompGainReductionDb() const { return compL_.getGainReductionDb(); }

private:
    double sr_ = 44100.0;
    VocalChannelParams params_;

    Biquad hpL_, hpR_;
    ThreeBandEQ eqL_, eqR_;
    OptoCompressor compL_, compR_;

    float blendSmooth_ = 1.0f;
    float outputSmooth_ = 1.0f;
};

} // namespace silktone
