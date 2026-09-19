#pragma once

#include "Pages.h"
#include "ParamControls.h"
#include "PanelBase.h"
#include "Displays.h"

namespace aeriform
{

/** Telemetry readout display for PIPE model:
    - Effective loop gain in dB below unity (with meter)
    - Total phase compensation in samples (interpolator + filters + ADAA)
    - Nominal vs measured decay time in seconds (Spec 1.4, 1.6)
    Reads atomic lock-free buffers; zero allocations on audio thread. */
class PipeTelemetryDisplay : public juce::Component, private juce::Timer
{
public:
    PipeTelemetryDisplay (AeriformProcessor& p, int slot);
    ~PipeTelemetryDisplay() override { stopTimer(); }

    void paint (juce::Graphics& g) override;
    void resized() override;
    void setSlot (int s) { slot = s; repaint(); }

private:
    AeriformProcessor& processor;
    int slot = 0;
    void timerCallback() override { if (isShowing()) repaint(); }
};

/** PIPE Model GUI Page:
    - Slot selector (Resonator A, B, C) and prominent model selector with status banner (Spec 1.1)
    - No page-local parameter storage (Spec 1.2)
    - STEAM Panel: Pressure, DC/Noise, Exciter Cutoff, Exciter Resonance, Key Track, Vel Track (Spec 1.3)
    - PIPE Panel: Nominal Decay (Spec 1.4), Decay KT, Damp, Loop LP, Loop HP, Filter KT, Drive, Hardness,
                  prominently sized Asymmetry (Spec 1.6), Bore, and Telemetry display (Spec 1.6)
    - SPACE Panel: Voiced Room & Reverb controls (Spec 1.3)
*/
class PipePage final : public Page, private juce::Timer
{
public:
    explicit PipePage (AeriformProcessor&);
    ~PipePage() override { stopTimer(); }

    void resized() override;
    void paint (juce::Graphics&) override;

    std::vector<ParamPanel*> getPanels() override;

    void selectSlot (int slotIndex);
    int getSelectedSlot() const noexcept { return selectedSlot; }

    /** STEAM panel owning the 6 exciter parameters for a given slot. */
    class SteamPanel final : public ParamPanel
    {
    public:
        SteamPanel (AeriformProcessor&, int slot);
        void resized() override;
        Knob *pressure, *dcNoise, *excCut, *excRes, *excKt, *excVt;
    };

    /** PIPE Resonator panel owning the 10 acoustic loop parameters and telemetry display. */
    class PipeResonatorPanel final : public ParamPanel
    {
    public:
        PipeResonatorPanel (AeriformProcessor&, int slot);
        void resized() override;
        ChoiceBox* bore;
        Knob *rt, *rtKt, *damp;
        Knob *lp, *hp, *filtKt;
        Knob *satDrive, *satKnee, *satSym;
        std::unique_ptr<PipeTelemetryDisplay> telemetry;
    };

    /** SPACE panel owning room and reverb controls with voiced defaults. */
    class SpaceControlsPanel final : public ParamPanel
    {
    public:
        explicit SpaceControlsPanel (AeriformProcessor&);
        void resized() override;
        ChoiceBox* reverbType;
        Knob *revMix, *revSize, *revDecay, *revDamp, *revPre;
        Toggle* roomOn;
        Knob *roomSend, *roomLevel, *roomSize, *roomFeedback, *roomWallDamping;
    };

private:
    AeriformProcessor& processor;
    int selectedSlot = 0;

    // Header controls: slot selection, model selector, banner
    std::array<std::unique_ptr<juce::TextButton>, 3> slotButtons;
    std::array<std::unique_ptr<ChoiceBox>, 3> modelSelectors;
    juce::Label bannerLabel;
    juce::TextButton setPipeButton { "SET TO PIPE" };

    // Per-slot STEAM and PIPE panels
    std::array<std::unique_ptr<SteamPanel>, 3> steamPanels;
    std::array<std::unique_ptr<PipeResonatorPanel>, 3> pipePanels;

    // Global SPACE panel
    std::unique_ptr<SpaceControlsPanel> spacePanel;

    void timerCallback() override;
    void updateModelBanner();
};

} // namespace aeriform
