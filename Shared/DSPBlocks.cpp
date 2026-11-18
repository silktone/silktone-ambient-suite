// DSPBlocks.cpp — shared DSP building blocks (see DSPBlocks.h).
#include "DSPBlocks.h"

namespace silktone
{
namespace
{
constexpr double kTwoPi = 6.2831853071795864769;
}

// ---------------------------------------------------------------- Biquad ----
void Biquad::setLowShelf(double sr, double freq, double gainDb, double q)
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = kTwoPi * freq / sr;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosw = std::cos(w0);
    const double sqA = std::sqrt(A);
    double b0 = A * ((A + 1.0) - (A - 1.0) * cosw + 2.0 * sqA * alpha);
    double b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosw);
    double b2 = A * ((A + 1.0) - (A - 1.0) * cosw - 2.0 * sqA * alpha);
    double a0 = (A + 1.0) + (A - 1.0) * cosw + 2.0 * sqA * alpha;
    double a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosw);
    double a2 = (A + 1.0) + (A - 1.0) * cosw - 2.0 * sqA * alpha;
    b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
    this->b0 = b0; this->b1 = b1; this->b2 = b2; this->a1 = a1; this->a2 = a2;
}

void Biquad::setHighShelf(double sr, double freq, double gainDb, double q)
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = kTwoPi * freq / sr;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosw = std::cos(w0);
    const double sqA = std::sqrt(A);
    double b0 = A * ((A + 1.0) + (A - 1.0) * cosw + 2.0 * sqA * alpha);
    double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw);
    double b2 = A * ((A + 1.0) + (A - 1.0) * cosw - 2.0 * sqA * alpha);
    double a0 = (A + 1.0) - (A - 1.0) * cosw + 2.0 * sqA * alpha;
    double a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosw);
    double a2 = (A + 1.0) - (A - 1.0) * cosw - 2.0 * sqA * alpha;
    b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
    this->b0 = b0; this->b1 = b1; this->b2 = b2; this->a1 = a1; this->a2 = a2;
}

void Biquad::setPeak(double sr, double freq, double gainDb, double q)
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = kTwoPi * freq / sr;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosw = std::cos(w0);
    double b0 = 1.0 + alpha * A;
    double b1 = -2.0 * cosw;
    double b2 = 1.0 - alpha * A;
    double a0 = 1.0 + alpha / A;
    double a1 = -2.0 * cosw;
    double a2 = 1.0 - alpha / A;
    b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
    this->b0 = b0; this->b1 = b1; this->b2 = b2; this->a1 = a1; this->a2 = a2;
}

void Biquad::setHighPass(double sr, double freq, double q)
{
    const double w0 = kTwoPi * freq / sr;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosw = std::cos(w0);
    double b0 = (1.0 + cosw) / 2.0;
    double b1 = -(1.0 + cosw);
    double b2 = (1.0 + cosw) / 2.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosw;
    double a2 = 1.0 - alpha;
    b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
    this->b0 = b0; this->b1 = b1; this->b2 = b2; this->a1 = a1; this->a2 = a2;
}

void Biquad::setLowPass(double sr, double freq, double q)
{
    const double w0 = kTwoPi * freq / sr;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosw = std::cos(w0);
    double b0 = (1.0 - cosw) / 2.0;
    double b1 = 1.0 - cosw;
    double b2 = (1.0 - cosw) / 2.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosw;
    double a2 = 1.0 - alpha;
    b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
    this->b0 = b0; this->b1 = b1; this->b2 = b2; this->a1 = a1; this->a2 = a2;
}

// ------------------------------------------------------------ ThreeBandEQ ---
void ThreeBandEQ::prepare(double sampleRate)
{
    sr_ = sampleRate > 0 ? sampleRate : 44100.0;
    setParams(lowDb_, midDb_, highDb_, midFreq_);
}

void ThreeBandEQ::reset()
{
    low_.reset(); mid_.reset(); high_.reset();
}

void ThreeBandEQ::setParams(float lowDb, float midDb, float highDb, float midFreqHz)
{
    lowDb_ = lowDb; midDb_ = midDb; highDb_ = highDb; midFreq_ = midFreqHz;
    low_.setLowShelf(sr_, 220.0, lowDb_);
    mid_.setPeak(sr_, clampf(midFreqHz, 100.0f, 12000.0f), midDb_, 0.9);
    high_.setHighShelf(sr_, 6000.0, highDb_);
}

float ThreeBandEQ::process(float x)
{
    return high_.process(mid_.process(low_.process(x)));
}

// --------------------------------------------------------- OptoCompressor ---
void OptoCompressor::prepare(double sampleRate)
{
    sr_ = sampleRate > 0 ? sampleRate : 44100.0;
    reset();
}

void OptoCompressor::reset()
{
    env_ = 0.0f;
    gainSmoothed_ = 1.0f;
    grDbSmoothed_ = 0.0f;
}

void OptoCompressor::setParams(float thresholdDb, float ratio, float attackMs,
                               float releaseMs, float makeupDb)
{
    thresholdDb_ = thresholdDb;
    ratio_ = std::max(1.0f, ratio);
    attackMs_ = std::max(0.1f, attackMs);
    releaseMs_ = std::max(10.0f, releaseMs);
    makeupDb_ = makeupDb;
}

float OptoCompressor::process(float x)
{
    if (!std::isfinite(x)) x = 0.0f;
    const float absx = std::fabs(x);
    // Envelope: fast attack, slow release (opto behaviour).
    const float atk = (float)(1.0 - std::exp(-1.0 / (sr_ * attackMs_ / 1000.0)));
    const float rel = (float)(1.0 - std::exp(-1.0 / (sr_ * releaseMs_ / 1000.0)));
    env_ += (absx > env_ ? atk : rel) * (absx - env_);

    // Gain computer with a 6 dB soft knee.
    const float envDb = 20.0f * std::log10(env_ + 1e-9f);
    const float knee = 6.0f;
    float gainDb = 0.0f;
    const float lo = thresholdDb_ - knee / 2.0f;
    const float hi = thresholdDb_ + knee / 2.0f;
    if (envDb > hi)
    {
        gainDb = (thresholdDb_ - envDb) * (1.0f - 1.0f / ratio_);
    }
    else if (envDb > lo)
    {
        const float t = (envDb - lo) / knee; // 0..1 through the knee
        gainDb = (thresholdDb_ - envDb) * (1.0f - 1.0f / ratio_) * t * t;
    }
    const float targetGain = dbToLin(gainDb + makeupDb_);
    // Smooth the gain itself (attack/release on the gain, not just the env).
    const float gAtk = (float)(1.0 - std::exp(-1.0 / (sr_ * attackMs_ / 1000.0)));
    const float gRel = (float)(1.0 - std::exp(-1.0 / (sr_ * releaseMs_ * 2.0 / 1000.0)));
    gainSmoothed_ += (targetGain < gainSmoothed_ ? gAtk : gRel) * (targetGain - gainSmoothed_);
    grDbSmoothed_ += 0.001f * ((gainDb < 0.0f ? gainDb : 0.0f) - grDbSmoothed_);
    return x * gainSmoothed_;
}

// -------------------------------------------------------------- DelayLine ---
void DelayLine::prepare(double sampleRate, float maxDelayMs)
{
    size_ = (int)(sampleRate * maxDelayMs / 1000.0f) + 64;
    buf_.assign((size_t)size_, 0.0f);
    writePos_ = 0;
}

void DelayLine::reset()
{
    std::fill(buf_.begin(), buf_.end(), 0.0f);
    writePos_ = 0;
}

void DelayLine::write(float x)
{
    buf_[(size_t)writePos_] = x;
    if (++writePos_ >= size_) writePos_ = 0;
}

float DelayLine::read(float delaySamples) const
{
    // Clamp to [1, size-2]: a 0-sample read must return the most recently
    // written sample (writePos already points at the next write slot),
    // and the interpolation needs one sample of headroom.
    delaySamples = clampf(delaySamples, 1.0f, (float)size_ - 2.0f);
    float readPos = (float)writePos_ - delaySamples;
    while (readPos < 0.0f) readPos += (float)size_;
    const int i0 = (int)readPos;
    int i1 = i0 + 1;
    if (i1 >= size_) i1 -= size_;
    const float frac = readPos - (float)i0;
    return buf_[(size_t)i0] * (1.0f - frac) + buf_[(size_t)i1] * frac;
}

// ----------------------------------------------------------------- Chorus ---
void Chorus::prepare(double sampleRate)
{
    sr_ = sampleRate > 0 ? sampleRate : 44100.0;
    line_.prepare(sr_, 40.0f); // up to 40 ms
    reset();
}

void Chorus::reset()
{
    line_.reset();
    phase_ = 0.0;
}

void Chorus::setParams(float rateHz, float depth01, float mix01, float baseDelayMs)
{
    rateHz_ = clampf(rateHz, 0.02f, 10.0f);
    depth01_ = clampf(depth01, 0.0f, 1.0f);
    mix01_ = clampf(mix01, 0.0f, 1.0f);
    baseDelayMs_ = clampf(baseDelayMs, 2.0f, 30.0f);
}

float Chorus::process(float x)
{
    phase_ += kTwoPi * rateHz_ / sr_;
    if (phase_ > kTwoPi) phase_ -= kTwoPi;
    const float depthSamples = depth01_ * 0.0045f * (float)sr_; // up to ~4.5 ms
    const float baseSamples = baseDelayMs_ * (float)sr_ / 1000.0f;
    // Two voices, opposite LFO phase (ensemble width).
    const float lfo1 = (float)std::sin(phase_ + phaseOffset_);
    const float lfo2 = (float)std::sin(phase_ + phaseOffset_ + kTwoPi * 0.5);
    const float v1 = line_.read(baseSamples + lfo1 * depthSamples);
    const float v2 = line_.read(baseSamples + lfo2 * depthSamples);
    line_.write(x);
    const float wet = 0.5f * (v1 + v2);
    return x + wet * mix01_;
}

// ------------------------------------------------------------ StereoDelay ---
void StereoDelay::prepare(double sampleRate)
{
    sr_ = sampleRate > 0 ? sampleRate : 44100.0;
    lineL_.prepare(sr_, 1100.0f);
    lineR_.prepare(sr_, 1100.0f);
    reset();
}

void StereoDelay::reset()
{
    lineL_.reset(); lineR_.reset();
    fbLpL_.reset(); fbLpR_.reset();
    timeSmoothed_ = timeMs_;
}

void StereoDelay::setParams(float timeMs, float feedback01, float mix01, bool tapeCharacter)
{
    timeMs_ = clampf(timeMs, 5.0f, 1000.0f);
    feedback_ = clampf(feedback01, 0.0f, 0.95f);
    mix_ = clampf(mix01, 0.0f, 1.0f);
    tapeCharacter_ = tapeCharacter;
}

void StereoDelay::process(float* const* in, float* const* out, int numSamples)
{
    // Slow slew on delay time: moving the time bends pitch like tape,
    // and a fast jump would chirp instead.
    const float timeCoeff = (float)std::exp(-1.0 / (sr_ * 0.060));
    const float fbCutoff = tapeCharacter_ ? 2200.0f : 6000.0f;
    const float fbCoeff = (float)(1.0 - std::exp(-kTwoPi * fbCutoff / sr_));

    for (int n = 0; n < numSamples; ++n)
    {
        timeSmoothed_ += (timeMs_ - timeSmoothed_) * (1.0f - timeCoeff);
        const float delaySamples = clampf(timeSmoothed_ * (float)sr_ / 1000.0f, 4.0f, 1000.0f * (float)sr_ / 1000.0f);
        const float dl = lineL_.read(delaySamples);
        const float dr = lineR_.read(delaySamples);

        float xL = in[0][n], xR = in[1][n];
        if (!std::isfinite(xL)) xL = 0.0f;
        if (!std::isfinite(xR)) xR = 0.0f;

        // Ping-pong: each channel's feedback comes from the other side.
        float fbL = fbLpL_.process(dr, fbCoeff);
        float fbR = fbLpR_.process(dl, fbCoeff);
        if (tapeCharacter_)
        {
            fbL = std::tanh(fbL * 1.4f) / std::tanh(1.4f);
            fbR = std::tanh(fbR * 1.4f) / std::tanh(1.4f);
        }
        lineL_.write(xL + fbL * feedback_);
        lineR_.write(xR + fbR * feedback_);

        out[0][n] = clampf(xL + dl * mix_, -1.5f, 1.5f);
        out[1][n] = clampf(xR + dr * mix_, -1.5f, 1.5f);
    }
}

// ---------------------------------------------------------- PitchShifterUp --
void PitchShifterUp::prepare(double sampleRate)
{
    (void)sampleRate;
    size_ = kWindow * 2 + 64;
    buf_.assign((size_t)size_, 0.0f);
    reset();
}

void PitchShifterUp::reset()
{
    std::fill(buf_.begin(), buf_.end(), 0.0f);
    writePos_ = 0;
    readPos_ = 0.0f;
    readPos2_ = (float)kWindow * 0.5f;
}

float PitchShifterUp::process(float x)
{
    buf_[(size_t)writePos_] = x;
    if (++writePos_ >= size_) writePos_ = 0;

    // Read pointers advance at 2x (one octave up). When a reader catches
    // the write pointer it wraps back by one window. The two readers sit
    // half a window apart and crossfade, so the wrap is inaudible.
    auto readAt = [&](float pos) -> float
    {
        float p = pos;
        while (p < 0.0f) p += (float)size_;
        while (p >= (float)size_) p -= (float)size_;
        const int i0 = (int)p;
        int i1 = i0 + 1;
        if (i1 >= size_) i1 -= size_;
        const float frac = p - (float)i0;
        return buf_[(size_t)i0] * (1.0f - frac) + buf_[(size_t)i1] * frac;
    };

    readPos_ += 2.0f;
    readPos2_ += 2.0f;
    // Distance from read pointer to write pointer (how far behind we are).
    auto distToWrite = [&](float pos) -> float
    {
        float d = (float)writePos_ - pos;
        while (d < 0.0f) d += (float)size_;
        return d;
    };
    if (distToWrite(readPos_) < 4.0f) readPos_ -= (float)kWindow;
    if (distToWrite(readPos2_) < 4.0f) readPos2_ -= (float)kWindow;

    // Raised-cosine window over the reader's position in its window.
    auto windowGain = [&](float pos) -> float
    {
        const float d = distToWrite(pos); // 0..window
        const float t = clampf(d / (float)kWindow, 0.0f, 1.0f);
        return 0.5f - 0.5f * (float)std::cos(kTwoPi * t);
    };
    const float w1 = windowGain(readPos_);
    const float w2 = windowGain(readPos2_);
    const float y = readAt(readPos_) * w1 + readAt(readPos2_) * w2;
    const float norm = w1 + w2;
    return norm > 0.05f ? y / norm : 0.0f;
}

} // namespace silktone
