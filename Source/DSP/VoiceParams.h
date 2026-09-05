#pragma once

#include <array>
#include <cmath>
#include "../Params/ParameterLayout.h"
#include "ModMatrix.h"

namespace aeriform::dsp
{
struct LfoParams
{
    LfoShape shape = LfoShape::Sine;
    float rateHz = 1.0f;
    bool sync = false;
    int division = 7;
    LfoMode mode = LfoMode::Free;
    float fadeMs = 0.0f;
    float phaseDeg = 0.0f;
};

/**
    Block-rate snapshot of every parameter (plain floats copied from the
    atomics once per block by the engine) plus the derived structures the voices
    need. Indexed by the generated P enum.
*/
struct VoiceParams
{
    std::array<float, (size_t) kNumParams> v {};

    float get (P p) const noexcept  { return v[(size_t) p]; }
    int   geti (P p) const noexcept { return (int) std::lround (v[(size_t) p]); }
    bool  getb (P p) const noexcept { return v[(size_t) p] > 0.5f; }
    template <typename E> E getEnum (P p, E maxExclusive) const noexcept
    {
        const int i = geti (p);
        return (E) (i < 0 ? 0 : (i >= (int) maxExclusive ? (int) maxExclusive - 1 : i));
    }

    ModConfig mod;
    LfoParams lfo[ids::numLFOs];
    double tempoBpm = 120.0;
    float lfoGlobalPhase[ids::numLFOs] { 0.0f, 0.0f, 0.0f };
    int osFactor = 2;          // exciter-chain oversampling factor (2 or 4)
    int controlInterval = 32;  // control-rate block size in samples

    /** Fills the derived structures from the raw values. */
    void derive() noexcept
    {
        for (int i = 0; i < ids::numModSlots; ++i)
        {
            auto& s = mod.slots[(size_t) i];
            s.source = getEnum (ids::modP (i + 1, ids::ModField::Src), ModSource::Count);
            s.dest   = getEnum (ids::modP (i + 1, ids::ModField::Dst), ModDest::Count);
            s.depth  = get (ids::modP (i + 1, ids::ModField::Depth));
        }
        for (int i = 0; i < ids::numLFOs; ++i)
        {
            auto& l = lfo[i];
            l.shape    = getEnum (ids::lfoP (i + 1, ids::LfoField::Shape), LfoShape::Count);
            l.rateHz   = get (ids::lfoP (i + 1, ids::LfoField::Rate));
            l.sync     = getb (ids::lfoP (i + 1, ids::LfoField::Sync));
            l.division = geti (ids::lfoP (i + 1, ids::LfoField::Div));
            l.mode     = getEnum (ids::lfoP (i + 1, ids::LfoField::Mode), LfoMode::Count);
            l.fadeMs   = get (ids::lfoP (i + 1, ids::LfoField::Fade));
            l.phaseDeg = get (ids::lfoP (i + 1, ids::LfoField::Phase));
        }
        const auto q = getEnum (P::quality, QualityMode::Count);
        osFactor = q == QualityMode::High ? 4 : 2;
        controlInterval = q == QualityMode::Eco ? 64 : 32;
    }
};
} // namespace aeriform::dsp
