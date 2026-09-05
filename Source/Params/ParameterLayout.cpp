#include "ParameterLayout.h"

namespace aeriform
{
namespace choices
{
    const juce::StringArray& lfoShapes()
    {
        static const juce::StringArray s { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Sample & Hold", "Smooth Random" };
        return s;
    }
    const juce::StringArray& lfoModes()
    {
        static const juce::StringArray s { "Free", "Retrigger" };
        return s;
    }
    const juce::StringArray& syncDivisions()
    {
        static const juce::StringArray s { "8/1", "4/1", "2/1", "1/1", "1/2", "1/2 D", "1/2 T", "1/4", "1/4 D", "1/4 T",
                                           "1/8", "1/8 D", "1/8 T", "1/16", "1/16 D", "1/16 T", "1/32" };
        return s;
    }
    double syncDivisionBeats (int index)
    {
        static constexpr double beats[] = { 32.0, 16.0, 8.0, 4.0, 2.0, 3.0, 4.0 / 3.0, 1.0, 1.5, 2.0 / 3.0,
                                            0.5, 0.75, 1.0 / 3.0, 0.25, 0.375, 1.0 / 6.0, 0.125 };
        constexpr int n = (int) (sizeof (beats) / sizeof (beats[0]));
        return beats[juce::jlimit (0, n - 1, index)];
    }
    const juce::StringArray& resModes()
    {
        static const juce::StringArray s { "Open Pipe", "Closed Pipe", "String" };
        return s;
    }
    const juce::StringArray& voiceModes()
    {
        static const juce::StringArray s { "Poly", "Mono", "Legato" };
        return s;
    }
    const juce::StringArray& modSources()
    {
        static const juce::StringArray s { "None", "LFO 1", "LFO 2", "LFO 3", "Mod Env", "Amp Env", "Velocity", "Mod Wheel",
                                           "Aftertouch", "Pitch Bend", "MPE Slide", "Key Track", "Random", "Breath CC2", "Expression CC11" };
        return s;
    }
    const juce::StringArray& modDests()
    {
        static const juce::StringArray s { "None", "Pressure", "Noise", "Noise Color", "Exciter LP", "Exciter HP", "Turbulence",
                                           "Pitch", "Feedback", "Damping", "Brightness", "Dispersion", "Shape", "Reflection",
                                           "Body Freq", "Body Mix", "Pan", "Amp", "Chorus Mix", "Delay Mix", "Reverb Mix",
                                           "LFO 1 Rate", "LFO 2 Rate", "LFO 3 Rate" };
        return s;
    }
} // namespace choices

namespace
{
    std::vector<ParamInfo>& infoStore()
    {
        static std::vector<ParamInfo> infos;
        return infos;
    }

    // ---- value <-> text formatting ---------------------------------------
    juce::String fmtHz (float v, int)
    {
        if (v >= 10000.0f) return juce::String (v / 1000.0f, 1) + " kHz";
        if (v >= 1000.0f)  return juce::String (v / 1000.0f, 2) + " kHz";
        if (v >= 100.0f)   return juce::String (juce::roundToInt (v)) + " Hz";
        return juce::String (v, 1) + " Hz";
    }
    juce::String fmtMs (float v, int)
    {
        if (v >= 1000.0f) return juce::String (v / 1000.0f, 2) + " s";
        if (v >= 100.0f)  return juce::String (juce::roundToInt (v)) + " ms";
        return juce::String (v, 1) + " ms";
    }
    juce::String fmtPercent (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
    juce::String fmtBipolarPercent (float v, int)
    {
        const int p = juce::roundToInt (v * 100.0f);
        return (p > 0 ? "+" : "") + juce::String (p) + " %";
    }
    juce::String fmtDb (float v, int) { return juce::String (v, 1) + " dB"; }
    juce::String fmtSemi (float v, int)
    {
        const int s = juce::roundToInt (v);
        return (s > 0 ? "+" : "") + juce::String (s) + " st";
    }
    juce::String fmtCents (float v, int)
    {
        const int c = juce::roundToInt (v);
        return (c > 0 ? "+" : "") + juce::String (c) + " ct";
    }
    juce::String fmtRatio (float v, int) { return juce::String (v, 3) + " x"; }
    juce::String fmtDegrees (float v, int) { return juce::String (juce::roundToInt (v)) + " deg"; }
    juce::String fmtLfoHz (float v, int) { return v < 1.0f ? juce::String (v, 3) + " Hz" : juce::String (v, 2) + " Hz"; }

    float parseNumber (const juce::String& text)
    {
        return text.retainCharacters ("0123456789.-+").getFloatValue();
    }

    juce::NormalisableRange<float> logRange (float lo, float hi, float centre)
    {
        juce::NormalisableRange<float> r (lo, hi, 0.0f);
        r.setSkewForCentre (centre);
        return r;
    }

    struct Builder
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        ParamSection section = ParamSection::Breath;

        void addFloat (const juce::String& id, const juce::String& name, juce::NormalisableRange<float> range,
                       float def, const juce::String& unit, std::function<juce::String (float, int)> toText,
                       const juce::String& tooltip)
        {
            std::function<float (const juce::String&)> fromText = [] (const juce::String& t) { return parseNumber (t); };
            auto attrs = juce::AudioParameterFloatAttributes().withLabel (unit)
                            .withStringFromValueFunction (toText)
                            .withValueFromStringFunction (fromText);
            layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, range, def, attrs));
            ParamInfo info;
            info.id = id; info.name = name; info.unit = unit; info.tooltip = tooltip; info.section = section;
            info.defaultValue = def; info.minValue = range.start; info.maxValue = range.end;
            infoStore().push_back (info);
        }

        void addChoice (const juce::String& id, const juce::String& name, const juce::StringArray& items, int def,
                        const juce::String& tooltip)
        {
            layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def));
            ParamInfo info;
            info.id = id; info.name = name; info.tooltip = tooltip; info.section = section;
            info.defaultValue = (float) def; info.minValue = 0.0f; info.maxValue = (float) (items.size() - 1);
            info.isChoice = true;
            infoStore().push_back (info);
        }

        void addBool (const juce::String& id, const juce::String& name, bool def, const juce::String& tooltip)
        {
            layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
            ParamInfo info;
            info.id = id; info.name = name; info.tooltip = tooltip; info.section = section;
            info.defaultValue = def ? 1.0f : 0.0f; info.isBool = true;
            infoStore().push_back (info);
        }

        void addInt (const juce::String& id, const juce::String& name, int lo, int hi, int def, const juce::String& unit,
                     const juce::String& tooltip)
        {
            auto attrs = juce::AudioParameterIntAttributes().withLabel (unit);
            layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { id, 1 }, name, lo, hi, def, attrs));
            ParamInfo info;
            info.id = id; info.name = name; info.unit = unit; info.tooltip = tooltip; info.section = section;
            info.defaultValue = (float) def; info.minValue = (float) lo; info.maxValue = (float) hi; info.isInt = true;
            infoStore().push_back (info);
        }
    };
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    infoStore().clear();
    Builder b;
    using namespace ids;
    const juce::NormalisableRange<float> unit01 (0.0f, 1.0f, 0.0f);
    const juce::NormalisableRange<float> bipolar (-1.0f, 1.0f, 0.0f);

    // =====================================================================
    // BREATH
    // =====================================================================
    b.section = ParamSection::Breath;
    b.addFloat (excNoise, "Noise", unit01, 0.6f, "%", fmtPercent,
                "Level of the continuous noise (breath) excitation feeding the resonator.");
    b.addFloat (excNoiseColor, "Noise Color", unit01, 0.35f, "%", fmtPercent,
                "Spectral tilt of the breath noise: white (0 %) to pink (100 %).");
    b.addFloat (excPressure, "Pressure", unit01, 0.5f, "%", fmtPercent,
                "Steady air pressure (DC flow) pushed into the resonator. Drives sustained tones and flow-to-pitch effects.");
    b.addFloat (excPluck, "Pluck", unit01, 0.0f, "%", fmtPercent,
                "Level of the short impulse burst at note-on. Turns the exciter into a pluck or strike.");
    b.addFloat (excPluckLength, "Pluck Length", logRange (0.3f, 80.0f, 6.0f), 5.0f, "ms", fmtMs,
                "Duration of the pluck impulse burst.");
    b.addFloat (excLowpass, "Exciter LP", logRange (200.0f, 20000.0f, 3000.0f), 7000.0f, "Hz", fmtHz,
                "Low-pass filter applied to the excitation before it enters the tube.");
    b.addFloat (excHighpass, "Exciter HP", logRange (10.0f, 5000.0f, 250.0f), 40.0f, "Hz", fmtHz,
                "High-pass filter applied to the excitation. Removes rumble and shapes the breath character.");
    b.addFloat (excTurbulence, "Turbulence", unit01, 0.25f, "%", fmtPercent,
                "Slow, chaotic fluctuation of the air stream. Adds breathy instability and life.");
    b.addFloat (excVelocity, "Velocity", unit01, 0.5f, "%", fmtPercent,
                "How strongly key velocity scales the excitation level and pluck strength.");
    b.addFloat (excExternalIn, "External In", unit01, 0.0f, "%", fmtPercent,
                "Amount of the plug-in audio input (sidechain / standalone input) injected as excitation.");
    b.addFloat (excKeyTrack, "Exciter Key Track", unit01, 0.5f, "%", fmtPercent,
                "How far the exciter filters follow the played pitch.");
    b.addFloat (excAttackClick, "Attack Transient", unit01, 0.15f, "%", fmtPercent,
                "Extra tongue / chiff transient added at note-on.");
    b.addFloat (excReleaseNoise, "Release Noise", unit01, 0.1f, "%", fmtPercent,
                "Short breath puff emitted when a note is released.");
    b.addFloat (excBreathRandom, "Breath Random", unit01, 0.15f, "%", fmtPercent,
                "Random per-note and slow drift of the breath pressure, like a human player.");
    b.addFloat (excReed, "Reed", unit01, 0.0f, "%", fmtPercent,
                "Reed / jet non-linearity at the mouth of the tube. With Reed up, Pressure makes the pipe speak by itself (clarinet, sax, brass); at 0 the tube is driven linearly by noise and plucks.");

    b.addFloat (envAttack, "Attack", logRange (0.5f, 8000.0f, 200.0f), 25.0f, "ms", fmtMs,
                "Time for the breath pressure to reach full level.");
    b.addFloat (envDecay, "Decay", logRange (1.0f, 8000.0f, 300.0f), 300.0f, "ms", fmtMs,
                "Time for the pressure to fall from peak to the sustain level.");
    b.addFloat (envSustain, "Sustain", unit01, 0.8f, "%", fmtPercent,
                "Pressure level held while the key is down.");
    b.addFloat (envRelease, "Release", logRange (2.0f, 12000.0f, 400.0f), 250.0f, "ms", fmtMs,
                "Time for the pressure to fade after the key is released.");
    b.addFloat (envVelToPressure, "Vel > Pressure", unit01, 0.6f, "%", fmtPercent,
                "How much velocity scales the envelope peak (breath pressure).");
    b.addFloat (artPressBright, "Pressure > Bright", unit01, 0.4f, "%", fmtPercent,
                "Pressure-dependent brightness: blowing harder opens the exciter filter.");
    b.addFloat (artFlowPitch, "Flow > Pitch", bipolar, 0.15f, "%", fmtBipolarPercent,
                "Flow-to-pitch interaction: air pressure bends the pitch slightly (+/- 50 cents at full).");
    b.addFloat (artInstability, "Instability", unit01, 0.1f, "%", fmtPercent,
                "Slow random pitch wander of the resonator, like an unstable air column.");
    b.addFloat (artVariation, "Variation", unit01, 0.2f, "%", fmtPercent,
                "Per-voice component variation: each voice gets slightly different tuning, damping and brightness.");
    b.addFloat (artCoupling, "Coupling", unit01, 0.0f, "%", fmtPercent,
                "Sympathetic coupling: a little of every other voice leaks into each tube.");

    // =====================================================================
    // RESONATOR
    // =====================================================================
    b.section = ParamSection::Resonator;
    b.addFloat (resCoarse, "Coarse", juce::NormalisableRange<float> (-24.0f, 24.0f, 1.0f), 0.0f, "st", fmtSemi,
                "Coarse tuning of the resonator in semitones.");
    b.addFloat (resFine, "Fine", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.0f), 0.0f, "ct", fmtCents,
                "Fine tuning in cents.");
    b.addFloat (resLength, "Length", logRange (0.5f, 2.0f, 1.0f), 1.0f, "x", fmtRatio,
                "Physical length multiplier of the tube. 1.0 x is in tune with the played note.");
    b.addFloat (resKeyTrack, "Key Track", juce::NormalisableRange<float> (0.0f, 2.0f, 0.0f), 1.0f, "%", fmtPercent,
                "How much the tube length follows the keyboard. 100 % = equal temperament, 0 % = fixed drone.");
    b.addFloat (resFeedback, "Feedback", unit01, 0.9f, "%", fmtPercent,
                "Loop gain of the waveguide. Above ~95 % the tube approaches self-oscillation (always bounded).");
    b.addFloat (resDamping, "Damping", unit01, 0.35f, "%", fmtPercent,
                "Frequency-dependent loss: how quickly high harmonics die inside the tube.");
    b.addFloat (resBrightness, "Brightness", unit01, 0.5f, "%", fmtPercent,
                "Spectral tilt of the energy injected into the tube.");
    b.addFloat (resDispersion, "Dispersion", unit01, 0.0f, "%", fmtPercent,
                "Inharmonicity / stiffness: spreads the partials like a metal bar or stiff string.");
    b.addFloat (resShape, "Shape", unit01, 0.5f, "%", fmtPercent,
                "Bore shape / excitation position along the tube. Creates comb-like formant colouring.");
    b.addFloat (resReflection, "Reflection", unit01, 0.3f, "%", fmtPercent,
                "End reflection character: hard closed end (0 %) to open, flared bell (100 %).");
    b.addFloat (resSaturation, "Saturation", unit01, 0.25f, "%", fmtPercent,
                "Non-linear feedback saturation. Bounds the loop and adds warmth or growl at high feedback.");
    b.addChoice (resMode, "Mode", choices::resModes(), 0,
                 "Resonator topology: open pipe (all harmonics), closed pipe (odd harmonics, reed-like) or string.");
    b.addFloat (resBodyFreq, "Body Freq", logRange (80.0f, 8000.0f, 800.0f), 900.0f, "Hz", fmtHz,
                "Centre frequency of the body / formant filter after the tube.");
    b.addFloat (resBodyRes, "Body Res", unit01, 0.4f, "%", fmtPercent,
                "Resonance of the body filter.");
    b.addFloat (resBodyMix, "Body Mix", unit01, 0.3f, "%", fmtPercent,
                "Amount of the body filter mixed into the voice output.");
    b.addFloat (resBodyTrack, "Body Track", unit01, 0.0f, "%", fmtPercent,
                "How much the body filter frequency follows the played note.");

    // =====================================================================
    // MOTION
    // =====================================================================
    b.section = ParamSection::Motion;
    for (int i = 1; i <= numLFOs; ++i)
    {
        const juce::String n = "LFO " + juce::String (i) + " ";
        b.addChoice (lfoParam (i, lfoShapeSuffix), n + "Shape", choices::lfoShapes(), 0, "Waveform of the LFO.");
        b.addFloat  (lfoParam (i, lfoRateSuffix), n + "Rate", logRange (0.02f, 40.0f, 2.0f), i == 1 ? 0.5f : (i == 2 ? 3.0f : 0.1f),
                     "Hz", fmtLfoHz, "LFO speed in Hz (when not tempo-synced).");
        b.addBool   (lfoParam (i, lfoSyncSuffix), n + "Sync", false, "Synchronise the LFO rate to host tempo.");
        b.addChoice (lfoParam (i, lfoDivSuffix), n + "Division", choices::syncDivisions(), 7, "Tempo-synced LFO period.");
        b.addChoice (lfoParam (i, lfoModeSuffix), n + "Mode", choices::lfoModes(), 0,
                     "Free: continuous phase shared by all voices. Retrigger: restarts at every note-on.");
        b.addFloat  (lfoParam (i, lfoFadeSuffix), n + "Fade In", logRange (0.0f, 5000.0f, 500.0f), 0.0f, "ms", fmtMs,
                     "Time for the LFO depth to fade in after note-on.");
        b.addFloat  (lfoParam (i, lfoPhaseSuffix), n + "Phase", juce::NormalisableRange<float> (0.0f, 360.0f, 0.0f), 0.0f, "deg", fmtDegrees,
                     "Start phase of the LFO when retriggered.");
    }

    b.addFloat (menvAttack, "Mod Attack", logRange (0.5f, 8000.0f, 200.0f), 100.0f, "ms", fmtMs, "Modulation envelope attack time.");
    b.addFloat (menvDecay, "Mod Decay", logRange (1.0f, 8000.0f, 300.0f), 600.0f, "ms", fmtMs, "Modulation envelope decay time.");
    b.addFloat (menvSustain, "Mod Sustain", unit01, 0.2f, "%", fmtPercent, "Modulation envelope sustain level.");
    b.addFloat (menvRelease, "Mod Release", logRange (2.0f, 12000.0f, 400.0f), 400.0f, "ms", fmtMs, "Modulation envelope release time.");

    for (int i = 1; i <= numModSlots; ++i)
    {
        const juce::String n = "Mod " + juce::String (i) + " ";
        b.addChoice (modParam (i, modSrcSuffix), n + "Source", choices::modSources(), 0, "Modulation source for this slot.");
        b.addChoice (modParam (i, modDstSuffix), n + "Destination", choices::modDests(), 0, "Parameter modulated by this slot.");
        b.addFloat  (modParam (i, modDepthSuffix), n + "Depth", bipolar, 0.0f, "%", fmtBipolarPercent,
                     "Bipolar modulation depth. Positive raises the destination, negative lowers it.");
    }

    // =====================================================================
    // SPACE
    // =====================================================================
    b.section = ParamSection::Space;
    b.addFloat (chorusMix, "Chorus Mix", unit01, 0.0f, "%", fmtPercent, "Wet amount of the stereo ensemble chorus.");
    b.addFloat (chorusRate, "Chorus Rate", logRange (0.05f, 5.0f, 0.5f), 0.4f, "Hz", fmtLfoHz, "Speed of the chorus modulation.");
    b.addFloat (chorusDepth, "Chorus Depth", unit01, 0.4f, "%", fmtPercent, "Depth of the chorus pitch modulation.");
    b.addFloat (chorusWidth, "Chorus Width", unit01, 0.8f, "%", fmtPercent, "Stereo spread of the chorus voices.");

    b.addFloat (delayMix, "Delay Mix", unit01, 0.0f, "%", fmtPercent, "Wet amount of the delay.");
    b.addFloat (delayTime, "Delay Time", logRange (10.0f, 2000.0f, 300.0f), 375.0f, "ms", fmtMs, "Delay time when not tempo-synced.");
    b.addBool  (delaySync, "Delay Sync", true, "Synchronise the delay time to host tempo.");
    b.addChoice (delayDiv, "Delay Division", choices::syncDivisions(), 11, "Tempo-synced delay time.");
    b.addFloat (delayFeedback, "Delay Feedback", juce::NormalisableRange<float> (0.0f, 0.95f, 0.0f), 0.35f, "%", fmtPercent, "Amount of delayed signal fed back.");
    b.addFloat (delayTone, "Delay Tone", logRange (400.0f, 20000.0f, 3000.0f), 4500.0f, "Hz", fmtHz, "Low-pass filter in the delay feedback path.");
    b.addBool  (delayPingPong, "Ping Pong", true, "Alternate the repeats between left and right.");

    b.addFloat (reverbMix, "Reverb Mix", unit01, 0.18f, "%", fmtPercent, "Wet amount of the algorithmic reverb.");
    b.addFloat (reverbSize, "Reverb Size", unit01, 0.6f, "%", fmtPercent, "Size of the virtual space.");
    b.addFloat (reverbDecay, "Reverb Decay", unit01, 0.5f, "%", fmtPercent, "Decay time of the reverb tail.");
    b.addFloat (reverbDamping, "Reverb Damping", unit01, 0.4f, "%", fmtPercent, "High-frequency absorption of the space.");
    b.addFloat (reverbPreDelay, "Pre-Delay", juce::NormalisableRange<float> (0.0f, 200.0f, 0.0f), 12.0f, "ms", fmtMs, "Delay before the reverb starts.");
    b.addFloat (reverbWidth, "Reverb Width", unit01, 1.0f, "%", fmtPercent, "Stereo width of the reverb.");
    b.addFloat (reverbModulation, "Reverb Motion", unit01, 0.3f, "%", fmtPercent, "Slow modulation inside the reverb, smoothing metallic resonances.");

    // =====================================================================
    // MASTER
    // =====================================================================
    b.section = ParamSection::Master;
    b.addChoice (voiceMode, "Voice Mode", choices::voiceModes(), 0, "Polyphonic, monophonic (retrigger) or legato (no retrigger while held).");
    b.addInt   (voiceCount, "Voices", 1, 16, 8, "", "Maximum number of simultaneous voices.");
    b.addFloat (glideTime, "Glide", logRange (0.0f, 2000.0f, 200.0f), 0.0f, "ms", fmtMs, "Portamento time between notes.");
    b.addBool  (glideLegatoOnly, "Glide Legato", true, "Only glide when notes overlap.");
    b.addInt   (unisonVoices, "Unison", 1, 4, 1, "", "Number of stacked, detuned tubes per note.");
    b.addFloat (unisonDetune, "Unison Detune", juce::NormalisableRange<float> (0.0f, 100.0f, 0.0f), 12.0f, "ct", fmtCents, "Detune spread between unison tubes.");
    b.addFloat (unisonSpread, "Unison Spread", unit01, 0.6f, "%", fmtPercent, "Stereo spread of unison tubes.");
    b.addInt   (bendRange, "Bend Range", 1, 24, 2, "st", "Pitch-bend range in semitones.");
    b.addBool  (mpeEnabled, "MPE", false, "Enable MIDI Polyphonic Expression (per-note pitch, pressure and slide).");
    b.addFloat (outGain, "Output", juce::NormalisableRange<float> (-60.0f, 12.0f, 0.0f), 0.0f, "dB", fmtDb, "Master output level.");
    b.addFloat (outHighpass, "Output HP", logRange (10.0f, 400.0f, 60.0f), 24.0f, "Hz", fmtHz, "Final high-pass / DC blocker.");
    b.addBool  (limiterOn, "Limiter", true, "Soft output limiter protecting against runaway levels.");

    return std::move (b.layout);
}

const std::vector<ParamInfo>& parameterInfos()
{
    return infoStore();
}

const ParamInfo* findParamInfo (const juce::String& id)
{
    for (const auto& info : infoStore())
        if (info.id == id)
            return &info;
    return nullptr;
}
} // namespace aeriform
