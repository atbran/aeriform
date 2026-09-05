#include "NetworkPanels.h"

namespace aeriform
{
// ---------------------------------------------------------------------------
ResonatorSlotPanel::ResonatorSlotPanel (AeriformProcessor& p, int s)
    : ParamPanel (p, s == 0 ? "RESONATOR A" : (s == 1 ? "RESONATOR B" : "RESONATOR C"),
                  s == 0 ? theme::nodeA : (s == 1 ? theme::nodeB : theme::nodeC)),
      slot (s)
{
    using namespace ids;
    const auto accent = getAccent();
    const int d = theme::knobSizeSmall;
    const juce::String px = slot == 1 ? "rb" : "rc";
    auto sid = [&] (const char* suffix) { return px + suffix; };

    if (slot == 0)
    {
        on   = control<Toggle> (processor, resOn, "On");
        type = control<ChoiceBox> (processor, resMode, "Model");
        tuneCaption = caption ("TUNING / LEVELS");
        tuneRow = { knob (resCoarse, "Coarse", {}, d), knob (resFine, "Fine", semitones (ModDest::Pitch, 2400.0f), d), knob (resLength, "Length", {}, d),
                    knob (resKeyTrack, "Key Track", {}, d), knob (resInput, "Input", {}, d), knob (resOutput, "Output", {}, d),
                    knob (resPan, "Pan", additive (ModDest::ResAPan), d) };
        loopCaption = caption ("LOOP");
        loopRow = { knob (resFeedback, "Feedback", additive (ModDest::Feedback), d), knob (resDamping, "Damping", additive (ModDest::Damping), d),
                    knob (resBrightness, "Brightness", additive (ModDest::Brightness), d), knob (resDispersion, "Dispersion", additive (ModDest::Dispersion), d),
                    knob (resInharm, "Inharmonic", {}, d), knob (resSize, "Size", {}, d), knob (resSaturation, "Saturation", {}, d) };
        charCaption = caption ("CHARACTER");
        charRow = { knob (resShape, "Shape", additive (ModDest::Shape), d), knob (resReflection, "Reflection", additive (ModDest::Reflection), d),
                    knob (excReed, "Reed", {}, d), knob (excPressure, "Pressure", additive (ModDest::Pressure), d),
                    knob (resPickup, "Pickup", {}, d), knob (resWidth, "Width", {}, d), nullptr };
        bodyCaption = caption ("BODY / FORMANT");
        bodyRow = { knob (resBodyFreq, "Body Freq", exponential (ModDest::BodyFreq, 3.0f), d), knob (resBodyRes, "Body Res", {}, d),
                    knob (resBodyMix, "Body Mix", additive (ModDest::BodyMix), d), knob (resBodyTrack, "Body Track", {}, d), nullptr, nullptr, nullptr };
    }
    else
    {
        const ModDest dPitch = slot == 1 ? ModDest::ResBPitch : ModDest::ResCPitch;
        const ModDest dFb    = slot == 1 ? ModDest::ResBFeedback : ModDest::ResCFeedback;
        const ModDest dDamp  = slot == 1 ? ModDest::ResBDamping : ModDest::ResCDamping;
        const ModDest dBright= slot == 1 ? ModDest::ResBBrightness : ModDest::ResCBrightness;
        on   = control<Toggle> (processor, sid ("_on"), "On");
        type = control<ChoiceBox> (processor, sid ("_type"), "Model");
        tuneCaption = caption ("TUNING / LEVELS");
        tuneRow = { knob (sid ("_coarse"), "Coarse", semitones (dPitch, 24.0f), d), knob (sid ("_fine"), "Fine", semitones (dPitch, 2400.0f), d),
                    knob (sid ("_ratio"), "Ratio", {}, d), knob (sid ("_keytrack"), "Key Track", {}, d), knob (sid ("_input"), "Input", {}, d),
                    knob (sid ("_output"), "Output", {}, d), knob (sid ("_pan"), "Pan", {}, d) };
        loopCaption = caption ("LOOP");
        loopRow = { knob (sid ("_feedback"), "Feedback", additive (dFb), d), knob (sid ("_damping"), "Damping", additive (dDamp), d),
                    knob (sid ("_brightness"), "Brightness", additive (dBright), d), knob (sid ("_dispersion"), "Dispersion", {}, d),
                    knob (sid ("_inharm"), "Inharmonic", {}, d), knob (sid ("_size"), "Size", {}, d), knob (sid ("_saturation"), "Saturation", {}, d) };
        charCaption = caption ("CHARACTER");
        charRow = { knob (sid ("_shape"), "Shape", {}, d), knob (sid ("_reflect"), "Reflection", {}, d), knob (sid ("_reed"), "Reed", {}, d),
                    knob (sid ("_pickup"), "Pickup", {}, d), knob (sid ("_width"), "Width", {}, d), nullptr, nullptr };
    }
    for (auto& k : knobs) k->setAccentColour (accent);
    energy = std::make_unique<EnergyBar> (processor.getVisualizerModel(), slot, accent);
    addAndMakeVisible (*energy);
}

void ResonatorSlotPanel::resized()
{
    auto r = getContentArea();
    auto head = r.removeFromTop (40);
    on->setBounds (head.removeFromLeft (56).withTrimmedTop (14));
    head.removeFromLeft (4);
    type->setBounds (head.removeFromLeft (150));
    head.removeFromLeft (10);
    energy->setBounds (head.withTrimmedTop (16));
    const int rowH = 62;
    auto row = [&] (juce::Label* cap, const std::vector<juce::Component*>& items)
    {
        r.removeFromTop (2);
        cap->setBounds (r.removeFromTop (14));
        knobRowV (r.removeFromTop (rowH), items, 0);
    };
    row (tuneCaption, tuneRow);
    row (loopCaption, loopRow);
    row (charCaption, charRow);
    if (bodyCaption != nullptr) row (bodyCaption, bodyRow);
}

// ---------------------------------------------------------------------------
NetworkControlsPanel::NetworkControlsPanel (AeriformProcessor& p) : ParamPanel (p, "NETWORK", theme::brass)
{
    using namespace ids;
    const int d = theme::knobSizeSmall;
    routing  = control<ChoiceBox> (processor, netMode, "Routing");
    inject   = control<ChoiceBox> (processor, netInject, "Inject");
    tap      = control<ChoiceBox> (processor, netTap, "Output Tap");
    polarity = control<ChoiceBox> (processor, netPolarity, "FB Polarity");
    repipe   = knob (netRepipe, "REPIPE", additive (ModDest::Repipe), theme::knobSizeLarge);
    feedback = knob (netFeedback, "Feedback", additive (ModDest::NetFeedback));
    damping  = knob (netDamping, "Damping");
    width    = knob (netWidth, "Width", additive (ModDest::NetWidth));
    mix      = knob (netMix, "Mix");
    fbDelay  = knob (netFbDelay, "FB Delay");
    fbFilter = knob (netFbFilter, "FB Filter");
    fbDrive  = knob (netFbDrive, "FB Drive");
    routesCaption = caption ("CROSS-FEEDBACK ROUTES  /  SERIAL SENDS  /  DRY INJECTION");
    ab = knob (netAB, "A > B", {}, d); ba = knob (netBA, "B > A", {}, d); bc = knob (netBC, "B > C", {}, d);
    cb = knob (netCB, "C > B", {}, d); ca = knob (netCA, "C > A", {}, d); ac = knob (netAC, "A > C", {}, d);
    sendAB = knob (netSendAB, "Send A>B", {}, d); sendBC = knob (netSendBC, "Send B>C", {}, d);
    injectB = knob (netInjectB, "Inject B", {}, d); injectC = knob (netInjectC, "Inject C", {}, d);
    for (auto* k : { ab, ba, bc, cb, ca, ac }) k->setAccentColour (theme::teal);
    loopCaption  = caption ("ENERGY LOOP  (experimental: resonator energy back into the excitation chain, bounded and governed)");
    loopOn       = control<Toggle> (processor, ids::loopOn, "Loop On");
    loopSource   = control<ChoiceBox> (processor, ids::loopSource, "Source");
    loopDest     = control<ChoiceBox> (processor, ids::loopDest, "Destination");
    loopPolarity = control<ChoiceBox> (processor, ids::loopPolarity, "Polarity");
    loopAmount   = knob (ids::loopAmount, "Return", additive (ModDest::LoopAmount), d);
    loopFilter   = knob (ids::loopFilter, "Filter", {}, d);
    loopDelay    = knob (ids::loopDelay, "Delay", {}, d);
    loopSat      = knob (ids::loopSat, "Saturation", {}, d);
    for (auto* k : { loopAmount, loopFilter, loopDelay, loopSat }) k->setAccentColour (theme::amber);
}

void NetworkControlsPanel::resized()
{
    auto r = getContentArea();
    auto head = r.removeFromTop (40);
    layoutFixed (head, { routing, inject, tap, polarity }, 150, 8);
    r.removeFromTop (4);
    knobRow (r.removeFromTop (78), { repipe, feedback, damping, width, mix, fbDelay, fbFilter, fbDrive });
    r.removeFromTop (2);
    routesCaption->setBounds (r.removeFromTop (14));
    knobRow (r.removeFromTop (60), { ab, ba, bc, cb, ca, ac, sendAB, sendBC, injectB, injectC }, 0);
    r.removeFromTop (2);
    loopCaption->setBounds (r.removeFromTop (14));
    auto loop = r.removeFromTop (60);
    loopOn->setBounds (loop.removeFromLeft (80).withTrimmedTop (18).withHeight (22));
    loopSource->setBounds (loop.removeFromLeft (110).withTrimmedTop (4).withHeight (40));
    loop.removeFromLeft (6);
    loopDest->setBounds (loop.removeFromLeft (130).withTrimmedTop (4).withHeight (40));
    loop.removeFromLeft (6);
    loopPolarity->setBounds (loop.removeFromLeft (100).withTrimmedTop (4).withHeight (40));
    loop.removeFromLeft (6);
    knobRow (loop, { loopAmount, loopFilter, loopDelay, loopSat }, 0);
}

// ---------------------------------------------------------------------------
NetworkOverviewPanel::NetworkOverviewPanel (AeriformProcessor& p) : ParamPanel (p, "NETWORK", theme::brass)
{
    using namespace ids;
    diagram  = std::make_unique<NetworkDiagram> (processor, true);
    addAndMakeVisible (*diagram);
    routing  = control<ChoiceBox> (processor, netMode, "Routing");
    tap      = control<ChoiceBox> (processor, netTap, "Output Tap");
    rbOn     = control<Toggle> (processor, ids::rbOn, "B");
    rcOn     = control<Toggle> (processor, ids::rcOn, "C");
    loopOn   = control<Toggle> (processor, ids::loopOn, "Energy Loop");
    repipe   = knob (netRepipe, "REPIPE", additive (ModDest::Repipe), theme::knobSizeLarge);
    feedback = knob (netFeedback, "Feedback", additive (ModDest::NetFeedback));
    damping  = knob (netDamping, "Damping");
    width    = knob (netWidth, "Width", additive (ModDest::NetWidth));
    mix      = knob (netMix, "Mix");
    loopAmount = knob (ids::loopAmount, "Loop Return", additive (ModDest::LoopAmount));
    loopAmount->setAccentColour (theme::amber);
}

void NetworkOverviewPanel::resized()
{
    auto r = getContentArea();
    diagram->setBounds (r.removeFromLeft (250));
    r.removeFromLeft (10);
    auto head = r.removeFromTop (40);
    routing->setBounds (head.removeFromLeft (130));
    head.removeFromLeft (8);
    tap->setBounds (head.removeFromLeft (120));
    head.removeFromLeft (8);
    rbOn->setBounds (head.removeFromLeft (52).withTrimmedTop (14));
    rcOn->setBounds (head.removeFromLeft (52).withTrimmedTop (14));
    head.removeFromLeft (4);
    loopOn->setBounds (head.removeFromLeft (110).withTrimmedTop (14));
    r.removeFromTop (6);
    knobRow (r.removeFromTop (84), { repipe, feedback, damping, width, mix, loopAmount });
}
} // namespace aeriform
