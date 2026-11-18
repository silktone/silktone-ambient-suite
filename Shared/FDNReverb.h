// FDNReverb.h — lush modulated feedback-delay-network reverb.
// Lexicon 224/480L + Valhalla style: a network of delay lines coupled by
// an orthogonal (Hadamard) feedback matrix, with slow random modulation of
// the delay lengths (the "Spin / Wander" movement that keeps the tail from
// ringing), diffusion allpasses on the input, and an octave-up shimmer
// voice inside the feedback loop (the Eno/Lanois trick).
// Pure C++ (no JUCE). Stereo in, stereo out. Lean: 8 lines, ~64 multiply
// per sample for the feedback matrix, no FFT, no oversampling.
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>
#include "DSPBlocks.h"

namespace silktone
{

struct FDNReverbParams
{
    float size = 0.55f;       // 0..1 — scales the delay lengths (room size)
    float decay = 0.55f;      // 0..1 — RT60 from ~0.3 s to ~12 s
    float damping = 0.5f;     // 0..1 — HF damping in the feedback (0 bright, 1 dark)
    float diffusion = 0.7f;   // 0..1 — how fast echo density builds up
    float modulation = 0.5f;  // 0..1 — Spin/Wander depth (random delay movement)
    float modRate = 0.3f;     // Hz — base modulation rate
    float preDelayMs = 20.0f; // 0..200 ms
    float shimmer = 0.0f;     // 0..1 — octave-up voice fed back into the network
    float earlyLevel = 0.5f;  // 0..1 — early reflections level
    float wetLevel = 1.0f;    // 0..1 — reverb output level
    // Dual space: two independent reverbs, one per side, each with its own
    // character and EQ, blended together. Space A lives on the left,
    // Space B on the right; spaceBlend crossfeeds them (0 = fully
    // separated, 0.5 = each space bleeds halfway to the other side).
    // Reverb type (the palette: hall, plate, room, chamber):
    //   0 = Hall    — long lines, open, the classic lush space
    //   1 = Plate   — short dense lines, bright (vintage EMT 140 flavour)
    //   2 = Room    — tight short lines, less diffuse, drier and closer
    //   3 = Chamber — dense and smooth, the concrete echo-chamber sound
    bool dual = false;
    float characterA = 0.0f;  // reverb type, 0 Hall / 1 Plate / 2 Room / 3 Chamber
    float characterB = 0.0f;
    float eqLowA = 0.0f, eqHighA = 0.0f; // dB, Space A output EQ
    float eqLowB = 0.0f, eqHighB = 0.0f; // dB, Space B output EQ
    float levelA = 1.0f, levelB = 1.0f;  // 0..1 output levels
    float spaceBlend = 0.25f; // 0..0.5 crossfeed between the two spaces
};

class FDNReverb
{
public:
    void prepare(double sampleRate);
    void reset();
    void setParams(const FDNReverbParams& p) { target_ = p; }
    const FDNReverbParams& getParams() const { return target_; }

    // Stereo in/out. in may alias out (sample is read before it is written).
    void process(float* const* in, float* const* out, int numSamples);

private:
    static constexpr int kLines = 8;
    // Base delay lengths in ms (mutually prime-ish, spread for a dense tail).
    static constexpr float kBaseMs[kLines] = {
        149.0f, 211.0f, 263.0f, 293.0f, 347.0f, 401.0f, 457.0f, 521.0f
    };

    float readLine(int line, float delaySamples) const;

    double sr_ = 44100.0;
    FDNReverbParams target_;

    // Pre-delay line (also the early-reflection tap source).
    DelayLine preDelayL_, preDelayR_;
    // Input diffusion: 2 serial allpasses per channel.
    struct Allpass
    {
        DelayLine line;
        float coeff = 0.5f;
        float process(float x, float delaySamples)
        {
            const float d = line.read(delaySamples);
            const float y = -coeff * x + d;
            line.write(x + coeff * y);
            return y;
        }
        void reset() { line.reset(); }
    };
    Allpass diffL_[2], diffR_[2];

    // FDN lines.
    std::vector<std::vector<float>> buf_; // [line][sample]
    std::vector<int> writePos_;
    std::vector<int> maxLen_;
    std::vector<float> lineLen_; // current length in samples (size-scaled)
    OnePoleLP dampLp_[kLines];
    double modPhase_[kLines] = {};
    double modPhase2_[kLines] = {};
    float hadamard_[kLines][kLines] = {};
    float hadamard4_[4][4] = {}; // dual mode: one 4x4 matrix per half
    float feedbackGain_[kLines] = {};
    // Per-side output EQ for dual mode (low shelf + high shelf each).
    Biquad eqLowA_, eqHighA_, eqLowB_, eqHighB_;

    PitchShifterUp shifter_;
    float shimmerSmooth_ = 0.0f;
};

} // namespace silktone
