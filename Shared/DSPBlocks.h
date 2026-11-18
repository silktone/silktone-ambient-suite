// DSPBlocks.h — shared DSP building blocks for the Silktone ambient suite.
// Pure C++ (no JUCE) so every block is unit-testable standalone.
// All blocks are stereo-safe by construction: use one instance per channel
// unless the block explicitly says it handles stereo (StereoDelay).
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace silktone
{

inline float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}
inline float dbToLin(float db) { return std::pow(10.0f, db / 20.0f); }

// ---------------------------------------------------------------------------
// One-pole filters (cheap tone shaping in feedback loops).
// ---------------------------------------------------------------------------
struct OnePoleLP
{
    float y = 0.0f;
    void reset() { y = 0.0f; }
    float process(float x, float coeff) // coeff = 1 - exp(-2*pi*fc/sr)
    {
        y += coeff * (x - y);
        if (std::fabs(y) < 1e-18f) y = 0.0f; // flush denormals
        return y;
    }
};

struct OnePoleHP
{
    float y = 0.0f, xPrev = 0.0f;
    void reset() { y = 0.0f; xPrev = 0.0f; }
    float process(float x, float alpha) // alpha = exp(-2*pi*fc/sr)
    {
        float out = alpha * (y + x - xPrev);
        xPrev = x;
        y = out;
        if (std::fabs(y) < 1e-18f) y = 0.0f;
        return out;
    }
};

// ---------------------------------------------------------------------------
// RBJ biquad (Direct Form II transposed). One instance per channel.
// ---------------------------------------------------------------------------
struct Biquad
{
    void reset() { z1 = 0.0; z2 = 0.0; }

    void setLowShelf(double sr, double freq, double gainDb, double q = 0.7071);
    void setHighShelf(double sr, double freq, double gainDb, double q = 0.7071);
    void setPeak(double sr, double freq, double gainDb, double q = 1.0);
    void setHighPass(double sr, double freq, double q = 0.7071);
    void setLowPass(double sr, double freq, double q = 0.7071);

    float process(float x)
    {
        double out = b0 * x + z1;
        z1 = b1 * x - a1 * out + z2;
        z2 = b2 * x - a2 * out;
        return (float)out;
    }

    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    double z1 = 0.0, z2 = 0.0;
};

// ---------------------------------------------------------------------------
// 3-band EQ: low shelf, mid peak, high shelf. One instance per channel.
// ---------------------------------------------------------------------------
struct ThreeBandEQ
{
    void prepare(double sampleRate);
    void reset();
    // Gains in dB (-12..+12), midFreq in Hz. Call when parameters change
    // (per block is fine — coefficient jumps are small and click-free).
    void setParams(float lowDb, float midDb, float highDb, float midFreqHz);
    float process(float x);

private:
    Biquad low_, mid_, high_;
    double sr_ = 44100.0;
    float lowDb_ = 0.0f, midDb_ = 0.0f, highDb_ = 0.0f, midFreq_ = 1000.0f;
};

// ---------------------------------------------------------------------------
// Saturation: tanh drive with level compensation (same idea as the tape
// echo preamp — thicker and more compressed as you push it, not just
// louder). One instance per channel (stateless).
// ---------------------------------------------------------------------------
struct Saturation
{
    // drive01: 0 = clean, 1 = fully driven.
    static float process(float x, float drive01)
    {
        const float k = 1.0f + drive01 * 2.6f;
        const float norm = std::tanh(k);
        return std::tanh(k * x) / norm * (1.0f - 0.18f * drive01) + x * 0.18f * drive01;
    }
};

// ---------------------------------------------------------------------------
// Opto-style compressor: feedforward, soft knee, smooth attack/release.
// Gentle by design — made for vocals, not for squashing.
// ---------------------------------------------------------------------------
struct OptoCompressor
{
    void prepare(double sampleRate);
    void reset();
    void setParams(float thresholdDb, float ratio, float attackMs,
                   float releaseMs, float makeupDb);
    float process(float x);
    float getGainReductionDb() const { return grDbSmoothed_; }

private:
    double sr_ = 44100.0;
    float thresholdDb_ = -20.0f, ratio_ = 3.0f;
    float attackMs_ = 8.0f, releaseMs_ = 200.0f, makeupDb_ = 0.0f;
    float env_ = 0.0f;        // level envelope (linear)
    float gainSmoothed_ = 1.0f; // smoothed linear gain
    float grDbSmoothed_ = 0.0f;
};

// ---------------------------------------------------------------------------
// Fractional delay line (linear interpolation). One instance per channel.
// ---------------------------------------------------------------------------
struct DelayLine
{
    void prepare(double sampleRate, float maxDelayMs);
    void reset();
    void write(float x);
    float read(float delaySamples) const;

    int writePos_ = 0;
    int size_ = 0;
    std::vector<float> buf_;
};

// ---------------------------------------------------------------------------
// Chorus: two modulated delay voices, opposite LFO phase (ensemble width).
// One instance per channel; the two channels share nothing, so feed them
// LFO phases 90 degrees apart for a wide stereo image (set stereoPhase).
// ---------------------------------------------------------------------------
struct Chorus
{
    void prepare(double sampleRate);
    void reset();
    void setParams(float rateHz, float depth01, float mix01, float baseDelayMs = 11.0f);
    void setStereoPhase(double phaseOffset) { phaseOffset_ = phaseOffset; }
    float process(float x);

private:
    DelayLine line_;
    double sr_ = 44100.0;
    double phase_ = 0.0, phaseOffset_ = 0.0;
    float rateHz_ = 0.8f, depth01_ = 0.4f, mix01_ = 0.3f, baseDelayMs_ = 11.0f;
};

// ---------------------------------------------------------------------------
// Stereo ping-pong delay: cross-coupled feedback, darkening lowpass in the
// loop, optional tape character (saturation + extra darkening, in the spirit
// of the tape echo's feedback loop). Dry + wet mix inside.
// ---------------------------------------------------------------------------
struct StereoDelay
{
    void prepare(double sampleRate);
    void reset();
    // timeMs 10..1000, feedback 0..0.95, mix 0..1, tapeCharacter on/off.
    void setParams(float timeMs, float feedback01, float mix01, bool tapeCharacter);
    void process(float* const* in, float* const* out, int numSamples);

private:
    DelayLine lineL_, lineR_;
    OnePoleLP fbLpL_, fbLpR_;
    double sr_ = 44100.0;
    float timeMs_ = 320.0f, feedback_ = 0.35f, mix_ = 0.3f;
    bool tapeCharacter_ = true;
    float timeSmoothed_ = 320.0f; // slow slew = pitch bend on time changes
};

// ---------------------------------------------------------------------------
// Octave-up pitch shifter (dual-reader modulated delay, crossfaded).
// Built for shimmer: it lives inside a reverb feedback loop, so it does
// not need to be pristine — it needs to be cheap and stable.
// ---------------------------------------------------------------------------
struct PitchShifterUp
{
    void prepare(double sampleRate);
    void reset();
    float process(float x);

private:
    static constexpr int kWindow = 2048; // ~46 ms at 44.1 kHz
    std::vector<float> buf_;
    int size_ = 0;
    int writePos_ = 0;
    float readPos_ = 0.0f; // advances at 2x (one octave up)
    float readPos2_ = 0.0f;
};

} // namespace silktone
