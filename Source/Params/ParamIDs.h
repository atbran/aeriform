#pragma once

#include <juce_core/juce_core.h>

// Stable parameter identifiers. NEVER rename an ID once released: hosts store
// automation and session state keyed by these strings. Add new IDs instead.
namespace aeriform::ids
{
// ---- BREATH: exciter ----------------------------------------------------
inline constexpr const char* excNoise         = "exc_noise";
inline constexpr const char* excNoiseColor    = "exc_noise_color";
inline constexpr const char* excPressure      = "exc_pressure";
inline constexpr const char* excPluck         = "exc_pluck";
inline constexpr const char* excPluckLength   = "exc_pluck_len";
inline constexpr const char* excLowpass       = "exc_lp";
inline constexpr const char* excHighpass      = "exc_hp";
inline constexpr const char* excTurbulence    = "exc_turb";
inline constexpr const char* excVelocity      = "exc_vel";
inline constexpr const char* excExternalIn    = "exc_ext_in";
inline constexpr const char* excKeyTrack      = "exc_keytrack";
inline constexpr const char* excAttackClick   = "exc_attack_click";
inline constexpr const char* excReleaseNoise  = "exc_release_noise";
inline constexpr const char* excBreathRandom  = "exc_breath_random";
inline constexpr const char* excReed          = "exc_reed";

// ---- BREATH: amplitude envelope / articulation --------------------------
inline constexpr const char* envAttack        = "env_attack";
inline constexpr const char* envDecay         = "env_decay";
inline constexpr const char* envSustain       = "env_sustain";
inline constexpr const char* envRelease       = "env_release";
inline constexpr const char* envVelToPressure = "env_vel_pressure";
inline constexpr const char* artPressBright   = "art_press_bright";
inline constexpr const char* artFlowPitch     = "art_flow_pitch";
inline constexpr const char* artInstability   = "art_instability";
inline constexpr const char* artVariation     = "art_variation";
inline constexpr const char* artCoupling      = "art_coupling";

// ---- RESONATOR ----------------------------------------------------------
inline constexpr const char* resCoarse        = "res_coarse";
inline constexpr const char* resFine          = "res_fine";
inline constexpr const char* resLength        = "res_length";
inline constexpr const char* resKeyTrack      = "res_keytrack";
inline constexpr const char* resFeedback      = "res_feedback";
inline constexpr const char* resDamping       = "res_damping";
inline constexpr const char* resBrightness    = "res_brightness";
inline constexpr const char* resDispersion    = "res_dispersion";
inline constexpr const char* resShape         = "res_shape";
inline constexpr const char* resReflection    = "res_reflect";
inline constexpr const char* resSaturation    = "res_saturation";
inline constexpr const char* resMode          = "res_mode";
inline constexpr const char* resBodyFreq      = "res_body_freq";
inline constexpr const char* resBodyRes       = "res_body_res";
inline constexpr const char* resBodyMix       = "res_body_mix";
inline constexpr const char* resBodyTrack     = "res_body_track";

// ---- MOTION: LFOs (index 1..3 -> lfo1_..lfo3_) ---------------------------
inline constexpr const char* lfoShapeSuffix   = "_shape";
inline constexpr const char* lfoRateSuffix    = "_rate";
inline constexpr const char* lfoSyncSuffix    = "_sync";
inline constexpr const char* lfoDivSuffix     = "_div";
inline constexpr const char* lfoModeSuffix    = "_mode";
inline constexpr const char* lfoFadeSuffix    = "_fade";
inline constexpr const char* lfoPhaseSuffix   = "_phase";

// ---- MOTION: modulation envelope -----------------------------------------
inline constexpr const char* menvAttack       = "menv_attack";
inline constexpr const char* menvDecay        = "menv_decay";
inline constexpr const char* menvSustain      = "menv_sustain";
inline constexpr const char* menvRelease      = "menv_release";

// ---- MOTION: modulation matrix (slot 1..8 -> mod1_src, mod1_dst, mod1_depth)
inline constexpr const char* modSrcSuffix     = "_src";
inline constexpr const char* modDstSuffix     = "_dst";
inline constexpr const char* modDepthSuffix   = "_depth";

// ---- SPACE --------------------------------------------------------------
inline constexpr const char* chorusMix        = "chorus_mix";
inline constexpr const char* chorusRate       = "chorus_rate";
inline constexpr const char* chorusDepth      = "chorus_depth";
inline constexpr const char* chorusWidth      = "chorus_width";
inline constexpr const char* delayMix         = "delay_mix";
inline constexpr const char* delayTime        = "delay_time";
inline constexpr const char* delaySync        = "delay_sync";
inline constexpr const char* delayDiv         = "delay_div";
inline constexpr const char* delayFeedback    = "delay_feedback";
inline constexpr const char* delayTone        = "delay_tone";
inline constexpr const char* delayPingPong    = "delay_pingpong";
inline constexpr const char* reverbMix        = "rev_mix";
inline constexpr const char* reverbSize       = "rev_size";
inline constexpr const char* reverbDecay      = "rev_decay";
inline constexpr const char* reverbDamping    = "rev_damp";
inline constexpr const char* reverbPreDelay   = "rev_predelay";
inline constexpr const char* reverbWidth      = "rev_width";
inline constexpr const char* reverbModulation = "rev_mod";

// ---- MASTER -------------------------------------------------------------
inline constexpr const char* voiceMode        = "voice_mode";
inline constexpr const char* voiceCount       = "voice_count";
inline constexpr const char* glideTime        = "glide_time";
inline constexpr const char* glideLegatoOnly  = "glide_legato";
inline constexpr const char* unisonVoices     = "unison_voices";
inline constexpr const char* unisonDetune     = "unison_detune";
inline constexpr const char* unisonSpread     = "unison_spread";
inline constexpr const char* bendRange        = "bend_range";
inline constexpr const char* mpeEnabled       = "mpe_enable";
inline constexpr const char* outGain          = "out_gain";
inline constexpr const char* outHighpass      = "out_hp";
inline constexpr const char* limiterOn        = "limiter_on";

inline constexpr int numLFOs     = 3;
inline constexpr int numModSlots = 8;

inline juce::String lfoParam (int lfoIndex1Based, const char* suffix)
{
    return "lfo" + juce::String (lfoIndex1Based) + suffix;
}

inline juce::String modParam (int slotIndex1Based, const char* suffix)
{
    return "mod" + juce::String (slotIndex1Based) + suffix;
}
} // namespace aeriform::ids
