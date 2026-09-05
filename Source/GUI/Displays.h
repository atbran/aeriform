#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Plugin/PluginProcessor.h"
#include "Theme.h"

namespace aeriform
{
/** Small oscilloscope reading a ScopeBuffer at frame rate, with a level bar on the right. */
class ScopeDisplay : public juce::Component,
                     private juce::Timer
{
public:
    ScopeDisplay (const ScopeBuffer& buffer, const std::atomic<float>* level, juce::Colour colour, juce::String label);
    ~ScopeDisplay() override;

    void paint (juce::Graphics&) override;
    void setActiveLook (bool active);

private:
    const ScopeBuffer& buffer;
    const std::atomic<float>* levelAtomic;
    juce::Colour colour;
    juce::String label;
    std::vector<float> data;
    float level = 0.0f, gainNorm = 1.0f;
    bool active = true;

    void timerCallback() override;
};

/** Wavefolder transfer curve (drawn from the exact fold function) with the live post-fold waveform behind it. */
class FoldCurveDisplay : public juce::Component,
                         private juce::Timer
{
public:
    explicit FoldCurveDisplay (AeriformProcessor&);
    ~FoldCurveDisplay() override;

    void paint (juce::Graphics&) override;

private:
    AeriformProcessor& processor;
    std::vector<float> scope;
    std::atomic<float>* pOn = nullptr; std::atomic<float>* pMode = nullptr; std::atomic<float>* pFold = nullptr;
    std::atomic<float>* pDrive = nullptr; std::atomic<float>* pSym = nullptr; std::atomic<float>* pBias = nullptr;
    std::atomic<float>* pStages = nullptr; std::atomic<float>* pShape = nullptr; std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pComp = nullptr;
    juce::Path curve;
    float lastSignature = -1.0f;

    void timerCallback() override;
};

/** Horizontal energy meter for one resonator slot (running state + loop energy). */
class EnergyBar : public juce::Component,
                  private juce::Timer
{
public:
    EnergyBar (VisualizerModel&, int slot, juce::Colour);
    ~EnergyBar() override;
    void paint (juce::Graphics&) override;

private:
    VisualizerModel& model;
    int slot;
    juce::Colour colour;
    float value = 0.0f;
    bool running = false;
    void timerCallback() override;
};

/**
    Interactive resonator-network diagram: the three resonator nodes with live
    energy, the serial sends and injection of the current routing mode, the six
    cross-feedback routes (thickness / brightness = effective amount including
    the Repipe macro) and the output taps.

    Interaction: drag vertically on a route arrow (or use the mouse wheel) to
    change that route; double-click a node to enable / disable the resonator.
*/
class NetworkDiagram : public juce::Component,
                       public juce::SettableTooltipClient,
                       private juce::Timer
{
public:
    NetworkDiagram (AeriformProcessor&, bool compact);
    ~NetworkDiagram() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    struct Route
    {
        const char* id;
        const char* name;
        int from, to;
        juce::Path path;
        juce::Point<float> mid;
        float value = 0.0f, effective = 0.0f;
    };

    AeriformProcessor& processor;
    bool compact;
    std::array<juce::Point<float>, 3> nodePos;
    float nodeRadius = 22.0f;
    std::array<Route, 6> routes;
    int hoverRoute = -1, hoverNode = -1, dragRoute = -1;
    int dragStartY = 0;
    float dragStartValue = 0.0f;
    juce::RangedAudioParameter* dragParam = nullptr;

    // live state
    float energy[3] = { 0.0f, 0.0f, 0.0f };
    bool running[3] = { true, false, false };
    float netEnergy = 0.0f, governor = 1.0f;

    std::atomic<float>* raw (const char* id) const;
    float value (const char* id) const;
    void rebuildGeometry();
    int routeAt (juce::Point<float>) const;
    int nodeAt (juce::Point<float>) const;
    void timerCallback() override;
    void setRouteValue (int route, float v);
    static const char* enableId (int node);
};
} // namespace aeriform
