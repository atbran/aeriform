#pragma once

#include "../DspUtils.h"

namespace aeriform::dsp
{
/**
    "Orbit": an original complex oscillator. Two sine operators are coupled by
    phase modulation in both directions (Complexity), the first operator feeds
    its own phase back (Feedback) and can be bent (Bend); a second detuned pair
    adds Spread; Phase Warp injects operator 2 into operator 1's read-out; a
    bounded logistic map, advanced once per cycle, perturbs the phases (Chaos);
    Instability random-walks the ratio. Everything passes through sin(), so the
    output is bounded for any setting, and the chaos sequence is seeded
    deterministically per note.
*/
class ComplexOsc
{
public:
    struct Params
    {
        float complexity = 0.3f, symmetry = 0.0f, bend = 0.2f, instability = 0.0f, spread = 0.0f,
              warp = 0.0f, feedback = 0.2f, chaos = 0.0f, ratio = 2.0f;
    };

    void prepare (float sampleRate) noexcept { sr = sampleRate; walk.setRate (1.5f, sr); }
    void seed (uint32_t s) noexcept { rng.seed (s); walk.seed (s ^ 0x5bd1e995u); chaosX = 0.37f + 0.3f * rng.next01(); }
    void setFrequency (float hz) noexcept { inc = std::clamp (hz, 0.0f, sr * 0.45f) / sr; }
    void reset (float phase01) noexcept
    {
        p1 = phase01 - std::floor (phase01); p2 = p1 * 2.0f; p2 -= std::floor (p2); p3 = p1; p4 = p2;
        y1 = y2 = y3 = y4 = 0.0f; chaosX = 0.37f + 0.3f * rng.next01(); chaosValue = 0.0f; wrappedFlag = false;
    }
    void hardSync() noexcept { p1 = p2 = p3 = p4 = 0.0f; }
    bool wrapped() const noexcept { return wrappedFlag; }
    float getPhase() const noexcept { return p1; }

    /** Current chaotic map state, for the Chaos X / Y modulation sources (0..1). */
    float chaosStateX() const noexcept { return chaosX; }
    float chaosStateY() const noexcept { return 0.5f + 0.5f * y2; }

    inline float next (const Params& q, float fmOctaves, float pmPhase) noexcept
    {
        const float step = inc * std::exp2 (std::clamp (fmOctaves, -6.0f, 6.0f));
        const float instab = q.instability * q.instability;
        const float ratio = q.ratio * std::exp2 (instab * 0.5f * walk.next());
        const float spreadDetune = 1.0f + q.spread * 0.03f;

        wrappedFlag = false;
        p1 += step;
        if (p1 >= 1.0f)
        {
            p1 -= 1.0f;
            wrappedFlag = true;
            // advance the logistic map once per cycle: deterministic, bounded in (0, 1)
            const float a = 3.57f + 0.42f * q.chaos;
            chaosX = a * chaosX * (1.0f - chaosX);
            chaosX = std::clamp (chaosX, 0.001f, 0.999f);
            chaosValue = (chaosX - 0.5f) * 2.0f;
        }
        p2 += step * ratio;             p2 -= std::floor (p2);
        p3 += step * spreadDetune;      p3 -= std::floor (p3);
        p4 += step * ratio / spreadDetune; p4 -= std::floor (p4);

        const float chaosMod = q.chaos * 0.35f * chaosValue;
        const float bendMod  = q.bend * 0.25f * y1 * std::fabs (y1);
        const float warpMod  = q.warp * 0.3f * std::sin (kTwoPi * p2);

        // operator 1: self feedback + coupling from operator 2 + warp + chaos + external PM
        const float ph1 = p1 + pmPhase + q.feedback * 0.45f * y1 + q.complexity * 0.5f * y2 + bendMod + warpMod + chaosMod;
        const float n1 = std::sin (kTwoPi * ph1);
        // operator 2: coupled from operator 1
        const float ph2 = p2 + q.complexity * 0.5f * y1 + chaosMod * 0.5f;
        const float n2 = std::sin (kTwoPi * ph2);
        y1 = n1; y2 = n2;

        float out = 0.65f * y1 + 0.35f * q.complexity * y2 + 0.15f * (1.0f - q.complexity) * y2;

        if (q.spread > 0.001f)
        {
            const float n3 = std::sin (kTwoPi * (p3 + q.feedback * 0.45f * y3 + q.complexity * 0.5f * y4));
            const float n4 = std::sin (kTwoPi * (p4 + q.complexity * 0.5f * y3));
            y3 = n3; y4 = n4;
            out = lerp (out, 0.5f * out + 0.5f * (0.65f * y3 + 0.35f * y4), q.spread);
        }

        // symmetry: asymmetric waveshaping (even harmonics), DC removed downstream
        if (std::fabs (q.symmetry) > 0.001f)
            out = (out + q.symmetry * 0.6f * out * std::fabs (out)) / (1.0f + 0.6f * std::fabs (q.symmetry));
        return out;
    }

private:
    float sr = 88200.0f, inc = 0.0f;
    float p1 = 0.0f, p2 = 0.0f, p3 = 0.0f, p4 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f, y3 = 0.0f, y4 = 0.0f;
    float chaosX = 0.5f, chaosValue = 0.0f;
    bool wrappedFlag = false;
    Noise rng;
    SlowRandom walk;
};
} // namespace aeriform::dsp
