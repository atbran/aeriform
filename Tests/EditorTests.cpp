#include "TestFramework.h"
#include "TestHelpers.h"
#include "Plugin/PluginEditor.h"
#include "GUI/GuiDiagnostics.h"
#include "GUI/WorkspacePage.h"
#include "GUI/EffectsWorkspace.h"
#include <set>
#include <cstdlib>

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

    for (int page = 0; page < 6; ++page)
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

namespace
{
    template <typename T>
    std::vector<T*> descendants (juce::Component& root, bool visibleOnly = false)
    {
        std::vector<T*> result;
        for (auto* child : root.getChildren())
        {
            if (visibleOnly && ! child->isVisible()) continue;
            if (auto* typed = dynamic_cast<T*> (child)) result.push_back (typed);
            auto nested = descendants<T> (*child, visibleOnly);
            result.insert (result.end(), nested.begin(), nested.end());
        }
        return result;
    }

    void checkControlBounds (juce::Component& root)
    {
        for (auto* c : descendants<juce::Component> (root, true))
        {
            if (c->getComponentID().isEmpty()) continue;
            auto* parent = c->getParentComponent();
            for (; parent != nullptr && parent != &root; parent = parent->getParentComponent())
            {
                if (dynamic_cast<juce::Viewport*> (parent) != nullptr || dynamic_cast<juce::Viewport*> (parent->getParentComponent()) != nullptr) break;
                const auto bounds = parent->getLocalArea (c, c->getLocalBounds());
                CHECK_MSG (! bounds.isEmpty() && parent->getLocalBounds().contains (bounds),
                           "clipped " + c->getComponentID().toStdString() + " at " + bounds.toString().toStdString()
                           + " in " + parent->getLocalBounds().toString().toStdString());
                if (dynamic_cast<Page*> (parent) != nullptr) break;
            }
        }
    }

    void captureUi (juce::Component& root, const char* name)
    {
        if (const char* folder = std::getenv ("AERIFORM_CAPTURE_DIR"))
        {
            const auto file = juce::File (juce::String::fromUTF8 (folder)).getChildFile (juce::String(name) + ".png");
            file.getParentDirectory().createDirectory();
            auto stream = file.createOutputStream();
            CHECK (stream != nullptr);
            if (stream != nullptr)
            {
                stream->setPosition (0); stream->truncate();
                CHECK (juce::PNGImageFormat().writeImageToStream (root.createComponentSnapshot (root.getLocalBounds()), *stream));
            }
        }
    }
}

AERIFORM_TEST (editor_overhaul_controls_tabs_and_layout)
{
    TestHost h;
    std::unique_ptr<juce::AudioProcessorEditor> ed (h.processor.createEditor());
    auto* editor = dynamic_cast<AeriformEditor*> (ed.get());
    CHECK (editor != nullptr); if (editor == nullptr) return;
    editor->showPage (0);
    auto mains = descendants<MainPage> (*editor);
    CHECK (mains.size() == 1); if (mains.empty()) return;
    auto& main = *mains.front();
    CHECK (descendants<ModMatrixPanel> (main).empty());
    auto mainDiagrams = descendants<NetworkDiagram> (main, true);
    CHECK (mainDiagrams.size() == 1);
    for (auto* diagram : mainDiagrams)
    {
        CHECK (diagram->getWidth() >= 180);
        CHECK (diagram->getHeight() >= 150);
        CHECK (diagram->getParentComponent()->getLocalBounds().contains (diagram->getBounds()));
    }
    auto tabs = descendants<PageTabs> (main);
    CHECK (tabs.size() == 2);
    auto resonators = descendants<ResonatorPanel> (main);
    CHECK (resonators.size() == 3);
    for (auto* resonator : resonators)
    {
        CHECK (resonator->getKnobs().size() == 11);
        for (const auto& knob : resonator->getKnobs()) CHECK (! knob->getParamID().contains ("body"));
    }
    for (int selected = 0; selected < 3; ++selected)
    {
        for (auto* tab : tabs) tab->setSelected (selected);
        checkControlBounds (main);
        const auto visible = descendants<ResonatorPanel> (main, true);
        CHECK (visible.size() == 1);
        if (! visible.empty())
        {
            const auto& knobs = visible.front()->getKnobs();
            const juce::String prefix = selected == 0 ? "res_" : selected == 1 ? "rb_" : "rc_";
            for (const auto& knob : knobs) CHECK (knob->getParamID().startsWith (prefix));
        }
    }
    for (auto* tab : tabs) tab->setSelected (0);
    for (auto* button : descendants<juce::TextButton> (main))
        if (button->getButtonText() == "CHORUS" || button->getButtonText() == "DELAY" || button->getButtonText() == "REVERB")
        {
            button->onClick(); checkControlBounds (main);
        }
    captureUi (*editor, "main");
    for (auto* tab : tabs) tab->setSelected (1);
    captureUi (*editor, "main-res-b-scope");
    for (auto* tab : tabs) tab->setSelected (2);
    captureUi (*editor, "main-res-c-spectrum");

    editor->showPage (3);
    auto matrices = descendants<ModMatrixPanel> (*editor, true);
    CHECK (matrices.size() == 1);
    if (! matrices.empty())
    {
        CHECK (descendants<ChoiceBox> (*matrices.front()).size() == 2 * ids::numModSlots);
        CHECK (descendants<HSlider> (*matrices.front()).size() == ids::numModSlots);
    }
    for (int page = 0; page < 6; ++page)
    {
        editor->showPage (page);
        for (auto* workspace : descendants<WorkspacePage> (*editor, true))
        {
            workspace->showSection (0); checkControlBounds (*workspace);
            if (page == 2)
            {
                auto diagrams = descendants<NetworkDiagram> (*workspace, true);
                CHECK (diagrams.size() == 1);
                for (auto* diagram : diagrams)
                {
                    CHECK (diagram->getWidth() >= 380);
                    CHECK (diagram->getHeight() >= 270);
                    CHECK (diagram->getParentComponent()->getLocalBounds().contains (diagram->getBounds()));
                }
            }
            captureUi (*editor, page == 2 ? "network" : "effects");
            workspace->showSection (1); checkControlBounds (*workspace);
            captureUi (*editor, page == 2 ? "network-advanced" : "acoustic");
        }
        checkControlBounds (*editor);
    }
    for (auto* collection : descendants<EffectsCollection> (*editor))
        captureUi (*collection, "effects-all");
    editor->showPage (3); captureUi (*editor, "mod-matrix");
    editor->showPage (1); captureUi (*editor, "exciters");
    editor->showPage (5); captureUi (*editor, "advanced");

    // The moved body controls and all acoustic parameter knobs remain bound.
    std::set<juce::String> idsInUi;
    for (auto* c : descendants<juce::Component> (*editor))
        if (c->getComponentID().isNotEmpty()) idsInUi.insert (c->getComponentID());
    for (const char* id : { ids::resBodyFreq, ids::resBodyRes, ids::resBodyMix, ids::resBodyTrack,
                           ids::contactGap, ids::contactAmount, ids::stereoMode, ids::stereoWidth,
                           ids::symSend, ids::symReturn, ids::roomNetworkReturn, ids::roomReturnDelay,
                           ids::chorusMix, ids::delayMix, ids::reverbMix }) CHECK (idsInUi.count (id) == 1);
    CHECK (gui::unboundControlCount() == 0);
}

AERIFORM_TEST (editor_network_diagrams_toggle_resonators)
{
    TestHost h;
    std::unique_ptr<juce::AudioProcessorEditor> ed (h.processor.createEditor());
    auto* editor = dynamic_cast<AeriformEditor*> (ed.get());
    CHECK (editor != nullptr); if (editor == nullptr) return;
    for (int page : { 0, 2 })
    {
        editor->showPage (page);
        for (auto* workspace : descendants<WorkspacePage> (*editor, true)) workspace->showSection (0);
        auto diagrams = descendants<NetworkDiagram> (*editor, true);
        CHECK (diagrams.size() == 1);
        for (auto* diagram : diagrams)
        {
            // A sits at the left of the graph, midway between input and output.
            const juce::Point<float> position (diagram->getWidth() * 0.25f, diagram->getHeight() * 0.5f);
            const auto now = juce::Time::getCurrentTime();
            const juce::MouseEvent event (juce::Desktop::getInstance().getMainMouseSource(), position,
                juce::ModifierKeys::leftButtonModifier, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                diagram, diagram, now, position, now, 2, false);
            auto* enabled = h.processor.getAPVTS().getParameter (ids::resOn);
            CHECK (enabled != nullptr); if (enabled == nullptr) continue;
            const float original = enabled->getValue();
            diagram->mouseDoubleClick (event);
            CHECK (enabled->getValue() == 1.0f - original);
            diagram->mouseDoubleClick (event);
            CHECK (enabled->getValue() == original);
        }
    }
}

AERIFORM_TEST (editor_spectrum_receives_full_rate_final_output)
{
    for (double rate : { 44100.0, 48000.0, 96000.0 })
    {
        TestHost h (rate, 256);
        h.noteOn (60);
        CHECK (h.render (0.1).finite);
        std::array<float, 256> samples {};
        h.processor.getVisualizerModel().spectrumScope.read (samples.data(), 256);
        CHECK (h.processor.getVisualizerModel().sampleRate.load() == (float)rate);
        for (int i = 0; i < 256; ++i)
            CHECK (samples[(size_t)i] == 0.5f * (h.buffer.getSample(0,i) + h.buffer.getSample(1,i)));
    }
}

AERIFORM_TEST (editor_old_workspace_locations_migrate_without_changing_patch)
{
    TestHost original;
    original.set (ids::roomReturnDelay, 47.0f);
    original.set (ids::chorusMix, 0.23f);
    for (int oldSection = 1; oldSection <= 3; ++oldSection)
    {
        auto state = original.processor.createStateXml();
        state->removeAttribute ("editorLayoutVersion");
        state->setAttribute ("editorPage", 2);
        state->setAttribute ("editorSection2", oldSection);
        TestHost restored;
        restored.processor.applyStateXml (*state);
        CHECK (restored.processor.getEditorPage() == 4);
        CHECK (restored.processor.getEditorSection (4) == 1);
        CHECK (restored.processor.getEditorSection (2) == 0);
        CHECK_NEAR (restored.get (ids::roomReturnDelay), 47.0f, 0.001f);
        CHECK_NEAR (restored.get (ids::chorusMix), 0.23f, 0.0001f);
    }
    for (int oldSection = 0; oldSection < 6; ++oldSection)
    {
        auto state = original.processor.createStateXml();
        state->removeAttribute ("editorLayoutVersion");
        state->setAttribute ("editorPage", 4);
        state->setAttribute ("editorSection4", oldSection);
        TestHost restored;
        restored.processor.applyStateXml (*state);
        CHECK (restored.processor.getEditorPage() == 4);
        CHECK (restored.processor.getEditorSection (4) == 0);
    }
    original.processor.setEditorPage (2);
    original.processor.setEditorSection (2, 1);
    TestHost restored;
    restored.processor.applyStateXml (*original.processor.createStateXml());
    CHECK (restored.processor.getEditorPage() == 2);
    CHECK (restored.processor.getEditorSection (2) == 1);
}

AERIFORM_TEST (editor_analyzers_observe_deep_morph_and_bypass)
{
    TestHost h;
    h.processor.getPatchTools().capture (0);
    h.set (ids::resCoarse, 12);
    h.processor.getPatchTools().capture (1);
    h.set (ids::morphOn, 1);
    h.set (ids::morphMode, 1);
    h.set (ids::morphPosition, 0.5f);
    h.noteOn (60);
    CHECK (h.render (0.15).finite);
    std::array<float, 256> samples {};
    auto& model = h.processor.getVisualizerModel();
    model.spectrumScope.read (samples.data(), 256);
    for (int i = 0; i < 256; ++i)
        CHECK_NEAR (samples[(size_t)i], 0.5f * (h.buffer.getSample(0,i)+h.buffer.getSample(1,i)), 0.0f);
    for (int block = 0; block < 8; ++block) h.processor.processBlockBypassed (h.buffer, h.midi);
    model.spectrumScope.read (samples.data(), 256);
    for (float sample : samples) CHECK_NEAR (sample, 0.0f, 0.0f);
    CHECK_NEAR (model.masterPeak.load(), 0.0f, 0.0f);
}
