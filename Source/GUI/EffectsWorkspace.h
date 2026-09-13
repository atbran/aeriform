#pragma once
#include "Pages.h"
#include "ContactPage.h"
#include "SympatheticPage.h"
#include "RoomPage.h"
#include "FiltersPage.h"
#include "RackPanel.h"
namespace aeriform {
class EffectsPage final:public Page,private juce::Timer {
    AeriformProcessor& processor;std::array<RackPanel*,4> slots;int lastOrder=-1;
    void timerCallback() override {int order=(int)processor.getAPVTS().getRawParameterValue(ids::rackOrder)->load();if(order!=lastOrder){lastOrder=order;resized();}}
public:
    explicit EffectsPage(AeriformProcessor& p):processor(p){for(int i=0;i<4;++i){slots[(size_t)i]=add<RackPanel>(p,i);slots[(size_t)i]->onMove=[this](int index,int delta){auto order=rackPermutation((int)processor.getAPVTS().getRawParameterValue(ids::rackOrder)->load());int pos=(int)(std::find(order.begin(),order.end(),index)-order.begin());if(pos+delta<0||pos+delta>3)return;std::swap(order[(size_t)pos],order[(size_t)(pos+delta)]);processor.getPatchTools().setParameter(ids::rackOrder,(float)rackOrderCode(order));timerCallback();};}startTimerHz(20);}
    ~EffectsPage() override {stopTimer();}
    void resized() override {auto r=getLocalBounds();const int w=(r.getWidth()-12)/2,h=(r.getHeight()-12)/2;auto order=rackPermutation((int)processor.getAPVTS().getRawParameterValue(ids::rackOrder)->load());for(int i=0;i<4;++i){auto* panel=slots[(size_t)order[(size_t)i]];panel->setBounds((i%2)*(w+12),(i/2)*(h+12),w,h);panel->setPosition(i);}}
};
/** Three acoustic modules together, with every existing parameter retained. */
class AcousticPage final : public Page
{
public:
    explicit AcousticPage (AeriformProcessor& p)
    {
        contact = add<ContactPage> (p); bank = add<SympatheticPage> (p); room = add<RoomPage> (p);
    }
    void resized() override
    {
        auto r = getLocalBounds();
        const int w = (r.getWidth() - 24) / 3;
        contact->setBounds (r.removeFromLeft (w)); r.removeFromLeft (12);
        bank->setBounds (r.removeFromLeft (w)); r.removeFromLeft (12);
        room->setBounds (r);
    }
    std::vector<ParamPanel*> getPanels() override
    {
        std::vector<ParamPanel*> result;
        for (auto* page : { contact, bank, room })
        {
            auto list = page->getPanels(); result.insert (result.end(), list.begin(), list.end());
        }
        return result;
    }
private:
    Page *contact, *bank, *room;
};
} // namespace aeriform
