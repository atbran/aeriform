#pragma once

#include "../DspUtils.h"
#include "../../Params/ParameterLayout.h"

namespace aeriform::dsp
{
/**
    Noise laboratory: twelve deterministic (seeded) noise models running at the
    oversampled rate. Model selection is by ExciterModel; parameters that do not
    apply to a model are ignored.
*/
class NoiseLab
{
public:
    struct Params
    {
        float color = 0.0f, density = 0.5f, grainMs = 20.0f, bandwidth = 0.5f, centerHz = 1000.0f,
              correlation = 0.0f, burstMs = 40.0f, burstEnv = 0.5f, turbulence = 0.3f, gustHz = 0.5f;
        int seed = 0;
    };

    void prepare (float sampleRate, int oversampleFactor) noexcept
    {
        sr = sampleRate;
        colorLP.setCutoff (2000.0f, sr);
        bp.setSampleRate (sr);
        for (auto& r : metal) r.setSampleRate (sr);
        turb.setRate (7.0f, sr);
        gust.setRate (0.5f, sr);
        sweep.setRate (0.2f, sr);
        tiltLP.setCutoff (1500.0f, sr);
        gainComp = std::sqrt ((float) std::max (1, oversampleFactor));   // decimation removes half the band per 2x: keep loudness
        reset();
    }

    /** Deterministic seeding: same seed + voice -> same sequence. */
    void seed (int userSeed, int voiceIndex) noexcept
    {
        const uint32_t s = 0x9E3779B9u * (uint32_t) (userSeed + 1) + 0x85EBCA6Bu * (uint32_t) (voiceIndex + 1);
        rng.seed (s);
        turb.seed (s ^ 0x1234567u);
        gust.seed (s ^ 0x89abcdefu);
        sweep.seed (s ^ 0x0badf00du);
        metalSeed = s;
        metalDirty = true;
    }

    void reset() noexcept
    {
        pink.reset(); brown = 0.0f; prevWhite = 0.0f; prevPink = 0.0f; bp.reset();
        for (auto& r : metal) r.reset();
        grainRemaining = 0; grainPos = 0; burstRemaining = 0; burstEnvValue = 0.0f; velvetCounter = 0; gustEnv = 0.0f;
        tiltLP.reset();
    }

    /** Update per control block. */
    void update (ExciterModel m, const Params& p, float keyHz) noexcept
    {
        model = m;
        params = p;
        turb.setRate (4.0f + 12.0f * p.turbulence, sr);
        gust.setRate (p.gustHz, sr);
        sweep.setRate (p.gustHz * 0.37f, sr);
        const float center = std::clamp (p.centerHz * (keyHz > 0.0f ? keyHz / 261.63f : 1.0f), 30.0f, sr * 0.45f);
        const float q = 0.7f + 30.0f * (1.0f - p.bandwidth) * (1.0f - p.bandwidth);
        bp.set (center, q);
        if (m == ExciterModel::NoiseMetallic && (metalDirty || std::fabs (center - metalCenter) > 1.0f || std::fabs (q - metalQ) > 0.01f))
        {
            metalDirty = false; metalCenter = center; metalQ = q;
            Noise r; r.seed (metalSeed);
            for (int i = 0; i < kMetalModes; ++i)
            {
                const float ratio = std::exp2 ((r.next01() * 2.0f - 1.0f) * (1.0f + 2.0f * p.bandwidth));
                metal[(size_t) i].set (std::clamp (center * ratio, 30.0f, sr * 0.45f), 4.0f + 40.0f * (1.0f - p.bandwidth));
            }
        }
        tiltLP.setCutoff (1500.0f, sr);
        grainSamples = std::max (4, (int) (p.grainMs * 0.001f * sr));
        burstSamples = std::max (4, (int) (p.burstMs * 0.001f * sr));
    }

    /** shared = sample of the engine-wide shared noise stream (for Correlation). */
    inline float next (float shared) noexcept
    {
        const float white = lerp (rng.next(), shared, params.correlation);
        float out = 0.0f;
        switch (model)
        {
            case ExciterModel::NoiseWhite:  out = white; break;
            case ExciterModel::NoisePink:   out = pink.process (white) * 2.4f; break;
            case ExciterModel::NoiseBrown:
                brown = 0.998f * brown + white * 0.05f;
                out = brown * 6.0f;
                break;
            case ExciterModel::NoiseBlue:
            {
                const float pk = pink.process (white) * 2.4f;
                out = (pk - prevPink) * 2.5f;   // differentiated pink ~ +3 dB/oct
                prevPink = pk;
                break;
            }
            case ExciterModel::NoiseViolet:
                out = (white - prevWhite) * 0.9f;   // differentiated white ~ +6 dB/oct
                prevWhite = white;
                break;
            case ExciterModel::NoiseBand:
                out = bp.bandpass (white) * (1.0f + 3.0f * (1.0f - params.bandwidth));
                break;
            case ExciterModel::NoiseVelvet:
            {
                // sparse +/-1 impulses: one per grid cell, cell size set by density
                const int cell = std::max (2, (int) (sr / (200.0f + 6000.0f * params.density)));
                if (--velvetCounter <= 0)
                {
                    velvetCounter = cell;
                    velvetPos = (int) (rng.next01() * (float) (cell - 1));
                    velvetSign = rng.next() < 0.0f ? -1.0f : 1.0f;
                }
                out = (velvetCounter == velvetPos + 1) ? velvetSign * 2.5f : 0.0f;
                break;
            }
            case ExciterModel::NoiseCrackle:
            {
                // random sparse bursts with random amplitude and exponential decay
                const float rate = 2.0f + 400.0f * params.density * params.density;
                if (rng.next01() < rate / sr) { burstRemaining = burstSamples; burstAmp = 0.3f + 0.7f * rng.next01(); burstEnvValue = 1.0f; }
                if (burstRemaining > 0)
                {
                    const float attackFrac = 0.05f + 0.9f * params.burstEnv;
                    const float pos = 1.0f - (float) burstRemaining / (float) burstSamples;
                    const float env = pos < attackFrac ? pos / attackFrac : std::exp (-6.0f * (pos - attackFrac) / (1.0f - attackFrac + 1.0e-3f));
                    out = white * env * burstAmp * 2.0f;
                    --burstRemaining;
                }
                break;
            }
            case ExciterModel::NoiseSteam:
            {
                const float pk = pink.process (white) * 2.4f;
                const float t = turb.next();
                const float hiss = bp.bandpass (white) * 1.5f;
                out = (pk * 0.7f + hiss * (0.3f + 0.5f * params.bandwidth)) * (1.0f + params.turbulence * (0.9f * t + 0.4f * turb2 (t)));
                break;
            }
            case ExciterModel::NoiseWind:
            {
                // gusts: slow random amplitude with density-controlled activity, swept low-pass
                const float g = gust.next();
                const float target = (0.15f + 0.85f * std::max (0.0f, 0.25f + 0.75f * g)) * (0.3f + 0.7f * params.density);
                gustEnv += 0.0005f * (target - gustEnv);
                const float sw = sweep.next();
                tiltLP.setCoefficient (std::clamp (0.01f + 0.08f * (0.5f + 0.5f * sw) * (0.5f + 0.5f * params.bandwidth), 0.002f, 0.5f));
                out = tiltLP.process (white) * 4.0f * gustEnv * (1.0f + 0.5f * params.turbulence * turb.next());
                break;
            }
            case ExciterModel::NoiseAerosol:
            {
                // grains: windowed noise bursts at a density-controlled rate
                if (grainRemaining <= 0)
                {
                    const float rate = 5.0f + 600.0f * params.density * params.density;
                    if (rng.next01() < rate / sr) { grainRemaining = grainSamples; grainPos = 0; grainAmp = 0.4f + 0.6f * rng.next01(); grainTilt = rng.next(); }
                }
                if (grainRemaining > 0)
                {
                    const float ph = (float) grainPos / (float) grainSamples;
                    const float win = 0.5f - 0.5f * std::cos (kTwoPi * ph);
                    const float src = grainTilt > 0.0f ? bp.bandpass (white) * 2.0f : white;
                    out = src * win * grainAmp * 2.5f;
                    ++grainPos; --grainRemaining;
                }
                break;
            }
            case ExciterModel::NoiseMetallic:
            {
                float sum = 0.0f;
                for (auto& r : metal) sum += r.bandpass (white);
                out = sum * (1.5f + 2.0f * (1.0f - params.bandwidth));
                break;
            }
            default: out = white; break;
        }

        // colour: spectral tilt (dark <- 0 -> bright)
        if (std::fabs (params.color) > 0.001f)
        {
            const float lp = colorLP.process (out);
            out = params.color < 0.0f ? lerp (out, lp * 1.5f, -params.color) : lerp (out, (out - lp) * 1.5f, params.color);
        }
        return sanitizeFast (out * gainComp);
    }

    void setColorCutoff (float hz) noexcept { colorLP.setCutoff (hz, sr); }

private:
    static constexpr int kMetalModes = 5;
    static inline float turb2 (float t) noexcept { return t * t * (t < 0.0f ? -1.0f : 1.0f); }
    static inline float sanitizeFast (float v) noexcept { return (v > -50.0f && v < 50.0f) ? v : 0.0f; }

    float sr = 88200.0f, gainComp = 1.0f;
    ExciterModel model = ExciterModel::NoiseWhite;
    Params params;
    Noise rng;
    PinkFilter pink;
    SVF bp;
    SVF metal[kMetalModes];
    uint32_t metalSeed = 1; bool metalDirty = true; float metalCenter = 0.0f, metalQ = 0.0f;
    SlowRandom turb, gust, sweep;
    OnePole tiltLP, colorLP;
    float brown = 0.0f, prevWhite = 0.0f, prevPink = 0.0f;
    int grainRemaining = 0, grainPos = 0, grainSamples = 800; float grainAmp = 1.0f, grainTilt = 0.0f;
    int burstRemaining = 0, burstSamples = 1000; float burstAmp = 1.0f, burstEnvValue = 0.0f;
    int velvetCounter = 0, velvetPos = 0; float velvetSign = 1.0f;
    float gustEnv = 0.0f;
};
} // namespace aeriform::dsp
