#include "Pages.h"

namespace aeriform
{
using namespace theme;

// ---------------------------------------------------------------------------
PageTabs::PageTabs (juce::StringArray n) : names (std::move (n)) {}

void PageTabs::setSelected (int index)
{
    index = juce::jlimit (0, names.size() - 1, index);
    if (index == selected) return;
    selected = index;
    repaint();
    if (onChange) onChange (selected);
}

void PageTabs::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    g.setColour (panel);
    g.fillRoundedRectangle (r.toFloat(), 5.0f);
    g.setColour (panelBorder);
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 5.0f, 1.0f);
    const int w = tabWidth();
    for (int i = 0; i < names.size(); ++i)
    {
        auto tab = juce::Rectangle<int> (r.getX() + i * w, r.getY(), w, r.getHeight());
        const bool sel = i == selected;
        if (sel)
        {
            g.setColour (panelRaised);
            g.fillRoundedRectangle (tab.reduced (2).toFloat(), 4.0f);
            g.setColour (copper);
            g.fillRect (tab.getX() + 12, tab.getBottom() - 4, tab.getWidth() - 24, 2);
        }
        else if (i == hover)
        {
            g.setColour (juce::Colours::white.withAlpha (0.03f));
            g.fillRoundedRectangle (tab.reduced (2).toFloat(), 4.0f);
        }
        g.setColour (sel ? textPrimary : textSecondary);
        g.setFont (titleFont (11.5f));
        g.drawText (names[i], tab, juce::Justification::centred);
        if (i > 0)
        {
            g.setColour (panelBorder);
            g.fillRect (tab.getX(), tab.getY() + 6, 1, tab.getHeight() - 12);
        }
    }
}

void PageTabs::mouseDown (const juce::MouseEvent& e) { setSelected (e.x / juce::jmax (1, tabWidth())); }
void PageTabs::mouseMove (const juce::MouseEvent& e)
{
    const int h = e.x / juce::jmax (1, tabWidth());
    if (h != hover) { hover = h; repaint(); }
}
void PageTabs::mouseExit (const juce::MouseEvent&) { hover = -1; repaint(); }

// ---------------------------------------------------------------------------
MainPage::MainPage (AeriformProcessor& p)
{
    visualizer = add<Visualizer> (p.getVisualizerModel());
    visualizerTabs = add<PageTabs> (juce::StringArray { "PIPE", "SCOPE", "SPECTRUM" });
    visualizerTabs->onChange = [this] (int i) { visualizer->setMode (i); };
    exciters = add<ExcitersOverviewPanel> (p);
    resonatorTabs = add<PageTabs> (juce::StringArray { "RES A", "RES B", "RES C" });
    for (int i = 0; i < 3; ++i)
    {
        resonators[(size_t) i] = add<ResonatorPanel> (p, i);
        resonators[(size_t) i]->setVisible (i == 0);
    }
    resonatorTabs->onChange = [this] (int selected)
    {
        for (int i = 0; i < 3; ++i) resonators[(size_t) i]->setVisible (i == selected);
    };
    motion = add<MotionPanel> (p, false);
    effects = add<SpacePanel> (p, false);
    network = add<NetworkOverviewPanel> (p);
    master = add<MasterPanel> (p);
    macros = add<MacroPanel> (p);
}

void MainPage::resized()
{
    auto r = getLocalBounds();
    auto bottom = r.removeFromBottom (200);
    master->setBounds (bottom.removeFromRight (392));
    bottom.removeFromRight (8);
    macros->setBounds (bottom.removeFromRight (150));
    bottom.removeFromRight (8);
    network->setBounds (bottom);
    r.removeFromBottom (8);

    auto left = r.removeFromLeft (392);
    r.removeFromLeft (8);
    auto right = r.removeFromRight (392);
    r.removeFromRight (8);
    exciters->setBounds (left);
    motion->setBounds (right.removeFromTop (340));
    right.removeFromTop (8);
    effects->setBounds (right);
    auto visual = r.removeFromTop (150);
    visualizerTabs->setBounds (visual.removeFromTop (26));
    visual.removeFromTop (4);
    visualizer->setBounds (visual);
    r.removeFromTop (8);
    resonatorTabs->setBounds (r.removeFromTop (28));
    r.removeFromTop (4);
    for (auto* resonator : resonators) resonator->setBounds (r);
}
// ---------------------------------------------------------------------------
ExcitersPage::EnvelopePanel::EnvelopePanel (AeriformProcessor& p) : ParamPanel (p, "BREATH ENVELOPE / ARTICULATION", theme::copper)
{
    using namespace ids;
    const int s = theme::knobSizeSmall;
    envCaption  = caption ("ENVELOPE");
    attack      = knob (envAttack, "Attack", {}, s);
    decay       = knob (envDecay, "Decay", {}, s);
    sustain     = knob (envSustain, "Sustain", {}, s);
    release     = knob (envRelease, "Release", {}, s);
    velPress    = knob (envVelToPressure, "Vel > Press", {}, s);
    pressBright = knob (artPressBright, "P > Bright", {}, s);
    artCaption  = caption ("ARTICULATION");
    flowPitch   = knob (artFlowPitch, "Flow > Pitch", {}, s);
    instability = knob (artInstability, "Instability", {}, s);
    variation   = knob (artVariation, "Variation", {}, s);
    coupling    = knob (artCoupling, "Coupling", {}, s);
}

void ExcitersPage::EnvelopePanel::resized()
{
    auto r = getContentArea();
    envCaption->setBounds (r.removeFromTop (14));
    knobRow (r.removeFromTop (64), { attack, decay, sustain, release, velPress, pressBright });
    r.removeFromTop (4);
    artCaption->setBounds (r.removeFromTop (14));
    knobRow (r.removeFromTop (64), { flowPitch, instability, variation, coupling, nullptr, nullptr });
}

ExcitersPage::ExcitersPage (AeriformProcessor& p)
{
    slotA       = add<ExciterSlotPanel> (p, 0, false);
    slotB       = add<ExciterSlotPanel> (p, 1, false);
    interaction = add<InteractionPanel> (p);
    preShaper   = add<PreShaperPanel> (p);
    envelope    = add<EnvelopePanel> (p);
    folder      = add<WavefolderPanel> (p);
}

void ExcitersPage::resized()
{
    auto r = getLocalBounds();
    auto slots = r.removeFromTop (400);
    slotA->setBounds (slots.removeFromLeft ((slots.getWidth() - 8) / 2));
    slots.removeFromLeft (8);
    slotB->setBounds (slots);
    r.removeFromTop (8);
    auto row = r.removeFromTop (196);
    interaction->setBounds (row.removeFromLeft (350));
    row.removeFromLeft (8);
    envelope->setBounds (row.removeFromRight (360));
    row.removeFromRight (8);
    preShaper->setBounds (row);
    r.removeFromTop (8);
    folder->setBounds (r);
}

// ---------------------------------------------------------------------------
NetworkPage::NetworkPage (AeriformProcessor& p)
{
    diagram  = add<DiagramPanel> (p);
    controls = add<NetworkControlsPanel> (p);
    resA     = add<ResonatorSlotPanel> (p, 0);
    resB     = add<ResonatorSlotPanel> (p, 1);
    resC     = add<ResonatorSlotPanel> (p, 2);
}

void NetworkPage::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (312);
    diagram->setBounds (top.removeFromLeft (400));
    top.removeFromLeft (8);
    controls->setBounds (top);
    r.removeFromTop (8);
    const int colW = (r.getWidth() - 16) / 3;
    resA->setBounds (r.removeFromLeft (colW)); r.removeFromLeft (8);
    resB->setBounds (r.removeFromLeft (colW)); r.removeFromLeft (8);
    resC->setBounds (r);
}

// ---------------------------------------------------------------------------
MotionPage::MotionPage (AeriformProcessor& p) { motion = add<MotionPanel> (p, true); }
void MotionPage::resized() { motion->setBounds (getLocalBounds()); }

// ---------------------------------------------------------------------------
SpacePage::SpacePage (AeriformProcessor& p) { space = add<SpacePanel> (p, true); }
void SpacePage::resized() { space->setBounds (getLocalBounds()); }
} // namespace aeriform
