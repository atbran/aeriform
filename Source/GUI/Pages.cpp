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
    exciters   = add<ExcitersOverviewPanel> (p);
    resonator  = add<ResonatorPanel> (p);
    motion     = add<MotionPanel> (p, false);
    network    = add<NetworkOverviewPanel> (p);
    master     = add<MasterPanel> (p);
}

void MainPage::resized()
{
    auto r = getLocalBounds();
    auto bottom = r.removeFromBottom (200);
    master->setBounds (bottom.removeFromRight (392));
    bottom.removeFromRight (8);
    network->setBounds (bottom);
    r.removeFromBottom (8);

    auto left = r.removeFromLeft (392);
    r.removeFromLeft (8);
    auto right = r.removeFromRight (392);
    r.removeFromRight (8);
    exciters->setBounds (left);
    motion->setBounds (right);
    visualizer->setBounds (r.removeFromTop (150));
    r.removeFromTop (8);
    resonator->setBounds (r);
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
    auto slots = r.removeFromTop (300);
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
    auto top = r.removeFromTop (320);
    diagram->setBounds (top.removeFromLeft (430));
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
SpacePage::FlowPanel::FlowPanel (AeriformProcessor& p) : SectionPanel ("SIGNAL FLOW", theme::copper), processor (p) { startTimerHz (4); }

void SpacePage::FlowPanel::paint (juce::Graphics& g)
{
    SectionPanel::paint (g);
    auto r = getContentArea().toFloat();
    auto& s = processor.getAPVTS();
    auto v = [&] (const char* id) { auto* a = s.getRawParameterValue (id); return a != nullptr ? a->load() : 0.0f; };
    const int q = juce::jlimit (0, 2, (int) v (ids::quality));
    const bool folderOn = v (ids::wfOn) > 0.5f;
    const int os = q == 2 ? 4 : (q == 1 ? 2 : (folderOn ? 2 : 1));
    const int netMode = juce::jlimit (0, (int) NetMode::Count - 1, (int) v (ids::netMode));
    const bool loop = v (ids::loopOn) > 0.5f;

    juce::StringArray stages;
    stages.add ("EXCITER A / B");
    stages.add ("INTERACTION");
    stages.add ("PRE-SHAPER");
    stages.add (folderOn ? "WAVEFOLDER " + juce::String (os) + "x" : "(folder off)");
    stages.add ("NETWORK: " + choices::netModes()[netMode].toUpperCase());
    stages.add ("BODY EQ");
    stages.add ("CHORUS > DELAY > REVERB");
    stages.add ("OUTPUT");

    const float boxH = 26.0f, gap = 8.0f;
    const float boxW = (r.getWidth() - gap * (float) (stages.size() - 1)) / (float) stages.size();
    float x = r.getX();
    const float y = r.getY() + 10.0f;
    for (int i = 0; i < stages.size(); ++i)
    {
        auto box = juce::Rectangle<float> (x, y, boxW, boxH);
        const bool dim = stages[i].startsWith ("(");
        g.setColour (dim ? inset : panelRaised);
        g.fillRoundedRectangle (box, 4.0f);
        g.setColour (i == 3 ? folder : (i == 4 ? brass : (i == 0 ? copper : panelBorder)));
        g.drawRoundedRectangle (box, 4.0f, 1.0f);
        g.setColour (dim ? textDim : textPrimary);
        g.setFont (font (9.5f, true));
        g.drawFittedText (stages[i], box.toNearestInt().reduced (3, 0), juce::Justification::centred, 2);
        if (i + 1 < stages.size())
        {
            g.setColour (textDim);
            g.drawText (">", juce::Rectangle<float> (box.getRight(), y, gap, boxH), juce::Justification::centred);
        }
        x += boxW + gap;
    }
    // energy loop return arrow
    if (loop)
    {
        g.setColour (amber.withAlpha (0.8f));
        const float ly = y + boxH + 14.0f;
        const float fromX = r.getX() + 4.0f * (boxW + gap) + boxW * 0.5f, toX = r.getX() + 1.0f * (boxW + gap) + boxW * 0.5f;
        g.drawLine (fromX, y + boxH, fromX, ly, 1.2f);
        g.drawLine (fromX, ly, toX, ly, 1.2f);
        g.drawLine (toX, ly, toX, y + boxH + 2.0f, 1.2f);
        g.drawText ("ENERGY LOOP RETURN", juce::Rectangle<float> (toX, ly + 2.0f, fromX - toX, 14.0f), juce::Justification::centred);
    }
    g.setColour (textSecondary);
    g.setFont (font (10.0f));
    juce::String info = "Quality: " + choices::qualityModes()[q] + "  -  exciter chain oversampling " + juce::String (os) + "x"
                        + "  -  control rate " + juce::String (q == 0 ? 64 : 32) + " samples";
    g.drawText (info, r.withTrimmedTop (boxH + 44.0f).withHeight (16.0f), juce::Justification::centredLeft);
    g.setColour (textDim);
    g.drawText ("Sidechain: route audio into the plugin's side-chain input and pick the Sidechain model in an exciter slot (or the Breath model's Sidechain knob).",
                r.withTrimmedTop (boxH + 62.0f).withHeight (16.0f), juce::Justification::centredLeft);
}

SpacePage::MidiPanel::MidiPanel (AeriformProcessor& p) : SectionPanel ("MIDI CONTROL", theme::teal), processor (p)
{
    clearButton.setTooltip ("Removes every MIDI CC mapping");
    clearButton.onClick = [this] { processor.getMidiLearn().clearAll(); repaint(); };
    addAndMakeVisible (clearButton);
    startTimerHz (2);
}

void SpacePage::MidiPanel::resized()
{
    clearButton.setBounds (getLocalBounds().removeFromTop (theme::sectionTitleHeight).removeFromRight (100).reduced (6, 2));
}

void SpacePage::MidiPanel::paint (juce::Graphics& g)
{
    SectionPanel::paint (g);
    auto r = getContentArea();
    g.setColour (textDim);
    g.setFont (font (10.0f));
    g.drawText ("Right-click any knob for MIDI Learn. Mappings are saved with the session and with presets exported as files.",
                r.removeFromTop (16), juce::Justification::centredLeft);
    r.removeFromTop (6);
    auto& learn = processor.getMidiLearn();
    juce::StringArray lines;
    for (int cc = 0; cc < MidiLearn::kNumCCs; ++cc)
    {
        const auto id = learn.getMappedParam (cc);
        if (id.isEmpty()) continue;
        const auto* info = findParamInfo (id);
        lines.add ("CC " + juce::String (cc).paddedLeft (' ', 3) + "   " + (info != nullptr ? info->name : id));
    }
    if (learn.isLearning())
        lines.insert (0, "Learning: move a controller for " + (findParamInfo (learn.getLearningParam()) != nullptr ? findParamInfo (learn.getLearningParam())->name : learn.getLearningParam()));
    if (lines.isEmpty())
    {
        g.setColour (textDim);
        g.drawText ("No MIDI mappings yet.", r.removeFromTop (18), juce::Justification::centredLeft);
        return;
    }
    const int rowH = 17, cols = 3;
    const int perCol = juce::jmax (1, r.getHeight() / rowH);
    const int colW = r.getWidth() / cols;
    g.setFont (monoFont (10.5f));
    for (int i = 0; i < lines.size() && i < perCol * cols; ++i)
    {
        const int c = i / perCol, row = i % perCol;
        g.setColour (i == 0 && learn.isLearning() ? amber : textSecondary);
        g.drawText (lines[i], juce::Rectangle<int> (r.getX() + c * colW, r.getY() + row * rowH, colW - 8, rowH), juce::Justification::centredLeft);
    }
}

SpacePage::SpacePage (AeriformProcessor& p)
{
    space  = add<SpacePanel> (p, true);
    master = add<MasterPanel> (p);
    flow   = add<FlowPanel> (p);
    midi   = add<MidiPanel> (p);
}

void SpacePage::resized()
{
    auto r = getLocalBounds();
    space->setBounds (r.removeFromTop (262));
    r.removeFromTop (8);
    auto row = r.removeFromTop (200);
    master->setBounds (row.removeFromRight (392));
    row.removeFromRight (8);
    flow->setBounds (row);
    r.removeFromTop (8);
    midi->setBounds (r);
}
} // namespace aeriform
