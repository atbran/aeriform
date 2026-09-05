#include "PluginEditor.h"
#include "../GUI/Theme.h"

using namespace aeriform;
using namespace aeriform::theme;

AeriformEditor::AeriformEditor (AeriformProcessor& p)
    : AudioProcessorEditor (p), processor (p), tooltips (this, 650), content (*this),
      presetBar (p), visualizer (p.getVisualizerModel()),
      breath (p), resonator (p), motion (p), space (p), master (p)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("AERIFORM", juce::dontSendNotification);
    titleLabel.setFont (titleFont (22.0f));
    titleLabel.setColour (juce::Label::textColourId, copperBright);
    subtitleLabel.setText ("BREATH-DRIVEN RESONATOR SYNTHESIZER", juce::dontSendNotification);
    subtitleLabel.setFont (font (9.5f, true));
    subtitleLabel.setColour (juce::Label::textColourId, textDim);
    statusLabel.setFont (monoFont (10.5f));
    statusLabel.setColour (juce::Label::textColourId, textSecondary);
    statusLabel.setJustificationType (juce::Justification::centredRight);

    scaleButton.setTooltip ("Interface size");
    scaleButton.onClick = [this] { showScaleMenu(); };

    for (auto* c : std::initializer_list<juce::Component*> { &titleLabel, &subtitleLabel, &statusLabel, &presetBar, &scaleButton,
                                                           &visualizer, &breath, &resonator, &motion, &space, &master })
        content.addAndMakeVisible (c);
    addAndMakeVisible (content);

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
    processor.getPresetManager().onPresetChanged = nullptr;
    setLookAndFeel (nullptr);
}

// ---------------------------------------------------------------------------
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

    // ---- top bar -----------------------------------------------------------
    auto top = r.removeFromTop (44);
    auto titleArea = top.removeFromLeft (250);
    titleLabel.setBounds (titleArea.removeFromTop (26));
    subtitleLabel.setBounds (titleArea);
    scaleButton.setBounds (top.removeFromRight (64).reduced (0, 9));
    top.removeFromRight (8);
    statusLabel.setBounds (top.removeFromRight (190).reduced (0, 8));
    top.removeFromRight (12);
    presetBar.setBounds (top.reduced (0, 7));
    r.removeFromTop (8);

    // ---- bottom row: SPACE + MASTER ------------------------------------------
    auto bottom = r.removeFromBottom (200);
    master.setBounds (bottom.removeFromRight (392));
    bottom.removeFromRight (8);
    space.setBounds (bottom);
    r.removeFromBottom (8);

    // ---- three columns ----------------------------------------------------------
    auto left = r.removeFromLeft (392);
    r.removeFromLeft (8);
    auto right = r.removeFromRight (392);
    r.removeFromRight (8);
    breath.setBounds (left);
    motion.setBounds (right);
    visualizer.setBounds (r.removeFromTop (150));
    r.removeFromTop (8);
    resonator.setBounds (r);
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
    // modulation rings
    auto config = processor.getEngine().getModConfig();
    processor.getVisualizerModel().readLiveMod (liveMod);
    for (auto* panel : std::initializer_list<ParamPanel*> { &breath, &resonator, &motion, &space, &master })
        for (auto& k : panel->getKnobs())
            k->updateModRing (config, liveMod);

    // MIDI learn completion
    if (processor.getMidiLearn().pollLearn())
        repaint();

    // preset name / dirty state
    if (presetDirtyFlag)
    {
        presetDirtyFlag = false;
        presetBar.refresh();
    }

    // status line: voices, CPU, MIDI activity, limiter
    auto& vis = processor.getVisualizerModel();
    const int activity = vis.midiActivity.exchange (0);
    midiActivityCounter = activity > 0 ? 6 : juce::jmax (0, midiActivityCounter - 1);
    const float cpu = processor.getCpuLoad() * 100.0f;
    const float lim = vis.limiterGain.load (std::memory_order_relaxed);
    juce::String status = "MIDI " + juce::String (midiActivityCounter > 0 ? "*" : "-")
                          + "   VOICES " + juce::String (vis.activeVoices.load (std::memory_order_relaxed))
                          + "   CPU " + juce::String (cpu, 1) + " %";
    if (lim < 0.98f) status += "   LIM";
    statusLabel.setText (status, juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, lim < 0.98f ? amber : textSecondary);
}
