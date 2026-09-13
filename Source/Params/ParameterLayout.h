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
enum class VoiceMode { Poly, Mono, Legato, Count };

/** Resonator models. The first three are the v0.1 waveguide topologies. */
enum class ResMode   { OpenPipe, ClosedPipe, String, Comb, DispersiveTube, ModalBank, MetallicBar, Membrane, FormantBody, Count };

enum class ExciterModel
{
    Off, Breath, Wave, Complex,
    NoiseWhite, NoisePink, NoiseBrown, NoiseBlue, NoiseViolet, NoiseBand, NoiseVelvet, NoiseCrackle,
    NoiseSteam, NoiseWind, NoiseAerosol, NoiseMetallic,
    Reed, Lip, Bow, Jet, Mallet, Pluck, Scrape, Impact,
    Sidechain, Count
};
enum class RetrigMode      { Free, Retrigger, Random, Count };
enum class InteractionMode { Crossfade, Add, Subtract, Ring, AM, FM, PM, Sync, Xor, MinMax, RectDiff, SampleHold, AudioXfade, Count };
enum class PreFilterType   { LowHigh, BandPass, Count };
enum class ShaperOrder     { ShapeThenFold, FoldThenShape, Count };
enum class FoldMode        { Smooth, Triangle, Sine, Diode, Chebyshev, Hard, Hybrid, Count };
enum class Polarity        { Positive, Negative, Count };
enum class InjectPoint     { A, B, C, All, Count };
enum class OutputTap       { Mix, A, B, C, Last, Count };
enum class LoopSource      { Mix, A, B, C, Count };
enum class LoopDest        { ShaperIn, FolderIn, NetworkIn, Count };
enum class NetMode         { Single, Serial, Parallel, Hybrid, Count };
enum class QualityMode     { Eco, Normal, High, Count };
enum class ChorusType      { Ensemble, JunoI, JunoII, JunoDual, Dimension, Count };
enum class ReverbType      { Hall, Room, Plate, Count };

enum class ModSource
{
    None, LFO1, LFO2, LFO3, ModEnv, AmpEnv, Velocity, ModWheel, Aftertouch,
    PitchBend, MpeSlide, KeyTrack, Random, BreathCC, ExpressionCC,
    // v2.1
    ExAEnv, ExBEnv, SidechainEnv, ResAEnergy, ResBEnergy, ResCEnergy, NetEnergy,
    SampleHold, SmoothRandom, ChaosX, ChaosY, NoteAge, KeyPosition, VoiceNumber, AlternateNote,
    Macro1, Macro2, Macro3, Macro4,
    Count
};

inline constexpr bool isModSourceUnipolar (ModSource s) noexcept
{
    switch (s)
    {
        case ModSource::ModEnv:
        case ModSource::AmpEnv:
        case ModSource::Velocity:
        case ModSource::ModWheel:
        case ModSource::Aftertouch:
        case ModSource::MpeSlide:
        case ModSource::Random:
        case ModSource::BreathCC:
        case ModSource::ExpressionCC:
        case ModSource::ExAEnv:
        case ModSource::ExBEnv:
        case ModSource::SidechainEnv:
        case ModSource::ResAEnergy:
        case ModSource::ResBEnergy:
        case ModSource::ResCEnergy:
        case ModSource::NetEnergy:
        case ModSource::SampleHold:
        case ModSource::NoteAge:
        case ModSource::KeyPosition:
        case ModSource::VoiceNumber:
        case ModSource::Macro1:
        case ModSource::Macro2:
        case ModSource::Macro3:
        case ModSource::Macro4:
            return true;

        case ModSource::None:
        case ModSource::LFO1:
        case ModSource::LFO2:
        case ModSource::LFO3:
        case ModSource::PitchBend:
        case ModSource::KeyTrack:
        case ModSource::SmoothRandom:
        case ModSource::ChaosX:
        case ModSource::ChaosY:
        case ModSource::AlternateNote:
        case ModSource::Count:
        default:
            return false;
    }
}

enum class ModDest
{
    None, Pressure, Noise, NoiseColor, ExciterLP, ExciterHP, Turbulence, Pitch,
    Feedback, Damping, Brightness, Dispersion, Shape, Reflection, BodyFreq, BodyMix,
    Pan, Amp, ChorusMix, DelayMix, ReverbMix, Lfo1Rate, Lfo2Rate, Lfo3Rate,
    // v2.1
    ExALevel, ExAPitch, ExATone, ExAShape, ExAChaos, ExBLevel, ExBPitch, ExBTone, ExBShape, ExBChaos,
    Interaction, Balance, PreDrive, Fold, FoldDrive, FoldSymmetry, FoldBias,
    ResBPitch, ResBFeedback, ResBDamping, ResBBrightness, ResCPitch, ResCFeedback, ResCDamping, ResCBrightness,
    NetFeedback, NetWidth, Repipe, LoopAmount, ResAPan,
    Macro1, Macro2, Macro3, Macro4,
    // v3.2: continuous sound controls
    ResAWet, ResBWet, ResCWet,
    ResAWidth, ResBWidth, ResCWidth,
    ResAInharm, ResBInharm, ResCInharm,
    ResASize, ResBSize, ResCSize,
    ResASaturation, ResBSaturation, ResCSaturation,
    Pluck, PluckLength,
    FoldMix, FoldShape,
    ChorusRate, ChorusDepth,
    DelayTimeL, DelayTimeR, DelayFeedback, DelayFilter,
    ReverbDecay, ReverbSize, ReverbDamp, ReverbPredelay,
#include "AdvancedModEnums.inc"
    Count
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
    const juce::StringArray& exciterModels();
    const juce::StringArray& retrigModes();
    const juce::StringArray& interactionModes();
    const juce::StringArray& preFilterTypes();
    const juce::StringArray& shaperOrders();
    const juce::StringArray& foldModes();
    const juce::StringArray& polarities();
    const juce::StringArray& injectPoints();
    const juce::StringArray& outputTaps();
    const juce::StringArray& loopSources();
    const juce::StringArray& loopDests();
    const juce::StringArray& netModes();
    const juce::StringArray& qualityModes();
    const juce::StringArray& chorusTypes();
    const juce::StringArray& reverbTypes();

    /** Length of a sync division expressed in quarter notes (beats). */
    double syncDivisionBeats (int index);
}

enum class ParamSection { Breath, Exciters, Shaping, Resonator, Network, Motion, Space, Master };
enum class ParamKind { Float, Choice, Bool, Int };
enum class Fmt { Percent, BipolarPercent, Hz, LfoHz, Ms, Db, Semi, Cents, Ratio, Degrees, Plain };
enum class ChoiceList
{
    None, LfoShapes, LfoModes, SyncDivs, ResTypes, VoiceModes, ModSources, ModDests, ExciterModels, RetrigModes,
    InteractionModes, PreFilterTypes, ShaperOrders, FoldModes, Polarities, InjectPoints, OutputTaps, LoopSources,
    LoopDests, NetModes, QualityModes, MorphModes, FilterPositions, FilterModels, FilterSlopes, FilterVowels, ContactNodes, PhysicalStereoModes, SympatheticTunings, DelayResTypes, SaturationModels, RackTypes,
    ChorusTypes, ReverbTypes
};

/** One row of the generated parameter table. */
struct ParamDef
{
    P p;
    const char* id;
    const char* name;
    ParamSection section;
    ParamKind kind;
    float minValue, maxValue, defaultValue, centre, step;
    const char* unit;
    Fmt fmt;
    ChoiceList choices;
    const char* tooltip;
};

/** Static description of one parameter, used by the GUI (tooltips, units), presets and tests. */
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

/** All parameters, in layout order (index == (int) P). Valid after the first createParameterLayout() call. */
const std::vector<ParamInfo>& parameterInfos();

/** Look up a parameter description by ID (nullptr if unknown). */
const ParamInfo* findParamInfo (const juce::String& id);

/** The generated definition of a parameter. */
const ParamDef& paramDef (P p);

/** The string array behind a choice list. */
const juce::StringArray& choiceStrings (ChoiceList list);

/** Current state-format version written into saved state / preset files.
    1 = v0.1 (single exciter / single resonator), 2 = v2.1 (dual exciters, folder, network). */
inline constexpr int kStateVersion = 3;
} // namespace aeriform
