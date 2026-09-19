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
                    knob (resInharm, "Inharmonic", additive (ModDest::ResAInharm), d), knob (resSize, "Size", additive (ModDest::ResASize), d), knob (resSaturation, "Saturation", additive (ModDest::ResASaturation), d) };
        charCaption = caption ("CHARACTER");
        charRow = { knob (resShape, "Shape", additive (ModDest::Shape), d), knob (resReflection, "Reflection", additive (ModDest::Reflection), d),
                    knob (excReed, "Reed", {}, d), knob (excPressure, "Pressure", additive (ModDest::Pressure), d),
                    knob (resPickup, "Pickup", {}, d), knob (resWidth, "Width", additive (ModDest::ResAWidth), d), knob (resWet, "Wet", additive (ModDest::ResAWet), d) };
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
        const ModDest dInharm= slot == 1 ? ModDest::ResBInharm : ModDest::ResCInharm;
        const ModDest dSize  = slot == 1 ? ModDest::ResBSize : ModDest::ResCSize;
        const ModDest dSat   = slot == 1 ? ModDest::ResBSaturation : ModDest::ResCSaturation;
        const ModDest dWidth = slot == 1 ? ModDest::ResBWidth : ModDest::ResCWidth;
        const ModDest dWet   = slot == 1 ? ModDest::ResBWet : ModDest::ResCWet;
        on   = control<Toggle> (processor, sid ("_on"), "On");
        type = control<ChoiceBox> (processor, sid ("_type"), "Model");
        tuneCaption = caption ("TUNING / LEVELS");
        tuneRow = { knob (sid ("_coarse"), "Coarse", semitones (dPitch, 24.0f), d), knob (sid ("_fine"), "Fine", semitones (dPitch, 2400.0f), d),
                    knob (sid ("_ratio"), "Ratio", {}, d), knob (sid ("_keytrack"), "Key Track", {}, d), knob (sid ("_input"), "Input", {}, d),
                    knob (sid ("_output"), "Output", {}, d), knob (sid ("_pan"), "Pan", {}, d) };
        loopCaption = caption ("LOOP");
        loopRow = { knob (sid ("_feedback"), "Feedback", additive (dFb), d), knob (sid ("_damping"), "Damping", additive (dDamp), d),
                    knob (sid ("_brightness"), "Brightness", additive (dBright), d), knob (sid ("_dispersion"), "Dispersion", {}, d),
                    knob (sid ("_inharm"), "Inharmonic", additive (dInharm), d), knob (sid ("_size"), "Size", additive (dSize), d), knob (sid ("_saturation"), "Saturation", additive (dSat), d) };
        charCaption = caption ("CHARACTER");
        charRow = { knob (sid ("_shape"), "Shape", {}, d), knob (sid ("_reflect"), "Reflection", {}, d), knob (sid ("_reed"), "Reed", {}, d),
                    knob (sid ("_pickup"), "Pickup", {}, d), knob (sid ("_width"), "Width", additive (dWidth), d), knob (sid ("_wet"), "Wet", additive (dWet), d), nullptr };
    }
    resonatorTab = control<juce::TextButton> ("Resonator");
    nonlinearTab = control<juce::TextButton> ("Nonlinearity");
    resonatorTab->onClick = [this] { selectNonlinear (false); };
    nonlinearTab->onClick = [this] { selectNonlinear (true); };
    const juce::String nlPrefix = slot == 0 ? "res" : px;
    nlOn = control<Toggle> (processor, nlPrefix+"_nl_on", "On");
    nlModel = control<ChoiceBox> (processor, nlPrefix+"_nl_model", "Model");
    nlPosition = control<ChoiceBox> (processor, nlPrefix+"_nl_pos", "Position");
    const auto driveDest = (ModDest) ((int) ModDest::ResANlDrive+slot*2);
    nlDrive = knob (nlPrefix+"_nl_drive", "Drive", additive (driveDest), d);
    nlAmount = knob (nlPrefix+"_nl_amount", "Amount", additive ((ModDest) ((int) driveDest+1)), d);
    nlBias = knob (nlPrefix+"_nl_bias", "Bias", {}, d);
    nlCurve = control<NonlinearCurveDisplay> (processor, slot, accent);
    nlCurve->setBiasControl (nlBias);
    nlCurve->setDriveControl (nlDrive);
    nonlinearControls = { nlOn, nlModel, nlPosition, nlDrive, nlAmount, nlBias, nlCurve };
    if (slot == 0)
    {
        nlAdaa = control<Toggle> (processor, ids::nlAdaa, "ADAA / all slots");
        nonlinearControls.push_back (nlAdaa);
    }
    selectNonlinear (false);
    for (auto& k : knobs) k->setAccentColour (accent);
    energy = std::make_unique<EnergyBar> (processor.getVisualizerModel(), slot, accent);
    addAndMakeVisible (*energy);
}

void ResonatorSlotPanel::selectNonlinear (bool selected)
{
    nonlinearSelected = selected;
    resonatorTab->setToggleState (!selected, juce::dontSendNotification);
    nonlinearTab->setToggleState (selected, juce::dontSendNotification);
    for (auto* item : {tuneCaption, loopCaption, charCaption, bodyCaption}) if (item) item->setVisible (!selected);
    for (const auto* row : {&tuneRow, &loopRow, &charRow, &bodyRow})
        for (auto* item : *row) if (item) item->setVisible (!selected);
    for (auto* item : nonlinearControls) item->setVisible (selected);
    if (energy) resized();
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
    auto tabs = r.removeFromTop (26).reduced (0,2);
    resonatorTab->setBounds (tabs.removeFromLeft (tabs.getWidth()/2));
    nonlinearTab->setBounds (tabs);
    if (nonlinearSelected)
    {
        r.removeFromTop (6);
        auto controls = r.removeFromTop (40);
        nlOn->setBounds (controls.removeFromLeft (50).withTrimmedTop (14));
        const int half = controls.getWidth()/2;
        nlModel->setBounds (controls.removeFromLeft (half).reduced (2,0));
        nlPosition->setBounds (controls.reduced (2,0));
        knobRow (r.removeFromTop (72), {nlDrive, nlAmount, nlBias});
        auto options = r.removeFromTop (24);
        if (nlAdaa) nlAdaa->setBounds (options);
        r.removeFromTop (6);
        nlCurve->setBounds (r);
        return;
    }
    const int rowH = 60;
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
    bypass=control<Toggle>(processor,netBypass,"BYPASS RESONATORS");
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
    loopCaption  = caption ("ENERGY LOOP");
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
    bypass->setBounds(head.removeFromRight(190).withTrimmedTop(14));
    head.removeFromRight (8);
    layoutFixed (head, { routing, inject, tap, polarity }, (head.getWidth() - 24) / 4, 8);
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
    diagram = std::make_unique<NetworkDiagram> (processor, true);
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
    diagram->setBounds (r.removeFromLeft (190));
    r.removeFromLeft (8);
    auto head = r.removeFromTop (40);
    routing->setBounds (head.removeFromLeft ((head.getWidth() - 8) / 2));
    head.removeFromLeft (8);
    tap->setBounds (head);
    r.removeFromTop (4);
    head = r.removeFromTop (22);
    rbOn->setBounds (head.removeFromLeft (52));
    rcOn->setBounds (head.removeFromLeft (52));
    head.removeFromLeft (4);
    loopOn->setBounds (head.removeFromLeft (110));
    r.removeFromTop (6);
    knobRow (r.removeFromTop (84), { repipe, feedback, damping, width, mix, loopAmount });
}
} // namespace aeriform
