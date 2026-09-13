#include "JunoChorus.h"

namespace aeriform::dsp
{

static constexpr float kBbdStagesOver2 = 128.0f;   // MN3009: 256 stages, two-phase clock
static constexpr float kMaxDelayMs     = 16.0f;    // headroom over 12.8 ms MN3009 max

void JunoChorus::prepare (double sr)
{
    sampleRate = sr;

    const int maxSamples = (int) std::ceil (kMaxDelayMs * 0.001 * sampleRate) + 32;
    lineL.prepare (maxSamples);
    lineR.prepare (maxSamples);

    preL.setSampleRate ((float) sampleRate);
    preR.setSampleRate ((float) sampleRate);
    postL.setSampleRate ((float) sampleRate);
    postR.setSampleRate ((float) sampleRate);

    preL.set (8000.0f, 0.707f);
    preR.set (8000.0f, 0.707f);
    postL.set (toneHz, 0.707f);
    postR.set (toneHz, 0.707f);

    dcL.setCutoff (12.0f, (float) sampleRate);
    dcR.setCutoff (12.0f, (float) sampleRate);

    htiL.setCutoff (6000.0f, (float) sampleRate);
    htiR.setCutoff (6000.0f, (float) sampleRate);

    // 20 ms smooth ramp for parameter/mode transitions
    rampCoeff = 1.0f - std::exp (-1.0f / (0.020f * (float) sampleRate));

    reset();
}

void JunoChorus::reset()
{
    lineL.clear();
    lineR.clear();
    htiL.reset();
    htiR.reset();
    dcL.reset();
    dcR.reset();

    lfoPhase = 0.0f;
    mode = pendingMode;

    const auto& s = specFor (mode);
    curMinS = tgtMinS = s.minDelayMs * 0.001f * (float) sampleRate;
    curMaxS = tgtMaxS = s.maxDelayMs * 0.001f * (float) sampleRate;
    mix = mixTarget;
}

float JunoChorus::mapDelay (float u, float minS, float maxS) const noexcept
{
    // (a) linear in delay time
    const float tauLin = minS + (maxS - minS) * u;

    // (b) linear in BBD clock frequency: tau = 128 / f_c, f_c linear in u
    const float invMax = 1.0f / std::max (1.0f, maxS);
    const float invMin = 1.0f / std::max (1.0f, minS);
    const float tauClk = 1.0f / (invMax + (invMin - invMax) * u);

    return tauLin + (tauClk - tauLin) * curve;
}

void JunoChorus::process (float* left, float* right, int numSamples) noexcept
{
    if (pendingMode != mode)
    {
        mode = pendingMode;
        const auto& s = specFor (mode);
        tgtMinS = s.minDelayMs * 0.001f * (float) sampleRate;
        tgtMaxS = s.maxDelayMs * 0.001f * (float) sampleRate;
    }

    if (mode == Mode::Off && mix <= 0.0001f && mixTarget <= 0.0001f)
        return;

    const auto& s = specFor (mode);
    lfoInc = (s.rateHz * rateScale) / (float) sampleRate;
    const bool sine = s.sineShape;

    postL.set (toneHz, 0.707f);
    postR.set (toneHz, 0.707f);

    const float limitS = (float) lineL.buffer.size() - 4.0f;
    const float invDriveNorm = 1.0f / std::tanh (drive);

    for (int i = 0; i < numSamples; ++i)
    {
        curMinS += (tgtMinS - curMinS) * rampCoeff;
        curMaxS += (tgtMaxS - curMaxS) * rampCoeff;
        mix     += (mixTarget - mix)   * rampCoeff;

        const float m = sine ? std::sin (kTwoPi * lfoPhase)
                             : triangle (lfoPhase);
        lfoPhase += lfoInc;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

        float dL = 0.0f, dR = 0.0f;
        float dL_cross = 0.0f, dR_cross = 0.0f;

        if (mode == Mode::Dimension)
        {
            const float p0 = lfoPhase;
            const float p1 = std::fmod (lfoPhase + 0.25f, 1.0f);
            const float p2 = std::fmod (lfoPhase + 0.50f, 1.0f);
            const float p3 = std::fmod (lfoPhase + 0.75f, 1.0f);

            const float m0 = std::sin (kTwoPi * p0);
            const float m1 = std::sin (kTwoPi * p1);
            const float m2 = std::sin (kTwoPi * p2);
            const float m3 = std::sin (kTwoPi * p3);

            const float span = curMaxS - curMinS;
            dL       = std::clamp (curMinS + span * 0.5f * (m0 * depth + 1.0f), 1.0f, limitS);
            dR       = std::clamp (curMinS + span * 0.5f * (m2 * depth * width + 1.0f), 1.0f, limitS);
            dL_cross = std::clamp (curMinS + span * 0.5f * (m1 * depth + 1.0f), 1.0f, limitS);
            dR_cross = std::clamp (curMinS + span * 0.5f * (m3 * depth * width + 1.0f), 1.0f, limitS);
        }
        else
        {
            const float uL = std::clamp (0.5f * ( m * depth + 1.0f), 0.0f, 1.0f);
            const float uR = std::clamp (0.5f * (-m * depth * width + 1.0f), 0.0f, 1.0f);

            dL = std::clamp (mapDelay (uL, curMinS, curMaxS), 1.0f, limitS);
            dR = std::clamp (mapDelay (uR, curMinS, curMaxS), 1.0f, limitS);
        }

        const float inL = left[i];
        const float inR = right[i];
        const float monoIn = 0.5f * (inL + inR);

        float wL = 0.0f, wR = 0.0f, bp = 0.0f, hp = 0.0f;
        preL.process (monoIn, wL, bp, hp);
        preR.process (monoIn, wR, bp, hp);

        // Asymmetric soft saturation (no compander in Juno BBD)
        wL = std::tanh (drive * wL + 0.05f * wL * wL) * invDriveNorm;
        wR = std::tanh (drive * wR + 0.05f * wR * wR) * invDriveNorm;

        wL = dcL.process (wL);
        wR = dcR.process (wR);

        lineL.push (wL);
        lineR.push (wR);

        float yL = lineL.read (dL);
        float yR = lineR.read (dR);

        if (mode == Mode::Dimension)
        {
            const float yL_c = lineL.read (dL_cross);
            const float yR_c = lineR.read (dR_cross);
            const float dimL = 0.7f * yL - 0.35f * yR + 0.25f * yL_c;
            const float dimR = 0.7f * yR - 0.35f * yL + 0.25f * yR_c;
            yL = dimL;
            yR = dimR;
        }

        // Clock-dependent HF loss: f_c = 128 / tau, f_hti = f_c / 20
        if ((i & 15) == 0)
        {
            const float tauL = dL / (float) sampleRate;
            const float tauR = dR / (float) sampleRate;
            htiL.setCutoff (std::clamp (kBbdStagesOver2 / tauL / 20.0f, 800.0f, 12000.0f), (float) sampleRate);
            htiR.setCutoff (std::clamp (kBbdStagesOver2 / tauR / 20.0f, 800.0f, 12000.0f), (float) sampleRate);
        }

        yL = htiL.process (yL);
        yR = htiR.process (yR);

        float postLOut = 0.0f, postROut = 0.0f;
        postL.process (yL, postLOut, bp, hp);
        postR.process (yR, postROut, bp, hp);

        left[i]  = inL + mix * postLOut;
        right[i] = inR + mix * postROut;
    }
}

} // namespace aeriform::dsp
