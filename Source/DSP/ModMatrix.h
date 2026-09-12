#pragma once

#include <array>
#include <algorithm>
#include "../Params/ParameterLayout.h"

namespace aeriform::dsp
{
struct ModSlot
{
    ModSource source = ModSource::None;
    ModDest   dest   = ModDest::None;
    float     depth  = 0.0f;
};

struct ModConfig
{
    std::array<ModSlot, ids::numModSlots> slots {};
};

using ModSources = std::array<float, (size_t) ModSource::Count>;   // current source values (-1..1 or 0..1)
using ModValues  = std::array<float, (size_t) ModDest::Count>;     // summed modulation per destination

/**
    Evaluates the routing matrix: out[dest] = sum(depth * source). Pure,
    allocation-free; the same code serves per-voice and global evaluation.
*/
class ModMatrix
{
public:
    static inline void evaluate (const ModConfig& cfg, const ModSources& sources, ModValues& out) noexcept
    {
        out.fill (0.0f);
        ModSources effSources = sources;

        auto isMacroDest = [] (ModDest d) noexcept
        {
            return d >= ModDest::Macro1 && d <= ModDest::Macro4;
        };

        // Pass 1 & 2 for macro destinations (allowing standard modulators & chaining between macros)
        for (int pass = 0; pass < 2; ++pass)
        {
            float macroMod[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            for (const auto& s : cfg.slots)
            {
                if (s.source == ModSource::None || ! isMacroDest (s.dest) || std::fabs (s.depth) < 1.0e-9f) continue;
                const int mIdx = (int) s.dest - (int) ModDest::Macro1;
                if (mIdx >= 0 && mIdx < 4)
                    macroMod[mIdx] += s.depth * effSources[(size_t) s.source];
            }
            for (int i = 0; i < 4; ++i)
            {
                const auto d = (ModDest) ((int) ModDest::Macro1 + i);
                const auto src = (ModSource) ((int) ModSource::Macro1 + i);
                out[(size_t) d] = std::clamp (macroMod[i], -2.0f, 2.0f);
                effSources[(size_t) src] = std::clamp (sources[(size_t) src] + out[(size_t) d], 0.0f, 1.0f);
            }
        }

        // Final pass: evaluate all non-macro destinations using the modulated source values
        for (const auto& s : cfg.slots)
        {
            if (s.source == ModSource::None || s.dest == ModDest::None || isMacroDest (s.dest) || std::fabs (s.depth) < 1.0e-9f) continue;
            out[(size_t) s.dest] += s.depth * effSources[(size_t) s.source];
        }

        for (size_t i = 0; i < out.size(); ++i)
        {
            if (! isMacroDest ((ModDest) i))
                out[i] = std::clamp (out[i], -2.0f, 2.0f);
        }
    }

    /** True if any slot targets the destination (used by the GUI to show modulation rings). */
    static bool targets (const ModConfig& cfg, ModDest dest) noexcept
    {
        for (const auto& s : cfg.slots)
            if (s.dest == dest && s.source != ModSource::None && std::fabs (s.depth) >= 1.0e-9f) return true;
        return false;
    }

    /** Maximum absolute modulation amount reaching a destination (for GUI rings). */
    static float maxDepth (const ModConfig& cfg, ModDest dest) noexcept
    {
        float d = 0.0f;
        for (const auto& s : cfg.slots)
            if (s.dest == dest && s.source != ModSource::None) d += std::fabs (s.depth);
        return std::min (d, 2.0f);
    }

    /** Minimum and maximum modulation depth reaching a destination, taking unipolar vs bipolar sources into account. */
    static std::pair<float, float> modulationRange (const ModConfig& cfg, ModDest dest) noexcept
    {
        float minMod = 0.0f;
        float maxMod = 0.0f;
        for (const auto& s : cfg.slots)
        {
            if (s.dest == dest && s.source != ModSource::None && std::fabs (s.depth) >= 1.0e-6f)
            {
                if (isModSourceUnipolar (s.source))
                {
                    minMod += std::min (0.0f, s.depth);
                    maxMod += std::max (0.0f, s.depth);
                }
                else
                {
                    const float absD = std::fabs (s.depth);
                    minMod -= absD;
                    maxMod += absD;
                }
            }
        }
        return { std::clamp (minMod, -2.0f, 2.0f), std::clamp (maxMod, -2.0f, 2.0f) };
    }
};
} // namespace aeriform::dsp
