#include "PluginEditor.h"
#include "../GUI/Theme.h"
#include <cstdlib>
#include "../GUI/PerformancePage.h"
#include "../GUI/FiltersPage.h"
#include "../GUI/ContactPage.h"
#include "../GUI/SympatheticPage.h"
#include "../GUI/RoomPage.h"
#include "../GUI/ResonantDelayPage.h"
#include "../GUI/ShimmerPage.h"
#include "../GUI/SpectralPage.h"
#include "../GUI/SaturationPage.h"
#include "../GUI/WorkspacePage.h"

using namespace aeriform;
using namespace aeriform::theme;

AeriformEditor::AeriformEditor (AeriformProcessor& p)
    : AudioProcessorEditor (p), processor (p), tooltips (this, 650), content (*this),
      presetBar (p), tabs ({ "MAIN", "EXCITERS", "NETWORK", "MOTION", "SPACE", "ADVANCED" })
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("EXP_Aeriform", juce::dontSendNotification);
    titleLabel.setFont (titleFont (22.0f));
    titleLabel.setColour (juce::Label::textColourId, copperBright);
    subtitleLabel.setText ("COMPLEX EXCITER / RESONATOR NETWORK SYNTHESIZER", juce::dontSendNotification);
    subtitleLabel.setFont (font (9.5f, true));
    subtitleLabel.setColour (juce::Label::textColourId, textDim);
    statusLabel.setFont (monoFont (10.5f));
    statusLabel.setColour (juce::Label::textColourId, textSecondary);
    statusLabel.setJustificationType (juce::Justification::centredRight);

    scaleButton.setTooltip ("Interface size");
    scaleButton.onClick = [this] { showScaleMenu(); };

    pages[0] = std::make_unique<MainPage> (p);
    pages[1] = std::make_unique<ExcitersPage> (p);
    auto network=std::make_unique<WorkspacePage>(p,2);network->addSection("RESONATORS / ROUTING",std::make_unique<NetworkPage>(p));network->addSection("CONTACT / STEREO",std::make_unique<ContactPage>(p));network->addSection("SYMPATHETIC BANK",std::make_unique<SympatheticPage>(p));network->addSection("COUPLED ROOM",std::make_unique<RoomPage>(p));network->showSection(p.getEditorSection(2));pages[2]=std::move(network);
    pages[3] = std::make_unique<MotionPage> (p);
    auto space=std::make_unique<WorkspacePage>(p,4);space->addSection("EFFECTS",std::make_unique<SpacePage>(p));space->addSection("MODULAR FILTERS",std::make_unique<FiltersPage>(p));space->addSection("RESONANT DELAY",std::make_unique<ResonantDelayPage>(p));space->addSection("SHIMMER",std::make_unique<ShimmerPage>(p));space->addSection("SPECTRAL FREEZE",std::make_unique<SpectralPage>(p));space->addSection("SATURATION",std::make_unique<SaturationPage>(p));space->showSection(p.getEditorSection(4));pages[4]=std::move(space);
    pages[5] = std::make_unique<PerformancePage>(p);
    undoButton.onClick=[this]{processor.getPatchTools().undo.undo();};
    redoButton.onClick=[this]{processor.getPatchTools().undo.redo();};
    content.addAndMakeVisible(undoButton);content.addAndMakeVisible(redoButton);
    setWantsKeyboardFocus(true);

    for (auto* c : std::initializer_list<juce::Component*> { &titleLabel, &subtitleLabel, &statusLabel, &presetBar, &scaleButton, &tabs })
        content.addAndMakeVisible (c);
    for (auto& page : pages)
    {
        content.addChildComponent (*page);
        page->setVisible (false);
    }
    addAndMakeVisible (content);

    tabs.onChange = [this] (int index) { showPage (index); };
    currentPage = -1;
    int initialPage = processor.getEditorPage();
    if (const char* env = std::getenv ("AERIFORM_PAGE")) initialPage = std::atoi (env);   // development hook (screenshots)
    showPage (juce::jlimit (0, 7, initialPage));

    processor.getPresetManager().onPresetChanged = [this] { presetDirtyFlag = true; };

    // read the stored scale before the constrainer triggers resized(), which writes the scale back
    const float storedScale = processor.getEditorScale();
    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) editorWidth / (double) editorHeight);
    setResizeLimits ((int) (editorWidth * 0.6f), (int) (editorHeight * 0.6f), editorWidth * 2, editorHeight * 2);

    applyScale (storedScale);
    startTimerHz (30);
}

AeriformEditor::~AeriformEditor()
{
    stopTimer();
    processor.setReturnAudition(AeriformProcessor::ReturnAudition::Off);
    processor.getPresetManager().onPresetChanged = nullptr;
    setLookAndFeel (nullptr);
}

// ---------------------------------------------------------------------------
void AeriformEditor::showPage (int index)
{
    // Preserve the saved locations from early experimental eight-tab builds.
    if(index==6){dynamic_cast<WorkspacePage*>(pages[4].get())->showSection(1);index=4;}
    if(index==7){dynamic_cast<WorkspacePage*>(pages[2].get())->showSection(1);index=2;}
    index = juce::jlimit (0, (int) pages.size() - 1, index);
    if (index == currentPage) return;
    currentPage = index;
    for (int i = 0; i < (int) pages.size(); ++i)
        pages[(size_t) i]->setVisible (i == index);
    tabs.setSelected (index);
    processor.setEditorPage (index);
}

void AeriformEditor::applyScale (float newScale)
{
    scale = juce::jlimit (0.5f, 2.0f, newScale);
    processor.setEditorScale (scale);
    scaleButton.setButtonText (juce::String (juce::roundToInt (scale * 100.0f)) + " %");
    setSize (juce::roundToInt (editorWidth * scale), juce::roundToInt (editorHeight * scale));
}

void AeriformEditor::showScaleMenu()
{
    juce::PopupMenu menu;
    for (int pct : { 75, 100, 125, 150, 200 })
        menu.addItem (pct, juce::String (pct) + " %", true, juce::roundToInt (scale * 100.0f) == pct);
    juce::Component::SafePointer<AeriformEditor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&scaleButton), [safe] (int r)
    {
        if (safe != nullptr && r > 0) safe->applyScale ((float) r / 100.0f);
    });
}

void AeriformEditor::resized()
{
    // uniform scaling of the logical layout; keeps the aspect ratio through the constrainer
    const float s = (float) getWidth() / (float) editorWidth;
    if (std::fabs (s - scale) > 0.001f)
    {
        scale = s;
        processor.setEditorScale (scale);
        scaleButton.setButtonText (juce::String (juce::roundToInt (scale * 100.0f)) + " %");
    }
    content.setTransform (juce::AffineTransform::scale (scale));
    content.setBounds (0, 0, editorWidth, editorHeight);
    layoutContent();
}

void AeriformEditor::layoutContent()
{
    auto r = juce::Rectangle<int> (0, 0, editorWidth, editorHeight).reduced (10);

    // ---- top bar: row 1 = title, page tabs, status, size; row 2 = subtitle + the full-width preset browser
    auto top = r.removeFromTop (56);
    auto row1 = top.removeFromTop (28);
    titleLabel.setBounds (row1.removeFromLeft (210));
    scaleButton.setBounds (row1.removeFromRight (64).reduced (0, 1));
    row1.removeFromRight (8);
    statusLabel.setBounds (row1.removeFromRight (200));
    row1.removeFromRight (12);
    tabs.setBounds (row1.removeFromRight (600).reduced (0, 1));
    top.removeFromTop (2);
    auto row2 = top.removeFromTop (26);
    subtitleLabel.setVisible(false);
    undoButton.setBounds(row2.removeFromLeft(58));row2.removeFromLeft(4);redoButton.setBounds(row2.removeFromLeft(58));row2.removeFromLeft(12);
    presetBar.setBounds (row2.reduced (0, 1));
    r.removeFromTop (6);

    for (auto& page : pages) page->setBounds (r);
}

void AeriformEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);
}

void AeriformEditor::Content::paint (juce::Graphics& g)
{
    g.fillAll (background);
    // faint radial vignette for depth
    juce::ColourGradient v (juce::Colours::white.withAlpha (0.025f), (float) getWidth() * 0.5f, 0.0f,
                            juce::Colours::transparentBlack, (float) getWidth() * 0.5f, (float) getHeight(), false);
    g.setGradientFill (v);
    g.fillRect (getLocalBounds());
}

void AeriformEditor::Content::resized() {}

// ---------------------------------------------------------------------------
void AeriformEditor::timerCallback()
{
    undoButton.setEnabled(processor.getPatchTools().undo.canUndo());redoButton.setEnabled(processor.getPatchTools().undo.canRedo());
    // modulation rings of the visible page
    auto config = processor.getEngine().getModConfig();
    processor.getVisualizerModel().readLiveMod (liveMod);
    if (currentPage >= 0)
        for (auto* panel : pages[(size_t) currentPage]->getPanels())
            for (auto& k : panel->getKnobs())
                k->updateModRing (config, liveMod);

    // MIDI learn completion
    if (processor.getMidiLearn().pollLearn())
        repaint();

    processor.getPresetManager().pollChanges();

    // preset name / dirty state
    if (presetDirtyFlag)
    {
        presetDirtyFlag = false;
        presetBar.refresh();
    }

    // status line: voices, CPU, MIDI activity, limiter, governor
    auto& vis = processor.getVisualizerModel();
    const int activity = vis.midiActivity.exchange (0);
    midiActivityCounter = activity > 0 ? 6 : juce::jmax (0, midiActivityCounter - 1);
    const float cpu = processor.getCpuLoad() * 100.0f;
    const float lim = vis.limiterGain.load (std::memory_order_relaxed);
    const float gov = vis.governorGain.load (std::memory_order_relaxed);
    juce::String status = "MIDI " + juce::String (midiActivityCounter > 0 ? "*" : "-")
                          + "  VOICES " + juce::String (vis.activeVoices.load (std::memory_order_relaxed))
                          + "  CPU " + juce::String (cpu, 1) + " %";
    if (gov < 0.98f) status += "  GOV";
    if (lim < 0.98f) status += "  LIM";
    statusLabel.setText (status, juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, (lim < 0.98f || gov < 0.98f) ? amber : textSecondary);
}

bool AeriformEditor::keyPressed(const juce::KeyPress& key) {
    if(!key.getModifiers().isCtrlDown())return false;
    const int code=key.getKeyCode();if(code=='Z'||code=='z'){if(key.getModifiers().isShiftDown())processor.getPatchTools().undo.redo();else processor.getPatchTools().undo.undo();return true;}
    if(code=='Y'||code=='y'){processor.getPatchTools().undo.redo();return true;}return false;
}
