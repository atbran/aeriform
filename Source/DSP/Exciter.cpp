#include "Exciter.h"

namespace aeriform::dsp
{
void Exciter::prepare (float sr, uint32_t seed)
{
    sampleRate = sr;
    rng.seed (seed);
    slowTurb.seed (seed * 7u + 1u);
    fastTurb.seed (seed * 13u + 5u);
    slowDrift.seed (seed * 31u + 9u);
    slowTurb.setRate (6.0f, sr);
    fastTurb.setRate (45.0f, sr);
    slowDrift.setRate (0.35f, sr);
    lpFilter.setSampleRate (sr);
    hpFilter.setSampleRate (sr);
    lpFilter.set (7000.0f, 0.707f);
    hpFilter.set (40.0f, 0.707f);
    reset();
}

void Exciter::reset()
{
    pinkFilter.reset();
    lpFilter.reset();
    hpFilter.reset();
    pluckRemaining = clickRemaining = puffRemaining = 0;
    noiseGain = 0.0f;
}

void Exciter::noteOn (float vel, float noteHz)
{
    velocity = clamp01 (vel);
    noteRandom = 1.0f + cached.breathRandom * 0.3f * rng.next();

    const float velScale = lerp (1.0f, velocity * velocity, cached.velocityAmount);

    // pluck burst: noise with exponential decay over the burst length
    const int len = std::max (4, (int) (cached.pluckLengthMs * 0.001f * sampleRate));
    pluckRemaining = cached.pluck > 0.001f ? len : 0;
    pluckLevel = cached.pluck * 1.6f * velScale;
    pluckEnv = 1.0f;
    pluckDecay = std::exp (-4.0f / (float) len);

    // tongue / chiff transient: short bright click scaled by attack transient amount
    clickLength = std::max (8, (int) (0.0025f * sampleRate + 0.004f * sampleRate * (1.0f - clamp01 (noteHz / 2000.0f))));
    clickRemaining = cached.attackClick > 0.001f ? clickLength : 0;
    clickLevel = cached.attackClick * 0.9f * velScale;
    clickEnv = 1.0f;
    clickDecay = std::exp (-5.0f / (float) clickLength);

    puffRemaining = 0;
}

void Exciter::noteOff()
{
    if (cached.releaseNoise > 0.001f)
    {
        const int len = (int) (0.09f * sampleRate);
        puffRemaining = len;
        puffLevel = cached.releaseNoise * 0.35f * lerp (1.0f, velocity, cached.velocityAmount);
        puffEnv = 1.0f;
        puffDecay = std::exp (-4.5f / (float) len);
    }
}

void Exciter::update (const ExciterParams& p, float noteHz, float pressureNow, float turbulenceMod)
{
    cached = p;
    color = clamp01 (p.noiseColor);
    turbAmount = clamp01 (p.turbulence + turbulenceMod);
    breathRandomAmount = p.breathRandom;

    const float velScale = lerp (1.0f, 0.25f + 0.75f * velocity, p.velocityAmount);
    noiseGain = p.noise * 0.5f * velScale * noteRandom;
    externalGain = p.externalIn * 2.0f;

    // key tracking of both filters around middle C, plus pressure-dependent brightness
    const float track = std::pow (std::max (noteHz, 20.0f) / 261.63f, 0.6f * p.keyTrack);
    const float pressBright = std::exp2 (p.pressureBright * 2.5f * (pressureNow - 0.5f));
    const float lp = std::clamp (p.lowpassHz * track * pressBright, 60.0f, sampleRate * 0.45f);
    const float hp = std::clamp (p.highpassHz * track, 5.0f, sampleRate * 0.3f);
    lpFilter.set (lp, 0.65f);
    hpFilter.set (hp, 0.6f);
}
} // namespace aeriform::dsp
