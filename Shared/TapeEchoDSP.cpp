// TapeEchoDSP.cpp — EP-3 style tape echo DSP core.
// Signal flow per channel:
//   input -> [EP-3 preamp, optional] -> tape write (input + filtered feedback)
//   tape (circular buffer, fractional read, smoothed delay time = pitch bend)
//   read -> wet out (x Echo Volume) + dry
// Feedback loop: one-pole lowpass (repeats get darker) + highpass (~80 Hz)
//   + tanh saturation, so repeats saturate gently and Sustain > 1 stays musical.
#include "TapeEchoDSP.h"

namespace
{
constexpr double kTwoPi = 6.2831853071795864769;
float clampf(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }
} // namespace

void TapeEchoDSP::prepare(double sampleRate, int numChannels)
{
    sampleRate_ = sampleRate > 0 ? sampleRate : 44100.0;
    numChannels_ = std::max(1, std::min(2, numChannels));
    bufferSize_ = (int)(sampleRate_ * 1.05) + 1024; // max 1.0 s + interpolation headroom
    buffer_.assign(numChannels_, std::vector<float>(bufferSize_, 0.0f));
    writePos_.assign(numChannels_, 0);
    lpState_.assign(numChannels_, 0.0f);
    hpY_.assign(numChannels_, 0.0f);
    hpXPrev_.assign(numChannels_, 0.0f);
    delayMsSmoothed_ = target_.delayMs;
    sustainSmoothed_ = target_.sustain;
    echoVolSmoothed_ = target_.echoVolume;
    ageSmoothed_ = target_.age;
    driveSmoothed_ = target_.drive;
}

void TapeEchoDSP::reset()
{
    for (auto& b : buffer_) std::fill(b.begin(), b.end(), 0.0f);
    std::fill(writePos_.begin(), writePos_.end(), 0);
    std::fill(lpState_.begin(), lpState_.end(), 0.0f);
    std::fill(hpY_.begin(), hpY_.end(), 0.0f);
    std::fill(hpXPrev_.begin(), hpXPrev_.end(), 0.0f);
    wowPhase_ = flutterPhase_ = driftPhase_ = 0.0;
    delayMsSmoothed_ = target_.delayMs;
    sustainSmoothed_ = target_.sustain;
    echoVolSmoothed_ = target_.echoVolume;
    ageSmoothed_ = target_.age;
    driveSmoothed_ = target_.drive;
}

void TapeEchoDSP::clearLoop() { reset(); }

float TapeEchoDSP::readDelay(int ch, float delaySamples) const
{
    // Fractional read with linear interpolation. Read position trails write pos.
    float readPos = (float)writePos_[ch] - delaySamples;
    while (readPos < 0.0f) readPos += (float)bufferSize_;
    int i0 = (int)readPos;
    int i1 = i0 + 1;
    if (i1 >= bufferSize_) i1 -= bufferSize_;
    float frac = readPos - (float)i0;
    const auto& buf = buffer_[ch];
    return buf[i0] * (1.0f - frac) + buf[i1] * frac;
}

float TapeEchoDSP::preamp(float x)
{
    // EP-3 preamp: small-signal lift + tanh soft-clip, driven by Drive.
    // k = 1.0 at Drive 0 (clean, almost no clip) -> 3.2 at Drive 100%
    // (thick, compressed). tanh(k*x)/tanh(k) is unity at x=1 and lifts
    // small signals, so the stage stays roughly level-matched.
    const float k = 1.0f + driveSmoothed_ * 2.2f;
    const float norm = std::tanh(k);
    return std::tanh(k * x) / norm * 0.92f + x * 0.08f;
}

void TapeEchoDSP::process(float* const* in, float* const* out, int numChannels, int numSamples)
{
    if (buffer_.empty() || numSamples <= 0) return;
    const int chs = std::max(1, std::min(numChannels, numChannels_));

    // One-pole smoothing coefficients. Delay uses a slow tau (~80 ms) on
    // purpose: slewing the tape head bends pitch like the hardware, and a
    // fast slew would chirp instead of bending.
    const float delayCoeff = (float)std::exp(-1.0 / (sampleRate_ * 0.080));
    const float paramCoeff = (float)std::exp(-1.0 / (sampleRate_ * 0.015));

    for (int n = 0; n < numSamples; ++n)
    {
        // Smooth parameters per sample (cheap, click-free).
        delayMsSmoothed_ += (target_.delayMs - delayMsSmoothed_) * (1.0f - delayCoeff);
        sustainSmoothed_ += (target_.sustain - sustainSmoothed_) * (1.0f - paramCoeff);
        echoVolSmoothed_ += (target_.echoVolume - echoVolSmoothed_) * (1.0f - paramCoeff);
        ageSmoothed_ += (target_.age - ageSmoothed_) * (1.0f - paramCoeff);
        driveSmoothed_ += (clampf(target_.drive, 0.0f, 1.0f) - driveSmoothed_) * (1.0f - paramCoeff);

        const float age = clampf(ageSmoothed_, 0.0f, 1.0f);
        // Wow/flutter: 0.25 Hz wow + 0.9 Hz flutter + 0.07 Hz drift, in ms.
        wowPhase_ += kTwoPi * 0.25 / sampleRate_;
        flutterPhase_ += kTwoPi * 0.90 / sampleRate_;
        driftPhase_ += kTwoPi * 0.07 / sampleRate_;
        if (wowPhase_ > kTwoPi) wowPhase_ -= kTwoPi;
        if (flutterPhase_ > kTwoPi) flutterPhase_ -= kTwoPi;
        if (driftPhase_ > kTwoPi) driftPhase_ -= kTwoPi;
        const float wowMs = (0.35f + age * 1.8f) * (float)std::sin(wowPhase_)
                          + (0.15f + age * 0.9f) * (float)std::sin(flutterPhase_)
                          + (0.20f + age * 0.6f) * (float)std::sin(driftPhase_ + 1.3);
        // Feedback-loop lowpass cutoff: fresh tape ~3.2 kHz, worn tape ~1.8 kHz.
        const float lpCutoff = 3200.0f - age * 1400.0f;
        const float lpCoeff = (float)(1.0 - std::exp(-kTwoPi * lpCutoff / sampleRate_));
        const float hpAlpha = (float)std::exp(-kTwoPi * 80.0 / sampleRate_); // ~0.989
        // Feedback saturation grows with Drive as well as Age: low drive =
        // clean warm repeats, high drive = thicker, more compressed repeats.
        const float satDrive = (1.0f + age * 0.6f) * (0.7f + driveSmoothed_ * 1.3f);
        // Tape-write saturation: 0.6 (clean) -> 2.2 (hot). The tanh limiter
        // in the feedback loop keeps even max Drive from blowing up.
        const float writeDrive = 0.6f + driveSmoothed_ * 1.6f;
        const float writeNorm = std::tanh(writeDrive);
        const float writeMakeup = 1.0f / (1.0f + 0.25f * driveSmoothed_);
        const float effSustain = target_.soundOnSound ? 1.0f
            : clampf(sustainSmoothed_, 0.0f, 1.02f);

        for (int ch = 0; ch < chs; ++ch)
        {
            float x = in[ch][n];
            if (!std::isfinite(x)) x = 0.0f;
            const float dry = x;
            if (target_.preampOn) x = preamp(x);

            float delaySamples = delayMsSmoothed_ * (float)sampleRate_ / 1000.0f
                               + wowMs * (float)sampleRate_ / 1000.0f;
            delaySamples = clampf(delaySamples, 8.0f, (float)bufferSize_ - 16.0f);
            const float delayed = readDelay(ch, delaySamples);

            // Feedback loop: darken (lowpass), tighten lows (highpass), saturate.
            float fb = delayed;
            lpState_[ch] += lpCoeff * (fb - lpState_[ch]);
            fb = lpState_[ch];
            // Standard one-pole highpass (~80 Hz): passes transients, cuts DC/rumble.
            float hpY = hpAlpha * (hpY_[ch] + fb - hpXPrev_[ch]);
            hpXPrev_[ch] = fb;
            hpY_[ch] = hpY;
            fb = hpY;
            fb = softClip(fb * satDrive) / softClip(satDrive);

            // Tape write: input + feedback. Drive sets how hard the signal
            // hits the tape — tanh saturation, level-compensated so turning
            // Drive up thickens the sound instead of just making it louder.
            float toTape = x + fb * effSustain;
            toTape = softClip(toTape * writeDrive) / writeNorm * writeMakeup;
            if (!std::isfinite(toTape)) toTape = 0.0f;
            buffer_[ch][writePos_[ch]] = toTape;
            if (++writePos_[ch] >= bufferSize_) writePos_[ch] = 0;

            float wet = delayed;
            if (target_.hissLevel > 0.0f)
                wet += nextNoise() * target_.hissLevel;
            float y = (dry + wet * echoVolSmoothed_) * target_.outputTrim;
            // Final safety clamp (should never engage in normal use:
            // the feedback loop is already tanh-limited).
            y = clampf(y, -1.5f, 1.5f);
            if (!std::isfinite(y)) y = 0.0f;
            out[ch][n] = y;
        }
    }
}

void TapeEchoDSP::processMono(const float* in, float* out, int numSamples)
{
    float* inPtr[1] = { const_cast<float*>(in) };
    float* outPtr[1] = { out };
    // Mono path through the stereo engine would use ch 0 only; do a direct
    // call with 1 channel so the test harness is unambiguous.
    process(inPtr, outPtr, 1, numSamples);
}
