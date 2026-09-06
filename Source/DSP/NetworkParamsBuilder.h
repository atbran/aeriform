#pragma once

#include "ResonatorNetwork.h"
#include "VoiceParams.h"
#include "ModMatrix.h"

namespace aeriform::dsp
{
/**
    Fills a NetworkParams block from the block-rate parameter snapshot. Shared by
    the polyphonic Voice (per-note excitation) and the Aeriform FX main-input path
    (continuous external excitation, no MIDI note) so the resonator maths, tuning
    and routing behave identically in both.

      baseNote  - tracking centre in MIDI-note units (the played note for a Voice,
                  the FX Root parameter for the effect path).
      pressure  - reed / jet mouth pressure 0..1 for the resonator junctions.
      mod       - evaluated modulation values (per-voice for a Voice, the global
                  matrix result for the FX path).
      var*      - per-voice component variation offsets (0 for the FX path).
*/
void buildNetworkParams (NetworkParams& n,
                         const VoiceParams& p,
                         float baseNote,
                         float pressure,
                         const ModValues& mod,
                         float varDamp = 0.0f,
                         float varBright = 0.0f,
                         float varShape = 0.0f) noexcept;
} // namespace aeriform::dsp
