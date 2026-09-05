#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ParamIDs.h"

namespace aeriform
{
// ---------------------------------------------------------------------------
// Choice enumerations shared between DSP, GUI and presets. The order of every
// enum is part of the saved-state format; only append, never reorder.
// ---------------------------------------------------------------------------
enum class LfoShape  { Sine, Triangle, SawUp, SawDown, Square, SampleHold, SmoothRandom, Count };
enum class LfoMode   { Free, Retrigger, Count };
enum class ResMode   { OpenPipe, ClosedPipe, String, Count };
enum class VoiceMode { Poly, Mono, Legato, Count };

enum class ModSource
{
    None, LFO1, LFO2, LFO3, ModEnv, AmpEnv, Velocity, ModWheel, Aftertouch,
    PitchBend, MpeSlide, KeyTrack, Random, BreathCC, ExpressionCC, Count
};

enum class ModDest
{
    None, Pressure, Noise, NoiseColor, ExciterLP, ExciterHP, Turbulence, Pitch,
    Feedback, Damping, Brightness, Dispersion, Shape, Reflection, BodyFreq, BodyMix,
    Pan, Amp, ChorusMix, DelayMix, ReverbMix, Lfo1Rate, Lfo2Rate, Lfo3Rate, Count
};

namespace choices
{
    const juce::StringArray& lfoShapes();
    const juce::StringArray& lfoModes();
    const juce::StringArray& syncDivisions();
    const juce::StringArray& resModes();
    const juce::StringArray& voiceModes();
    const juce::StringArray& modSources();
    const juce::StringArray& modDests();

    /** Length of a sync division expressed in quarter notes (beats). */
    double syncDivisionBeats (int index);
}

enum class ParamSection { Breath, Resonator, Motion, Space, Master };

/** Static description of one parameter, used by the GUI (tooltips, units),
    presets and tests. Built once when the layout is created. */
struct ParamInfo
{
    juce::String id;
    juce::String name;
    juce::String unit;
    juce::String tooltip;
    ParamSection section = ParamSection::Breath;
    float defaultValue = 0.0f;   // in DSP units (for choice/bool: index / 0-1)
    float minValue = 0.0f;
    float maxValue = 1.0f;
    bool  isChoice = false;
    bool  isBool   = false;
    bool  isInt    = false;
};

/** Creates the complete APVTS parameter layout. Every parameter here is
    connected to DSP, automatable and serialised. */
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

/** All parameters, in layout order. Valid after the first createParameterLayout() call. */
const std::vector<ParamInfo>& parameterInfos();

/** Look up a parameter description by ID (nullptr if unknown). */
const ParamInfo* findParamInfo (const juce::String& id);

/** Current state-format version written into saved state / preset files. */
inline constexpr int kStateVersion = 1;
} // namespace aeriform
