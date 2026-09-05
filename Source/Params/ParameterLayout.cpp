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
        static const juce::StringArray s { "Open Pipe", "Closed Pipe", "String", "Comb", "Dispersive Tube", "Modal Bank",
                                           "Metallic Bar", "Membrane", "Formant Body" };
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
                                           "Aftertouch", "Pitch Bend", "MPE Slide", "Key Track", "Random", "Breath CC2", "Expression CC11",
                                           "Exciter A Env", "Exciter B Env", "Sidechain Env", "Res A Energy", "Res B Energy", "Res C Energy",
                                           "Network Energy", "Sample & Hold", "Smooth Random", "Chaos X", "Chaos Y", "Note Age", "Key Position",
                                           "Voice Number", "Alternate Note" };
        return s;
    }
    const juce::StringArray& modDests()
    {
        static const juce::StringArray s { "None", "Pressure", "Noise", "Noise Color", "Exciter LP", "Exciter HP", "Turbulence",
                                           "Pitch", "Feedback", "Damping", "Brightness", "Dispersion", "Shape", "Reflection",
                                           "Body Freq", "Body Mix", "Pan", "Amp", "Chorus Mix", "Delay Mix", "Reverb Mix",
                                           "LFO 1 Rate", "LFO 2 Rate", "LFO 3 Rate",
                                           "Ex A Level", "Ex A Pitch", "Ex A Tone", "Ex A Shape", "Ex A Chaos",
                                           "Ex B Level", "Ex B Pitch", "Ex B Tone", "Ex B Shape", "Ex B Chaos",
                                           "Interaction", "A/B Balance", "Shaper Drive", "Fold", "Fold Drive", "Fold Symmetry", "Fold Bias",
                                           "Res B Pitch", "Res B Feedback", "Res B Damping", "Res B Brightness",
                                           "Res C Pitch", "Res C Feedback", "Res C Damping", "Res C Brightness",
                                           "Network Feedback", "Network Width", "Repipe", "Loop Return", "Res A Pan" };
        return s;
    }
    const juce::StringArray& exciterModels()
    {
        static const juce::StringArray s { "Off", "Breath", "Wave", "Complex",
                                           "Noise: White", "Noise: Pink", "Noise: Brown", "Noise: Blue", "Noise: Violet", "Noise: Band",
                                           "Noise: Velvet", "Noise: Crackle", "Noise: Steam", "Noise: Wind", "Noise: Aerosol", "Noise: Metallic",
                                           "Reed", "Lip", "Bow", "Jet", "Mallet", "Pluck", "Scrape", "Impact", "Sidechain" };
        return s;
    }
    const juce::StringArray& retrigModes()
    {
        static const juce::StringArray s { "Free", "Retrigger", "Random" };
        return s;
    }
    const juce::StringArray& interactionModes()
    {
        static const juce::StringArray s { "Crossfade", "Add", "Subtract", "Ring", "AM", "FM", "PM", "Sync", "XOR", "Min / Max",
                                           "Rectified Diff", "Sample & Hold", "Audio Crossfade" };
        return s;
    }
    const juce::StringArray& preFilterTypes()
    {
        static const juce::StringArray s { "Low + High", "Band-Pass" };
        return s;
    }
    const juce::StringArray& shaperOrders()
    {
        static const juce::StringArray s { "Shape > Fold", "Fold > Shape" };
        return s;
    }
    const juce::StringArray& foldModes()
    {
        static const juce::StringArray s { "Smooth", "Triangle", "Sine", "Diode", "Chebyshev", "Hard", "Hybrid" };
        return s;
    }
    const juce::StringArray& polarities()
    {
        static const juce::StringArray s { "Positive", "Negative" };
        return s;
    }
    const juce::StringArray& injectPoints()
    {
        static const juce::StringArray s { "Res A", "Res B", "Res C", "All" };
        return s;
    }
    const juce::StringArray& outputTaps()
    {
        static const juce::StringArray s { "Mix", "Res A", "Res B", "Res C", "Last" };
        return s;
    }
    const juce::StringArray& loopSources()
    {
        static const juce::StringArray s { "Mix", "Res A", "Res B", "Res C" };
        return s;
    }
    const juce::StringArray& loopDests()
    {
        static const juce::StringArray s { "Shaper In", "Folder In", "Network In" };
        return s;
    }
    const juce::StringArray& netModes()
    {
        static const juce::StringArray s { "Single", "Serial", "Parallel", "Hybrid" };
        return s;
    }
    const juce::StringArray& qualityModes()
    {
        static const juce::StringArray s { "Eco", "Normal", "High" };
        return s;
    }
} // namespace choices

const juce::StringArray& choiceStrings (ChoiceList list)
{
    switch (list)
    {
        case ChoiceList::LfoShapes:        return choices::lfoShapes();
        case ChoiceList::LfoModes:         return choices::lfoModes();
        case ChoiceList::SyncDivs:         return choices::syncDivisions();
        case ChoiceList::ResTypes:         return choices::resModes();
        case ChoiceList::VoiceModes:       return choices::voiceModes();
        case ChoiceList::ModSources:       return choices::modSources();
        case ChoiceList::ModDests:         return choices::modDests();
        case ChoiceList::ExciterModels:    return choices::exciterModels();
        case ChoiceList::RetrigModes:      return choices::retrigModes();
        case ChoiceList::InteractionModes: return choices::interactionModes();
        case ChoiceList::PreFilterTypes:   return choices::preFilterTypes();
        case ChoiceList::ShaperOrders:     return choices::shaperOrders();
        case ChoiceList::FoldModes:        return choices::foldModes();
        case ChoiceList::Polarities:       return choices::polarities();
        case ChoiceList::InjectPoints:     return choices::injectPoints();
        case ChoiceList::OutputTaps:       return choices::outputTaps();
        case ChoiceList::LoopSources:      return choices::loopSources();
        case ChoiceList::LoopDests:        return choices::loopDests();
        case ChoiceList::NetModes:         return choices::netModes();
        case ChoiceList::QualityModes:     return choices::qualityModes();
        case ChoiceList::None:
        default:
        {
            static const juce::StringArray empty;
            return empty;
        }
    }
}

#include "ParamTable.inc"

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
    juce::String fmtPlain (float v, int) { return juce::String (v, 2); }

    std::function<juce::String (float, int)> formatter (Fmt f)
    {
        switch (f)
        {
            case Fmt::Percent:        return fmtPercent;
            case Fmt::BipolarPercent: return fmtBipolarPercent;
            case Fmt::Hz:             return fmtHz;
            case Fmt::LfoHz:          return fmtLfoHz;
            case Fmt::Ms:             return fmtMs;
            case Fmt::Db:             return fmtDb;
            case Fmt::Semi:           return fmtSemi;
            case Fmt::Cents:          return fmtCents;
            case Fmt::Ratio:          return fmtRatio;
            case Fmt::Degrees:        return fmtDegrees;
            case Fmt::Plain:
            default:                  return fmtPlain;
        }
    }

    float parseNumber (const juce::String& text)
    {
        return text.retainCharacters ("0123456789.-+").getFloatValue();
    }

    juce::NormalisableRange<float> makeRange (const ParamDef& d)
    {
        juce::NormalisableRange<float> r (d.minValue, d.maxValue, d.step);
        if (d.centre > d.minValue && d.centre < d.maxValue)
            r.setSkewForCentre (d.centre);
        return r;
    }
} // namespace

const ParamDef& paramDef (P p)
{
    return kParamDefs[juce::jlimit (0, kNumParams - 1, (int) p)];
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    auto& infos = infoStore();
    infos.clear();
    infos.reserve ((size_t) kNumParams);
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < kNumParams; ++i)
    {
        const ParamDef& d = kParamDefs[i];
        jassert ((int) d.p == i);   // table order must match the enum
        const juce::ParameterID pid { d.id, 1 };

        ParamInfo info;
        info.id = d.id; info.name = d.name; info.unit = d.unit; info.tooltip = d.tooltip; info.section = d.section;
        info.defaultValue = d.defaultValue; info.minValue = d.minValue; info.maxValue = d.maxValue;

        switch (d.kind)
        {
            case ParamKind::Float:
            {
                std::function<float (const juce::String&)> fromText = [] (const juce::String& t) { return parseNumber (t); };
                auto attrs = juce::AudioParameterFloatAttributes().withLabel (d.unit)
                                .withStringFromValueFunction (formatter (d.fmt))
                                .withValueFromStringFunction (fromText);
                layout.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name, makeRange (d), d.defaultValue, attrs));
                break;
            }
            case ParamKind::Choice:
            {
                const auto& items = choiceStrings (d.choices);
                jassert (items.size() > 0);
                layout.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, items, juce::jlimit (0, items.size() - 1, (int) d.defaultValue)));
                info.isChoice = true; info.minValue = 0.0f; info.maxValue = (float) (items.size() - 1);
                break;
            }
            case ParamKind::Bool:
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.defaultValue > 0.5f));
                info.isBool = true; info.minValue = 0.0f; info.maxValue = 1.0f;
                break;
            case ParamKind::Int:
            {
                auto attrs = juce::AudioParameterIntAttributes().withLabel (d.unit);
                layout.add (std::make_unique<juce::AudioParameterInt> (pid, d.name, (int) d.minValue, (int) d.maxValue, (int) d.defaultValue, attrs));
                info.isInt = true;
                break;
            }
        }
        infos.push_back (info);
    }
    return layout;
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
