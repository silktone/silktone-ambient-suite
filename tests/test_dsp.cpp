// tests/test_dsp.cpp — standalone DSP verification + demo WAV renders
// for the Silktone ambient suite. No JUCE needed. Build:
//   g++ -std=c++17 -O2 tests/test_dsp.cpp Shared/DSPBlocks.cpp Shared/FDNReverb.cpp
//       Shared/TapeEchoDSP.cpp Shared/AmbientDSP.cpp Shared/VocalChannelDSP.cpp
//       -IShared -o tests/test_dsp
//   ./tests/test_dsp [ambient-demo.wav] [vocal-demo.wav]
// Exit 0 = all checks passed.
#include "DSPBlocks.h"
#include "FDNReverb.h"
#include "TapeEchoDSP.h"
#include "AmbientDSP.h"
#include "VocalChannelDSP.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace silktone;

static int failures = 0;
static void check(bool ok, const char* name, const char* detail = "")
{
    std::printf("[%s] %s %s\n", ok ? "PASS" : "FAIL", name, detail);
    if (!ok) ++failures;
}
static float peak(const std::vector<float>& v)
{
    float p = 0; for (float x : v) p = std::max(p, std::fabs(x)); return p;
}
static float rmsRange(const std::vector<float>& v, int a, int b)
{
    double s = 0; int n = 0;
    for (int i = a; i < b && i < (int)v.size(); ++i) { s += (double)v[i] * v[i]; ++n; }
    return n ? (float)std::sqrt(s / n) : 0.0f;
}
static float peakRange(const std::vector<float>& v, int a, int b)
{
    float p = 0;
    for (int i = a; i < b && i < (int)v.size(); ++i) p = std::max(p, std::fabs(v[i]));
    return p;
}
static float goertzel(const std::vector<float>& v, int a, int b, float freq, int sr)
{
    double coeff = 2.0 * std::cos(6.2831853 * freq / sr);
    double s1 = 0, s2 = 0;
    for (int i = a; i < b && i < (int)v.size(); ++i)
    { double s0 = v[i] + coeff * s1 - s2; s2 = s1; s1 = s0; }
    double power = s2 * s2 + s1 * s1 - coeff * s1 * s2;
    return (float)std::sqrt(std::max(0.0, power));
}
static int zeroCrossings(const std::vector<float>& v, int a, int b)
{
    int n = 0;
    for (int i = a + 1; i < b && i < (int)v.size(); ++i)
        if ((v[i] >= 0) != (v[i-1] >= 0)) ++n;
    return n;
}

// ---- minimal 16-bit stereo WAV writer ----
static void writeWav(const std::string& path, const std::vector<float>& L,
                     const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { std::printf("cannot open %s\n", path.c_str()); ++failures; return; }
    auto w32 = [&](unsigned v){ std::fputc(v&255,f); std::fputc((v>>8)&255,f); std::fputc((v>>16)&255,f); std::fputc((v>>24)&255,f); };
    auto w16 = [&](unsigned v){ std::fputc(v&255,f); std::fputc((v>>8)&255,f); };
    auto tag = [&](const char* t){ std::fwrite(t,1,4,f); };
    unsigned n = (unsigned)L.size(), dataBytes = n * 4;
    tag("RIFF"); w32(36 + dataBytes); tag("WAVE"); tag("fmt "); w32(16);
    w16(1); w16(2); w32((unsigned)sr); w32((unsigned)sr * 4); w16(4); w16(16);
    tag("data"); w32(dataBytes);
    for (unsigned i = 0; i < n; ++i)
    {
        auto s = [](float x){ x = std::max(-1.0f, std::min(1.0f, x));
                              return (unsigned)(int)std::lrint(x * 32767.0f) & 0xffff; };
        w16(s(L[i])); w16(s(R[i]));
    }
    std::fclose(f);
}

static float pluck(float t, float freq)
{
    if (t < 0 || t > 1.2f) return 0.0f;
    float env = std::exp(-t * 5.0f);
    return env * (std::sin(6.2831853f * freq * t)
                + 0.45f * std::sin(6.2831853f * 2.0f * freq * t)
                + 0.18f * std::sin(6.2831853f * 3.0f * freq * t));
}

// Process a stereo DSP in host-like blocks.
template <typename DSP>
static void processStereo(DSP& dsp, std::vector<float>& L, std::vector<float>& R)
{
    const int block = 512;
    for (size_t off = 0; off < L.size(); off += block)
    {
        const int n = (int)std::min<size_t>(block, L.size() - off);
        float* ip[2] = { L.data() + off, R.data() + off };
        float* op[2] = { L.data() + off, R.data() + off };
        dsp.process(ip, op, n);
    }
}

int main(int argc, char** argv)
{
    const int sr = 44100;
    const std::string ambientDemo = argc > 1 ? argv[1] : "demo/silktone-ambient-demo.wav";
    const std::string vocalDemo = argc > 2 ? argv[2] : "demo/silktone-vocal-demo.wav";

    // --- 1. FDN reverb: impulse tail decays, roughly the set RT60 ---
    // Measure the tail envelope in successive windows: it must fall
    // steadily (RT60 ~2.2 s at decay 0.55).
    {
        FDNReverb rev; FDNReverbParams p; p.decay = 0.55f; p.preDelayMs = 0; p.earlyLevel = 0;
        rev.setParams(p); rev.prepare(sr); rev.reset();
        std::vector<float> L(sr * 4, 0), R(sr * 4, 0); L[100] = 1.0f; R[100] = 1.0f;
        processStereo(rev, L, R);
        const float early = rmsRange(L, 100 + 6615, 100 + 26460);   // 0.15-0.6 s
        const float mid = rmsRange(L, 100 + 44100, 100 + 57330);    // 1.0-1.3 s
        const float late = rmsRange(L, 100 + 88200, 100 + 101430);  // 2.0-2.3 s
        char buf[192]; std::snprintf(buf, sizeof buf, "(RMS 0.15-0.6 s %.4f, 1.0-1.3 s %.4f, 2.0-2.3 s %.4f)", early, mid, late);
        check(early > 0.001f && mid < early && late < mid * 0.3f, "reverb tail decays", buf);
        bool finite = true; for (float x : L) if (!std::isfinite(x)) finite = false;
        check(finite && peak(L) < 1.5f, "reverb finite and bounded", "");
    }
    // --- 1b. Reverb types: Room and Chamber are real flavours, not labels.
    // A Room's first return must land earlier than a Hall's (shorter lines),
    // and a Chamber's too (denser, shorter lines than a hall).
    {
        auto firstReturn = [&](float type)
        {
            FDNReverb rev; FDNReverbParams p; p.decay = 0.55f; p.preDelayMs = 0; p.earlyLevel = 0;
            p.characterA = type; p.characterB = type;
            rev.setParams(p); rev.prepare(sr); rev.reset();
            std::vector<float> L(sr * 2, 0), R(sr * 2, 0); L[100] = 1.0f; R[100] = 1.0f;
            processStereo(rev, L, R);
            int first = -1; bool finite = true;
            for (size_t i = 101; i < L.size(); ++i)
            {
                if (!std::isfinite(L[i])) finite = false;
                if (first < 0 && std::fabs(L[i]) > 0.005f) first = (int)i - 100;
            }
            return std::pair<int, bool>(first, finite);
        };
        const auto hall = firstReturn(0.0f);    // Hall
        const auto room = firstReturn(2.0f);    // Room
        const auto chamber = firstReturn(3.0f); // Chamber
        char buf[192]; std::snprintf(buf, sizeof buf,
            "(first return: Hall %d samples, Room %d, Chamber %d)",
            hall.first, room.first, chamber.first);
        check(hall.second && room.second && chamber.second
              && hall.first > 0 && room.first > 0 && chamber.first > 0
              && room.first < hall.first && chamber.first < hall.first,
              "reverb types: Room/Chamber return earlier than Hall", buf);
    }
    // --- 2. Longer decay setting -> longer tail ---
    {
        auto tailAt = [&](float decay)
        {
            FDNReverb rev; FDNReverbParams p; p.decay = decay; p.preDelayMs = 0; p.earlyLevel = 0;
            rev.setParams(p); rev.prepare(sr); rev.reset();
            std::vector<float> L(sr * 4, 0), R(sr * 4, 0); L[100] = 1.0f; R[100] = 1.0f;
            processStereo(rev, L, R);
            return rmsRange(L, 100 + 88200, 100 + 97020);
        };
        const float shortTail = tailAt(0.3f), longTail = tailAt(0.9f);
        char buf[160]; std::snprintf(buf, sizeof buf, "(2 s RMS decay .3 = %.4f, decay .9 = %.4f)", shortTail, longTail);
        check(longTail > shortTail * 3.0f, "decay control changes tail length", buf);
    }
    // --- 3. Stereo tail is decorrelated (L != R) ---
    {
        FDNReverb rev; FDNReverbParams p; p.decay = 0.6f; p.preDelayMs = 0; p.earlyLevel = 0;
        rev.setParams(p); rev.prepare(sr); rev.reset();
        std::vector<float> L(sr * 3, 0), R(sr * 3, 0); L[100] = 1.0f; R[100] = 1.0f;
        processStereo(rev, L, R);
        double num = 0, dL = 0, dR = 0;
        for (int i = 1000; i < sr * 2; ++i)
        { num += (double)L[i] * R[i]; dL += (double)L[i] * L[i]; dR += (double)R[i] * R[i]; }
        const float corr = (float)(num / std::sqrt(std::max(1e-12, dL * dR)));
        char buf[128]; std::snprintf(buf, sizeof buf, "(L/R tail correlation %.3f)", corr);
        check(std::fabs(corr) < 0.9f, "stereo tail decorrelated", buf);
    }
    // --- 4. Plate is brighter/denser than Hall at the same decay ---
    // The damping's real effect is on how fast the highs die in the tail.
    // Measure it in the time domain: run the tail through a 1-pole
    // lowpass (1 kHz) and a 1-pole highpass (3 kHz) and compare how each
    // band decays between an early and a late tail window. (A Goertzel
    // at a few discrete frequencies is the wrong tool — the FDN's comb
    // puts deep nulls between resonances, so it samples the comb, not
    // the spectral tilt, and even disagrees between L and R.) A plate's
    // highs ring on; a hall's highs die first.
    {
        auto hfDecayRatio = [&](const std::vector<float>& v)
        {
            auto bandRms = [&](int a, int b, float cutoff, bool highpass)
            {
                const float c = (float)(1.0 - std::exp(-6.2831853 * cutoff / sr));
                float lp = 0; double s = 0; int n = 0;
                for (int i = 0; i < (int)v.size(); ++i)
                {
                    lp += c * (v[i] - lp);
                    const float out = highpass ? (v[i] - lp) : lp;
                    if (i >= a && i < b) { s += (double)out * out; ++n; }
                }
                return n ? (float)std::sqrt(s / n) : 0.0f;
            };
            const int e0 = (int)(1.2 * sr), e1 = (int)(1.7 * sr);
            const int l0 = (int)(2.5 * sr), l1 = (int)(3.0 * sr);
            const double lpDecay = bandRms(l0, l1, 1000.0f, false) / std::max(1e-9f, bandRms(e0, e1, 1000.0f, false));
            const double hpDecay = bandRms(l0, l1, 3000.0f, true) / std::max(1e-9f, bandRms(e0, e1, 3000.0f, true));
            return hpDecay / std::max(1e-9, lpDecay);
        };
        auto runChar = [&](float character)
        {
            FDNReverb rev; FDNReverbParams p; p.decay = 0.6f; p.preDelayMs = 0; p.earlyLevel = 0;
            p.characterA = character;
            rev.setParams(p); rev.prepare(sr); rev.reset();
            const int n = sr * 4;
            std::vector<float> L(n, 0), R(n, 0);
            unsigned seed = 12345;
            for (int i = 0; i < sr; ++i) // 1 s of noise, then silence
            {
                seed = seed * 1664525u + 1013904223u;
                const float x = ((seed >> 8) & 0xffffff) / 8388608.0f - 1.0f;
                L[i] = 0.3f * x; R[i] = 0.3f * x;
            }
            processStereo(rev, L, R);
            return 0.5 * (hfDecayRatio(L) + hfDecayRatio(R));
        };
        const double hallRatio = runChar(0.0f), plateRatio = runChar(1.0f);
        char buf[192]; std::snprintf(buf, sizeof buf, "(high-band decay relative to low-band decay: hall %.3f, plate %.3f)", hallRatio, plateRatio);
        check(plateRatio > hallRatio * 1.3, "plate brighter than hall", buf);
    }
    // --- 5. Dual space: plate left / hall right, different tails ---
    // Same time-domain band-decay method as test 4, per channel, run in
    // both assignments (plate-left and plate-right) so the result cannot
    // depend on which side the plate sits on.
    {
        auto hfDecayRatio = [&](const std::vector<float>& v)
        {
            auto bandRms = [&](int a, int b, float cutoff, bool highpass)
            {
                const float c = (float)(1.0 - std::exp(-6.2831853 * cutoff / sr));
                float lp = 0; double s = 0; int n = 0;
                for (int i = 0; i < (int)v.size(); ++i)
                {
                    lp += c * (v[i] - lp);
                    const float out = highpass ? (v[i] - lp) : lp;
                    if (i >= a && i < b) { s += (double)out * out; ++n; }
                }
                return n ? (float)std::sqrt(s / n) : 0.0f;
            };
            const int e0 = (int)(1.2 * sr), e1 = (int)(1.7 * sr);
            const int l0 = (int)(2.5 * sr), l1 = (int)(3.0 * sr);
            const double lpDecay = bandRms(l0, l1, 1000.0f, false) / std::max(1e-9f, bandRms(e0, e1, 1000.0f, false));
            const double hpDecay = bandRms(l0, l1, 3000.0f, true) / std::max(1e-9f, bandRms(e0, e1, 3000.0f, true));
            return hpDecay / std::max(1e-9, lpDecay);
        };
        // Returns { plate-side ratio, hall-side ratio } for one assignment.
        auto runDual = [&](float charA, float charB, float eqHighA)
        {
            FDNReverb rev; FDNReverbParams p; p.decay = 0.6f; p.preDelayMs = 0; p.earlyLevel = 0;
            p.dual = true; p.characterA = charA; p.characterB = charB; p.spaceBlend = 0.0f;
            p.eqHighA = eqHighA;
            rev.setParams(p); rev.prepare(sr); rev.reset();
            const int n = sr * 4;
            std::vector<float> L(n, 0), R(n, 0);
            unsigned seed = 777;
            for (int i = 0; i < sr; ++i) // 1 s of noise, then silence
            {
                seed = seed * 1664525u + 1013904223u;
                const float x = ((seed >> 8) & 0xffffff) / 8388608.0f - 1.0f;
                L[i] = 0.3f * x; R[i] = 0.3f * x;
            }
            processStereo(rev, L, R);
            const double rL = hfDecayRatio(L), rR = hfDecayRatio(R);
            // Plate is A (left) when charA == 1, else B (right).
            return charA > 0.5f ? std::pair<double, double>(rL, rR)
                                : std::pair<double, double>(rR, rL);
        };
        const auto ph = runDual(1.0f, 0.0f, 0.0f); // plate left, hall right
        const auto hp = runDual(0.0f, 1.0f, 0.0f); // hall left, plate right
        const double plateAvg = 0.5 * (ph.first + hp.first);
        const double hallAvg = 0.5 * (ph.second + hp.second);
        char buf[224]; std::snprintf(buf, sizeof buf, "(high-band decay relative to low-band: plate %.3f (%.3f / %.3f), hall %.3f (%.3f / %.3f))",
                                      plateAvg, ph.first, hp.first, hallAvg, ph.second, hp.second);
        check(plateAvg > hallAvg * 1.3, "dual space: plate side brighter than hall side", buf);
        // Per-side EQ: cutting the highs on A darkens the A side only.
        // (The EQ is a static output filter, so it changes the band
        // LEVELS, not the decay ratio — measure the high/low level ratio
        // in the early tail window.) A is the left side here, plate.
        auto levelRatio = [&](const std::vector<float>& v)
        {
            auto bandRms = [&](int a, int b, float cutoff, bool highpass)
            {
                const float c = (float)(1.0 - std::exp(-6.2831853 * cutoff / sr));
                float lp = 0; double s = 0; int n = 0;
                for (int i = 0; i < (int)v.size(); ++i)
                {
                    lp += c * (v[i] - lp);
                    const float out = highpass ? (v[i] - lp) : lp;
                    if (i >= a && i < b) { s += (double)out * out; ++n; }
                }
                return n ? (float)std::sqrt(s / n) : 0.0f;
            };
            const int e0 = (int)(1.2 * sr), e1 = (int)(1.7 * sr);
            return bandRms(e0, e1, 3000.0f, true) / std::max(1e-9f, bandRms(e0, e1, 1000.0f, false));
        };
        auto runDualLevels = [&](float eqHighA)
        {
            FDNReverb rev; FDNReverbParams p; p.decay = 0.6f; p.preDelayMs = 0; p.earlyLevel = 0;
            p.dual = true; p.characterA = 1.0f; p.characterB = 0.0f; p.spaceBlend = 0.0f;
            p.eqHighA = eqHighA;
            rev.setParams(p); rev.prepare(sr); rev.reset();
            const int n = sr * 4;
            std::vector<float> L(n, 0), R(n, 0);
            unsigned seed = 777;
            for (int i = 0; i < sr; ++i)
            {
                seed = seed * 1664525u + 1013904223u;
                const float x = ((seed >> 8) & 0xffffff) / 8388608.0f - 1.0f;
                L[i] = 0.3f * x; R[i] = 0.3f * x;
            }
            processStereo(rev, L, R);
            return std::pair<double, double>(levelRatio(L), levelRatio(R));
        };
        const auto baseLevels = runDualLevels(0.0f);
        const auto cutLevels = runDualLevels(-12.0f);
        char buf2[224]; std::snprintf(buf2, sizeof buf2, "(A high EQ -12: A-side high/low level %.3f -> %.3f, B-side %.3f -> %.3f)",
                                      baseLevels.first, cutLevels.first, baseLevels.second, cutLevels.second);
        check(cutLevels.first < baseLevels.first * 0.7
              && std::fabs(cutLevels.second - baseLevels.second) < baseLevels.second * 0.15 + 0.01,
              "dual space: per-side EQ affects only its side", buf2);
    }
    // --- 6. Shimmer: octave-up energy appears in the tail ---
    // Compare shimmer ON vs OFF at the same decay: the FDN is linear, so
    // with shimmer off a 220 Hz sine leaves (almost) no 440 Hz in the tail.
    {
        auto shimmer440 = [&](float shimmer)
        {
            FDNReverb rev; FDNReverbParams p; p.decay = 0.7f; p.preDelayMs = 0; p.earlyLevel = 0;
            p.shimmer = shimmer;
            rev.setParams(p); rev.prepare(sr); rev.reset();
            std::vector<float> L(sr * 4, 0), R(sr * 4, 0);
            for (int i = 0; i < sr; ++i) { L[i] = 0.4f * std::sin(6.2831853f * 220.0f * i / sr); R[i] = L[i]; }
            processStereo(rev, L, R);
            return goertzel(L, (int)(1.5 * sr), (int)(3.0 * sr), 440.0f, sr);
        };
        const float off = shimmer440(0.0f), on = shimmer440(0.7f);
        char buf[160]; std::snprintf(buf, sizeof buf, "(tail 440 Hz shimmer off %.2f, on %.2f)", off, on);
        check(on > off * 3.0f && on > 1.0f, "shimmer adds octave-up energy", buf);
    }
    // --- 7. Pitch shifter alone: roughly doubles the frequency ---
    {
        PitchShifterUp ps; ps.prepare(sr); ps.reset();
        std::vector<float> in(sr, 0), out(sr, 0);
        for (int i = 0; i < sr; ++i) in[i] = 0.5f * std::sin(6.2831853f * 220.0f * i / sr);
        for (int i = 0; i < sr; ++i) out[i] = ps.process(in[i]);
        const int zcIn = zeroCrossings(in, sr / 4, sr * 3 / 4);
        const int zcOut = zeroCrossings(out, sr / 4, sr * 3 / 4);
        const float ratio = (float)zcOut / std::max(1, zcIn);
        char buf[160]; std::snprintf(buf, sizeof buf, "(zero crossings in %d, out %d, ratio %.2f)", zcIn, zcOut, ratio);
        check(ratio > 1.5f && ratio < 2.6f, "pitch shifter ~one octave up", buf);
    }
    // --- 8. Stereo delay: ping-pong (L echo, then R echo) ---
    {
        StereoDelay dly; dly.prepare(sr); dly.reset();
        dly.setParams(320.0f, 0.5f, 1.0f, false);
        std::vector<float> L(sr * 2, 0), R(sr * 2, 0); L[100] = 1.0f;
        processStereo(dly, L, R);
        const int t1 = 100 + (int)(0.320 * sr);
        const int t2 = 100 + (int)(0.640 * sr);
        const float echoL1 = peakRange(L, t1 - 200, t1 + 200);
        const float echoR2 = peakRange(R, t2 - 200, t2 + 200);
        const float echoL2 = peakRange(L, t2 - 200, t2 + 200);
        char buf[192]; std::snprintf(buf, sizeof buf, "(L echo @320ms %.3f, R echo @640ms %.3f, L @640ms %.3f)", echoL1, echoR2, echoL2);
        check(echoL1 > 0.3f && echoR2 > 0.1f && echoR2 > echoL2 * 2.0f, "ping-pong delay alternates sides", buf);
    }
    // --- 9. Tape echo (shared copy): timing + decay + bounded ---
    {
        TapeEchoDSP dsp; TapeEchoParams p; p.delayMs = 320; p.sustain = 0.5f;
        p.echoVolume = 1.0f; p.preampOn = false; p.age = 0.0f;
        dsp.setParams(p); dsp.prepare(sr, 1); dsp.reset();
        std::vector<float> in(sr * 2, 0), out(sr * 2, 0); in[100] = 1.0f;
        dsp.processMono(in.data(), out.data(), (int)in.size());
        int firstEcho = -1;
        for (int i = 200; i < sr; ++i) if (std::fabs(out[i]) > 0.05f) { firstEcho = i; break; }
        const int expected = 100 + (int)(0.320 * sr);
        check(firstEcho > 0 && std::abs(firstEcho - expected) < 90, "tape echo delay time", "");
        const float r1 = peakRange(out, 100 + 14112 - 500, 100 + 14112 + 500);
        const float r2 = peakRange(out, 100 + 28224 - 500, 100 + 28224 + 500);
        check(r1 > 0.05f && r2 < r1 * 0.85f, "tape echo repeats decay", "");
    }
    // --- 10. Chorus: output differs from input, stays finite ---
    {
        Chorus ch; ch.prepare(sr); ch.reset();
        ch.setParams(0.8f, 0.5f, 0.5f);
        std::vector<float> in(sr, 0), out(sr, 0);
        for (int i = 0; i < sr; ++i) in[i] = 0.4f * std::sin(6.2831853f * 440.0f * i / sr);
        for (int i = 0; i < sr; ++i) out[i] = ch.process(in[i]);
        double diff = 0; for (int i = 0; i < sr; ++i) diff += std::fabs(out[i] - in[i]);
        bool finite = true; for (float x : out) if (!std::isfinite(x)) finite = false;
        check(finite && diff > 1.0 && peak(out) < 1.5f, "chorus modulates, stays bounded", "");
    }
    // --- 11. Compressor: reduces loud, leaves quiet alone ---
    {
        auto grAt = [&](float levelDb)
        {
            OptoCompressor comp; comp.prepare(sr); comp.reset();
            comp.setParams(-20.0f, 3.0f, 8.0f, 200.0f, 0.0f);
            const float amp = std::pow(10.0f, levelDb / 20.0f);
            for (int i = 0; i < sr; ++i)
                comp.process(amp * std::sin(6.2831853f * 220.0f * i / sr));
            return comp.getGainReductionDb();
        };
        const float grLoud = grAt(-6.0f), grQuiet = grAt(-40.0f);
        char buf[160]; std::snprintf(buf, sizeof buf, "(GR loud %.1f dB, quiet %.1f dB)", grLoud, grQuiet);
        check(grLoud < -2.0f && grQuiet > -0.5f, "compressor gain reduction", buf);
    }
    // --- 12. EQ: low shelf boosts lows, high shelf cuts highs ---
    {
        auto bandGain = [&](float lowDb, float highDb, float freq)
        {
            ThreeBandEQ eq; eq.prepare(sr); eq.reset();
            eq.setParams(lowDb, 0.0f, highDb, 1000.0f);
            std::vector<float> in(sr / 2, 0), out(sr / 2, 0);
            for (int i = 0; i < (int)in.size(); ++i) in[i] = 0.4f * std::sin(6.2831853f * freq * i / sr);
            for (int i = 0; i < (int)in.size(); ++i) out[i] = eq.process(in[i]);
            return goertzel(out, sr / 8, sr / 2, freq, sr) / goertzel(in, sr / 8, sr / 2, freq, sr);
        };
        const float lowBoost = bandGain(12.0f, 0.0f, 100.0f);
        const float highCut = bandGain(0.0f, -12.0f, 8000.0f);
        char buf[160]; std::snprintf(buf, sizeof buf, "(100 Hz gain %.2f, 8 kHz gain %.2f)", lowBoost, highCut);
        check(lowBoost > 2.5f && highCut < 0.4f, "EQ shelves work", buf);
    }
    // --- 13. Saturation: more drive = more harmonics, still bounded ---
    {
        auto thirdShare = [&](float drive)
        {
            std::vector<float> in(sr / 2, 0), out(sr / 2, 0);
            for (int i = 0; i < (int)in.size(); ++i) in[i] = 0.5f * std::sin(6.2831853f * 220.0f * i / sr);
            for (int i = 0; i < (int)in.size(); ++i) out[i] = Saturation::process(in[i], drive);
            const int a = sr / 8, b = sr / 2;
            return goertzel(out, a, b, 660.0f, sr) / std::max(1e-6f, goertzel(out, a, b, 220.0f, sr));
        };
        const float lo = thirdShare(0.0f), hi = thirdShare(1.0f);
        char buf[160]; std::snprintf(buf, sizeof buf, "(3rd-harmonic share drive 0: %.4f, drive 1: %.4f)", lo, hi);
        check(hi > lo * 2.0f, "saturation adds harmonics with drive", buf);
    }
    // --- 14. Ambient chain: blend 0 = dry passthrough; blend 1 = wet, bounded ---
    {
        AmbientDSP dsp; AmbientParams p; p.blend = 0.0f;
        dsp.setParams(p); dsp.prepare(sr); dsp.reset();
        std::vector<float> L(sr, 0), R(sr, 0);
        for (int i = 0; i < sr; ++i) { L[i] = 0.4f * std::sin(6.2831853f * 330.0f * i / sr); R[i] = L[i]; }
        std::vector<float> dryL = L, dryR = R;
        processStereo(dsp, L, R);
        double maxDiff = 0;
        for (int i = 0; i < sr; ++i) maxDiff = std::max(maxDiff, (double)std::fabs(L[i] - dryL[i]));
        check(maxDiff < 0.01, "ambient blend 0 = dry passthrough", "");
        AmbientDSP dsp2; AmbientParams p2; p2.blend = 1.0f;
        dsp2.setParams(p2); dsp2.prepare(sr); dsp2.reset();
        std::vector<float> L2(sr * 3, 0), R2(sr * 3, 0);
        for (int i = 0; i < sr; ++i) { L2[i] = 0.4f * std::sin(6.2831853f * 330.0f * i / sr); R2[i] = L2[i]; }
        processStereo(dsp2, L2, R2);
        bool finite = true; for (float x : L2) if (!std::isfinite(x)) finite = false;
        const float tailRms = rmsRange(L2, sr * 2, sr * 3);
        char buf[160]; std::snprintf(buf, sizeof buf, "(peak %.3f, tail RMS after input %.4f)", peak(L2), tailRms);
        check(finite && peak(L2) < 1.5f && tailRms > 0.001f, "ambient blend 1 = wet tail, bounded", buf);
    }
    // --- 15. Vocal channel: blend 0 = dry; blend 1 = processed, bounded ---
    {
        VocalChannelDSP dsp; VocalChannelParams p; p.blend = 0.0f;
        dsp.setParams(p); dsp.prepare(sr); dsp.reset();
        std::vector<float> L(sr, 0), R(sr, 0);
        for (int i = 0; i < sr; ++i) { L[i] = 0.4f * std::sin(6.2831853f * 220.0f * i / sr); R[i] = L[i]; }
        std::vector<float> dryL = L;
        processStereo(dsp, L, R);
        double maxDiff = 0;
        for (int i = 0; i < sr; ++i) maxDiff = std::max(maxDiff, (double)std::fabs(L[i] - dryL[i]));
        check(maxDiff < 0.01, "vocal blend 0 = dry passthrough", "");
        VocalChannelDSP dsp2; VocalChannelParams p2; p2.blend = 1.0f;
        dsp2.setParams(p2); dsp2.prepare(sr); dsp2.reset();
        std::vector<float> L2(sr, 0), R2(sr, 0);
        for (int i = 0; i < sr; ++i) { L2[i] = 0.5f * std::sin(6.2831853f * 220.0f * i / sr); R2[i] = L2[i]; }
        processStereo(dsp2, L2, R2);
        bool finite = true; for (float x : L2) if (!std::isfinite(x)) finite = false;
        check(finite && peak(L2) < 1.5f, "vocal blend 1 = processed, bounded", "");
    }
    // --- 16. Torture: everything maxed, loud input, 3 s — must stay bounded ---
    {
        AmbientDSP dsp; AmbientParams p;
        p.blend = 1.0f; p.reverb.decay = 1.0f; p.reverb.shimmer = 1.0f;
        p.reverb.size = 1.0f; p.delayFeedback = 0.95f; p.delayMix = 1.0f;
        p.tapeOn = true; p.tapeFeedback = 1.0f; p.tapeMix = 1.0f; p.satDrive = 1.0f;
        dsp.setParams(p); dsp.prepare(sr); dsp.reset();
        std::vector<float> L(sr * 3, 0), R(sr * 3, 0);
        for (int i = 0; i < sr; ++i) { L[i] = 0.8f * std::sin(6.2831853f * 220.0f * i / sr); R[i] = L[i]; }
        processStereo(dsp, L, R);
        bool finite = true; for (float x : L) if (!std::isfinite(x)) finite = false;
        char buf[128]; std::snprintf(buf, sizeof buf, "(peak %.3f — safety clamp at 1.5 may engage at extreme settings)", peak(L));
        check(finite && peak(L) <= 1.5001f, "torture: maxed settings bounded, no NaN/inf", buf);
    }

    // --- Demo renders ---
    {
        const float notes[8] = { 220.0f, 261.63f, 329.63f, 392.0f, 329.63f, 261.63f, 246.94f, 220.0f };
        const int phraseLen = sr * 4;
        std::vector<float> phrase(phraseLen, 0.0f);
        for (int k = 0; k < 8; ++k)
        {
            int start = (int)(k * 0.5 * sr);
            for (int i = 0; i < (int)(1.2 * sr) && start + i < phraseLen; ++i)
                phrase[start + i] += 0.4f * pluck(i / (float)sr, notes[k]);
        }
        // Ambient demo: 0-4 s dry, 4-8 s through "AIR Lyndhurst Hall".
        std::vector<float> L(phraseLen * 2, 0.0f), R(phraseLen * 2, 0.0f);
        for (int i = 0; i < phraseLen; ++i) { L[i] = phrase[i]; R[i] = phrase[i]; }
        AmbientDSP dsp; AmbientParams p;
        p.reverb.size = 0.95f; p.reverb.decay = 0.87f; p.reverb.damping = 0.35f;
        p.reverb.diffusion = 0.85f; p.reverb.modulation = 0.55f; p.reverb.preDelayMs = 45;
        p.blend = 0.6f;
        dsp.setParams(p); dsp.prepare(sr); dsp.reset();
        std::vector<float> outL(phraseLen, 0), outR(phraseLen, 0);
        {
            std::vector<float> inL = phrase, inR = phrase;
            float* ip[2] = { inL.data(), inR.data() };
            float* op[2] = { outL.data(), outR.data() };
            for (int off = 0; off < phraseLen; off += 512)
            {
                const int n = std::min(512, phraseLen - off);
                float* ipb[2] = { ip[0] + off, ip[1] + off };
                float* opb[2] = { op[0] + off, op[1] + off };
                dsp.process(ipb, opb, n);
            }
        }
        for (int i = 0; i < phraseLen; ++i) { L[phraseLen + i] = outL[i]; R[phraseLen + i] = outR[i]; }
        writeWav(ambientDemo, L, R, sr);
        std::printf("ambient demo written: %s (%.1f s: 0-4 s dry, 4-8 s AIR Lyndhurst Hall)\n",
                    ambientDemo.c_str(), L.size() / (float)sr);
        check(peak(L) > 0.1f && peak(L) < 1.5f, "ambient demo render level sane", "");

        // Vocal demo: vibrato phrase, 0-4 s dry, 4-8 s through "Airy Vocal".
        std::vector<float> vox(phraseLen, 0.0f);
        for (int k = 0; k < 8; ++k)
        {
            const float f0 = notes[k];
            int start = (int)(k * 0.5 * sr);
            for (int i = 0; i < (int)(0.45 * sr) && start + i < phraseLen; ++i)
            {
                const float t = i / (float)sr;
                const float vib = 1.0f + 0.006f * std::sin(6.2831853f * 5.5f * t);
                const float env = std::min(1.0f, t * 12.0f) * std::exp(-t * 1.2f);
                vox[start + i] += 0.35f * env * (std::sin(6.2831853f * f0 * vib * t)
                    + 0.3f * std::sin(6.2831853f * 2.0f * f0 * vib * t)
                    + 0.12f * std::sin(6.2831853f * 3.0f * f0 * vib * t));
            }
        }
        std::vector<float> VL(phraseLen * 2, 0.0f), VR(phraseLen * 2, 0.0f);
        for (int i = 0; i < phraseLen; ++i) { VL[i] = vox[i]; VR[i] = vox[i]; }
        VocalChannelDSP vdsp; VocalChannelParams vp;
        vp.hpFreqHz = 100; vp.highDb = 3; vp.compThresholdDb = -20; vp.compRatio = 3;
        vp.compMakeupDb = 4; vp.satDrive = 0.08f; vp.blend = 1.0f;
        vdsp.setParams(vp); vdsp.prepare(sr); vdsp.reset();
        std::vector<float> voutL(phraseLen, 0), voutR(phraseLen, 0);
        {
            std::vector<float> inL = vox, inR = vox;
            for (int off = 0; off < phraseLen; off += 512)
            {
                const int n = std::min(512, phraseLen - off);
                float* ipb[2] = { inL.data() + off, inR.data() + off };
                float* opb[2] = { voutL.data() + off, voutR.data() + off };
                vdsp.process(ipb, opb, n);
            }
        }
        for (int i = 0; i < phraseLen; ++i) { VL[phraseLen + i] = voutL[i]; VR[phraseLen + i] = voutR[i]; }
        writeWav(vocalDemo, VL, VR, sr);
        std::printf("vocal demo written: %s (%.1f s: 0-4 s dry, 4-8 s Airy Vocal)\n",
                    vocalDemo.c_str(), VL.size() / (float)sr);
        check(peak(VL) > 0.05f && peak(VL) < 1.5f, "vocal demo render level sane", "");
    }

    std::printf(failures == 0 ? "ALL CHECKS PASSED\n" : "%d CHECK(S) FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
