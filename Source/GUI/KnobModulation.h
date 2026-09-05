#pragma once
#include "../Plugin/PluginProcessor.h"
namespace aeriform {
/** Message-thread matrix edits shared by the context menu and depth-ring drag. */
struct KnobModulation {
    static float value(AeriformProcessor& p,int slot,ids::ModField field){return p.getAPVTS().getRawParameterValue(ids::id(ids::modP(slot,field)))->load();}
    static int find(AeriformProcessor& p,ModDest dest,ModSource source=ModSource::None){
        for(int i=1;i<=ids::numModSlots;++i)if((int)value(p,i,ids::ModField::Dst)==(int)dest&&value(p,i,ids::ModField::Src)>0&&(source==ModSource::None||(int)value(p,i,ids::ModField::Src)==(int)source))return i;return -1;
    }
    static int empty(AeriformProcessor& p){for(int i=1;i<=ids::numModSlots;++i)if(value(p,i,ids::ModField::Src)==0||value(p,i,ids::ModField::Dst)==0)return i;return -1;}
    static int assign(AeriformProcessor& p,ModDest dest,ModSource source){
        if(dest==ModDest::None||source==ModSource::None)return -1;int slot=find(p,dest,source);if(slot>0)return slot;slot=empty(p);if(slot<0)return -1;
        p.getPatchTools().perform("Assign modulation",[&]{p.getPatchTools().setParameter(ids::id(ids::modP(slot,ids::ModField::Src)),(float)source);p.getPatchTools().setParameter(ids::id(ids::modP(slot,ids::ModField::Dst)),(float)dest);p.getPatchTools().setParameter(ids::id(ids::modP(slot,ids::ModField::Depth)),.25f);});return slot;
    }
    static void remove(AeriformProcessor& p,int slot){if(slot<1||slot>ids::numModSlots)return;p.getPatchTools().perform("Remove modulation",[&]{for(auto f:{ids::ModField::Src,ids::ModField::Dst,ids::ModField::Depth})p.getPatchTools().setParameter(ids::id(ids::modP(slot,f)),0);});}
};
}
