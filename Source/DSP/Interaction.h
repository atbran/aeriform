#pragma once

#include "DspUtils.h"
#include "../Params/ParameterLayout.h"

namespace aeriform::dsp
{
/**
    Combines Exciter A and B. FM / PM / Sync are applied inside the exciter
    slots (B modulates A's pitch / phase); this stage handles the amplitude-
    domain modes, DC blocking, level normalisation and drive.
*/
class Interaction
{
public:
    struct Params
    {
        InteractionMode mode = InteractionMode::Crossfade;
        float interaction = 0.5f, balance = 0.0f, depth = 0.5f, a2b = 0.0f, normalize = 0.5f, drive = 0.0f;
        bool dcBlock = true, aActive = true, bActive = false;
    };

    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        dc.setCutoff (10.0f, sr);
        env.setCutoff (15.0f, sr);
        reset();
    }
    void reset() noexcept { dc.reset(); env.reset (0.3f); hold = 0.0f; lastB = 0.0f; }
    void update (const Params& p) noexcept { params = p; }

    inline float next (float a, float b) noexcept
    {
        const auto& p = params;
        if (! p.bActive) return finish (p.aActive ? a : 0.0f, false);
        if (! p.aActive) return finish (b, false);

        const float gA = std::min (1.0f, 1.0f - p.balance), gB = std::min (1.0f, 1.0f + p.balance);
        a *= gA; b *= gB;
        // A -> B amplitude modulation (always available)
        if (p.a2b > 0.0005f) b *= 1.0f + p.a2b * a;

        const float i = p.interaction;
        const float plain = 0.5f * (a + b);
        float combined;
        switch (p.mode)
        {
            case InteractionMode::Crossfade:   return finish (a * (1.0f - i) + b * i, true);
            case InteractionMode::Add:         combined = (a + b) * 0.7f; break;
            case InteractionMode::Subtract:    combined = (a - b) * 0.7f; break;
            case InteractionMode::Ring:        combined = a * b * (1.0f + 2.0f * i); break;
            case InteractionMode::AM:          combined = a * (1.0f + i * b) * 0.6f + 0.2f * b * (1.0f - i); break;
            case InteractionMode::FM:
            case InteractionMode::PM:
            case InteractionMode::Sync:        return finish (a * (1.0f - 0.5f * p.depth) + b * 0.5f * p.depth * (1.0f - i), true);
            case InteractionMode::Xor:
            {
                const int qa = (int) std::lround (std::clamp (a, -1.0f, 1.0f) * 127.0f);
                const int qb = (int) std::lround (std::clamp (b, -1.0f, 1.0f) * 127.0f);
                const int x = ((qa + 128) ^ (qb + 128)) - 128;
                combined = lerp (plain, (float) x / 127.0f, i);
                break;
            }
            case InteractionMode::MinMax:      combined = lerp (std::min (a, b), std::max (a, b), i); break;
            case InteractionMode::RectDiff:    combined = std::fabs (a - b) * (1.0f + i) - 0.5f; break;
            case InteractionMode::SampleHold:
                if (b >= 0.0f && lastB < 0.0f) hold = a;
                lastB = b;
                combined = lerp (a, hold, i);
                break;
            case InteractionMode::AudioXfade:
            {
                const float pos = std::clamp (i + b * p.depth, 0.0f, 1.0f);
                return finish (a * (1.0f - pos) + b * pos, true);
            }
            default: combined = plain; break;
        }
        return finish (lerp (plain, combined, p.depth), true);
    }

    float getEnvelope() const noexcept { return env.getState(); }

private:
    inline float finish (float x, bool active) noexcept
    {
        const auto& p = params;
        if (active && p.dcBlock) x = dc.process (x);
        if (active && p.normalize > 0.0005f)
        {
            const float e = env.process (std::fabs (x));
            const float g = 0.35f / std::max (e, 0.05f);
            x *= lerp (1.0f, std::min (g, 4.0f), p.normalize);
        }
        if (p.drive > 0.0005f)
        {
            const float d = 1.0f + 6.0f * p.drive;
            x = fastTanh (x * d) / std::sqrt (d);
        }
        return x;
    }

    float sr = 88200.0f;
    Params params;
    DcBlocker dc;
    OnePole env;
    float hold = 0.0f, lastB = 0.0f;
};
} // namespace aeriform::dsp
