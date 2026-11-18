// VocalChannelDSP.cpp — Silktone Vocal Channel strip (see header).
#include "VocalChannelDSP.h"

namespace silktone
{

void VocalChannelDSP::prepare(double sampleRate)
{
    sr_ = sampleRate > 0 ? sampleRate : 44100.0;
    eqL_.prepare(sr_); eqR_.prepare(sr_);
    compL_.prepare(sr_); compR_.prepare(sr_);
    reset();
}

void VocalChannelDSP::reset()
{
    hpL_.reset(); hpR_.reset();
    eqL_.reset(); eqR_.reset();
    compL_.reset(); compR_.reset();
    blendSmooth_ = params_.blend;
    outputSmooth_ = dbToLin(params_.outputDb);
}

void VocalChannelDSP::setParams(const VocalChannelParams& p)
{
    params_ = p;
    hpL_.setHighPass(sr_, clampf(p.hpFreqHz, 20.0f, 400.0f));
    hpR_.setHighPass(sr_, clampf(p.hpFreqHz, 20.0f, 400.0f));
    eqL_.setParams(p.lowDb, p.midDb, p.highDb, p.midFreqHz);
    eqR_.setParams(p.lowDb, p.midDb, p.highDb, p.midFreqHz);
    compL_.setParams(p.compThresholdDb, p.compRatio, p.compAttackMs, p.compReleaseMs, p.compMakeupDb);
    compR_.setParams(p.compThresholdDb, p.compRatio, p.compAttackMs, p.compReleaseMs, p.compMakeupDb);
}

void VocalChannelDSP::process(float* const* in, float* const* out, int numSamples)
{
    const VocalChannelParams& p = params_;
    const float coeff = (float)(1.0 - std::exp(-1.0 / (sr_ * 0.010)));
    const float outTarget = dbToLin(p.outputDb);

    for (int n = 0; n < numSamples; ++n)
    {
        float xL = in[0][n], xR = in[1][n];
        if (!std::isfinite(xL)) xL = 0.0f;
        if (!std::isfinite(xR)) xR = 0.0f;
        const float dryL = xL, dryR = xR;

        if (p.hpOn)
        {
            xL = hpL_.process(xL);
            xR = hpR_.process(xR);
        }
        xL = eqL_.process(xL);
        xR = eqR_.process(xR);
        xL = compL_.process(xL);
        xR = compR_.process(xR);
        xL = Saturation::process(xL, p.satDrive);
        xR = Saturation::process(xR, p.satDrive);

        blendSmooth_ += (p.blend - blendSmooth_) * (1.0f - coeff);
        outputSmooth_ += (outTarget - outputSmooth_) * (1.0f - coeff);
        float yL = dryL * (1.0f - blendSmooth_) + xL * blendSmooth_;
        float yR = dryR * (1.0f - blendSmooth_) + xR * blendSmooth_;
        yL *= outputSmooth_;
        yR *= outputSmooth_;
        out[0][n] = clampf(yL, -1.5f, 1.5f);
        out[1][n] = clampf(yR, -1.5f, 1.5f);
    }
}

} // namespace silktone
