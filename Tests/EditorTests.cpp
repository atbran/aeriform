#include "TestFramework.h"
#include "TestHelpers.h"
#include "Plugin/PluginEditor.h"
#include "GUI/GuiDiagnostics.h"
#include "GUI/WorkspacePage.h"
#include "GUI/EffectsWorkspace.h"
#include "GUI/PipePage.h"
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
        CHECK (resonator->getKnobs().size() == 12);
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
    for (auto* collection : descendants<EffectsPage> (*editor))
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

AERIFORM_TEST (preset_manager_categories_and_lookup)
{
    TestHost h;
    auto& pm = h.processor.getPresetManager();
    auto cats = pm.getCategories();
    CHECK (! cats.isEmpty());
    CHECK (cats.contains ("Basses") || cats.contains ("Pads") || cats.contains ("Leads") || cats.contains ("User"));

    // Check sorted and unique
    for (int i = 0; i < cats.size() - 1; ++i)
    {
        CHECK (cats[i] != cats[i + 1]);
        CHECK (cats[i].compareIgnoreCase (cats[i + 1]) <= 0);
    }

    const auto& entries = pm.getEntries();
    CHECK (! entries.empty());
    for (int i = 0; i < (int) entries.size(); ++i)
    {
        CHECK (pm.findEntryIndex (entries[(size_t) i].stableId) == i);
        if (entries[(size_t) i].isFactory)
            CHECK (entries[(size_t) i].author == "AERIFORM");
    }
}

AERIFORM_TEST (preset_browser_overlay_and_interaction)
{
    TestHost h;
    std::unique_ptr<juce::AudioProcessorEditor> ed (h.processor.createEditor());
    auto* editor = dynamic_cast<AeriformEditor*> (ed.get());
    CHECK (editor != nullptr);
    if (editor == nullptr) return;

    auto& browser = editor->getPresetBrowser();
    auto& bar = editor->getPresetBar();
    auto& pm = h.processor.getPresetManager();

    // Initially browser is hidden
    CHECK (! browser.isVisible());

    // Trigger open via onOpenBrowser
    CHECK (bar.onOpenBrowser != nullptr);
    bar.onOpenBrowser();
    CHECK (browser.isVisible());
    captureUi (*editor, "preset-browser");

    const int totalPresets = (int) pm.getEntries().size();
    CHECK (browser.getFilteredCount() == totalPresets);

    // Filter by Category
    auto cats = pm.getCategories();
    if (! cats.isEmpty())
    {
        const auto testCat = cats[0];
        browser.setCategoryFilter (testCat);
        CHECK (browser.getFilteredCount() <= totalPresets);
        CHECK (browser.getFilteredCount() > 0);

        // Reset to All Categories
        browser.setCategoryFilter ("All Categories");
        CHECK (browser.getFilteredCount() == totalPresets);
    }

    // Filter by Search Query
    browser.setSearchQuery ("Init");
    CHECK (browser.getFilteredCount() >= 1);
    CHECK (browser.getFilteredCount() < totalPresets);

    // Search by Author
    browser.setSearchQuery ("AERIFORM");
    CHECK (browser.getFilteredCount() >= 1);
    browser.setSearchQuery ("");
    CHECK (browser.getFilteredCount() == totalPresets);

    // Verify columns count (Star, Name, Category, Author)
    CHECK (browser.getTable().getHeader().getNumColumns (true) == 4);

    // Star / Favorite toggle in browser
    if (totalPresets > 0)
    {
        const auto& firstEntry = pm.getEntries()[0];
        const bool initialFav = pm.isFavorite (firstEntry.stableId);

        // Toggle star on row 0
        browser.toggleStarForRow (0);
        CHECK (pm.isFavorite (firstEntry.stableId) == ! initialFav);

        // Toggle favorites only filter
        browser.getFavToggle().setToggleState (true, juce::sendNotificationSync);
        if (pm.isFavorite (firstEntry.stableId))
        {
            CHECK (browser.getFilteredCount() >= 1);
        }
        browser.getFavToggle().setToggleState (false, juce::sendNotificationSync);
        CHECK (browser.getFilteredCount() == totalPresets);

        // Revert favorite
        browser.toggleStarForRow (0);
        CHECK (pm.isFavorite (firstEntry.stableId) == initialFav);
    }

    // Row selection loads preset
    if (totalPresets > 1)
    {
        browser.selectedRowsChanged (1);
        CHECK (pm.getCurrentIndex() == 1);
    }

    // Sort by Author column (column 4)
    browser.sortOrderChanged (4, true);
    browser.sortOrderChanged (4, false);

    // Escape key closes browser
    juce::KeyPress esc (juce::KeyPress::escapeKey);
    CHECK (editor->keyPressed (esc));
    CHECK (! browser.isVisible());
}

AERIFORM_TEST (nonlinear_phase1_network_controls_and_plot)
{
    TestHost host;
    host.set (ids::resNlOn, 1.0f); host.set (ids::resNlDrive, 75.0f);
    host.noteOn (57); host.render (0.08);
    NonlinearScopeBuffer::Point pairs[512];
    CHECK (host.processor.getVisualizerModel().nonlinearScope[0].drain (pairs, 512) > 0);
    std::unique_ptr<juce::AudioProcessorEditor> editor (host.processor.createEditor());
    auto* ed = dynamic_cast<AeriformEditor*> (editor.get());
    CHECK (ed != nullptr); if (!ed) return;
    ed->showPage (2);
    for (auto* button : descendants<juce::TextButton> (*ed))
        if (button->getButtonText() == "Nonlinearity") button->onClick();
    juce::Thread::sleep (50); juce::Timer::callPendingTimersSynchronously();
    for (int i = 0; i < 8; ++i)
    {
        host.renderBlock();
        juce::Thread::sleep (5); juce::Timer::callPendingTimersSynchronously();
    }
    const auto plots = descendants<NonlinearCurveDisplay> (*ed, true);
    CHECK (plots.size() == 3);
    for (auto* plot : plots) { CHECK (plot->getHeight() > 80); CHECK (plot->getWidth() > 200); }
    checkControlBounds (*ed);
    captureUi (*ed, "nonlinear-phase1-network");
    host.set (ids::resNlModel, 1.0f);
    for (int i = 0; i < 16; ++i)
    {
        host.renderBlock();
        juce::Thread::sleep (5); juce::Timer::callPendingTimersSynchronously();
    }
    bool sawBias = false;
    for (auto* knob : descendants<Knob> (*ed, true))
        if (knob->getParamID() == ids::resNlBias) { sawBias = true; CHECK (!knob->isEnabled()); }
    CHECK (sawBias);
    captureUi (*ed, "nonlinear-phase2-network");
    host.set (ids::resNlModel, 2.0f); host.set (ids::resNlAmount, 100.0f);
    for (int i = 0; i < 16; ++i)
    {
        host.renderBlock();
        juce::Thread::sleep (5); juce::Timer::callPendingTimersSynchronously();
    }
    const float ratio = host.processor.getVisualizerModel().nonlinearTensionRatio[0].load();
    CHECK (ratio >= 1.0f && ratio <= 1.030001f);
    for (auto* knob : descendants<Knob> (*ed, true))
        if (knob->getParamID() == ids::resNlDrive) CHECK (!knob->isEnabled());
    checkControlBounds (*ed);
    captureUi (*ed, "nonlinear-phase3-network");
    host.set (ids::resNlModel, 3.0f);
    for (int i = 0; i < 16; ++i)
    {
        host.renderBlock();
        juce::Thread::sleep (5); juce::Timer::callPendingTimersSynchronously();
    }
    for (auto* knob : descendants<Knob> (*ed, true))
    {
        if (knob->getParamID() == ids::resNlDrive) CHECK (knob->isEnabled());
        if (knob->getParamID() == ids::resNlBias) CHECK (!knob->isEnabled());
    }
    checkControlBounds (*ed);
    captureUi (*ed, "nonlinear-phase4-network");
}

AERIFORM_TEST (pipe_model_ui_parameter_mapping_and_enumeration)
{
    TestHost h;
    gui::unboundControlCount() = 0;
    std::unique_ptr<juce::AudioProcessorEditor> ed (h.processor.createEditor());
    CHECK (ed != nullptr);
    auto* editor = dynamic_cast<AeriformEditor*> (ed.get());
    CHECK (editor != nullptr);
    if (editor == nullptr) return;

    CHECK_MSG (gui::unboundControlCount() == 0, "Unbound controls detected during editor creation");

    const std::vector<const char*> suffixes = {
        "pressure", "dcnoise", "exc_cut", "exc_res", "exc_kt", "exc_vt",
        "rt", "rt_kt", "damp", "lp", "hp", "filt_kt",
        "sat_drive", "sat_knee", "sat_sym", "bore"
    };

    const std::vector<const char*> prefixes = { "res_", "rb_", "rc_" };

    std::set<juce::String> allPipeParamIds;
    for (auto* px : prefixes)
    {
        for (auto* sfx : suffixes)
        {
            juce::String fullId = juce::String (px) + sfx;
            allPipeParamIds.insert (fullId);
            auto* p = h.processor.getAPVTS().getParameter (fullId);
            CHECK_MSG (p != nullptr, "Missing parameter in APVTS: " + fullId.toStdString());
        }
    }
    CHECK (allPipeParamIds.size() == 48);

    editor->showPage (2);
    auto workspaces = descendants<WorkspacePage> (*editor, true);
    CHECK (workspaces.size() == 1);
    if (! workspaces.empty())
    {
        workspaces.front()->showSection (2);
    }

    auto pipePages = descendants<PipePage> (*editor, true);
    CHECK (pipePages.size() == 1);
    if (pipePages.empty()) return;
    auto& pipePage = *pipePages.front();

    for (int slot = 0; slot < 3; ++slot)
    {
        pipePage.selectSlot (slot);
        CHECK (pipePage.getSelectedSlot() == slot);
        paintEditor (*ed);

        const juce::String expectedPx = prefixes[(size_t) slot];
        for (auto* knob : descendants<Knob> (pipePage, true))
        {
            if (allPipeParamIds.count (knob->getParamID()) > 0)
            {
                CHECK (knob->getParamID().startsWith (expectedPx));
            }
        }
    }
    CHECK (gui::unboundControlCount() == 0);
}

AERIFORM_TEST (pipe_model_ui_bidirectional_binding_and_no_local_state)
{
    TestHost h;
    std::unique_ptr<juce::AudioProcessorEditor> ed (h.processor.createEditor());
    auto* editor = dynamic_cast<AeriformEditor*> (ed.get());
    CHECK (editor != nullptr); if (! editor) return;

    editor->showPage (2);
    auto workspaces = descendants<WorkspacePage> (*editor, true);
    if (! workspaces.empty()) workspaces.front()->showSection (2);

    auto pipePages = descendants<PipePage> (*editor, true);
    CHECK (pipePages.size() == 1); if (pipePages.empty()) return;
    auto& pipePage = *pipePages.front();

    // 1. Host API to PIPE
    h.set (ids::resPipePressure, 75.0f);
    h.set (ids::resPipeRt, 3.5f);
    h.set (ids::resPipeSatSym, 40.0f);
    h.set (ids::rbPipePressure, 60.0f);

    paintEditor (*ed);

    for (auto* knob : descendants<Knob> (pipePage))
    {
        if (knob->getParamID() == ids::resPipePressure)
            CHECK_NEAR (h.get (ids::resPipePressure), 75.0f, 0.01f);
        if (knob->getParamID() == ids::resPipeRt)
            CHECK_NEAR (h.get (ids::resPipeRt), 3.5f, 0.01f);
        if (knob->getParamID() == ids::resPipeSatSym)
            CHECK_NEAR (h.get (ids::resPipeSatSym), 40.0f, 0.01f);
        if (knob->getParamID() == ids::rbPipePressure)
            CHECK_NEAR (h.get (ids::rbPipePressure), 60.0f, 0.01f);
    }

    // 2. PIPE to Host and to NETWORK
    h.processor.getPatchTools().setParameter (ids::resPipePressure, 25.0f);
    h.processor.getPatchTools().setParameter (ids::resPipeRt, 1.2f);
    CHECK_NEAR (h.get (ids::resPipePressure), 25.0f, 0.01f);
    CHECK_NEAR (h.get (ids::resPipeRt), 1.2f, 0.01f);

    if (! workspaces.empty()) workspaces.front()->showSection (0);
    paintEditor (*ed);
    CHECK_NEAR (h.get (ids::resPipePressure), 25.0f, 0.01f);

    if (! workspaces.empty()) workspaces.front()->showSection (2);
    paintEditor (*ed);
    CHECK_NEAR (h.get (ids::resPipePressure), 25.0f, 0.01f);
}

AERIFORM_TEST (pipe_model_ui_comprehensibility_when_not_pipe)
{
    TestHost h;
    h.set (ids::resMode, 2.0f); // String

    std::unique_ptr<juce::AudioProcessorEditor> ed (h.processor.createEditor());
    auto* editor = dynamic_cast<AeriformEditor*> (ed.get());
    CHECK (editor != nullptr); if (! editor) return;

    editor->showPage (2);
    auto workspaces = descendants<WorkspacePage> (*editor, true);
    if (! workspaces.empty()) workspaces.front()->showSection (2);

    auto pipePages = descendants<PipePage> (*editor, true);
    CHECK (pipePages.size() == 1); if (pipePages.empty()) return;
    auto& pipePage = *pipePages.front();
    pipePage.selectSlot (0);

    paintEditor (*ed);

    auto labels = descendants<juce::Label> (pipePage);
    bool foundNotice = false;
    for (auto* label : labels)
    {
        if (label->getText().contains ("NOTICE") && label->getText().contains ("PIPE"))
        {
            foundNotice = true;
            break;
        }
    }
    CHECK (foundNotice);

    auto buttons = descendants<juce::TextButton> (pipePage);
    juce::TextButton* setPipeBtn = nullptr;
    for (auto* btn : buttons)
    {
        if (btn->getButtonText() == "SET TO PIPE")
        {
            setPipeBtn = btn;
            break;
        }
    }
    CHECK (setPipeBtn != nullptr);
    if (setPipeBtn != nullptr)
    {
        CHECK (setPipeBtn->isVisible());
        setPipeBtn->onClick();
        CHECK ((int) h.get (ids::resMode) == (int) ResMode::Pipe);

        paintEditor (*ed);
        bool foundActive = false;
        for (auto* label : labels)
        {
            if (label->getText().contains ("ACTIVE") && label->getText().contains ("PIPE"))
            {
                foundActive = true;
                break;
            }
        }
        CHECK (foundActive);
        CHECK (! setPipeBtn->isVisible());
    }
}

AERIFORM_TEST (pipe_model_ui_nominal_decay_and_asymmetry_prominence)
{
    TestHost h;
    std::unique_ptr<juce::AudioProcessorEditor> ed (h.processor.createEditor());
    auto* editor = dynamic_cast<AeriformEditor*> (ed.get());
    CHECK (editor != nullptr); if (! editor) return;

    editor->showPage (2);
    auto workspaces = descendants<WorkspacePage> (*editor, true);
    if (! workspaces.empty()) workspaces.front()->showSection (2);

    auto pipePages = descendants<PipePage> (*editor, true);
    CHECK (pipePages.size() == 1); if (pipePages.empty()) return;
    auto& pipePage = *pipePages.front();

    for (const char* rtId : { ids::resPipeRt, ids::rbPipeRt, ids::rcPipeRt })
    {
        bool foundRt = false;
        for (auto* knob : descendants<Knob> (pipePage))
        {
            if (knob->getParamID() == rtId)
            {
                foundRt = true;
                auto labels = descendants<juce::Label> (*knob);
                bool labelledNominal = false;
                for (auto* l : labels)
                    if (l->getText().contains ("Nominal Decay")) labelledNominal = true;
                CHECK_MSG (labelledNominal, std::string (rtId) + " is not labelled 'Nominal Decay'");
            }
        }
        CHECK (foundRt);
    }

    for (const char* symId : { ids::resPipeSatSym, ids::rbPipeSatSym, ids::rcPipeSatSym })
    {
        bool foundSym = false;
        for (auto* knob : descendants<Knob> (pipePage))
        {
            if (knob->getParamID() == symId)
            {
                foundSym = true;
                CHECK (knob->getWidth() >= theme::knobSizeLarge || knob->getHeight() >= theme::knobSizeLarge || !knob->isVisible());
            }
        }
        CHECK (foundSym);
    }
}

AERIFORM_TEST (pipe_model_ui_scaling_and_bounds_safety)
{
    TestHost h;
    std::unique_ptr<juce::AudioProcessorEditor> ed (h.processor.createEditor());
    auto* editor = dynamic_cast<AeriformEditor*> (ed.get());
    CHECK (editor != nullptr); if (! editor) return;

    editor->showPage (2);
    auto workspaces = descendants<WorkspacePage> (*editor, true);
    if (! workspaces.empty()) workspaces.front()->showSection (2);

    auto pipePages = descendants<PipePage> (*editor, true);
    CHECK (pipePages.size() == 1); if (pipePages.empty()) return;
    auto& pipePage = *pipePages.front();

    for (int slot = 0; slot < 3; ++slot)
    {
        pipePage.selectSlot (slot);

        for (float s : { 0.75f, 1.0f, 1.25f, 1.5f, 2.0f })
        {
            ed->setSize (juce::roundToInt (theme::editorWidth * s), juce::roundToInt (theme::editorHeight * s));
            paintEditor (*ed);
            checkControlBounds (*editor);
        }
    }

    captureUi (*editor, "pipe-model-page-slot-a");
    pipePage.selectSlot (1);
    captureUi (*editor, "pipe-model-page-slot-b");
    pipePage.selectSlot (2);
    captureUi (*editor, "pipe-model-page-slot-c");

    auto& vis = h.processor.getVisualizerModel();
    CHECK (vis.pipeLoopGainDb[0].is_lock_free());
    CHECK (vis.pipePhaseCompSamples[0].is_lock_free());
    CHECK (vis.pipeMeasuredDecaySec[0].is_lock_free());

    vis.pipeLoopGainDb[0].store (-4.25f, std::memory_order_relaxed);
    vis.pipePhaseCompSamples[0].store (1.45f, std::memory_order_relaxed);
    vis.pipeMeasuredDecaySec[0].store (0.85f, std::memory_order_relaxed);
    paintEditor (*ed);
}

