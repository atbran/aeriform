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
#include "../GUI/EffectsWorkspace.h"

using namespace aeriform;
using namespace aeriform::theme;

AeriformEditor::AeriformEditor (AeriformProcessor& p)
    : AudioProcessorEditor (p), processor (p), tooltips (this, 650), content (*this),
      presetBar (p), presetBrowser (p), tabs ({ "MAIN", "EXCITERS", "NETWORK", "MOD MATRIX", "EFFECTS", "ADVANCED" })
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("AERIFORM", juce::dontSendNotification);
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
    auto network = std::make_unique<WorkspacePage> (p, 2);
    network->addSection ("RESONATORS / ROUTING", std::make_unique<NetworkPage> (p));
    auto stereo = std::make_unique<ContactPage> (p);
    stereo->showStereo (true);
    network->addSection ("ADVANCED / PHYSICAL STEREO", std::move (stereo));
    network->showSection (p.getEditorSection (2) == 1 ? 1 : 0);
    pages[2] = std::move (network);
    pages[3] = std::make_unique<MotionPage> (p);
    auto effects = std::make_unique<WorkspacePage> (p, 4);
    effects->addSection ("EFFECTS", std::make_unique<EffectsPage> (p));
    effects->addSection ("ACOUSTIC", std::make_unique<AcousticPage> (p));
    effects->addSection ("FILTERS", std::make_unique<FiltersPage> (p));
    effects->showSection (juce::jlimit(0,2,p.getEditorSection (4)));
    pages[4] = std::move (effects);
    pages[5] = std::make_unique<PerformancePage>(p);
    undoButton.onClick=[this]{processor.getPatchTools().undo.undo();};
    redoButton.onClick=[this]{processor.getPatchTools().undo.redo();};
    aButton.onClick=[this]{processor.getPatchTools().selectEndpoint(0);};
    bButton.onClick=[this]{processor.getPatchTools().selectEndpoint(1);};
    aButton.setTooltip("Select A snapshot for editing / audition");
    bButton.setTooltip("Select B snapshot for editing / audition");
    content.addAndMakeVisible(undoButton);content.addAndMakeVisible(redoButton);
    content.addAndMakeVisible(aButton);content.addAndMakeVisible(bButton);
    setWantsKeyboardFocus(true);

    for (auto* c : std::initializer_list<juce::Component*> { &titleLabel, &subtitleLabel, &statusLabel, &presetBar, &scaleButton, &tabs })
        content.addAndMakeVisible (c);
    for (auto& page : pages)
    {
        content.addChildComponent (*page);
        page->setVisible (false);
    }
    content.addChildComponent (presetBrowser);
    presetBrowser.setVisible (false);

    presetBar.onOpenBrowser = [this] {
        const bool shouldShow = ! presetBrowser.isVisible();
        presetBrowser.setVisible (shouldShow);
        if (shouldShow)
        {
            presetBrowser.toFront (true);
            presetBrowser.refresh();
        }
    };
    presetBrowser.onClose = [this] { presetBar.refresh(); };
    presetBrowser.onPresetLoaded = [this] { presetBar.refresh(); };
    presetBrowser.onFavoriteToggled = [this] { presetBar.refresh(); };

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
    setResizeLimits (editorWidth, editorHeight, editorWidth * 2, editorHeight * 2);

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
    if(index==6){dynamic_cast<WorkspacePage*>(pages[4].get())->showSection(0);index=4;}
    if(index==7){dynamic_cast<WorkspacePage*>(pages[4].get())->showSection(1);index=4;}
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
    scale = juce::jlimit (1.0f, 2.0f, newScale);
    processor.setEditorScale (scale);
    scaleButton.setButtonText (juce::String (juce::roundToInt (scale * 100.0f)) + " %");
    setSize (juce::roundToInt (editorWidth * scale), juce::roundToInt (editorHeight * scale));
}

void AeriformEditor::showScaleMenu()
{
    juce::PopupMenu menu;
    for (int pct : { 100, 125, 150, 200 })
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
    titleLabel.setBounds (row1.removeFromLeft (180));
    scaleButton.setBounds (row1.removeFromRight (64).reduced (0, 1));
    row1.removeFromRight (8);
    statusLabel.setBounds (row1.removeFromRight (180));
    row1.removeFromRight (12);
    tabs.setBounds (row1.removeFromRight (600).reduced (0, 1));
    top.removeFromTop (2);
    auto row2 = top.removeFromTop (26);
    subtitleLabel.setVisible(false);
    undoButton.setBounds(row2.removeFromLeft(54));row2.removeFromLeft(4);
    redoButton.setBounds(row2.removeFromLeft(54));row2.removeFromLeft(8);
    aButton.setBounds(row2.removeFromLeft(28));row2.removeFromLeft(2);
    bButton.setBounds(row2.removeFromLeft(28));row2.removeFromLeft(10);
    presetBar.setBounds (row2.reduced (0, 1));
    r.removeFromTop (6);

    for (auto& page : pages) page->setBounds (r);
    presetBrowser.setBounds (r);
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
    const int sel = processor.getPatchTools().selectedEndpoint();
    aButton.setColour(juce::TextButton::buttonColourId, sel == 0 ? aeriform::theme::copperBright.withAlpha(0.6f) : aeriform::theme::panel);
    bButton.setColour(juce::TextButton::buttonColourId, sel == 1 ? aeriform::theme::teal.withAlpha(0.6f) : aeriform::theme::panel);
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
        if (presetBrowser.isVisible())
            presetBrowser.refresh();
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
    if (presetBrowser.isVisible() && key.isKeyCode (juce::KeyPress::escapeKey)) {
        presetBrowser.setVisible (false);
        presetBar.refresh();
        return true;
    }
    if(!key.getModifiers().isCtrlDown())return false;
    const int code=key.getKeyCode();if(code=='Z'||code=='z'){if(key.getModifiers().isShiftDown())processor.getPatchTools().undo.redo();else processor.getPatchTools().undo.undo();return true;}
    if(code=='Y'||code=='y'){processor.getPatchTools().undo.redo();return true;}return false;
}
