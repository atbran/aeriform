#include "TestFramework.h"
#include "TestHelpers.h"
#include "Plugin/PluginEditor.h"
#include "GUI/GuiDiagnostics.h"
using namespace aeriform;
using namespace aeriform::test;
namespace {
PatchStateManager::Values values(TestHost& h){PatchStateManager::Values v{};for(int i=0;i<kNumParams;++i)v[(size_t)i]=h.get(ids::all[i]);return v;}
void endpoints(TestHost& h){auto& s=h.processor.getPatchTools();h.set(ids::excLowpass,1000);h.set(ids::resMode,0);s.capture(0);s.selectEndpoint(1);h.set(ids::excLowpass,9000);h.set(ids::resMode,2);s.capture(1);s.selectEndpoint(0);}
}
AERIFORM_TEST(patchtools_log_and_pitch_interpolation) {
    CHECK_NEAR(PatchStateManager::interpolate(P::excLowpass,1000,9000,.5f),3000,.01);
    CHECK_NEAR(PatchStateManager::interpolate(P::resFine,-50,50,.75f),25,.001);
    CHECK_NEAR(PatchStateManager::interpolate(P::envAttack,10,1000,.5f),100,.001);
}
AERIFORM_TEST(patchtools_snapshot_serialization_and_effective_values) {
    TestHost h;endpoints(h);auto& s=h.processor.getPatchTools();h.set(ids::morphOn,1);h.set(ids::morphPosition,.5f);
    s.setSeed(1984);s.setLocked(ids::excLowpass,true);
    juce::MemoryBlock data;h.processor.getStateInformation(data);TestHost b;b.processor.setStateInformation(data.getData(),(int)data.getSize());
    auto& t=b.processor.getPatchTools();CHECK(t.getSeed()==1984);CHECK(t.isLocked(ids::excLowpass));CHECK(t.selectedEndpoint()==0);
    PatchStateManager::Values a{},c{};t.evaluate(100000,a,c);
    CHECK_NEAR(a[(size_t)P::excLowpass],3000,1);CHECK_NEAR(a[(size_t)P::resMode],0,.001); // Parameter mode holds structure.
    b.set(ids::morphMode,1);t.evaluate(64,a,c);CHECK_NEAR(a[(size_t)P::resMode],0,.001);CHECK_NEAR(c[(size_t)P::resMode],2,.001);
}
AERIFORM_TEST(patchtools_seed_and_locks_are_reproducible) {
    TestHost h;auto& s=h.processor.getPatchTools();s.setSeed(42);s.setLocked(ids::resDamping,true);
    const float damping=h.get(ids::resDamping),output=h.get(ids::outGain);
    s.randomize(PatchStateManager::Scope::All,1,false);auto first=values(h);s.randomize(PatchStateManager::Scope::All,1,false);auto second=values(h);
    for(size_t i=0;i<first.size();++i)CHECK_NEAR(first[i],second[i],1e-5);
    CHECK_NEAR(h.get(ids::resDamping),damping,1e-6);CHECK_NEAR(h.get(ids::outGain),output,1e-6);
    s.lockAll(true);auto locked=values(h);s.randomize(PatchStateManager::Scope::All,1,false);CHECK(locked==values(h));
}
AERIFORM_TEST(patchtools_mutation_radius_and_undo_grouping) {
    TestHost h;auto& s=h.processor.getPatchTools();s.undo.clearUndoHistory();const auto before=values(h);
    s.randomize(PatchStateManager::Scope::All,.05f,true);auto after=values(h);
    for(int i=0;i<kNumParams;++i)if(paramDef((P)i).kind==ParamKind::Float){auto* p=h.processor.getAPVTS().getParameter(ids::all[i]);CHECK(std::abs(p->convertTo0to1(before[(size_t)i])-p->convertTo0to1(after[(size_t)i]))<=.0501f);}
    CHECK(s.undo.getNumActionsInCurrentTransaction()==1);CHECK(s.undo.undo());auto reverted=values(h);
    for(size_t i=0;i<before.size();++i)CHECK_NEAR(reverted[i],before[i],1e-4);
    CHECK(!s.undo.canUndo());CHECK(s.undo.redo());auto redone=values(h);for(int i=0;i<kNumParams;++i){auto* p=h.processor.getAPVTS().getParameter(ids::all[i]);CHECK_NEAR(p->convertTo0to1(redone[(size_t)i]),p->convertTo0to1(after[(size_t)i]),1e-5);}
    s.randomize(PatchStateManager::Scope::All,0,true);CHECK(values(h)==redone);
}
AERIFORM_TEST(patchtools_one_drag_one_transaction_and_automation_no_history) {
    TestHost h;auto& s=h.processor.getPatchTools();s.undo.clearUndoHistory();float initial=h.get(ids::excNoise);
    s.begin("Edit exc_noise");for(int i=0;i<100;++i)h.set(ids::excNoise,(float)i/100);s.end();
    CHECK(s.undo.getNumActionsInCurrentTransaction()==1);CHECK(s.undo.undo());CHECK_NEAR(h.get(ids::excNoise),initial,1e-5);CHECK(!s.undo.canUndo());
    s.undo.clearUndoHistory();for(int i=0;i<100;++i)h.set(ids::morphPosition,(float)i/100);CHECK(!s.undo.canUndo());
}
AERIFORM_TEST(patchtools_deep_morph_automation_preserves_host_values_and_stays_finite) {
    TestHost h;endpoints(h);auto& s=h.processor.getPatchTools();h.set(ids::morphOn,1);h.set(ids::morphMode,1);h.noteOn(60);
    const float cutoff=h.get(ids::excLowpass);float largestDelta=0,previous=0;
    for(int b=0;b<80;++b){h.set(ids::morphPosition,(float)b/79);auto stats=h.renderBlock();CHECK(stats.finite);CHECK(stats.peak<1.0f);
        for(int i=0;i<256;++i){float x=h.buffer.getSample(0,i);largestDelta=std::max(largestDelta,std::abs(x-previous));previous=x;}}
    CHECK(largestDelta<.8f);CHECK_NEAR(h.get(ids::excLowpass),cutoff,1e-4);CHECK(s.deep());
}
AERIFORM_TEST(patchtools_preset_roundtrip_contains_snapshot_and_locks) {
    TestHost h;endpoints(h);auto& s=h.processor.getPatchTools();s.setSeed(765);auto xml=h.processor.getPresetManager().createPresetXml("Two structures","Experimental");
    CHECK(xml->getChildByName("PatchTools")!=nullptr);s.setSeed(99);CHECK(h.processor.getPresetManager().applyPresetXml(*xml));CHECK(s.getSeed()==765);
}
AERIFORM_TEST(patchtools_favorites_persist_and_keep_missing_identifiers) {
    TestHost h;auto& pm=h.processor.getPresetManager();auto dir=juce::File::getCurrentWorkingDirectory().getChildFile("build/test-favorites");dir.createDirectory();
    const auto file=dir.getChildFile(juce::Uuid().toString()+".xml");pm.setFavoriteStorage(file);
    auto id=pm.getEntries()[1].stableId;CHECK(pm.setFavorite(id,true));CHECK(pm.setFavorite("missing:user:123",true));
    TestHost b;auto& other=b.processor.getPresetManager();other.setFavoriteStorage(file);CHECK(other.isFavorite(id));CHECK(other.isFavorite("missing:user:123"));
    h.processor.getPatchTools().toggleFavorite(id);CHECK(!pm.isFavorite(id));CHECK(h.processor.getPatchTools().undo.undo());CHECK(pm.isFavorite(id));
}
AERIFORM_TEST(patchtools_play_page_paints_and_exports_screenshot) {
    TestHost h;gui::unboundControlCount()=0;std::unique_ptr<juce::AudioProcessorEditor> base(h.processor.createEditor());auto* ed=dynamic_cast<AeriformEditor*>(base.get());CHECK(ed!=nullptr);if(!ed)return;
    ed->showPage(5);CHECK(ed->getCurrentPage()==5);CHECK(gui::unboundControlCount()==0);
    juce::Image image(juce::Image::ARGB,ed->getWidth(),ed->getHeight(),true);juce::Graphics g(image);ed->paintEntireComponent(g,true);
    auto file=juce::File::getCurrentWorkingDirectory().getChildFile("docs/experimental/play.png");file.getParentDirectory().createDirectory();auto stream=file.createOutputStream();CHECK(stream!=nullptr);if(stream){stream->setPosition(0);stream->truncate();CHECK(juce::PNGImageFormat().writeImageToStream(image,*stream));}
}
