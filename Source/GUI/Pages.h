#pragma once

#include "PanelBase.h"
#include "Visualizer.h"
#include "Panels/ExcitersOverviewPanel.h"
#include "Panels/ExciterSlotPanel.h"
#include "Panels/ShapingPanels.h"
#include "Panels/ResonatorPanel.h"
#include "Panels/NetworkPanels.h"
#include "Panels/MotionPanel.h"
#include "Panels/SpacePanel.h"
#include "Panels/MasterPanel.h"

namespace aeriform
{
/** Tab strip selecting the editor page. */
class PageTabs : public juce::Component
{
public:
    explicit PageTabs (juce::StringArray names);
    void setSelected (int index);
    int getSelected() const noexcept { return selected; }
    std::function<void (int)> onChange;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    juce::StringArray names;
    int selected = 0, hover = -1;
    int tabWidth() const { return getWidth() / juce::jmax (1, names.size()); }
};

/** One editor page: owns its panels and exposes them for modulation-ring updates. */
class Page : public juce::Component
{
public:
    ~Page() override = default;
    virtual std::vector<ParamPanel*> getPanels()
    {
        std::vector<ParamPanel*> out;
        for (auto* p : panels) p->collectPanels (out);
        return out;
    }

protected:
    template <typename T, typename... Args>
    T* add (Args&&... args)
    {
        auto c = std::make_unique<T> (std::forward<Args> (args)...);
        T* raw = c.get();
        addAndMakeVisible (*raw);
        if (auto* pp = dynamic_cast<ParamPanel*> (raw)) panels.push_back (pp);
        owned.push_back (std::move (c));
        return raw;
    }
    std::vector<ParamPanel*> panels;
    std::vector<std::unique_ptr<juce::Component>> owned;
};

/** MAIN: the playing page. Exciters overview, airflow visualiser, resonator A, motion, network overview, master. */
class MainPage : public Page
{
public:
    explicit MainPage (AeriformProcessor&);
    void resized() override;
private:
    Visualizer* visualizer;
    ExcitersOverviewPanel* exciters;
    ResonatorPanel* resonator;
    MotionPanel* motion;
    NetworkOverviewPanel* network;
    MasterPanel* master;
};

/** EXCITERS: both slots in full, interaction, pre-shaper, envelope / articulation, wavefolder. */
class ExcitersPage : public Page
{
public:
    explicit ExcitersPage (AeriformProcessor&);
    void resized() override;
private:
    class EnvelopePanel : public ParamPanel
    {
    public:
        explicit EnvelopePanel (AeriformProcessor&);
        void resized() override;
    private:
        juce::Label *envCaption, *artCaption;
        Knob *attack, *decay, *sustain, *release, *velPress, *pressBright;
        Knob *flowPitch, *instability, *variation, *coupling;
    };
    ExciterSlotPanel *slotA, *slotB;
    InteractionPanel* interaction;
    PreShaperPanel* preShaper;
    EnvelopePanel* envelope;
    WavefolderPanel* folder;
};

/** NETWORK: the interactive diagram, network controls and the three resonator slots. */
class NetworkPage : public Page
{
public:
    explicit NetworkPage (AeriformProcessor&);
    void resized() override;
private:
    class DiagramPanel : public SectionPanel
    {
    public:
        explicit DiagramPanel (AeriformProcessor& p) : SectionPanel ("RESONATOR NETWORK", theme::brass), diagram (p, false) { addAndMakeVisible (diagram); }
        void resized() override { diagram.setBounds (getContentArea()); }
    private:
        NetworkDiagram diagram;
    };
    DiagramPanel* diagram;
    NetworkControlsPanel* controls;
    ResonatorSlotPanel *resA, *resB, *resC;
};

/** MOTION: LFOs, modulation envelope and all 16 matrix slots. */
class MotionPage : public Page
{
public:
    explicit MotionPage (AeriformProcessor&);
    void resized() override;
private:
    MotionPanel* motion;
};

/** SPACE: effects, master duplicate and the signal-flow legend. */
class SpacePage : public Page
{
public:
    explicit SpacePage (AeriformProcessor&);
    void resized() override;
private:
    class FlowPanel : public SectionPanel, private juce::Timer
    {
    public:
        explicit FlowPanel (AeriformProcessor& p);
        ~FlowPanel() override { stopTimer(); }
        void paint (juce::Graphics&) override;
    private:
        AeriformProcessor& processor;
        void timerCallback() override { repaint(); }
    };
    class MidiPanel : public SectionPanel, private juce::Timer
    {
    public:
        explicit MidiPanel (AeriformProcessor& p);
        ~MidiPanel() override { stopTimer(); }
        void paint (juce::Graphics&) override;
        void resized() override;
    private:
        AeriformProcessor& processor;
        juce::TextButton clearButton { "CLEAR ALL" };
        void timerCallback() override { repaint(); }
    };
    SpacePanel* space;
    MasterPanel* master;
    FlowPanel* flow;
    MidiPanel* midi;
};
} // namespace aeriform
