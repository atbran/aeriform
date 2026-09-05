#include "FactoryPresets.h"
#include "../Params/ParameterLayout.h"

namespace aeriform
{
namespace
{
    using namespace ids;
    using V = std::vector<std::pair<juce::String, float>>;

    struct Build
    {
        V v;
        Build& p (const juce::String& id, float value) { v.emplace_back (id, value); return *this; }
        Build& lfo (int n, LfoShape shape, float rateHz, LfoMode mode = LfoMode::Free, float fadeMs = 0.0f, bool sync = false, int div = 7)
        {
            p (lfoParam (n, lfoShapeSuffix), (float) shape);
            p (lfoParam (n, lfoRateSuffix), rateHz);
            p (lfoParam (n, lfoModeSuffix), (float) mode);
            p (lfoParam (n, lfoFadeSuffix), fadeMs);
            p (lfoParam (n, lfoSyncSuffix), sync ? 1.0f : 0.0f);
            p (lfoParam (n, lfoDivSuffix), (float) div);
            return *this;
        }
        Build& mod (int slot, ModSource src, ModDest dst, float depth)
        {
            p (modParam (slot, modSrcSuffix), (float) src);
            p (modParam (slot, modDstSuffix), (float) dst);
            p (modParam (slot, modDepthSuffix), depth);
            return *this;
        }
        Build& env (float a, float d, float s, float r) { return p (envAttack, a).p (envDecay, d).p (envSustain, s).p (envRelease, r); }
        Build& tube (float feedback, float damping, float brightness, ResMode mode, float reflect, float sat)
        {
            return p (resFeedback, feedback).p (resDamping, damping).p (resBrightness, brightness)
                   .p (resMode, (float) mode).p (resReflection, reflect).p (resSaturation, sat);
        }
        Build& body (float hz, float res, float mix, float track = 0.0f)
        {
            return p (resBodyFreq, hz).p (resBodyRes, res).p (resBodyMix, mix).p (resBodyTrack, track);
        }
        Build& space (float chorus, float delay, float reverb)
        {
            return p (chorusMix, chorus).p (delayMix, delay).p (reverbMix, reverb);
        }
        Build& reverb (float size, float decay, float damp, float pre = 12.0f)
        {
            return p (reverbSize, size).p (reverbDecay, decay).p (reverbDamping, damp).p (reverbPreDelay, pre);
        }
        Build& delay (int div, float feedback, float tone = 4500.0f, bool pingPong = true)
        {
            return p (delaySync, 1.0f).p (delayDiv, div).p (delayFeedback, feedback).p (delayTone, tone).p (delayPingPong, pingPong ? 1.0f : 0.0f);
        }
        Build& unison (int voices, float detune, float spread) { return p (unisonVoices, (float) voices).p (unisonDetune, detune).p (unisonSpread, spread); }
        Build& vibrato (float rate, float fade, float depth) { return lfo (1, LfoShape::Sine, rate, LfoMode::Retrigger, fade).mod (1, ModSource::LFO1, ModDest::Pitch, depth); }
    };

    // sync division indices (see choices::syncDivisions)
    constexpr int d1_2D = 5, d1_4 = 7, d1_4D = 8, d1_4T = 9, d1_8 = 10, d1_8D = 11, d1_8T = 12, d1_16 = 13;
}

const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets = []
    {
        std::vector<FactoryPreset> list;

        // 1 ---------------------------------------------------------------- neutral init
        list.push_back ({ "Init", "Init", {} });

        // 2 ---------------------------------------------------------------- airy flute
        list.push_back ({ "Airy Flute", "Winds", Build()
            .p (excNoise, 0.72f).p (excNoiseColor, 0.3f).p (excPressure, 0.35f).p (excReed, 0.0f).p (excPluck, 0.0f)
            .p (excLowpass, 6000.0f).p (excHighpass, 80.0f).p (excTurbulence, 0.38f).p (excBreathRandom, 0.3f)
            .p (excAttackClick, 0.25f).p (excReleaseNoise, 0.2f)
            .env (40.0f, 300.0f, 0.85f, 220.0f).p (artPressBright, 0.5f).p (artFlowPitch, 0.25f).p (artInstability, 0.15f)
            .tube (0.93f, 0.45f, 0.45f, ResMode::OpenPipe, 0.6f, 0.15f).p (resShape, 0.5f)
            .body (1200.0f, 0.3f, 0.25f, 0.2f)
            .vibrato (5.2f, 350.0f, 0.008f)
            .mod (2, ModSource::ModWheel, ModDest::Turbulence, 0.5f)
            .mod (3, ModSource::Aftertouch, ModDest::Pressure, 0.4f)
            .space (0.0f, 0.12f, 0.28f).delay (d1_8D, 0.25f).reverb (0.6f, 0.5f, 0.45f).v });

        // 3 ---------------------------------------------------------------- warm wooden pipe
        list.push_back ({ "Warm Wooden Pipe", "Winds", Build()
            .p (excNoise, 0.55f).p (excNoiseColor, 0.6f).p (excPressure, 0.5f).p (excReed, 0.2f)
            .p (excLowpass, 3500.0f).p (excHighpass, 60.0f).p (excTurbulence, 0.3f).p (excAttackClick, 0.2f)
            .env (60.0f, 300.0f, 0.85f, 300.0f).p (artVariation, 0.35f).p (artFlowPitch, 0.15f)
            .tube (0.94f, 0.55f, 0.35f, ResMode::OpenPipe, 0.4f, 0.25f).p (resShape, 0.42f)
            .body (600.0f, 0.5f, 0.45f, 0.3f)
            .vibrato (4.8f, 500.0f, 0.006f)
            .mod (2, ModSource::ModWheel, ModDest::Pressure, 0.3f)
            .space (0.0f, 0.0f, 0.3f).reverb (0.5f, 0.45f, 0.5f).v });

        // 4 ---------------------------------------------------------------- reed
        list.push_back ({ "Reed Song", "Reeds", Build()
            .p (excReed, 0.85f).p (excPressure, 0.75f).p (excNoise, 0.25f).p (excNoiseColor, 0.2f)
            .p (excLowpass, 9000.0f).p (excTurbulence, 0.2f).p (excAttackClick, 0.1f)
            .env (30.0f, 200.0f, 0.9f, 150.0f).p (envVelToPressure, 0.8f).p (artPressBright, 0.6f).p (artFlowPitch, 0.1f)
            .tube (0.96f, 0.3f, 0.6f, ResMode::ClosedPipe, 0.25f, 0.35f).p (resShape, 0.5f)
            .body (1800.0f, 0.5f, 0.35f)
            .vibrato (5.5f, 400.0f, 0.006f)
            .mod (2, ModSource::ModWheel, ModDest::Pressure, 0.3f)
            .mod (3, ModSource::Aftertouch, ModDest::Pressure, 0.3f)
            .space (0.0f, 0.0f, 0.22f).reverb (0.5f, 0.45f, 0.4f).v });

        // 5 ---------------------------------------------------------------- organ
        list.push_back ({ "Cathedral Organ", "Organ", Build()
            .p (excNoise, 0.35f).p (excPressure, 0.5f).p (excReed, 0.3f).p (excTurbulence, 0.1f).p (excBreathRandom, 0.05f)
            .p (excAttackClick, 0.15f).p (excReleaseNoise, 0.05f)
            .env (15.0f, 100.0f, 1.0f, 120.0f).p (artFlowPitch, 0.0f).p (artInstability, 0.03f).p (artVariation, 0.1f)
            .tube (0.97f, 0.3f, 0.55f, ResMode::OpenPipe, 0.5f, 0.2f)
            .body (800.0f, 0.3f, 0.2f)
            .unison (2, 6.0f, 0.5f)
            .space (0.25f, 0.0f, 0.45f).p (chorusRate, 0.6f).reverb (0.9f, 0.75f, 0.4f, 25.0f).v });

        // 6 ---------------------------------------------------------------- plucked tube
        list.push_back ({ "Plucked Tube", "Plucked", Build()
            .p (excPluck, 0.9f).p (excPluckLength, 3.0f).p (excNoise, 0.08f).p (excPressure, 0.2f)
            .p (excVelocity, 0.8f).p (excAttackClick, 0.4f).p (excReleaseNoise, 0.0f)
            .env (0.5f, 200.0f, 0.6f, 800.0f)
            .tube (0.985f, 0.4f, 0.6f, ResMode::OpenPipe, 0.3f, 0.2f).p (resShape, 0.28f).p (resDispersion, 0.08f)
            .body (900.0f, 0.3f, 0.25f)
            .space (0.0f, 0.15f, 0.2f).delay (d1_8D, 0.3f).reverb (0.5f, 0.4f, 0.4f).v });

        // 7 ---------------------------------------------------------------- glassy resonator
        list.push_back ({ "Glass Resonator", "Mallets", Build()
            .p (excPluck, 0.7f).p (excPluckLength, 1.5f).p (excNoise, 0.03f).p (excAttackClick, 0.2f).p (excReleaseNoise, 0.0f)
            .env (0.5f, 500.0f, 0.3f, 2000.0f).p (artVariation, 0.4f)
            .tube (0.995f, 0.15f, 0.8f, ResMode::String, 0.1f, 0.05f).p (resShape, 0.2f).p (resDispersion, 0.55f)
            .body (3200.0f, 0.6f, 0.3f)
            .space (0.15f, 0.0f, 0.35f).reverb (0.7f, 0.7f, 0.3f).v });

        // 8 ---------------------------------------------------------------- brass-like horn
        list.push_back ({ "Brass Horn", "Brass", Build()
            .p (excReed, 0.9f).p (excPressure, 0.9f).p (excNoise, 0.2f).p (excNoiseColor, 0.1f)
            .p (excLowpass, 12000.0f).p (excHighpass, 60.0f).p (excAttackClick, 0.3f)
            .env (45.0f, 250.0f, 0.85f, 180.0f).p (envVelToPressure, 0.9f).p (artPressBright, 0.8f).p (artFlowPitch, 0.35f)
            .tube (0.97f, 0.25f, 0.75f, ResMode::OpenPipe, 0.7f, 0.6f)
            .body (1400.0f, 0.4f, 0.4f)
            .vibrato (5.0f, 500.0f, 0.005f)
            .mod (2, ModSource::ModWheel, ModDest::Pressure, 0.35f)
            .mod (3, ModSource::Aftertouch, ModDest::Brightness, 0.3f)
            .space (0.0f, 0.0f, 0.25f).reverb (0.6f, 0.5f, 0.4f).v });

        // 9 ---------------------------------------------------------------- bass pipe
        list.push_back ({ "Bass Pipe", "Bass", Build()
            .p (resCoarse, -12.0f)
            .p (excNoise, 0.5f).p (excNoiseColor, 0.7f).p (excPressure, 0.6f).p (excReed, 0.5f)
            .p (excLowpass, 2500.0f).p (excHighpass, 25.0f).p (excAttackClick, 0.3f)
            .env (20.0f, 300.0f, 0.9f, 200.0f).p (artFlowPitch, 0.2f)
            .tube (0.96f, 0.5f, 0.4f, ResMode::ClosedPipe, 0.3f, 0.4f).p (resShape, 0.45f)
            .body (300.0f, 0.5f, 0.4f)
            .p (glideTime, 60.0f).p (outHighpass, 20.0f)
            .space (0.0f, 0.0f, 0.1f).reverb (0.4f, 0.35f, 0.6f).v });

        // 10 --------------------------------------------------------------- evolving drone
        list.push_back ({ "Evolving Drone", "Drones", Build()
            .p (excNoise, 0.6f).p (excNoiseColor, 0.5f).p (excPressure, 0.5f).p (excReed, 0.35f).p (excTurbulence, 0.5f)
            .env (3000.0f, 2000.0f, 1.0f, 5000.0f).p (artInstability, 0.3f).p (artCoupling, 0.5f).p (artVariation, 0.5f)
            .tube (0.99f, 0.35f, 0.5f, ResMode::OpenPipe, 0.4f, 0.5f).p (resDispersion, 0.15f)
            .body (700.0f, 0.5f, 0.4f)
            .unison (3, 9.0f, 0.8f)
            .lfo (1, LfoShape::SmoothRandom, 0.15f).mod (1, ModSource::LFO1, ModDest::Damping, 0.3f)
            .lfo (2, LfoShape::Sine, 0.07f).mod (2, ModSource::LFO2, ModDest::Shape, 0.4f)
            .lfo (3, LfoShape::Triangle, 0.11f).mod (3, ModSource::LFO3, ModDest::BodyFreq, 0.5f)
            .mod (4, ModSource::ModEnv, ModDest::Brightness, 0.3f).p (menvAttack, 4000.0f).p (menvDecay, 3000.0f)
            .space (0.3f, 0.25f, 0.5f).delay (d1_2D, 0.5f, 3000.0f).reverb (1.0f, 0.85f, 0.5f, 30.0f).v });

        // 11 --------------------------------------------------------------- dark cinematic pad
        list.push_back ({ "Dark Cinematic Pad", "Pads", Build()
            .p (excNoise, 0.5f).p (excNoiseColor, 0.8f).p (excPressure, 0.4f).p (excReed, 0.2f).p (excLowpass, 1800.0f)
            .env (1500.0f, 1000.0f, 0.9f, 4000.0f)
            .tube (0.97f, 0.65f, 0.3f, ResMode::OpenPipe, 0.6f, 0.3f)
            .body (400.0f, 0.4f, 0.5f)
            .unison (2, 10.0f, 0.9f)
            .lfo (1, LfoShape::Sine, 0.09f).mod (1, ModSource::LFO1, ModDest::ExciterLP, 0.3f)
            .lfo (2, LfoShape::Sine, 0.13f).mod (2, ModSource::LFO2, ModDest::Pan, 0.4f)
            .space (0.4f, 0.2f, 0.55f).p (chorusRate, 0.25f).delay (d1_4D, 0.45f, 2000.0f).reverb (0.9f, 0.8f, 0.6f, 40.0f).v });

        // 12 --------------------------------------------------------------- metallic ambience
        list.push_back ({ "Metallic Ambience", "Metallic", Build()
            .p (excPluck, 0.5f).p (excNoise, 0.25f).p (excNoiseColor, 0.2f).p (excPressure, 0.3f).p (excAttackClick, 0.3f)
            .env (5.0f, 800.0f, 0.4f, 3500.0f).p (artCoupling, 0.3f).p (artVariation, 0.6f)
            .tube (0.995f, 0.1f, 0.85f, ResMode::String, 0.05f, 0.15f).p (resShape, 0.15f).p (resDispersion, 0.8f)
            .body (4500.0f, 0.7f, 0.35f)
            .lfo (3, LfoShape::SmoothRandom, 0.2f).mod (1, ModSource::LFO3, ModDest::Dispersion, 0.15f)
            .space (0.2f, 0.35f, 0.45f).delay (d1_8T, 0.55f).reverb (0.8f, 0.8f, 0.3f).v });

        // 13 --------------------------------------------------------------- unstable feedback texture
        list.push_back ({ "Unstable Feedback", "Experimental", Build()
            .p (excNoise, 0.4f).p (excPressure, 0.85f).p (excReed, 0.6f).p (excTurbulence, 0.7f)
            .env (200.0f, 500.0f, 0.8f, 1500.0f).p (artInstability, 0.7f).p (artFlowPitch, 0.6f).p (artCoupling, 0.8f)
            .tube (1.0f, 0.2f, 0.7f, ResMode::ClosedPipe, 0.2f, 0.85f).p (resShape, 0.6f).p (resDispersion, 0.3f)
            .body (2200.0f, 0.8f, 0.5f)
            .lfo (1, LfoShape::SmoothRandom, 0.8f).mod (1, ModSource::LFO1, ModDest::Feedback, -0.2f)
            .lfo (2, LfoShape::SampleHold, 6.0f).mod (2, ModSource::LFO2, ModDest::BodyFreq, 0.5f)
            .mod (3, ModSource::ModWheel, ModDest::Pressure, 0.5f)
            .mod (4, ModSource::Aftertouch, ModDest::Feedback, 0.1f)
            .space (0.0f, 0.3f, 0.3f).delay (d1_4, 0.6f).reverb (0.6f, 0.5f, 0.4f).v });

        // 14 --------------------------------------------------------------- soft breath pad
        list.push_back ({ "Soft Breath Pad", "Pads", Build()
            .p (excNoise, 0.8f).p (excNoiseColor, 0.55f).p (excPressure, 0.25f).p (excReed, 0.0f)
            .p (excLowpass, 4000.0f).p (excHighpass, 150.0f).p (excTurbulence, 0.4f).p (excBreathRandom, 0.4f)
            .env (900.0f, 500.0f, 0.9f, 2500.0f)
            .tube (0.9f, 0.5f, 0.4f, ResMode::OpenPipe, 0.7f, 0.1f)
            .body (1000.0f, 0.2f, 0.2f)
            .unison (2, 7.0f, 0.7f)
            .lfo (1, LfoShape::Sine, 0.2f).mod (1, ModSource::LFO1, ModDest::Noise, 0.2f)
            .space (0.35f, 0.15f, 0.5f).delay (d1_4D, 0.35f).reverb (0.8f, 0.7f, 0.5f).v });

        // 15 --------------------------------------------------------------- percussive click / pluck
        list.push_back ({ "Percussive Click", "Percussion", Build()
            .p (excPluck, 1.0f).p (excPluckLength, 0.8f).p (excAttackClick, 1.0f).p (excNoise, 0.05f).p (excPressure, 0.1f)
            .p (excVelocity, 1.0f).p (excReleaseNoise, 0.0f)
            .env (0.5f, 60.0f, 0.0f, 120.0f).p (envVelToPressure, 1.0f).p (artFlowPitch, 0.4f)
            .tube (0.9f, 0.6f, 0.7f, ResMode::String, 0.3f, 0.3f).p (resShape, 0.1f).p (resDispersion, 0.2f)
            .body (2500.0f, 0.6f, 0.5f)
            .space (0.0f, 0.2f, 0.15f).delay (d1_16, 0.4f).reverb (0.3f, 0.3f, 0.5f).v });

        // 16 --------------------------------------------------------------- experimental noise instrument
        list.push_back ({ "Noise Machine", "Experimental", Build()
            .p (excNoise, 1.0f).p (excNoiseColor, 0.0f).p (excPressure, 0.9f).p (excReed, 0.4f)
            .p (excLowpass, 20000.0f).p (excHighpass, 300.0f).p (excTurbulence, 1.0f)
            .env (50.0f, 300.0f, 0.7f, 600.0f)
            .tube (0.7f, 0.0f, 1.0f, ResMode::String, 0.0f, 1.0f).p (resShape, 0.9f).p (resDispersion, 1.0f).p (resKeyTrack, 0.3f)
            .body (6000.0f, 0.9f, 0.6f)
            .lfo (1, LfoShape::SampleHold, 12.0f).mod (1, ModSource::LFO1, ModDest::ExciterLP, 0.6f)
            .lfo (2, LfoShape::Square, 2.0f, LfoMode::Free, 0.0f, true, d1_8).mod (2, ModSource::LFO2, ModDest::Pitch, 0.25f)
            .lfo (3, LfoShape::SawDown, 2.0f).mod (3, ModSource::LFO3, ModDest::BodyFreq, 0.8f)
            .mod (4, ModSource::ModWheel, ModDest::Dispersion, -0.5f)
            .p (outGain, -3.0f)
            .space (0.0f, 0.4f, 0.25f).delay (d1_16, 0.7f, 3000.0f).reverb (0.5f, 0.4f, 0.3f).v });

        // 17 --------------------------------------------------------------- whistle lead (mono / legato)
        list.push_back ({ "Whistle Lead", "Winds", Build()
            .p (resCoarse, 12.0f).p (voiceMode, (float) VoiceMode::Legato).p (glideTime, 80.0f)
            .p (excNoise, 0.5f).p (excPressure, 0.55f).p (excReed, 0.5f).p (excLowpass, 9000.0f).p (excHighpass, 200.0f)
            .env (25.0f, 200.0f, 0.9f, 180.0f).p (artFlowPitch, 0.3f)
            .tube (0.985f, 0.2f, 0.7f, ResMode::OpenPipe, 0.6f, 0.2f)
            .body (2400.0f, 0.4f, 0.3f)
            .vibrato (5.8f, 300.0f, 0.006f)
            .mod (2, ModSource::ModWheel, ModDest::Turbulence, 0.4f)
            .space (0.0f, 0.2f, 0.3f).delay (d1_8D, 0.3f).reverb (0.6f, 0.5f, 0.4f).v });

        // 18 --------------------------------------------------------------- sub drone engine
        list.push_back ({ "Sub Drone Engine", "Drones", Build()
            .p (resCoarse, -24.0f)
            .p (excReed, 0.7f).p (excPressure, 0.8f).p (excNoise, 0.3f).p (excNoiseColor, 0.9f)
            .env (2000.0f, 1000.0f, 1.0f, 3000.0f)
            .tube (0.98f, 0.7f, 0.3f, ResMode::ClosedPipe, 0.3f, 0.7f)
            .body (120.0f, 0.6f, 0.5f)
            .unison (2, 4.0f, 0.6f)
            .lfo (1, LfoShape::Sine, 0.05f).mod (1, ModSource::LFO1, ModDest::Pressure, 0.15f)
            .lfo (2, LfoShape::Triangle, 0.03f).mod (2, ModSource::LFO2, ModDest::Damping, 0.2f)
            .p (outHighpass, 15.0f)
            .space (0.0f, 0.0f, 0.3f).reverb (1.0f, 0.9f, 0.7f).v });

        // 19 --------------------------------------------------------------- ceramic bells
        list.push_back ({ "Ceramic Bells", "Mallets", Build()
            .p (resCoarse, 12.0f)
            .p (excPluck, 0.8f).p (excPluckLength, 1.2f).p (excAttackClick, 0.15f).p (excNoise, 0.02f).p (excKeyTrack, 0.8f).p (excReleaseNoise, 0.0f)
            .env (0.5f, 1200.0f, 0.2f, 3500.0f).p (artVariation, 0.5f)
            .tube (0.997f, 0.1f, 0.9f, ResMode::String, 0.0f, 0.02f).p (resShape, 0.12f).p (resDispersion, 0.7f)
            .body (5000.0f, 0.8f, 0.4f)
            .space (0.1f, 0.2f, 0.4f).delay (d1_4T, 0.35f).reverb (0.75f, 0.8f, 0.35f).v });

        // 20 --------------------------------------------------------------- steam vent
        list.push_back ({ "Steam Vent", "Experimental", Build()
            .p (excNoise, 0.9f).p (excNoiseColor, 0.15f).p (excPressure, 0.6f).p (excReed, 0.25f).p (excTurbulence, 0.9f)
            .env (300.0f, 800.0f, 0.6f, 2500.0f)
            .tube (0.85f, 0.15f, 0.8f, ResMode::OpenPipe, 0.9f, 0.6f).p (resShape, 0.7f).p (resDispersion, 0.4f).p (resKeyTrack, 0.25f)
            .body (3000.0f, 0.5f, 0.5f)
            .lfo (1, LfoShape::SmoothRandom, 0.4f).mod (1, ModSource::LFO1, ModDest::ExciterLP, 0.5f)
            .lfo (2, LfoShape::SmoothRandom, 0.25f).mod (2, ModSource::LFO2, ModDest::Shape, 0.4f)
            .lfo (3, LfoShape::SmoothRandom, 0.15f).mod (3, ModSource::LFO3, ModDest::Feedback, 0.1f)
            .space (0.0f, 0.35f, 0.5f).delay (d1_8, 0.5f).reverb (0.9f, 0.75f, 0.4f).v });

        return list;
    }();
    return presets;
}
} // namespace aeriform
