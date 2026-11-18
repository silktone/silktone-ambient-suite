// AmbientDSP.cpp — Silktone Ambient chain (see AmbientDSP.h).
#include "AmbientDSP.h"

namespace silktone
{

void AmbientDSP::prepare(double sampleRate)
{
    sr_ = sampleRate > 0 ? sampleRate : 44100.0;
    eqL_.prepare(sr_); eqR_.prepare(sr_);
    compL_.prepare(sr_); compR_.prepare(sr_);
    chorusL_.prepare(sr_); chorusR_.prepare(sr_);
    chorusL_.setStereoPhase(0.0);
    chorusR_.setStereoPhase(1.5707963); // 90 degrees apart = wide stereo
    delay_.prepare(sr_);
    tape_.prepare(sr_, 2);
    reverb_.prepare(sr_);
    reset();
}

void AmbientDSP::reset()
{
    eqL_.reset(); eqR_.reset();
    compL_.reset(); compR_.reset();
    chorusL_.reset(); chorusR_.reset();
    delay_.reset();
    tape_.reset();
    reverb_.reset();
    blendSmooth_ = params_.blend;
    outputSmooth_ = dbToLin(params_.outputDb);
}

void AmbientDSP::setParams(const AmbientParams& p)
{
    params_ = p;
    eqL_.setParams(p.lowDb, p.midDb, p.highDb, p.midFreqHz);
    eqR_.setParams(p.lowDb, p.midDb, p.highDb, p.midFreqHz);
    compL_.setParams(p.compThresholdDb, p.compRatio, p.compAttackMs, p.compReleaseMs, p.compMakeupDb);
    compR_.setParams(p.compThresholdDb, p.compRatio, p.compAttackMs, p.compReleaseMs, p.compMakeupDb);
    chorusL_.setParams(p.chorusRateHz, p.chorusDepth, p.chorusMix);
    chorusR_.setParams(p.chorusRateHz, p.chorusDepth, p.chorusMix);
    delay_.setParams(p.delayTimeMs, p.delayFeedback, p.delayMix, p.delayTape);
    TapeEchoParams tp;
    tp.delayMs = p.tapeTimeMs;
    tp.sustain = p.tapeFeedback;
    tp.echoVolume = p.tapeMix * 1.25f; // tape mix 0..1 -> echo volume 0..1.25
    tp.drive = p.tapeDrive;
    tp.preampOn = true;
    tp.age = p.tapeAge;
    tp.soundOnSound = false;
    tp.hissLevel = 0.0f;
    tp.outputTrim = 1.0f;
    tape_.setParams(tp);
    reverb_.setParams(p.reverb);
}

void AmbientDSP::process(float* const* in, float* const* out, int numSamples)
{
    const AmbientParams& p = params_;
    const float blendCoeff = (float)(1.0 - std::exp(-1.0 / (sr_ * 0.010)));
    const float outCoeff = blendCoeff;
    const float outTarget = dbToLin(p.outputDb);

    // Scratch buffers for the stereo blocks (max block 8192 samples).
    // Allocated on the stack via std::vector is wasteful per call, so use
    // fixed thread-local scratch sized for a generous block.
    thread_local std::vector<float> tmpL, tmpR, tmpL2, tmpR2;
    if ((int)tmpL.size() < numSamples)
    {
        tmpL.assign((size_t)numSamples, 0.0f);
        tmpR.assign((size_t)numSamples, 0.0f);
        tmpL2.assign((size_t)numSamples, 0.0f);
        tmpR2.assign((size_t)numSamples, 0.0f);
    }
    float* wetL = tmpL.data();
    float* wetR = tmpR.data();
    float* auxL = tmpL2.data();
    float* auxR = tmpR2.data();

    for (int n = 0; n < numSamples; ++n)
    {
        float xL = in[0][n], xR = in[1][n];
        if (!std::isfinite(xL)) xL = 0.0f;
        if (!std::isfinite(xR)) xR = 0.0f;
        const float dryL = xL, dryR = xR;

        // Tone: saturation -> EQ -> compressor.
        xL = Saturation::process(xL, p.satDrive);
        xR = Saturation::process(xR, p.satDrive);
        xL = eqL_.process(xL);
        xR = eqR_.process(xR);
        if (p.compOn)
        {
            xL = compL_.process(xL);
            xR = compR_.process(xR);
        }

        // Modulation.
        if (p.chorusOn)
        {
            xL = chorusL_.process(xL);
            xR = chorusR_.process(xR);
        }
        wetL[n] = xL;
        wetR[n] = xR;

        // Blend + output are applied after the stereo blocks below.
        // (Store the dry signal for the blend.)
        auxL[n] = dryL;
        auxR[n] = dryR;
    }

    // Stereo delay (dry + wet inside).
    if (p.delayOn)
    {
        float* ip[2] = { wetL, wetR };
        float* op[2] = { wetL, wetR };
        delay_.process(ip, op, numSamples);
    }

    // Tape echo (dry + wet inside, same convention as the tape plugin).
    if (p.tapeOn)
    {
        float* ip[2] = { wetL, wetR };
        float* op[2] = { wetL, wetR };
        tape_.process(ip, op, 2, numSamples);
    }

    // Reverb: wet-only output; mix with the pre-reverb signal.
    if (p.reverbOn)
    {
        float* ip[2] = { wetL, wetR };
        float* op[2] = { auxL, auxR }; // reuse aux as the reverb output
        // aux currently holds the dry signal — copy it aside first is not
        // needed: we blend from the original dry below using `in`.
        reverb_.process(ip, op, numSamples);
        for (int n = 0; n < numSamples; ++n)
        {
            wetL[n] = wetL[n] + auxL[n];
            wetR[n] = wetR[n] + auxR[n];
        }
    }

    // Master blend + output trim (smoothed, click-free).
    for (int n = 0; n < numSamples; ++n)
    {
        blendSmooth_ += (p.blend - blendSmooth_) * (1.0f - blendCoeff);
        outputSmooth_ += (outTarget - outputSmooth_) * (1.0f - outCoeff);
        const float dryL = in[0][n], dryR = in[1][n];
        float yL = dryL * (1.0f - blendSmooth_) + wetL[n] * blendSmooth_;
        float yR = dryR * (1.0f - blendSmooth_) + wetR[n] * blendSmooth_;
        yL *= outputSmooth_;
        yR *= outputSmooth_;
        out[0][n] = clampf(yL, -1.5f, 1.5f);
        out[1][n] = clampf(yR, -1.5f, 1.5f);
    }
}

} // namespace silktone
