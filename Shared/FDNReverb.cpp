// FDNReverb.cpp — modulated feedback-delay-network reverb (see FDNReverb.h).
#include "FDNReverb.h"

namespace silktone
{
namespace
{
constexpr double kTwoPi = 6.2831853071795864769;
constexpr float kInvSqrt8 = 0.3535533905932738f; // 1/sqrt(8)
} // namespace

void FDNReverb::prepare(double sampleRate)
{
    sr_ = sampleRate > 0 ? sampleRate : 44100.0;
    preDelayL_.prepare(sr_, 250.0f);
    preDelayR_.prepare(sr_, 250.0f);
    for (int i = 0; i < 2; ++i)
    {
        diffL_[i].line.prepare(sr_, 30.0f);
        diffR_[i].line.prepare(sr_, 30.0f);
    }
    // Longest line at max size scale (size 1 -> scale 1.7): 521 ms * 1.7.
    buf_.assign(kLines, {});
    writePos_.assign(kLines, 0);
    maxLen_.assign(kLines, 0);
    lineLen_.assign(kLines, 0.0f);
    for (int i = 0; i < kLines; ++i)
    {
        maxLen_[(size_t)i] = (int)(kBaseMs[i] * 1.75f * sr_ / 1000.0f) + 64;
        buf_[(size_t)i].assign((size_t)maxLen_[(size_t)i], 0.0f);
    }
    // 8x8 Hadamard matrix (Sylvester construction), normalised so the
    // network is energy-preserving (orthogonal feedback = stable).
    // H2 = [[1,1],[1,-1]]; H_{2n} = [[H_n, H_n],[H_n, -H_n]].
    float m[8][8] = {};
    m[0][0] = 1.0f; m[0][1] = 1.0f; m[1][0] = 1.0f; m[1][1] = -1.0f;
    for (int n = 2; n < 8; n *= 2)
    {
        float tmp[8][8] = {};
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
            {
                tmp[i][j] = m[i][j];
                tmp[i][j + n] = m[i][j];
                tmp[i + n][j] = m[i][j];
                tmp[i + n][j + n] = -m[i][j];
            }
        for (int i = 0; i < 2 * n; ++i)
            for (int j = 0; j < 2 * n; ++j)
                m[i][j] = tmp[i][j];
    }
    for (int i = 0; i < kLines; ++i)
        for (int j = 0; j < kLines; ++j)
            hadamard_[i][j] = m[i][j] * kInvSqrt8;
    // 4x4 Hadamard for dual mode (one per half): H4 = [[H2,H2],[H2,-H2]] / 2.
    const float h4[4][4] = {
        { 0.5f, 0.5f, 0.5f, 0.5f },
        { 0.5f, -0.5f, 0.5f, -0.5f },
        { 0.5f, 0.5f, -0.5f, -0.5f },
        { 0.5f, -0.5f, -0.5f, 0.5f },
    };
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            hadamard4_[i][j] = h4[i][j];

    shifter_.prepare(sr_);
    reset();
}

void FDNReverb::reset()
{
    preDelayL_.reset(); preDelayR_.reset();
    for (int i = 0; i < 2; ++i) { diffL_[i].reset(); diffR_[i].reset(); }
    for (int i = 0; i < kLines; ++i)
    {
        std::fill(buf_[(size_t)i].begin(), buf_[(size_t)i].end(), 0.0f);
        writePos_[(size_t)i] = 0;
        dampLp_[i].reset();
        modPhase_[i] = 0.0;
        modPhase2_[i] = 0.0;
    }
    shifter_.reset();
    shimmerSmooth_ = 0.0f;
    eqLowA_.reset(); eqHighA_.reset(); eqLowB_.reset(); eqHighB_.reset();
}

float FDNReverb::readLine(int line, float delaySamples) const
{
    const auto& buf = buf_[(size_t)line];
    const int size = maxLen_[(size_t)line];
    float readPos = (float)writePos_[(size_t)line] - delaySamples;
    while (readPos < 0.0f) readPos += (float)size;
    const int i0 = (int)readPos;
    int i1 = i0 + 1;
    if (i1 >= size) i1 -= size;
    const float frac = readPos - (float)i0;
    return buf[(size_t)i0] * (1.0f - frac) + buf[(size_t)i1] * frac;
}

void FDNReverb::process(float* const* in, float* const* out, int numSamples)
{
    const FDNReverbParams& p = target_;
    const bool dual = p.dual;
    const float sizeScale = 0.5f + p.size * 1.2f; // 0.5..1.7
    const float t60 = 0.3f * std::pow(40.0f, clampf(p.decay, 0.0f, 1.0f)); // 0.3..12 s
    // Reverb type (0 Hall / 1 Plate / 2 Room / 3 Chamber). Each type sets
    // the line-length scale, the diffusion offset and the damping-cutoff
    // multiplier — the four flavours of the palette.
    struct TypeCoeffs { float lenScale, diffOffset, dampMult; };
    auto typeCoeffs = [](float t) -> TypeCoeffs
    {
        switch ((int)std::lrint(clampf(t, 0.0f, 3.0f)))
        {
            case 1: return { 0.70f, 0.15f, 2.5f }; // Plate: short, dense, bright
            case 2: return { 0.55f, -0.05f, 0.8f }; // Room: tight, less diffuse, darker
            case 3: return { 0.85f, 0.25f, 1.3f }; // Chamber: dense, smooth
            default: return { 1.00f, 0.00f, 1.0f }; // Hall
        }
    };
    // In single-space mode characterA sets the whole space; in dual mode
    // each side has its own type.
    const float charA = clampf(p.characterA, 0.0f, 3.0f);
    const float charB = dual ? clampf(p.characterB, 0.0f, 3.0f) : charA;
    const TypeCoeffs tcA = typeCoeffs(charA);
    const TypeCoeffs tcB = typeCoeffs(charB);
    const float lenScaleA = tcA.lenScale;
    const float lenScaleB = tcB.lenScale;
    const float dampCutA = 12000.0f * std::pow(800.0f / 12000.0f, clampf(p.damping, 0.0f, 1.0f))
                         * tcA.dampMult;
    const float dampCutB = 12000.0f * std::pow(800.0f / 12000.0f, clampf(p.damping, 0.0f, 1.0f))
                         * tcB.dampMult;
    const float dampCoeffA = (float)(1.0 - std::exp(-kTwoPi * dampCutA / sr_));
    const float dampCoeffB = (float)(1.0 - std::exp(-kTwoPi * dampCutB / sr_));
    const float modDepth = clampf(p.modulation, 0.0f, 1.0f) * 0.0035f * (float)sr_; // up to ~3.5 ms
    const float diffBase = 0.25f + clampf(p.diffusion, 0.0f, 1.0f) * 0.55f;
    const float diffCoeffA = clampf(diffBase + tcA.diffOffset, 0.0f, 0.95f);
    const float diffCoeffB = clampf(diffBase + tcB.diffOffset, 0.0f, 0.95f);
    const float preDelaySamples = clampf(p.preDelayMs, 0.0f, 200.0f) * (float)sr_ / 1000.0f;
    const float inputGain = 0.5f;

    // Per-side output EQ (dual mode): set from the parameters.
    eqLowA_.setLowShelf(sr_, 220.0, p.eqLowA);
    eqHighA_.setHighShelf(sr_, 6000.0, p.eqHighA);
    eqLowB_.setLowShelf(sr_, 220.0, p.eqLowB);
    eqHighB_.setHighShelf(sr_, 6000.0, p.eqHighB);
    const float levelA = clampf(p.levelA, 0.0f, 1.0f);
    const float levelB = clampf(p.levelB, 0.0f, 1.0f);
    const float spaceBlend = clampf(p.spaceBlend, 0.0f, 0.5f);

    // Per-line lengths + feedback gains (recomputed per block — cheap).
    for (int i = 0; i < kLines; ++i)
    {
        const float sideScale = (i < 4) ? lenScaleA : lenScaleB;
        lineLen_[(size_t)i] = kBaseMs[i] * sizeScale * sideScale * (float)sr_ / 1000.0f;
        const float g = std::pow(10.0f, -3.0f * lineLen_[(size_t)i] / (t60 * (float)sr_));
        feedbackGain_[(size_t)i] = clampf(g, 0.0f, 0.995f);
    }

    // Early-reflection taps (ms into the pre-delay line).
    static constexpr float kEarlyMs[6] = { 7.0f, 13.0f, 23.0f, 31.0f, 47.0f, 61.0f };
    const float earlyGain = clampf(p.earlyLevel, 0.0f, 1.0f) * 0.35f;

    for (int n = 0; n < numSamples; ++n)
    {
        float xL = in[0][n], xR = in[1][n];
        if (!std::isfinite(xL)) xL = 0.0f;
        if (!std::isfinite(xR)) xR = 0.0f;

        // Pre-delay, then input diffusion (2 serial allpasses per channel).
        preDelayL_.write(xL);
        preDelayR_.write(xR);
        float dL = preDelayL_.read(preDelaySamples);
        float dR = preDelayR_.read(preDelaySamples);
        diffL_[0].coeff = diffCoeffA; diffL_[1].coeff = diffCoeffA;
        diffR_[0].coeff = diffCoeffB; diffR_[1].coeff = diffCoeffB;
        dL = diffL_[1].process(diffL_[0].process(dL, 0.011f * (float)sr_), 0.019f * (float)sr_);
        dR = diffR_[1].process(diffR_[0].process(dR, 0.013f * (float)sr_), 0.017f * (float)sr_);

        // Shimmer: octave-up voice fed back into the network (Eno/Lanois).
        shimmerSmooth_ += (clampf(p.shimmer, 0.0f, 1.0f) - shimmerSmooth_) * 0.001f;

        // Read the 8 lines with per-line random modulation (Spin/Wander).
        float y[kLines];
        for (int i = 0; i < kLines; ++i)
        {
            const double rate = clampf(p.modRate, 0.05f, 2.0f) * (0.55 + 0.11 * i);
            modPhase_[i] += kTwoPi * rate / sr_;
            modPhase2_[i] += kTwoPi * rate * 0.37 / sr_;
            if (modPhase_[i] > kTwoPi) modPhase_[i] -= kTwoPi;
            if (modPhase2_[i] > kTwoPi) modPhase2_[i] -= kTwoPi;
            const float mod = modDepth * (0.7f * (float)std::sin(modPhase_[i])
                                        + 0.3f * (float)std::sin(modPhase2_[i] + 1.7));
            const float len = clampf(lineLen_[(size_t)i] + mod, 16.0f,
                                     (float)maxLen_[(size_t)i] - 16.0f);
            y[i] = readLine(i, len);
        }

        float outL = 0.0f, outR = 0.0f;
        if (!dual)
        {
            // One shared space: even lines -> L, odd lines -> R.
            for (int i = 0; i < kLines; ++i)
            {
                if (i % 2 == 0) outL += y[i];
                else outR += y[i];
            }
            outL *= 0.45f;
            outR *= 0.45f;
        }
        else
        {
            // Dual space: lines 0-3 are Space A (left), 4-7 Space B (right),
            // each with its own EQ and level, then crossfed by spaceBlend.
            float a = 0.0f, b = 0.0f;
            for (int i = 0; i < 4; ++i) a += y[i];
            for (int i = 4; i < 8; ++i) b += y[i];
            a = eqHighA_.process(eqLowA_.process(a * 0.45f)) * levelA;
            b = eqHighB_.process(eqLowB_.process(b * 0.45f)) * levelB;
            outL = a + spaceBlend * b;
            outR = b + spaceBlend * a;
        }

        // Early reflections tapped from the pre-delay line.
        for (int t = 0; t < 6; ++t)
        {
            const float tapMs = kEarlyMs[t];
            const float tapL = preDelayL_.read(preDelaySamples + tapMs * (float)sr_ / 1000.0f);
            const float tapR = preDelayR_.read(preDelaySamples + tapMs * (float)sr_ / 1000.0f);
            const float w = (t % 2 == 0) ? 1.0f : -0.7f;
            outL += tapL * earlyGain * w * 0.5f;
            outR += tapR * earlyGain * w * 0.5f;
        }

        // Shimmer feedback: pitch-shift the mono tail mix and feed it back.
        float shimmerFb = 0.0f;
        if (shimmerSmooth_ > 0.001f)
        {
            const float tailMix = 0.5f * (outL + outR);
            const float shifted = shifter_.process(tailMix);
            shimmerFb = shifted * shimmerSmooth_ * 1.0f;
        }

        // Feedback: damp each line, run through the feedback matrix, scale
        // by the per-line decay gain, write back with the new input.
        // Dual mode uses one 4x4 matrix per half (no cross-talk between
        // the two spaces inside the network).
        float damped[kLines];
        for (int i = 0; i < kLines; ++i)
            damped[i] = dampLp_[i].process(y[i], (i < 4) ? dampCoeffA : dampCoeffB);

        for (int i = 0; i < kLines; ++i)
        {
            float fb = 0.0f;
            if (!dual)
            {
                for (int j = 0; j < kLines; ++j)
                    fb += hadamard_[i][j] * damped[j];
            }
            else
            {
                const int half = (i < 4) ? 0 : 4;
                for (int j = 0; j < 4; ++j)
                    fb += hadamard4_[i - half][j] * damped[half + j];
            }
            fb *= feedbackGain_[(size_t)i];
            // Lines 0-3 take the left diffused input, 4-7 the right.
            const float input = (i < 4 ? dL : dR) * inputGain + shimmerFb;
            float w = input + fb;
            if (!std::isfinite(w)) w = 0.0f;
            buf_[(size_t)i][(size_t)writePos_[(size_t)i]] = clampf(w, -2.0f, 2.0f);
            if (++writePos_[(size_t)i] >= maxLen_[(size_t)i]) writePos_[(size_t)i] = 0;
        }

        out[0][n] = clampf(outL * p.wetLevel, -1.5f, 1.5f);
        out[1][n] = clampf(outR * p.wetLevel, -1.5f, 1.5f);
    }
}

} // namespace silktone
