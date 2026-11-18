// TapeEchoDSP.h — EP-3 style tape echo DSP core.
// No JUCE dependency: pure C++ so it can be unit-tested standalone.
// Stereo, sample-rate aware (44.1–96 kHz), max delay 1.0 s.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

struct TapeEchoParams
{
    float delayMs = 320.0f;      // 80–750 ms (slider 0–35 maps here in the processor)
    float sustain = 0.45f;       // 0–1.02; >1.0 at max = runaway (tamed by tanh limiter)
    float echoVolume = 0.65f;    // wet level, 0–~1.25
    float drive = 0.28f;         // 0–1 tape/record drive: how hard the signal hits
                                 // the tape saturation + preamp. 0.28 = clean-ish EP-3.
    bool  preampOn = true;       // EP-3 preamp colour, always on by default
    float age = 0.35f;           // 0–1 tape wear: darker repeats, more wow/flutter/saturation
    bool  soundOnSound = false;  // SOS looper mode: feedback ~1.0, layers build
    float hissLevel = 0.0f;      // 0 = off (default); up to ~0.0015, very low tape hiss
    float outputTrim = 1.0f;     // linear gain, 1.0 = unity
};

class TapeEchoDSP
{
public:
    TapeEchoDSP() = default;

    void prepare(double sampleRate, int numChannels = 2);
    void reset();
    void clearLoop(); // wipe the tape (SOS clear)
    void setParams(const TapeEchoParams& p) { target_ = p; }
    const TapeEchoParams& getParams() const { return target_; }
    float getCurrentDelayMs() const { return delayMsSmoothed_ * 1000.0f / (float)sampleRate_; }

    // Interleaved stereo not used here: per-channel float buffers, numSamples each.
    // in/out may alias. channels = 1 or 2.
    void process(float* const* in, float* const* out, int numChannels, int numSamples);

    // Single-channel helper used by the test harness.
    void processMono(const float* in, float* out, int numSamples);

private:
    float readDelay(int ch, float delaySamples) const;
    float preamp(float x); // uses driveSmoothed_ for the tanh drive amount
    static float softClip(float x) { return std::tanh(x); }

    double sampleRate_ = 44100.0;
    int numChannels_ = 2;
    std::vector<std::vector<float>> buffer_; // [ch][sample]
    std::vector<int> writePos_;
    int bufferSize_ = 0;

    TapeEchoParams target_;
    // Smoothed (one-pole, ~20–30 ms tau) copies — no zipper noise.
    float delayMsSmoothed_ = 320.0f;
    float sustainSmoothed_ = 0.45f;
    float echoVolSmoothed_ = 0.65f;
    float ageSmoothed_ = 0.35f;
    float driveSmoothed_ = 0.28f;

    // Feedback-loop filter states (one-pole lowpass + highpass), per channel.
    // HP is the standard form y = a*(yPrev + x - xPrev), a = exp(-2*pi*fc/sr).
    std::vector<float> lpState_, hpY_, hpXPrev_;
    // Wow / flutter LFO phases.
    double wowPhase_ = 0.0, flutterPhase_ = 0.0, driftPhase_ = 0.0;
    // Hiss RNG (xorshift, deterministic).
    unsigned int rngState_ = 22222u;
    float nextNoise()
    {
        rngState_ ^= rngState_ << 13; rngState_ ^= rngState_ >> 17; rngState_ ^= rngState_ << 5;
        return (float)((int)rngState_) / 2147483647.0f;
    }
};
