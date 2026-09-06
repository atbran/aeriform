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


        // =================================================================== v2.1 experimental bank
        // 21 ------------------------------------------------------------- dual-exciter cross-modulation
        list.push_back ({ "Orbit Crossmod", "Experimental", Build().p (outGain, -7.0f)
            .p (exaModel, (float) ExciterModel::Wave).p (exaWaveShape, 0.62f).p (exaLevel, 0.8f)
            .p (exbModel, (float) ExciterModel::Complex).p (exbCoarse, 7.0f).p (exbCxComplexity, 0.55f).p (exbCxFeedback, 0.35f).p (exbCxRatio, 1.5f)
            .p (mixMode, (float) InteractionMode::FM).p (mixInteraction, 0.6f).p (mixB2A, 0.7f).p (mixDepth, 1.0f).p (mixNormalize, 0.6f)
            .p (preEnv, 0.4f).p (excLowpass, 9000.0f)
            .env (15.0f, 400.0f, 0.7f, 600.0f)
            .tube (0.92f, 0.4f, 0.55f, ResMode::OpenPipe, 0.4f, 0.3f)
            .mod (1, ModSource::ModWheel, ModDest::Interaction, 0.4f)
            .mod (2, ModSource::Aftertouch, ModDest::ExBPitch, 0.1f)
            .space (0.2f, 0.2f, 0.3f).delay (d1_8D, 0.35f).reverb (0.6f, 0.5f, 0.4f).v });

        // 22 ------------------------------------------------------------- chaotic generation
        list.push_back ({ "Strange Attractor", "Experimental", Build().p (outGain, -6.0f)
            .p (exaModel, (float) ExciterModel::Complex).p (exaCxComplexity, 0.8f).p (exaCxChaos, 0.9f).p (exaCxInstab, 0.4f)
            .p (exaCxBend, 0.5f).p (exaCxWarp, 0.3f).p (exaCxFeedback, 0.5f).p (exaCxSpread, 0.3f)
            .p (exbModel, (float) ExciterModel::Off).p (preEnv, 0.3f).p (dynAmount, 0.5f)
            .env (200.0f, 1000.0f, 0.8f, 1500.0f)
            .tube (0.9f, 0.45f, 0.5f, ResMode::DispersiveTube, 0.5f, 0.4f).p (resDispersion, 0.35f)
            .mod (1, ModSource::ChaosX, ModDest::ExATone, 0.4f)
            .mod (2, ModSource::ChaosY, ModDest::Damping, 0.3f)
            .mod (3, ModSource::ModWheel, ModDest::ExAChaos, -0.6f)
            .space (0.15f, 0.3f, 0.45f).delay (d1_4T, 0.45f).reverb (0.8f, 0.7f, 0.5f).v });

        // 23 ------------------------------------------------------------- wavefolded noise
        list.push_back ({ "Folded Static", "Noise", Build().p (outGain, -1.5f)
            .p (exaModel, (float) ExciterModel::NoiseBand).p (exaNzCenter, 800.0f).p (exaNzBandwidth, 0.2f).p (exaLevel, 0.9f)
            .p (wfOn, 1.0f).p (wfMode, (float) FoldMode::Triangle).p (wfFold, 0.75f).p (wfDrive, 0.5f).p (wfStages, 2.0f).p (wfSymmetry, 0.3f)
            .p (preEnv, 1.0f).p (excLowpass, 14000.0f)
            .env (30.0f, 300.0f, 0.8f, 400.0f)
            .tube (0.94f, 0.3f, 0.6f, ResMode::Comb, 0.2f, 0.35f)
            .lfo (1, LfoShape::Triangle, 0.3f).mod (1, ModSource::LFO1, ModDest::Fold, 0.35f)
            .mod (2, ModSource::ModWheel, ModDest::FoldBias, 0.6f)
            .space (0.0f, 0.25f, 0.35f).delay (d1_8, 0.5f).reverb (0.7f, 0.6f, 0.4f).v });

        // 24 ------------------------------------------------------------- bowed metal
        list.push_back ({ "Bowed Metal", "Physical", Build().p (outGain, -6.0f)
            .p (exaModel, (float) ExciterModel::Bow).p (exaPhStiffness, 0.7f).p (exaPhSpeed, 0.7f).p (exaPhPosition, 0.2f).p (exaPhTurb, 0.25f)
            .p (exbModel, (float) ExciterModel::Off).p (excLowpass, 12000.0f)
            .env (120.0f, 400.0f, 0.9f, 900.0f).p (envVelToPressure, 0.8f)
            .tube (0.99f, 0.15f, 0.75f, ResMode::MetallicBar, 0.1f, 0.2f).p (resInharm, 0.3f).p (resSize, 0.6f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::String).p (rbRatio, 1.5f).p (rbFeedback, 0.985f).p (rbDispersion, 0.3f)
            .p (netMode, (float) NetMode::Parallel).p (netWidth, 0.7f)
            .mod (1, ModSource::Aftertouch, ModDest::ExAShape, 0.3f)
            .mod (2, ModSource::ModWheel, ModDest::ExAChaos, 0.5f)
            .space (0.2f, 0.0f, 0.45f).reverb (0.8f, 0.75f, 0.35f).v });

        // 25 ------------------------------------------------------------- reed into a resonator chain
        list.push_back ({ "Reed Chain", "Physical", Build().p (outGain, -7.0f)
            .p (exaModel, (float) ExciterModel::Reed).p (exaPhStiffness, 0.45f).p (exaPhOpening, 0.6f).p (exaPhSpeed, 0.8f).p (exaPhTurb, 0.15f)
            .p (excLowpass, 8000.0f)
            .env (40.0f, 200.0f, 0.9f, 250.0f).p (envVelToPressure, 0.9f).p (artFlowPitch, 0.15f)
            .tube (0.95f, 0.35f, 0.6f, ResMode::ClosedPipe, 0.25f, 0.3f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::OpenPipe).p (rbCoarse, 12.0f).p (rbFeedback, 0.93f).p (rbDamping, 0.5f)
            .p (rcOn, 1.0f).p (rcType, (float) ResMode::FormantBody).p (rcSize, 0.55f).p (rcFeedback, 0.6f)
            .p (netMode, (float) NetMode::Serial).p (netSendAB, 0.8f).p (netSendBC, 0.9f).p (netInjectB, 0.2f).p (netTap, (float) OutputTap::Mix)
            .p (rbOutput, 0.5f).p (rcOutput, 0.8f)
            .vibrato (5.4f, 400.0f, 0.006f)
            .mod (2, ModSource::ModWheel, ModDest::ExAShape, 0.3f)
            .space (0.0f, 0.1f, 0.3f).delay (d1_8D, 0.25f).reverb (0.6f, 0.5f, 0.4f).v });

        // 26 ------------------------------------------------------------- mallet into parallel resonators
        list.push_back ({ "Mallet Triad", "Mallets", Build().p (outGain, -7.0f)
            .p (exaModel, (float) ExciterModel::Mallet).p (exaPhHardness, 0.6f).p (exaPhPosition, 0.35f).p (exaPhSpeed, 0.0f)
            .p (excLowpass, 16000.0f).p (preEnv, 0.0f)
            .env (0.5f, 300.0f, 0.5f, 2500.0f)
            .tube (0.99f, 0.25f, 0.7f, ResMode::ModalBank, 0.2f, 0.1f).p (resInharm, 0.1f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::MetallicBar).p (rbRatio, 1.5f).p (rbFeedback, 0.97f).p (rbDamping, 0.3f)
            .p (rcOn, 1.0f).p (rcType, (float) ResMode::Membrane).p (rcRatio, 0.5f).p (rcFeedback, 0.9f).p (rcDamping, 0.5f).p (rcSize, 0.7f)
            .p (netMode, (float) NetMode::Parallel).p (netWidth, 0.9f).p (rbOutput, 0.6f).p (rcOutput, 0.7f)
            .space (0.1f, 0.2f, 0.4f).delay (d1_4T, 0.3f).reverb (0.75f, 0.7f, 0.4f).v });

        // 27 ------------------------------------------------------------- cross-feedback bells
        list.push_back ({ "Crossfed Bells", "Metallic", Build().p (outGain, 6.0f)
            .p (exaModel, (float) ExciterModel::Impact).p (exaPhHardness, 0.8f).p (exaPhSpeed, 0.0f).p (preEnv, 0.0f).p (excLowpass, 18000.0f)
            .env (0.5f, 500.0f, 0.3f, 4000.0f)
            .tube (0.995f, 0.15f, 0.8f, ResMode::MetallicBar, 0.05f, 0.1f).p (resInharm, 0.2f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::ModalBank).p (rbRatio, 2.0f).p (rbFeedback, 0.99f).p (rbInharm, 0.35f).p (rbDamping, 0.2f)
            .p (rcOn, 1.0f).p (rcType, (float) ResMode::DispersiveTube).p (rcRatio, 0.75f).p (rcFeedback, 0.98f).p (rcDispersion, 0.6f)
            .p (netMode, (float) NetMode::Parallel).p (netAB, 0.5f).p (netBC, 0.45f).p (netCA, 0.4f).p (netFeedback, 0.7f)
            .p (netFbDelay, 12.0f).p (netFbFilter, 5000.0f).p (netFbDrive, 0.4f).p (netDamping, 0.25f).p (netWidth, 0.8f)
            .space (0.1f, 0.25f, 0.5f).delay (d1_8T, 0.4f).reverb (0.85f, 0.8f, 0.3f).v });

        // 28 ------------------------------------------------------------- repipe macro
        list.push_back ({ "Repipe Morph", "Network", Build().p (outGain, 4.0f)
            .p (exaModel, (float) ExciterModel::Breath).p (excNoise, 0.6f).p (excPressure, 0.6f).p (excReed, 0.4f)
            .env (60.0f, 300.0f, 0.85f, 500.0f)
            .tube (0.95f, 0.4f, 0.5f, ResMode::OpenPipe, 0.5f, 0.3f)
            .p (rbType, (float) ResMode::ClosedPipe).p (rbRatio, 1.5f).p (rbFeedback, 0.94f)
            .p (rcType, (float) ResMode::MetallicBar).p (rcRatio, 2.0f).p (rcFeedback, 0.96f).p (rcDamping, 0.4f)
            .p (netRepipe, 0.0f).p (netFbFilter, 4000.0f).p (netDamping, 0.3f)
            .mod (1, ModSource::ModWheel, ModDest::Repipe, 1.0f)
            .mod (2, ModSource::ModEnv, ModDest::Repipe, 0.5f).p (menvAttack, 3000.0f).p (menvDecay, 2000.0f).p (menvSustain, 0.6f)
            .space (0.15f, 0.2f, 0.4f).delay (d1_4D, 0.35f).reverb (0.7f, 0.6f, 0.4f).v });

        // 29 ------------------------------------------------------------- energy-loop drone
        list.push_back ({ "Energy Loop Drone", "Drones", Build()
            .p (exaModel, (float) ExciterModel::NoiseSteam).p (exaNzTurb, 0.5f).p (exaLevel, 0.5f)
            .p (wfOn, 1.0f).p (wfMode, (float) FoldMode::Smooth).p (wfFold, 0.5f).p (wfDrive, 0.3f)
            .env (2500.0f, 1500.0f, 1.0f, 4000.0f).p (preEnv, 0.6f)
            .tube (0.985f, 0.4f, 0.5f, ResMode::OpenPipe, 0.4f, 0.5f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::DispersiveTube).p (rbRatio, 0.5f).p (rbFeedback, 0.98f).p (rbDispersion, 0.4f)
            .p (netMode, (float) NetMode::Serial).p (netSendAB, 0.7f).p (netBA, 0.3f).p (netFeedback, 0.6f).p (netDamping, 0.35f)
            .p (loopOn, 1.0f).p (loopAmount, 0.55f).p (loopSource, (float) LoopSource::B).p (loopDest, (float) LoopDest::FolderIn)
            .p (loopFilter, 2500.0f).p (loopDelay, 18.0f).p (loopSat, 0.7f)
            .lfo (1, LfoShape::SmoothRandom, 0.12f).mod (1, ModSource::LFO1, ModDest::LoopAmount, 0.25f)
            .lfo (2, LfoShape::Sine, 0.05f).mod (2, ModSource::LFO2, ModDest::Fold, 0.3f)
            .mod (3, ModSource::NetEnergy, ModDest::Damping, 0.3f)
            .unison (2, 5.0f, 0.8f)
            .space (0.3f, 0.3f, 0.55f).delay (d1_2D, 0.5f, 2500.0f).reverb (1.0f, 0.85f, 0.5f, 30.0f).v });

        // 30 ------------------------------------------------------------- metallic steam
        list.push_back ({ "Metallic Steam", "Metallic", Build().p (outGain, -5.0f)
            .p (exaModel, (float) ExciterModel::NoiseSteam).p (exaNzTurb, 0.7f).p (exaNzBandwidth, 0.4f)
            .p (exbModel, (float) ExciterModel::NoiseMetallic).p (exbNzCenter, 2400.0f).p (exbNzBandwidth, 0.3f).p (exbNzSeed, 42.0f).p (exbLevel, 0.6f)
            .p (mixMode, (float) InteractionMode::Add).p (mixBalance, 0.2f).p (mixDrive, 0.3f)
            .env (150.0f, 500.0f, 0.8f, 1200.0f)
            .tube (0.97f, 0.2f, 0.7f, ResMode::DispersiveTube, 0.3f, 0.4f).p (resDispersion, 0.5f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::MetallicBar).p (rbRatio, 1.25f).p (rbFeedback, 0.97f).p (rbInharm, 0.4f)
            .p (netMode, (float) NetMode::Hybrid).p (netSendAB, 0.6f).p (netWidth, 0.7f)
            .lfo (3, LfoShape::SmoothRandom, 0.3f).mod (1, ModSource::LFO3, ModDest::ExAShape, 0.4f)
            .space (0.0f, 0.3f, 0.5f).delay (d1_8, 0.45f).reverb (0.85f, 0.7f, 0.3f).v });

        // 31 ------------------------------------------------------------- subharmonic pipe
        list.push_back ({ "Subharmonic Pipe", "Bass", Build().p (outGain, -7.0f)
            .p (exaModel, (float) ExciterModel::Wave).p (exaWaveShape, 0.9f).p (exaWaveSub, 0.8f).p (exaWavePw, 0.3f).p (exaLevel, 0.7f)
            .p (exbModel, (float) ExciterModel::Breath).p (exbLevel, 0.5f).p (excNoise, 0.4f).p (excPressure, 0.7f).p (excReed, 0.5f)
            .p (mixMode, (float) InteractionMode::Crossfade).p (mixInteraction, 0.4f)
            .p (excLowpass, 2500.0f).p (excHighpass, 20.0f)
            .env (25.0f, 300.0f, 0.9f, 250.0f)
            .tube (0.96f, 0.5f, 0.4f, ResMode::ClosedPipe, 0.3f, 0.5f).p (resCoarse, -12.0f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::OpenPipe).p (rbRatio, 0.5f).p (rbFeedback, 0.95f).p (rbDamping, 0.6f)
            .p (netMode, (float) NetMode::Parallel).p (rbOutput, 0.7f).p (netWidth, 0.2f)
            .p (outHighpass, 15.0f)
            .space (0.0f, 0.0f, 0.15f).reverb (0.4f, 0.35f, 0.6f).v });

        // 32 ------------------------------------------------------------- broken transmission
        list.push_back ({ "Broken Transmission", "Experimental", Build().p (outGain, -1.5f)
            .p (exaModel, (float) ExciterModel::Wave).p (exaWaveShape, 1.0f).p (exaWavePw, 0.15f)
            .p (exbModel, (float) ExciterModel::NoiseCrackle).p (exbNzDensity, 0.35f).p (exbNzBurst, 25.0f).p (exbLevel, 0.9f)
            .p (mixMode, (float) InteractionMode::Xor).p (mixInteraction, 0.8f).p (mixDepth, 0.9f)
            .p (wfOn, 1.0f).p (wfMode, (float) FoldMode::Hard).p (wfFold, 0.6f).p (wfShape, 0.8f)
            .p (preSlew, 0.4f).p (preTransient, 0.3f).p (preEnv, 0.5f)
            .env (5.0f, 200.0f, 0.7f, 300.0f)
            .tube (0.9f, 0.5f, 0.6f, ResMode::Comb, 0.6f, 0.5f)
            .lfo (1, LfoShape::SampleHold, 6.0f).mod (1, ModSource::LFO1, ModDest::Interaction, 0.5f)
            .lfo (2, LfoShape::Square, 1.0f, LfoMode::Free, 0.0f, true, d1_8).mod (2, ModSource::LFO2, ModDest::ExBLevel, -0.6f)
            .mod (3, ModSource::SampleHold, ModDest::ExAPitch, 0.2f)
            .space (0.0f, 0.4f, 0.2f).delay (d1_16, 0.6f, 2500.0f).reverb (0.4f, 0.3f, 0.5f).v });

        // 33 ------------------------------------------------------------- industrial horn
        list.push_back ({ "Industrial Horn", "Brass", Build().p (outGain, -3.0f)
            .p (exaModel, (float) ExciterModel::Lip).p (exaPhStiffness, 0.4f).p (exaPhOpening, 0.6f).p (exaPhSpeed, 0.9f).p (exaPhTurb, 0.2f)
            .p (wfOn, 1.0f).p (wfMode, (float) FoldMode::Diode).p (wfFold, 0.4f).p (wfDrive, 0.6f).p (wfSymmetry, 0.5f)
            .p (excLowpass, 10000.0f)
            .env (60.0f, 300.0f, 0.9f, 200.0f).p (envVelToPressure, 0.9f).p (artFlowPitch, 0.4f)
            .tube (0.96f, 0.3f, 0.7f, ResMode::OpenPipe, 0.8f, 0.6f).p (resCoarse, -12.0f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::FormantBody).p (rbSize, 0.3f).p (rbFeedback, 0.7f).p (rbOutput, 0.6f)
            .p (netMode, (float) NetMode::Serial).p (netSendAB, 0.8f).p (netTap, (float) OutputTap::Mix)
            .mod (1, ModSource::ModWheel, ModDest::ExAShape, 0.4f)
            .mod (2, ModSource::Aftertouch, ModDest::Fold, 0.3f)
            .space (0.0f, 0.15f, 0.3f).delay (d1_4, 0.3f).reverb (0.7f, 0.5f, 0.4f).v });

        // 34 ------------------------------------------------------------- granular wind
        list.push_back ({ "Granular Wind", "Noise", Build().p (outGain, -4.0f)
            .p (exaModel, (float) ExciterModel::NoiseWind).p (exaNzDensity, 0.7f).p (exaNzGust, 0.3f).p (exaNzTurb, 0.5f).p (exaLevel, 0.8f)
            .p (exbModel, (float) ExciterModel::NoiseAerosol).p (exbNzDensity, 0.5f).p (exbNzGrain, 35.0f).p (exbLevel, 0.6f).p (exbNzWidth, 1.0f)
            .p (mixMode, (float) InteractionMode::AudioXfade).p (mixInteraction, 0.5f).p (mixDepth, 0.7f)
            .env (1200.0f, 800.0f, 0.9f, 3000.0f)
            .tube (0.93f, 0.5f, 0.4f, ResMode::OpenPipe, 0.8f, 0.2f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::OpenPipe).p (rbRatio, 1.5f).p (rbFeedback, 0.92f)
            .p (rcOn, 1.0f).p (rcType, (float) ResMode::OpenPipe).p (rcRatio, 2.0f).p (rcFeedback, 0.9f)
            .p (netMode, (float) NetMode::Parallel).p (netWidth, 1.0f)
            .lfo (1, LfoShape::SmoothRandom, 0.2f).mod (1, ModSource::LFO1, ModDest::ExciterLP, 0.4f)
            .lfo (2, LfoShape::Sine, 0.07f).mod (2, ModSource::LFO2, ModDest::NetWidth, 0.4f)
            .unison (2, 8.0f, 0.9f)
            .space (0.4f, 0.2f, 0.6f).delay (d1_2D, 0.4f, 2000.0f).reverb (0.95f, 0.8f, 0.5f, 40.0f).v });

        // 35 ------------------------------------------------------------- feedback organism
        list.push_back ({ "Feedback Organism", "Experimental", Build().p (outGain, -3.0f)
            .p (exaModel, (float) ExciterModel::Jet).p (exaPhSpeed, 0.6f).p (exaPhTurb, 0.4f)
            .p (exbModel, (float) ExciterModel::Complex).p (exbCxChaos, 0.6f).p (exbCxComplexity, 0.4f).p (exbLevel, 0.4f)
            .p (mixMode, (float) InteractionMode::Ring).p (mixInteraction, 0.5f).p (mixDepth, 0.6f)
            .env (400.0f, 800.0f, 0.9f, 2500.0f)
            .tube (0.98f, 0.3f, 0.6f, ResMode::OpenPipe, 0.4f, 0.6f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::String).p (rbRatio, 1.5f).p (rbFeedback, 0.985f).p (rbDispersion, 0.2f)
            .p (rcOn, 1.0f).p (rcType, (float) ResMode::Membrane).p (rcRatio, 0.75f).p (rcFeedback, 0.95f)
            .p (netMode, (float) NetMode::Serial).p (netAB, 0.3f).p (netBA, 0.5f).p (netCB, 0.4f).p (netCA, 0.35f).p (netFeedback, 0.8f)
            .p (netFbDelay, 25.0f).p (netFbDrive, 0.6f).p (netDamping, 0.15f).p (netPolarity, (float) Polarity::Negative)
            .p (loopOn, 1.0f).p (loopAmount, 0.4f).p (loopDest, (float) LoopDest::ShaperIn).p (loopFilter, 1800.0f).p (loopDelay, 40.0f)
            .mod (1, ModSource::ResBEnergy, ModDest::ExAShape, -0.4f)
            .mod (2, ModSource::NetEnergy, ModDest::NetFeedback, -0.3f)
            .mod (3, ModSource::ModWheel, ModDest::LoopAmount, 0.5f)
            .space (0.0f, 0.3f, 0.4f).delay (d1_4D, 0.45f).reverb (0.8f, 0.7f, 0.4f).v });

        // 36 ------------------------------------------------------------- unstable membrane
        list.push_back ({ "Unstable Membrane", "Percussion", Build()
            .p (exaModel, (float) ExciterModel::Impact).p (exaPhHardness, 0.4f).p (exaPhSpeed, 0.3f).p (exaPhStiffness, 0.6f).p (preEnv, 0.0f)
            .env (0.5f, 300.0f, 0.4f, 1800.0f)
            .tube (0.96f, 0.4f, 0.5f, ResMode::Membrane, 0.3f, 0.5f).p (resSize, 0.65f).p (resInharm, 0.5f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::Membrane).p (rbRatio, 1.1f).p (rbFeedback, 0.95f).p (rbSize, 0.5f)
            .p (netMode, (float) NetMode::Parallel).p (netAB, 0.6f).p (netBA, 0.6f).p (netFeedback, 0.9f).p (netFbDrive, 0.8f).p (netDamping, 0.1f)
            .lfo (1, LfoShape::SmoothRandom, 1.5f).mod (1, ModSource::LFO1, ModDest::ResBPitch, 0.1f)
            .mod (2, ModSource::Random, ModDest::Pitch, 0.05f)
            .space (0.0f, 0.2f, 0.35f).delay (d1_16, 0.35f).reverb (0.5f, 0.4f, 0.5f).v });

        // 37 ------------------------------------------------------------- serial body transformations
        list.push_back ({ "Serial Bodies", "Network", Build()
            .p (exaModel, (float) ExciterModel::Pluck).p (exaPhHardness, 0.7f).p (exaPhPosition, 0.25f).p (preEnv, 0.0f)
            .env (0.5f, 400.0f, 0.5f, 3000.0f)
            .tube (0.99f, 0.3f, 0.6f, ResMode::String, 0.2f, 0.2f).p (resDispersion, 0.1f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::FormantBody).p (rbSize, 0.5f).p (rbFeedback, 0.75f).p (rbKeytrack, 0.0f)
            .p (rcOn, 1.0f).p (rcType, (float) ResMode::ModalBank).p (rcRatio, 1.0f).p (rcFeedback, 0.9f).p (rcInharm, 0.15f).p (rcDamping, 0.6f)
            .p (netMode, (float) NetMode::Serial).p (netSendAB, 0.9f).p (netSendBC, 0.7f).p (netInjectC, 0.2f).p (netTap, (float) OutputTap::Mix)
            .p (rbOutput, 0.5f).p (rcOutput, 0.7f).p (netMix, 0.9f)
            .lfo (1, LfoShape::Sine, 0.08f).mod (1, ModSource::LFO1, ModDest::ResBPitch, 0.15f)
            .mod (2, ModSource::NoteAge, ModDest::ResCBrightness, 0.4f)
            .space (0.15f, 0.2f, 0.4f).delay (d1_8D, 0.3f).reverb (0.7f, 0.6f, 0.4f).v });

        // 38 ------------------------------------------------------------- stereo resonator cloud
        list.push_back ({ "Resonator Cloud", "Pads", Build().p (outGain, -5.0f)
            .p (exaModel, (float) ExciterModel::NoiseVelvet).p (exaNzDensity, 0.3f).p (exaLevel, 0.6f)
            .p (exbModel, (float) ExciterModel::Wave).p (exbWaveShape, 0.1f).p (exbLevel, 0.3f).p (exbCoarse, 12.0f)
            .p (mixMode, (float) InteractionMode::AM).p (mixInteraction, 0.6f).p (mixDepth, 0.5f)
            .env (1800.0f, 1200.0f, 0.9f, 5000.0f)
            .tube (0.99f, 0.35f, 0.5f, ResMode::ModalBank, 0.3f, 0.1f).p (resWidth, 0.8f).p (resPickup, 0.3f).p (resPan, -0.4f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::ModalBank).p (rbRatio, 1.5f).p (rbFeedback, 0.985f).p (rbWidth, 0.8f).p (rbPan, 0.5f).p (rbInharm, 0.1f)
            .p (rcOn, 1.0f).p (rcType, (float) ResMode::DispersiveTube).p (rcRatio, 2.0f).p (rcFeedback, 0.97f).p (rcWidth, 0.6f).p (rcPan, 0.0f)
            .p (netMode, (float) NetMode::Parallel).p (netWidth, 1.0f).p (netAB, 0.2f).p (netBC, 0.2f).p (netFeedback, 0.5f)
            .lfo (1, LfoShape::Sine, 0.06f).mod (1, ModSource::LFO1, ModDest::ResAPan, 0.5f)
            .lfo (2, LfoShape::Triangle, 0.09f).mod (2, ModSource::LFO2, ModDest::ResBPitch, 0.03f)
            .unison (2, 6.0f, 1.0f)
            .space (0.5f, 0.25f, 0.6f).p (chorusRate, 0.2f).delay (d1_2D, 0.45f, 2500.0f).reverb (1.0f, 0.85f, 0.5f, 35.0f).v });

        // 39 ------------------------------------------------------------- sidechain through wavefolder
        list.push_back ({ "Sidechain Fold", "Sidechain", Build().p (outGain, -4.0f)
            .p (exaModel, (float) ExciterModel::Sidechain).p (exaScHp, 60.0f).p (exaScTransient, 0.3f).p (exaLevel, 1.0f)
            .p (exbModel, (float) ExciterModel::Off).p (preEnv, 0.3f)
            .p (wfOn, 1.0f).p (wfMode, (float) FoldMode::Sine).p (wfFold, 0.65f).p (wfDrive, 0.4f).p (wfMix, 0.8f)
            .p (excLowpass, 12000.0f).p (excHighpass, 40.0f)
            .env (10.0f, 200.0f, 1.0f, 300.0f)
            .tube (0.96f, 0.3f, 0.6f, ResMode::OpenPipe, 0.4f, 0.3f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::ModalBank).p (rbRatio, 2.0f).p (rbFeedback, 0.95f).p (rbOutput, 0.5f)
            .p (netMode, (float) NetMode::Parallel).p (netWidth, 0.6f).p (netMix, 0.85f)
            .mod (1, ModSource::SidechainEnv, ModDest::Fold, 0.4f)
            .mod (2, ModSource::ModWheel, ModDest::Fold, 0.4f)
            .space (0.0f, 0.2f, 0.3f).delay (d1_8, 0.3f).reverb (0.6f, 0.5f, 0.4f).v });

        // 40 ------------------------------------------------------------- musically playable experimental lead
        list.push_back ({ "Glass Reed Lead", "Leads", Build().p (outGain, -2.0f)
            .p (voiceMode, (float) VoiceMode::Legato).p (glideTime, 60.0f)
            .p (exaModel, (float) ExciterModel::Reed).p (exaPhStiffness, 0.55f).p (exaPhOpening, 0.55f).p (exaPhSpeed, 0.85f).p (exaPhBright, 0.7f)
            .p (exbModel, (float) ExciterModel::Wave).p (exbWaveShape, 0.33f).p (exbCoarse, 12.0f).p (exbLevel, 0.35f).p (exbRetrig, (float) RetrigMode::Free)
            .p (mixMode, (float) InteractionMode::Crossfade).p (mixInteraction, 0.3f)
            .p (wfOn, 1.0f).p (wfMode, (float) FoldMode::Smooth).p (wfFold, 0.3f).p (wfDrive, 0.2f).p (wfMix, 0.6f)
            .p (excLowpass, 9000.0f)
            .env (20.0f, 250.0f, 0.9f, 200.0f).p (envVelToPressure, 0.8f).p (artFlowPitch, 0.25f)
            .tube (0.97f, 0.3f, 0.65f, ResMode::ClosedPipe, 0.3f, 0.3f)
            .p (rbOn, 1.0f).p (rbType, (float) ResMode::ModalBank).p (rbRatio, 2.0f).p (rbFeedback, 0.93f).p (rbOutput, 0.35f).p (rbDamping, 0.5f)
            .p (netMode, (float) NetMode::Hybrid).p (netSendAB, 0.5f).p (netWidth, 0.5f)
            .vibrato (5.6f, 350.0f, 0.006f)
            .mod (2, ModSource::ModWheel, ModDest::ExAShape, 0.3f)
            .mod (3, ModSource::Aftertouch, ModDest::Fold, 0.4f)
            .mod (4, ModSource::Aftertouch, ModDest::Pressure, 0.3f)
            .space (0.15f, 0.25f, 0.35f).delay (d1_8D, 0.35f).reverb (0.6f, 0.5f, 0.4f).v });

        // Append only: original factory ordinals are persistent favourite IDs.
        list.push_back ({ "Reed in a Small Room", "Experimental", Build().p(outGain,-4)
            .p(exaModel,(float)ExciterModel::Reed).p(exaPhStiffness,.45f).p(exaPhSpeed,.7f)
            .env(15,250,.75f,800).tube(.94f,.35f,.6f,ResMode::OpenPipe,.3f,.2f)
            .p(roomOn,1).p(roomSize,.45f).p(roomShape,.65f).p(roomDiffusion,.85f)
            .p(roomSend,1).p(roomLevel,1.25f).p(roomFeedback,.8f).p(roomWallDamping,.25f)
            .p(roomNetworkReturn,.75f).p(roomReturnDelay,12).space(0,0,0).v });

        list.push_back ({ "Modal Echo Pluck", "Experimental", Build().p(outGain,-3)
            .p(exaModel,(float)ExciterModel::Pluck).p(exaPhHardness,.65f).p(exaPhBright,.7f)
            .env(1,180,.15f,1000).tube(.94f,.4f,.6f,ResMode::String,.3f,.15f)
            .p(rdOn,1).p(rdTime,280).p(rdFeedback,.7f).p(rdType,1).p(rdTuning,220)
            .p(rdTrack,1).p(rdAmount,.75f).p(rdSaturation,.1f).p(rdMix,.45f).p(rdOffset,12)
            .space(0,0,.12f).reverb(.5f,.4f,.45f).v });

        return list;
    }();
    return presets;
}
} // namespace aeriform
