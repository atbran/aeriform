#include "PipePage.h"
#include "Theme.h"

namespace aeriform
{
using namespace theme;

// ===========================================================================
// PipeTelemetryDisplay
// ===========================================================================
PipeTelemetryDisplay::PipeTelemetryDisplay (AeriformProcessor& p, int s)
    : processor (p), slot (s)
{
    startTimerHz (30);
}

void PipeTelemetryDisplay::resized() {}

void PipeTelemetryDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().reduced (8, 4);

    // Frame & dark background
    g.setColour (panelBorder.withAlpha (0.4f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f, 1.0f);
    g.setColour (inset.withAlpha (0.7f));
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.5f), 3.0f);

    // Title
    auto titleArea = r.removeFromTop (16);
    g.setColour (textSecondary);
    g.setFont (font (10.5f, true));
    g.drawText ("DIAGNOSTICS & TELEMETRY", titleArea, juce::Justification::centredLeft);

    r.removeFromTop (4);

    // Telemetry atomics (read without locks or allocations on audio thread)
    const float gainDb = processor.getVisualizerModel().pipeLoopGainDb[(size_t) slot].load (std::memory_order_relaxed);
    const float phaseComp = processor.getVisualizerModel().pipePhaseCompSamples[(size_t) slot].load (std::memory_order_relaxed);
    const float measuredDecay = processor.getVisualizerModel().pipeMeasuredDecaySec[(size_t) slot].load (std::memory_order_relaxed);

    // APVTS nominal decay
    const char* rtIds[3] = { ids::resPipeRt, ids::rbPipeRt, ids::rcPipeRt };
    auto* rtParam = processor.getAPVTS().getParameter (rtIds[slot]);
    const float nominalRt = rtParam != nullptr ? rtParam->convertFrom0to1 (rtParam->getValue()) : 0.5f;

    // 1. Effective Loop Gain meter (dB below unity)
    auto gainArea = r.removeFromTop (22);
    auto gainLabelArea = gainArea.removeFromLeft (100);
    g.setColour (textDim);
    g.setFont (font (10.0f));
    g.drawText ("Loop Gain:", gainLabelArea, juce::Justification::centredLeft);

    auto gainMeterArea = gainArea.removeFromLeft (160).reduced (0, 6).toFloat();
    g.setColour (panelRaised);
    g.fillRoundedRectangle (gainMeterArea, 2.0f);

    // Fill bar (-60 dB to 0 dB)
    const float normGain = std::clamp ((gainDb + 60.0f) / 60.0f, 0.0f, 1.0f);
    if (normGain > 0.0f)
    {
        auto fill = gainMeterArea.withWidth (gainMeterArea.getWidth() * normGain);
        g.setColour (teal);
        g.fillRoundedRectangle (fill, 2.0f);
    }
    gainArea.removeFromLeft (8);
    g.setColour (textPrimary);
    g.setFont (monoFont (10.0f));
    g.drawText (juce::String::formatted ("%.2f dB", gainDb), gainArea, juce::Justification::centredLeft);

    r.removeFromTop (4);

    // 2. Total Phase Compensation
    auto phaseArea = r.removeFromTop (18);
    auto phaseLabelArea = phaseArea.removeFromLeft (100);
    g.setColour (textDim);
    g.setFont (font (10.0f));
    g.drawText ("Phase Comp:", phaseLabelArea, juce::Justification::centredLeft);

    g.setColour (textPrimary);
    g.setFont (monoFont (10.0f));
    g.drawText (juce::String::formatted ("%.2f smp", phaseComp), phaseArea, juce::Justification::centredLeft);

    r.removeFromTop (4);

    // 3. Nominal vs Measured Decay Readout (Spec 1.4)
    auto decayArea = r.removeFromTop (18);
    auto decayLabelArea = decayArea.removeFromLeft (100);
    g.setColour (textDim);
    g.setFont (font (10.0f));
    g.drawText ("Decay Readout:", decayLabelArea, juce::Justification::centredLeft);

    g.setColour (textPrimary);
    g.setFont (monoFont (10.0f));
    if (measuredDecay > 0.001f)
        g.drawText (juce::String::formatted ("Nominal: %.2f s | Loop at f0: %.2f s", nominalRt, measuredDecay), decayArea, juce::Justification::centredLeft);
    else
        g.drawText (juce::String::formatted ("Nominal: %.2f s (lossless g law)", nominalRt), decayArea, juce::Justification::centredLeft);
}

// ===========================================================================
// SteamPanel
// ===========================================================================
PipePage::SteamPanel::SteamPanel (AeriformProcessor& p, int slot)
    : ParamPanel (p, "STEAM EXCITATION", slot == 0 ? nodeA : (slot == 1 ? nodeB : nodeC))
{
    const juce::String px = slot == 0 ? "res_" : (slot == 1 ? "rb_" : "rc_");

    pressure = knob (px + "pressure", "Pressure", {}, knobSize);
    dcNoise  = knob (px + "dcnoise", "DC / Noise", {}, knobSize);
    excCut   = knob (px + "exc_cut", "Exc Cutoff", {}, knobSize);
    excRes   = knob (px + "exc_res", "Exc Res", {}, knobSize);
    excKt    = knob (px + "exc_kt", "Key Track", {}, knobSize);
    excVt    = knob (px + "exc_vt", "Vel Track", {}, knobSize);

    for (auto* k : { pressure, dcNoise, excCut, excRes, excKt, excVt })
        k->setAccentColour (getAccent());
}

void PipePage::SteamPanel::resized()
{
    auto r = getContentArea().reduced (4, 4);
    knobRow (r.removeFromTop (88), { pressure, dcNoise, excCut }, 4);
    r.removeFromTop (14);
    knobRow (r.removeFromTop (88), { excRes, excKt, excVt }, 4);
}

// ===========================================================================
// PipeResonatorPanel
// ===========================================================================
PipePage::PipeResonatorPanel::PipeResonatorPanel (AeriformProcessor& p, int slot)
    : ParamPanel (p, "PIPE RESONATOR", slot == 0 ? nodeA : (slot == 1 ? nodeB : nodeC))
{
    const juce::String px = slot == 0 ? "res_" : (slot == 1 ? "rb_" : "rc_");

    // Spec 1.4: MUST be explicitly labelled "Nominal Decay"
    rt     = knob (px + "rt", "Nominal Decay", {}, knobSize);
    rtKt   = knob (px + "rt_kt", "Decay KT", {}, knobSize);
    damp   = knob (px + "damp", "Damping", {}, knobSize);
    bore   = control<ChoiceBox> (processor, px + "bore", "Bore");

    lp     = knob (px + "lp", "Loop LP", {}, knobSize);
    hp     = knob (px + "hp", "Loop HP", {}, knobSize);
    filtKt = knob (px + "filt_kt", "Filter KT", {}, knobSize);

    satDrive = knob (px + "sat_drive", "Drive", {}, knobSize);
    satKnee  = knob (px + "sat_knee", "Hardness", {}, knobSize);

    // Spec 1.6: Asymmetry deserves visual prominence (large 72px knob with amber accent)
    satSym   = knob (px + "sat_sym", "Asymmetry", {}, knobSizeLarge);
    satSym->setAccentColour (amber);

    for (auto* k : { rt, rtKt, damp, lp, hp, filtKt, satDrive, satKnee })
        k->setAccentColour (getAccent());

    telemetry = std::make_unique<PipeTelemetryDisplay> (processor, slot);
    addAndMakeVisible (*telemetry);
}

void PipePage::PipeResonatorPanel::resized()
{
    auto r = getContentArea().reduced (4, 4);

    // Row 1: Timing & Bore
    auto row1 = r.removeFromTop (88);
    const int colW = (row1.getWidth() - 3 * 4) / 4;
    rt->setBounds (row1.removeFromLeft (colW)); row1.removeFromLeft (4);
    rtKt->setBounds (row1.removeFromLeft (colW)); row1.removeFromLeft (4);
    damp->setBounds (row1.removeFromLeft (colW)); row1.removeFromLeft (4);
    bore->setBounds (row1.reduced (0, 14));

    r.removeFromTop (10);

    // Row 2: Loop Filters
    knobRow (r.removeFromTop (88), { lp, hp, filtKt }, 6);

    r.removeFromTop (10);

    // Row 3: Saturation with prominent Asymmetry
    auto satArea = r.removeFromTop (108);
    const int satW = satArea.getWidth();
    satDrive->setBounds (satArea.removeFromLeft (satW / 3).reduced (4, 4));
    satKnee->setBounds (satArea.removeFromLeft (satW / 3).reduced (4, 4));
    satSym->setBounds (satArea.reduced (4, 0));

    r.removeFromTop (12);

    // Row 4: Telemetry display
    telemetry->setBounds (r.removeFromTop (114));
}

// ===========================================================================
// SpaceControlsPanel
// ===========================================================================
PipePage::SpaceControlsPanel::SpaceControlsPanel (AeriformProcessor& p)
    : ParamPanel (p, "SPACE", teal)
{
    reverbType = control<ChoiceBox> (processor, ids::reverbType, "Reverb Type");
    revMix     = knob (ids::reverbMix, "Rev Mix", additive (ModDest::ReverbMix), knobSizeSmall);
    revSize    = knob (ids::reverbSize, "Rev Size", additive (ModDest::ReverbSize), knobSizeSmall);
    revDecay   = knob (ids::reverbDecay, "Rev Decay", additive (ModDest::ReverbDecay), knobSizeSmall);
    revDamp    = knob (ids::reverbDamping, "Rev Damp", additive (ModDest::ReverbDamp), knobSizeSmall);
    revPre     = knob (ids::reverbPreDelay, "Rev Pre", additive (ModDest::ReverbPredelay), knobSizeSmall);

    roomOn          = control<Toggle> (processor, ids::roomOn, "Room Enabled");
    roomSend        = knob (ids::roomSend, "Voice Send", {}, knobSizeSmall);
    roomLevel       = knob (ids::roomLevel, "Room Level", {}, knobSizeSmall);
    roomSize        = knob (ids::roomSize, "Room Size", {}, knobSizeSmall);
    roomFeedback    = knob (ids::roomFeedback, "Feedback", {}, knobSizeSmall);
    roomWallDamping = knob (ids::roomWallDamping, "Wall Damp", {}, knobSizeSmall);

    for (auto& k : knobs) k->setAccentColour (teal);
}

void PipePage::SpaceControlsPanel::resized()
{
    auto r = getContentArea().reduced (4, 4);

    // Reverb section
    auto revHeader = r.removeFromTop (26);
    reverbType->setBounds (revHeader.removeFromLeft (160));
    r.removeFromTop (4);
    knobRow (r.removeFromTop (76), { revMix, revSize, revDecay, revDamp, revPre }, 4);

    r.removeFromTop (16);

    // Coupled Room section
    auto roomHeader = r.removeFromTop (26);
    roomOn->setBounds (roomHeader.removeFromLeft (120));
    r.removeFromTop (4);
    knobRow (r.removeFromTop (76), { roomSend, roomLevel, roomSize }, 4);
    r.removeFromTop (8);
    knobRow (r.removeFromTop (76), { roomFeedback, roomWallDamping, nullptr }, 4);
}

// ===========================================================================
// PipePage
// ===========================================================================
PipePage::PipePage (AeriformProcessor& p)
    : processor (p)
{
    // Slot selector buttons
    const char* slotNames[3] = { "RESONATOR A", "RESONATOR B", "RESONATOR C" };
    for (int i = 0; i < 3; ++i)
    {
        slotButtons[(size_t) i] = std::make_unique<juce::TextButton> (slotNames[i]);
        auto& btn = *slotButtons[(size_t) i];
        btn.setClickingTogglesState (true);
        btn.setRadioGroupId (1001);
        btn.onClick = [this, i] { selectSlot (i); };
        addAndMakeVisible (btn);
    }
    slotButtons[0]->setToggleState (true, juce::dontSendNotification);

    // Model selectors for slots 0, 1, 2
    const char* modelIds[3] = { ids::resMode, ids::rbType, ids::rcType };
    for (int i = 0; i < 3; ++i)
    {
        modelSelectors[(size_t) i] = std::make_unique<ChoiceBox> (processor, modelIds[i], "Slot Model");
        addAndMakeVisible (*modelSelectors[(size_t) i]);
        modelSelectors[(size_t) i]->setVisible (i == 0);
    }

    // Explanatory Banner & Set-to-PIPE button
    bannerLabel.setFont (font (11.0f));
    bannerLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (bannerLabel);

    setPipeButton.onClick = [this]
    {
        const char* ids[3] = { ids::resMode, ids::rbType, ids::rcType };
        processor.getPatchTools().setParameter (ids[selectedSlot], (float) ResMode::Pipe);
        updateModelBanner();
    };
    addAndMakeVisible (setPipeButton);

    // Per-slot STEAM and PIPE panels
    for (int i = 0; i < 3; ++i)
    {
        steamPanels[(size_t) i] = std::make_unique<SteamPanel> (processor, i);
        pipePanels[(size_t) i]  = std::make_unique<PipeResonatorPanel> (processor, i);
        addAndMakeVisible (*steamPanels[(size_t) i]);
        addAndMakeVisible (*pipePanels[(size_t) i]);
        steamPanels[(size_t) i]->setVisible (i == 0);
        pipePanels[(size_t) i]->setVisible (i == 0);
    }

    // Global SPACE panel
    spacePanel = std::make_unique<SpaceControlsPanel> (processor);
    addAndMakeVisible (*spacePanel);

    updateModelBanner();
    startTimerHz (20);
}

void PipePage::selectSlot (int slotIndex)
{
    selectedSlot = std::clamp (slotIndex, 0, 2);

    for (int i = 0; i < 3; ++i)
    {
        slotButtons[(size_t) i]->setToggleState (i == selectedSlot, juce::dontSendNotification);
        modelSelectors[(size_t) i]->setVisible (i == selectedSlot);
        steamPanels[(size_t) i]->setVisible (i == selectedSlot);
        pipePanels[(size_t) i]->setVisible (i == selectedSlot);
    }

    updateModelBanner();
    resized();
}

std::vector<ParamPanel*> PipePage::getPanels()
{
    std::vector<ParamPanel*> out;
    for (int i = 0; i < 3; ++i)
    {
        steamPanels[(size_t) i]->collectPanels (out);
        pipePanels[(size_t) i]->collectPanels (out);
    }
    if (spacePanel != nullptr) spacePanel->collectPanels (out);
    return out;
}

void PipePage::updateModelBanner()
{
    const char* modelIds[3] = { ids::resMode, ids::rbType, ids::rcType };
    auto* param = processor.getAPVTS().getParameter (modelIds[selectedSlot]);
    const int modelIdx = param != nullptr ? std::clamp ((int) std::lround (param->convertFrom0to1 (param->getValue())), 0, (int) ResMode::Count - 1) : 0;

    const char* slotLetters[3] = { "A", "B", "C" };
    const bool isPipe = (modelIdx == (int) ResMode::Pipe);

    if (isPipe)
    {
        bannerLabel.setText (juce::String ("ACTIVE: Resonator ") + slotLetters[selectedSlot]
                             + " is running the PIPE model. Steam excitation and acoustic loop parameters below are active.",
                             juce::dontSendNotification);
        bannerLabel.setColour (juce::Label::textColourId, teal);
        setPipeButton.setVisible (false);
    }
    else
    {
        auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (param);
        const juce::String modelName = (choiceParam != nullptr && modelIdx < choiceParam->choices.size())
                                     ? choiceParam->choices[modelIdx] : juce::String (modelIdx);
        bannerLabel.setText (juce::String ("NOTICE: Resonator ") + slotLetters[selectedSlot] + " is currently set to ["
                             + modelName + "]. These controls apply when the slot's model is PIPE (index 9).",
                             juce::dontSendNotification);
        bannerLabel.setColour (juce::Label::textColourId, amber);
        setPipeButton.setVisible (true);
    }
}

void PipePage::timerCallback()
{
    updateModelBanner();
}

void PipePage::paint (juce::Graphics& g)
{
    g.fillAll (background);
}

void PipePage::resized()
{
    auto r = getLocalBounds().reduced (8, 6);

    // Top Header Bar: slot selector, model selector, banner, set-to-pipe button
    auto topBar = r.removeFromTop (38);
    const int btnW = 100;
    for (int i = 0; i < 3; ++i)
    {
        slotButtons[(size_t) i]->setBounds (topBar.removeFromLeft (btnW).reduced (2, 4));
    }
    topBar.removeFromLeft (12);

    for (int i = 0; i < 3; ++i)
    {
        modelSelectors[(size_t) i]->setBounds (topBar.removeFromLeft (150).reduced (0, 1));
    }
    topBar.removeFromLeft (12);

    if (setPipeButton.isVisible())
        setPipeButton.setBounds (topBar.removeFromRight (100).reduced (0, 6));

    bannerLabel.setBounds (topBar);

    r.removeFromTop (8);

    // Main Columns: STEAM (left), PIPE (middle), SPACE (right)
    const int gap = 8;
    const int totalW = r.getWidth() - 2 * gap;
    const int col1W = totalW * 30 / 100; // ~370px
    const int col2W = totalW * 42 / 100; // ~520px
    const int col3W = totalW - col1W - col2W; // ~350px

    auto steamArea = r.removeFromLeft (col1W); r.removeFromLeft (gap);
    auto pipeArea  = r.removeFromLeft (col2W); r.removeFromLeft (gap);
    auto spaceArea = r;

    for (int i = 0; i < 3; ++i)
    {
        steamPanels[(size_t) i]->setBounds (steamArea);
        pipePanels[(size_t) i]->setBounds (pipeArea);
    }

    if (spacePanel != nullptr)
        spacePanel->setBounds (spaceArea);
}

} // namespace aeriform
