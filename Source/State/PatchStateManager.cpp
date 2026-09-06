#include "PatchStateManager.h"
#include "../Plugin/PluginProcessor.h"
#include <cstring>
namespace aeriform {
namespace {
class StateAction final : public juce::UndoableAction {
public:
    StateAction(AeriformProcessor& p,std::unique_ptr<juce::XmlElement> a,std::unique_ptr<juce::XmlElement> b):processor(p),oldState(std::move(a)),newState(std::move(b)){}
    bool perform() override { if(first) first=false; else processor.applyStateXml(*newState); return true; }
    bool undo() override { processor.applyStateXml(*oldState); return true; }
    int getSizeInUnits() override { return kNumParams*4; }
private:
    AeriformProcessor& processor;
    std::unique_ptr<juce::XmlElement> oldState,newState;
    bool first=true;
};
class FavoriteAction final:public juce::UndoableAction {
public:
    FavoriteAction(PresetManager& p,juce::String i,bool old):pm(p),id(std::move(i)),was(old){}
    bool perform() override{return pm.setFavorite(id,!was);}
    bool undo() override{return pm.setFavorite(id,was);}
private:PresetManager& pm;juce::String id;bool was;
};
bool starts(const char* id,const char* prefix) {return std::strncmp(id,prefix,std::strlen(prefix))==0;}
float uniform(uint32_t& x) {x^=x<<13;x^=x>>17;x^=x<<5;return (float)(x>>8)/16777216.0f;}
}
PatchStateManager::PatchStateManager(AeriformProcessor& p):processor(p) {
    undo.setMaxNumberOfStoredUnits(kNumParams*4*100,20);
    for(int i=0;i<kNumParams;++i) {
        raw[(size_t)i]=p.getAPVTS().getRawParameterValue(ids::all[i]);
        parameters[(size_t)i]=p.getAPVTS().getParameter(ids::all[i]);
        locks.set((size_t)i,protectedRandom((P)i));
    }
    writeSnapshot(0,current());writeSnapshot(1,current());
    audioCache[0]=readSnapshot(0);audioCache[1]=readSnapshot(1);
}
int PatchStateManager::indexOf(const juce::String& id) const { for(int i=0;i<kNumParams;++i) if(id==ids::all[i])return i;return -1; }
bool PatchStateManager::administrative(P p) noexcept {
    const char* id=ids::id(p);
    return p==P::sfCapture||p==P::sfRelease||p==P::roomClear||p==P::symClear||p==P::symCapture||starts(id,"morph_")||starts(id,"random_")||starts(id,"out_")||starts(id,"voice_")||starts(id,"mpe_")||std::strcmp(id,"quality")==0||p==P::bendRange;
}
bool PatchStateManager::protectedRandom(P p) noexcept {
    const char* id=ids::id(p);
    return administrative(p)||p==P::limiterOn||std::strstr(id,"sc_")!=nullptr||p==P::excExternalIn||p==P::exaLevel||p==P::exbLevel||p==P::envSustain||p==P::resOn||p==P::netMix;
}
PatchStateManager::Values PatchStateManager::current() const {
    Values v{}; for(size_t i=0;i<v.size();++i)v[i]=raw[i]->load(std::memory_order_relaxed);return v;
}
void PatchStateManager::writeSnapshot(int slot,const Values& v) {
    const auto s=(size_t)std::clamp(slot,0,1);revisions[s].fetch_add(1,std::memory_order_acq_rel);
    for(size_t i=0;i<v.size();++i)snapshots[s][i].store(v[i],std::memory_order_relaxed);
    revisions[s].fetch_add(1,std::memory_order_release);
}
PatchStateManager::Values PatchStateManager::readSnapshot(int slot) const {
    Values v{};for(size_t i=0;i<v.size();++i)v[i]=snapshots[(size_t)std::clamp(slot,0,1)][i].load();return v;
}
void PatchStateManager::apply(const Values& v) {
    for(size_t i=0;i<v.size();++i)if(!administrative((P)i)&&v[i]!=raw[i]->load(std::memory_order_relaxed))parameters[i]->setValueNotifyingHost(parameters[i]->convertTo0to1(v[i]));
}
void PatchStateManager::begin(const juce::String& name) {
    if(restoring)return;
    if(transactionDepth++==0) {transactionName=name;before=processor.createStateXml();}
}
void PatchStateManager::end() {
    if(restoring||transactionDepth<=0)return;
    if(--transactionDepth==0&&before) {
        auto after=processor.createStateXml();
        if(!before->isEquivalentTo(after.get(),false)) {undo.beginNewTransaction(transactionName);undo.perform(new StateAction(processor,std::move(before),std::move(after)));}
        before.reset();
    }
}
void PatchStateManager::perform(const juce::String& name,const std::function<void()>& action){begin(name);action();end();}
void PatchStateManager::setParameter(const juce::String& id,float value) {
    const int i=indexOf(id);if(i<0)return;
    perform("Edit "+id,[&]{auto* p=parameters[(size_t)i];p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(value));p->endChangeGesture();});
}
void PatchStateManager::capture(int slot) {
    slot=std::clamp(slot,0,1);perform("Capture snapshot",[&]{writeSnapshot(slot,current());names[(size_t)slot]=processor.getPresetManager().getCurrentName();});
}
bool PatchStateManager::loadSnapshot(int slot,int presetIndex) {
    auto& pm=processor.getPresetManager();if(presetIndex<0||presetIndex>=(int)pm.getEntries().size())return false;
    slot=std::clamp(slot,0,1);const auto entry=pm.getEntries()[(size_t)presetIndex];Values v{};
    for(size_t i=0;i<v.size();++i)v[i]=paramDef((P)i).defaultValue;
    if(entry.isFactory) {
        for(const auto& pair:factoryPresets()[(size_t)entry.factoryIndex].values){const int i=indexOf(pair.first);if(i>=0)v[(size_t)i]=pair.second;}
    } else {
        auto xml=juce::XmlDocument::parse(entry.file);if(!xml||!xml->hasTagName("AeriformPreset"))return false;
        if(auto* params=xml->getChildByName("Parameters"))for(auto* item:params->getChildWithTagNameIterator("Param")) {
            const int i=indexOf(item->getStringAttribute("id"));if(i<0)continue;
            const float x=(float)item->getDoubleAttribute("value",v[(size_t)i]);
            if(std::isfinite(x))v[(size_t)i]=parameters[(size_t)i]->convertFrom0to1(parameters[(size_t)i]->convertTo0to1(x));
        }
    }
    perform("Load snapshot",[&]{writeSnapshot(slot,v);names[(size_t)slot]=entry.name;if(slot==selected.load())apply(v);});return true;
}
void PatchStateManager::selectEndpoint(int slot) {
    slot=std::clamp(slot,0,1);if(slot==selected.load())return;
    perform("Select endpoint",[&]{writeSnapshot(selected.load(),current());selected.store(slot);apply(readSnapshot(slot));});
}
float PatchStateManager::interpolate(P p,float a,float b,float t) noexcept {
    const auto& d=paramDef(p);
    if(d.kind!=ParamKind::Float)return t<0.5f?a:b;
    if((d.fmt==Fmt::Hz||d.fmt==Fmt::LfoHz||d.fmt==Fmt::Ms||d.fmt==Fmt::Ratio)&&a>0&&b>0)return std::exp(std::log(a)+(std::log(b)-std::log(a))*t);
    // Mix and pan remain coordinates; existing DSP performs their constant-power mapping.
    return a+(b-a)*t;
}
void PatchStateManager::commitMorph() {
    if(deep()&&raw[(size_t)P::morphPosition]->load()>0.001f&&raw[(size_t)P::morphPosition]->load()<0.999f)return;
    perform("Commit parameter morph",[&]{
        auto live=current();auto a=readSnapshot(0),b=readSnapshot(1);(selected.load()==0?a:b)=live;
        const float t=live[(size_t)P::morphPosition];
        for(size_t i=0;i<live.size();++i)if(!administrative((P)i))live[i]=(paramDef((P)i).kind==ParamKind::Float||deep())?interpolate((P)i,a[i],b[i],t):live[i];
        apply(live);parameters[(size_t)P::morphOn]->setValueNotifyingHost(0);writeSnapshot(selected.load(),live);
    });
}
void PatchStateManager::setSeed(uint32_t s){perform("Random seed",[&]{seed=s;});}
void PatchStateManager::toggleFavorite(const juce::String& id){auto& pm=processor.getPresetManager();undo.beginNewTransaction("Favorite preset");undo.perform(new FavoriteAction(pm,id,pm.isFavorite(id)));}
void PatchStateManager::newSeed(){setSeed((uint32_t)juce::Random::getSystemRandom().nextInt());}
bool PatchStateManager::isLocked(const juce::String& id) const {const int i=indexOf(id);return i<0||locks.test((size_t)i);}
void PatchStateManager::setLocked(const juce::String& id,bool value){const int i=indexOf(id);if(i>=0)perform("Parameter lock",[&]{locks.set((size_t)i,value);});}
void PatchStateManager::lockSection(ParamSection sec,bool value){perform("Section locks",[&]{for(int i=0;i<kNumParams;++i)if(paramDef((P)i).section==sec)locks.set((size_t)i,value);});}
void PatchStateManager::lockAll(bool value){perform(value?"Lock all":"Unlock all",[&]{if(value)locks.set();else locks.reset();});}
void PatchStateManager::randomize(Scope scope,float amount,bool mutation) {
    amount=std::clamp(amount,0.0f,1.0f);if(mutation&&amount==0)return;perform(mutation?"Mutate patch":"Randomize patch",[&]{
        auto values=current();uint32_t rng=seed?seed:0x9e3779b9u;const bool wild=values[(size_t)P::randomWild]>0.5f;
        for(int i=0;i<kNumParams;++i) {
            const auto p=(P)i;const auto& d=paramDef(p);const auto sec=d.section;
            const float u=uniform(rng),v=uniform(rng); // Every ID consumes the same draws, regardless of locks.
            if(locks.test((size_t)i)||administrative(p))continue;
            if(scope==Scope::Exciters&&sec!=ParamSection::Breath&&sec!=ParamSection::Exciters&&sec!=ParamSection::Shaping)continue;
            if(scope==Scope::Network&&sec!=ParamSection::Resonator&&sec!=ParamSection::Network)continue;
            if(scope==Scope::Modulation&&sec!=ParamSection::Motion)continue;
            if(scope==Scope::Effects&&sec!=ParamSection::Space)continue;
            auto* parameter=parameters[(size_t)i];const float base=parameter->convertTo0to1(values[(size_t)i]);
            float target=mutation?std::clamp(base+(u-v)*amount,0.0f,1.0f):0.1f+0.8f*(u+v)*0.5f;
            if(wild&&!mutation)target=u;
            float value=parameter->convertFrom0to1(target);
            if(d.kind==ParamKind::Bool) {if(mutation&&u>amount)continue;value=u>(wild?0.3f:0.7f)?1.0f:0.0f;}
            if((d.kind==ParamKind::Choice||d.kind==ParamKind::Int)&&mutation&&u>amount)continue;
            const char* id=d.id;
            if(!mutation && (std::strstr(id,"feedback")||p==P::loopAmount||starts(id,"net_x")||p==P::netFeedback))value=std::min(value,wild?0.9f:0.7f);
            if(!mutation&&p==P::exaModel)value=std::max(1.0f,value); // Never silence both slots.
            if(!mutation&&p==P::envRelease)value=std::min(value,wild?6000.0f:2000.0f);
            if(!mutation&&p==P::envAttack)value=std::min(value,wild?2000.0f:400.0f);
            values[(size_t)i]=value;
        }
        apply(values);
    });
}
std::unique_ptr<juce::XmlElement> PatchStateManager::toXml() const {
    auto xml=std::make_unique<juce::XmlElement>("PatchTools");xml->setAttribute("version",1);xml->setAttribute("selected",selected.load());xml->setAttribute("seed",juce::String((juce::int64)seed));
    for(int slot=0;slot<2;++slot){auto* snap=xml->createNewChildElement("Snapshot");snap->setAttribute("slot",slot);snap->setAttribute("name",names[(size_t)slot]);const auto values=slot==selected.load()?current():readSnapshot(slot);
        for(int i=0;i<kNumParams;++i){auto* p=snap->createNewChildElement("P");p->setAttribute("id",ids::all[i]);p->setAttribute("value",(double)values[(size_t)i]);}}
    auto* chord=xml->createNewChildElement("SympatheticChord");const auto notes=processor.getCapturedChord();for(int i=0;i<12;++i)chord->setAttribute("note"+juce::String(i),notes[(size_t)i]);
    auto* lock=xml->createNewChildElement("Locks");for(int i=0;i<kNumParams;++i)if(locks.test((size_t)i)){auto* p=lock->createNewChildElement("P");p->setAttribute("id",ids::all[i]);}return xml;
}
void PatchStateManager::fromXml(const juce::XmlElement* xml) {
    restoring=true;
    SynthEngine::CapturedChord chord;chord.fill(-1);if(xml)if(auto* c=xml->getChildByName("SympatheticChord"))for(int i=0;i<12;++i)chord[(size_t)i]=std::clamp(c->getIntAttribute("note"+juce::String(i),-1),-1,127);processor.setCapturedChord(chord);
    if(!xml){selected.store(0);seed=20260905;locks.reset();for(int i=0;i<kNumParams;++i)locks.set((size_t)i,protectedRandom((P)i));writeSnapshot(0,current());writeSnapshot(1,current());names={"Snapshot A","Snapshot B"};}
    else {
        selected.store(std::clamp(xml->getIntAttribute("selected"),0,1));seed=(uint32_t)xml->getStringAttribute("seed","20260905").getLargeIntValue();
        for(auto* snap:xml->getChildWithTagNameIterator("Snapshot")){int slot=snap->getIntAttribute("slot",-1);if(slot<0||slot>1)continue;Values v{};for(int i=0;i<kNumParams;++i)v[(size_t)i]=paramDef((P)i).defaultValue;
            for(auto* p:snap->getChildWithTagNameIterator("P")){int i=indexOf(p->getStringAttribute("id"));if(i<0)continue;float x=(float)p->getDoubleAttribute("value",v[(size_t)i]);if(std::isfinite(x))v[(size_t)i]=parameters[(size_t)i]->convertFrom0to1(parameters[(size_t)i]->convertTo0to1(x));}writeSnapshot(slot,v);names[(size_t)slot]=snap->getStringAttribute("name","Snapshot");}
        if(auto* l=xml->getChildByName("Locks")){locks.reset();for(auto* p:l->getChildWithTagNameIterator("P")){int i=indexOf(p->getStringAttribute("id"));if(i>=0)locks.set((size_t)i);}}
    }
    restoring=false;
}
void PatchStateManager::prepare(double sr){sampleRate=sr;smoothedPosition=raw[(size_t)P::morphPosition]->load();for(int s=0;s<2;++s)audioCache[(size_t)s]=readSnapshot(s);}
bool PatchStateManager::enabled() const noexcept{return raw[(size_t)P::morphOn]->load()>0.5f;}
bool PatchStateManager::deep() const noexcept{return enabled()&&raw[(size_t)P::morphMode]->load()>0.5f;}
void PatchStateManager::evaluate(int samples,Values& a,Values& b) noexcept {
    const auto live=current();const int liveSlot=selected.load(std::memory_order_acquire);
    for(int s=0;s<2;++s){const auto slot=(size_t)s;for(int attempt=0;attempt<2;++attempt){unsigned rev=revisions[slot].load(std::memory_order_acquire);if(rev&1u)continue;Values candidate{};for(size_t i=0;i<candidate.size();++i)candidate[i]=snapshots[slot][i].load(std::memory_order_relaxed);if(rev==revisions[slot].load(std::memory_order_acquire)){audioCache[slot]=candidate;break;}}}
    audioCache[(size_t)liveSlot]=live;
    const float target=std::clamp(live[(size_t)P::morphPosition],0.0f,1.0f);
    smoothedPosition+=(target-smoothedPosition)*(float)(1-std::exp(-samples/(sampleRate*0.035)));
    for(int i=0;i<kNumParams;++i){const auto p=(P)i;const auto k=(size_t)i;
        if(!enabled()||administrative(p)){a[k]=b[k]=live[k];continue;}
        const auto& d=paramDef(p);
        if(d.kind==ParamKind::Float){a[k]=b[k]=interpolate(p,audioCache[0][k],audioCache[1][k],smoothedPosition);}
        else if(deep()){a[k]=audioCache[0][k];b[k]=audioCache[1][k];}
        else a[k]=b[k]=live[k];
    }
}
}
