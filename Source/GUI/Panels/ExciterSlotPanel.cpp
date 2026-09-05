#include "ExciterSlotPanel.h"

namespace aeriform
{
namespace
{
    bool isNoise (ExciterModel m) { return m >= ExciterModel::NoiseWhite && m <= ExciterModel::NoiseMetallic; }
    bool isBlown (ExciterModel m) { return m == ExciterModel::Reed || m == ExciterModel::Lip || m == ExciterModel::Bow || m == ExciterModel::Jet; }
    bool isStruck (ExciterModel m) { return m == ExciterModel::Mallet || m == ExciterModel::Pluck || m == ExciterModel::Scrape || m == ExciterModel::Impact; }
}

ExciterSlotPanel::ExciterSlotPanel (AeriformProcessor& p, int s, bool c)
    : ParamPanel (p, s == 0 ? "EXCITER A" : "EXCITER B", s == 0 ? theme::exciterA : theme::exciterB, ! c),
      slot (s), compact (c), px (s == 0 ? "exa" : "exb")
{
    const auto accent = slot == 0 ? theme::exciterA : theme::exciterB;
    const ModDest dLevel = slot == 0 ? ModDest::ExALevel : ModDest::ExBLevel;
    const ModDest dPitch = slot == 0 ? ModDest::ExAPitch : ModDest::ExBPitch;
    const ModDest dTone  = slot == 0 ? ModDest::ExATone : ModDest::ExBTone;
    const ModDest dShape = slot == 0 ? ModDest::ExAShape : ModDest::ExBShape;
    const ModDest dChaos = slot == 0 ? ModDest::ExAChaos : ModDest::ExBChaos;

    modelId = id ("_model");
    model = control<ChoiceBox> (processor, modelId, compact ? juce::String() : juce::String ("Model"));
    if (compact)
    {
        model->setCaptionVisible (false);
        nameLabel = caption (slot == 0 ? "EXCITER A" : "EXCITER B");
        nameLabel->setColour (juce::Label::textColourId, accent);
    }
    else
    {
        retrig = control<ChoiceBox> (processor, id ("_retrig"), "Phase");
        if (slot == 1) sync = control<Toggle> (processor, ids::exbSync, "Sync to A");
        freeze = control<Toggle> (processor, id ("_sc_freeze"), "Freeze");
        commonCaption = caption ("COMMON");
        modelCaption = caption ("MODEL");
    }
    auto& vis = processor.getVisualizerModel();
    scope = std::make_unique<ScopeDisplay> (slot == 0 ? vis.exciterAScope : vis.exciterBScope,
                                            slot == 0 ? &vis.exciterAEnv : &vis.exciterBEnv, accent, slot == 0 ? "A" : "B");
    addAndMakeVisible (*scope);

    // ---- common ----------------------------------------------------------------
    const int common = compact ? theme::knobSize : theme::knobSizeSmall;
    make (id ("_level"), "Level", additive (dLevel), common);
    make (id ("_tone"), "Tone", additive (dTone), common);
    if (! compact)
    {
        make (id ("_coarse"), "Coarse", semitones (dPitch, 24.0f));
        make (id ("_fine"), "Fine", semitones (dPitch, 2400.0f));
        make (id ("_keytrack"), "Key Track");
        make (id ("_variation"), "Variation");
        make (id ("_vel"), "Velocity");
        make (id ("_press"), "Pressure");
        make (id ("_drift"), "Drift");
        make (id ("_phase"), "Phase");
    }
    for (auto& k : knobs) commonKnobs.push_back (k.get());

    // ---- model specific (all created, shown by model) ----------------------------------
    const int md = compact ? theme::knobSize : 52;
    // breath (the v0.1 exciter: global ids)
    make (ids::excNoise, "Noise", additive (ModDest::Noise), md);
    make (ids::excNoiseColor, "Color", additive (ModDest::NoiseColor), md);
    make (ids::excPressure, "Pressure", additive (ModDest::Pressure), md);
    make (ids::excReed, "Reed", {}, md);
    make (ids::excPluck, "Pluck", {}, md);
    make (ids::excExternalIn, "Sidechain", {}, md);
    make (ids::excPluckLength, "Pluck Len", {}, md);
    make (ids::excTurbulence, "Turbulence", additive (ModDest::Turbulence), md);
    make (ids::excVelocity, "Vel > Lvl", {}, md);
    make (ids::excAttackClick, "Transient", {}, md);
    make (ids::excReleaseNoise, "Rel. Noise", {}, md);
    make (ids::excBreathRandom, "Breath Rnd", {}, md);
    // wave
    make (id ("_wave_shape"), "Shape", additive (dShape), md);
    make (id ("_wave_pw"), "Pulse W", {}, md);
    make (id ("_wave_sub"), "Sub", {}, md);
    make (id ("_wave_pd"), "Ph. Dist", additive (dChaos), md);
    // complex
    make (id ("_cx_complexity"), "Complexity", additive (dShape), md);
    make (id ("_cx_symmetry"), "Symmetry", {}, md);
    make (id ("_cx_bend"), "Bend", {}, md);
    make (id ("_cx_instab"), "Instability", {}, md);
    make (id ("_cx_spread"), "Spread", {}, md);
    make (id ("_cx_warp"), "Warp", {}, md);
    make (id ("_cx_feedback"), "Feedback", {}, md);
    make (id ("_cx_chaos"), "Chaos", additive (dChaos), md);
    make (id ("_cx_ratio"), "Ratio", {}, md);
    // noise
    make (id ("_nz_color"), "Color", additive (dShape), md);
    make (id ("_nz_density"), "Density", {}, md);
    make (id ("_nz_grain"), "Grain", {}, md);
    make (id ("_nz_bandwidth"), "Bandwidth", {}, md);
    make (id ("_nz_center"), "Center", {}, md);
    make (id ("_nz_correlation"), "Correlation", {}, md);
    make (id ("_nz_seed"), "Seed", {}, md);
    make (id ("_nz_width"), "Width", {}, md);
    make (id ("_nz_burst"), "Burst", {}, md);
    make (id ("_nz_burstenv"), "Burst Shape", {}, md);
    make (id ("_nz_turb"), "Turbulence", additive (dChaos), md);
    make (id ("_nz_gust"), "Gust", {}, md);
    // physical
    make (id ("_ph_stiffness"), "Stiffness", {}, md);
    make (id ("_ph_opening"), "Opening", {}, md);
    make (id ("_ph_position"), "Position", {}, md);
    make (id ("_ph_speed"), "Speed", additive (dShape), md);
    make (id ("_ph_turb"), "Turbulence", additive (dChaos), md);
    make (id ("_ph_hardness"), "Hardness", {}, md);
    make (id ("_ph_bright"), "Brightness", {}, md);
    // sidechain
    make (id ("_sc_lp"), "SC Low-Pass", {}, md);
    make (id ("_sc_hp"), "SC High-Pass", {}, md);
    make (id ("_sc_follow"), "Follow", {}, md);
    make (id ("_sc_transient"), "Transients", {}, md);

    processor.getAPVTS().addParameterListener (modelId, this);
    updateVisibility();
}

ExciterSlotPanel::~ExciterSlotPanel()
{
    processor.getAPVTS().removeParameterListener (modelId, this);
    cancelPendingUpdate();
}

Knob* ExciterSlotPanel::make (const juce::String& paramId, const juce::String& name, Knob::ModMapping mapping, int diameter)
{
    auto* k = knob (paramId, name, mapping, diameter);
    k->setAccentColour (slot == 0 ? theme::exciterA : theme::exciterB);
    byId[paramId] = k;
    return k;
}

ExciterModel ExciterSlotPanel::currentModel() const
{
    auto* v = processor.getAPVTS().getRawParameterValue (modelId);
    return (ExciterModel) juce::jlimit (0, (int) ExciterModel::Count - 1, v != nullptr ? (int) v->load() : 0);
}

std::vector<juce::String> ExciterSlotPanel::idsForModel (ExciterModel m, bool compactSet) const
{
    using M = ExciterModel;
    std::vector<juce::String> out;
    auto add = [&] (std::initializer_list<const char*> suffixes) { for (auto* s : suffixes) out.push_back (id (s)); };
    auto addGlobal = [&] (std::initializer_list<const char*> full) { for (auto* s : full) out.push_back (s); };

    if (compactSet)
    {
        if (m == M::Breath) addGlobal ({ ids::excNoise, ids::excPressure, ids::excReed });
        else if (m == M::Wave) add ({ "_wave_shape", "_wave_pw", "_wave_sub" });
        else if (m == M::Complex) add ({ "_cx_complexity", "_cx_feedback", "_cx_chaos" });
        else if (m == M::NoiseBand || m == M::NoiseMetallic) add ({ "_nz_center", "_nz_bandwidth", "_nz_color" });
        else if (m == M::NoiseVelvet || m == M::NoiseCrackle || m == M::NoiseAerosol) add ({ "_nz_density", "_nz_grain", "_nz_burst" });
        else if (m == M::NoiseSteam) add ({ "_nz_bandwidth", "_nz_density", "_nz_turb" });
        else if (m == M::NoiseWind) add ({ "_nz_density", "_nz_gust", "_nz_turb" });
        else if (isNoise (m)) add ({ "_nz_color", "_nz_correlation", "_nz_turb" });
        else if (isBlown (m)) add ({ "_ph_stiffness", "_ph_opening", "_ph_speed" });
        else if (isStruck (m)) add ({ "_ph_hardness", "_ph_position", "_ph_speed" });
        else if (m == M::Sidechain) add ({ "_sc_follow", "_sc_transient", "_sc_lp" });
        return out;
    }

    if (m == M::Breath)
        addGlobal ({ ids::excNoise, ids::excNoiseColor, ids::excPressure, ids::excReed, ids::excPluck, ids::excExternalIn,
                     ids::excPluckLength, ids::excTurbulence, ids::excVelocity, ids::excAttackClick, ids::excReleaseNoise, ids::excBreathRandom });
    else if (m == M::Wave) add ({ "_wave_shape", "_wave_pw", "_wave_sub", "_wave_pd" });
    else if (m == M::Complex) add ({ "_cx_complexity", "_cx_symmetry", "_cx_bend", "_cx_instab", "_cx_spread", "_cx_warp", "_cx_feedback", "_cx_chaos", "_cx_ratio" });
    else if (m == M::NoiseBand || m == M::NoiseMetallic) add ({ "_nz_center", "_nz_bandwidth", "_nz_color", "_nz_turb", "_nz_correlation", "_nz_seed", "_nz_width" });
    else if (m == M::NoiseVelvet || m == M::NoiseCrackle || m == M::NoiseAerosol) add ({ "_nz_density", "_nz_grain", "_nz_burst", "_nz_burstenv", "_nz_color", "_nz_turb", "_nz_correlation", "_nz_seed", "_nz_width" });
    else if (m == M::NoiseSteam) add ({ "_nz_bandwidth", "_nz_density", "_nz_gust", "_nz_color", "_nz_turb", "_nz_correlation", "_nz_seed", "_nz_width" });
    else if (m == M::NoiseWind) add ({ "_nz_density", "_nz_gust", "_nz_color", "_nz_turb", "_nz_correlation", "_nz_seed", "_nz_width" });
    else if (isNoise (m)) add ({ "_nz_color", "_nz_turb", "_nz_correlation", "_nz_seed", "_nz_width" });
    else if (isBlown (m)) add ({ "_ph_stiffness", "_ph_opening", "_ph_speed", "_ph_position", "_ph_turb", "_ph_bright" });
    else if (isStruck (m)) add ({ "_ph_hardness", "_ph_position", "_ph_speed", "_ph_stiffness", "_ph_turb", "_ph_bright" });
    else if (m == M::Sidechain) add ({ "_sc_lp", "_sc_hp", "_sc_follow", "_sc_transient" });
    return out;
}

void ExciterSlotPanel::updateVisibility()
{
    const auto m = currentModel();
    const auto wanted = idsForModel (m, compact);
    modelKnobs.clear();
    for (auto& [pid, k] : byId)
    {
        const bool isCommon = std::find (commonKnobs.begin(), commonKnobs.end(), k) != commonKnobs.end();
        if (isCommon) continue;
        const bool show = std::find (wanted.begin(), wanted.end(), pid) != wanted.end();
        k->setVisible (show);
    }
    for (const auto& pid : wanted) modelKnobs.push_back (byId[pid]);
    if (freeze != nullptr) freeze->setVisible (m == ExciterModel::Sidechain);
    if (modelCaption != nullptr) modelCaption->setText (m == ExciterModel::Off ? "OFF" : choices::exciterModels()[(int) m].toUpperCase(), juce::dontSendNotification);
    scope->setActiveLook (m != ExciterModel::Off);
    resized();
    repaint();
}

void ExciterSlotPanel::paint (juce::Graphics& g)
{
    ParamPanel::paint (g);
    if (compact)
    {
        // thin separator under the compact module
        g.setColour (theme::panelBorder);
        g.fillRect (getLocalBounds().removeFromBottom (1));
    }
}

void ExciterSlotPanel::resized()
{
    auto r = getContentArea();
    if (compact)
    {
        auto head = r.removeFromTop (30);
        scope->setBounds (head.removeFromRight (130).reduced (0, 1));
        head.removeFromRight (6);
        nameLabel->setBounds (head.removeFromLeft (76));
        model->setBounds (head.reduced (0, 2));
        r.removeFromTop (4);
        std::vector<juce::Component*> row;
        for (auto* k : commonKnobs) row.push_back (k);
        for (auto* k : modelKnobs) row.push_back (k);
        while (row.size() < 5) row.push_back (nullptr);
        knobRowV (r.removeFromTop (72), row);
        return;
    }

    auto head = r.removeFromTop (40);
    scope->setBounds (head.removeFromRight (150).reduced (0, 2));
    head.removeFromRight (8);
    model->setBounds (head.removeFromLeft (170));
    head.removeFromLeft (8);
    retrig->setBounds (head.removeFromLeft (100));
    head.removeFromLeft (8);
    if (sync != nullptr) { sync->setBounds (head.removeFromLeft (84).withTrimmedTop (14)); head.removeFromLeft (4); }
    if (freeze != nullptr) freeze->setBounds (head.removeFromLeft (70).withTrimmedTop (14));

    r.removeFromTop (4);
    commonCaption->setBounds (r.removeFromTop (14));
    std::vector<juce::Component*> common (commonKnobs.begin(), commonKnobs.end());
    knobRowV (r.removeFromTop (60), common);
    r.removeFromTop (4);
    modelCaption->setBounds (r.removeFromTop (14));

    const int perRow = 7;
    const int rowH = 66;
    const int count = (int) modelKnobs.size();
    const int rows = (count + perRow - 1) / perRow;
    for (int row = 0; row < rows; ++row)
    {
        auto line = r.removeFromTop (rowH);
        std::vector<juce::Component*> items;
        for (int i = row * perRow; i < juce::jmin (count, (row + 1) * perRow); ++i) items.push_back (modelKnobs[(size_t) i]);
        layoutFixed (line, items, line.getWidth() / perRow, 0);
    }
}
} // namespace aeriform
