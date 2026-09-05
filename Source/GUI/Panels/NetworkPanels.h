#pragma once

#include "../PanelBase.h"
#include "../Displays.h"

namespace aeriform
{
/** One resonator slot (A, B or C): model, tuning, loop, character, levels. A also carries the v0.1 body EQ. */
class ResonatorSlotPanel : public ParamPanel
{
public:
    ResonatorSlotPanel (AeriformProcessor&, int slot);
    void resized() override;

private:
    int slot;
    Toggle* on;
    ChoiceBox* type;
    std::unique_ptr<EnergyBar> energy;
    juce::Label *tuneCaption, *loopCaption, *charCaption, *bodyCaption = nullptr;
    std::vector<juce::Component*> tuneRow, loopRow, charRow, bodyRow;
};

/** Full network controls: routing, injection, taps, mix, Repipe, cross routes, sends, feedback path, energy loop. */
class NetworkControlsPanel : public ParamPanel
{
public:
    explicit NetworkControlsPanel (AeriformProcessor&);
    void resized() override;

private:
    ChoiceBox *routing, *inject, *tap, *polarity;
    Knob *repipe, *feedback, *damping, *width, *mix, *fbDelay, *fbFilter, *fbDrive;
    juce::Label *routesCaption, *loopCaption;
    Knob *ab, *ba, *bc, *cb, *ca, *ac, *sendAB, *sendBC, *injectB, *injectC;
    Toggle *loopOn,*bypass;
    ChoiceBox *loopSource, *loopDest, *loopPolarity;
    Knob *loopAmount, *loopFilter, *loopDelay, *loopSat;
};

/** MAIN-page overview: compact diagram, routing, Repipe and the essential network knobs. */
class NetworkOverviewPanel : public ParamPanel
{
public:
    explicit NetworkOverviewPanel (AeriformProcessor&);
    void resized() override;

private:
    std::unique_ptr<NetworkDiagram> diagram;
    ChoiceBox *routing, *tap;
    Toggle *rbOn, *rcOn, *loopOn;
    Knob *repipe, *feedback, *damping, *width, *mix, *loopAmount;
};
} // namespace aeriform
