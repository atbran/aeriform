#include "TestFramework.h"
#include "TestHelpers.h"
#include "Plugin/PluginEditor.h"
#include "GUI/GuiDiagnostics.h"

using namespace aeriform;
using namespace aeriform::test;

namespace
{
    void paintEditor (juce::AudioProcessorEditor& ed)
    {
        juce::Image img (juce::Image::ARGB, juce::jmax (1, ed.getWidth()), juce::jmax (1, ed.getHeight()), true);
        juce::Graphics g (img);
        ed.paintEntireComponent (g, true);
    }
}

AERIFORM_TEST (editor_opens_paints_every_page_and_binds_every_control)
{
    TestHost h;
    gui::unboundControlCount() = 0;
    std::unique_ptr<juce::AudioProcessorEditor> ed (h.processor.createEditor());
    CHECK (ed != nullptr);
    auto* editor = dynamic_cast<AeriformEditor*> (ed.get());
    CHECK (editor != nullptr);
    if (editor == nullptr) return;
    CHECK_MSG (gui::unboundControlCount() == 0, "controls bound to missing parameters: " + std::to_string (gui::unboundControlCount().load()));

    for (int page = 0; page < 5; ++page)
    {
        editor->showPage (page);
        CHECK (editor->getCurrentPage() == page);
        paintEditor (*ed);
    }
    // scaling
    for (float s : { 0.75f, 1.5f, 1.0f })
    {
        ed->setSize (juce::roundToInt (theme::editorWidth * s), juce::roundToInt (theme::editorHeight * s));
        paintEditor (*ed);
    }
    CHECK (h.processor.getEditorScale() > 0.9f && h.processor.getEditorScale() < 1.1f);
}

AERIFORM_TEST (editor_follows_exciter_model_and_interaction_mode_changes)
{
    TestHost h;
    std::unique_ptr<juce::AudioProcessorEditor> ed (h.processor.createEditor());
    auto* editor = dynamic_cast<AeriformEditor*> (ed.get());
    CHECK (editor != nullptr);
    if (editor == nullptr) return;
    editor->showPage (1);
    h.noteOn (57);
    for (int m = 0; m < (int) ExciterModel::Count; ++m)
    {
        h.set (ids::exaModel, (float) m);                                       // synchronous on the message thread
        h.set (ids::exbModel, (float) ((m * 7 + 3) % (int) ExciterModel::Count));
        h.set (ids::mixMode, (float) (m % (int) InteractionMode::Count));
        paintEditor (*ed);
        CHECK (h.render (0.02).finite);
    }
    editor->showPage (2);
    for (int t = 0; t < (int) ResMode::Count; ++t)
    {
        h.set (ids::resMode, (float) t);
        h.set (ids::rbType, (float) ((t + 4) % (int) ResMode::Count));
        h.set (ids::netMode, (float) (t % (int) NetMode::Count));
        h.set (ids::netRepipe, t % 2 == 0 ? 1.0f : 0.0f);
        paintEditor (*ed);
        CHECK (h.render (0.02).finite);
    }
}

AERIFORM_TEST (editor_open_close_cycles_while_playing)
{
    TestHost h;
    h.noteOn (60);
    h.render (0.1);
    for (int i = 0; i < 4; ++i)
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (h.processor.createEditor());
        auto* editor = dynamic_cast<AeriformEditor*> (ed.get());
        if (editor != nullptr) editor->showPage (i % 5);
        paintEditor (*ed);
        CHECK (h.render (0.05).finite);
        ed.reset();
        CHECK (h.render (0.05).finite);
    }
    // the page survives a state round trip
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (h.processor.createEditor());
        dynamic_cast<AeriformEditor*> (ed.get())->showPage (3);
    }
    juce::MemoryBlock state;
    h.processor.getStateInformation (state);
    TestHost b;
    b.processor.setStateInformation (state.getData(), (int) state.getSize());
    CHECK (b.processor.getEditorPage() == 3);
}
