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
        for (const auto& s : cfg.slots)
        {
            if (s.source == ModSource::None || s.dest == ModDest::None || std::fabs (s.depth) < 1.0e-9f) continue;
            out[(size_t) s.dest] += s.depth * sources[(size_t) s.source];
        }
        for (auto& v : out) v = std::clamp (v, -2.0f, 2.0f);
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
};
} // namespace aeriform::dsp
