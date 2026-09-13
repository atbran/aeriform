#pragma once
#include "PanelBase.h"
#include "../Params/RackParameters.h"
namespace aeriform {
class RackPanel final:public ParamPanel,private juce::Timer {
    int index,lastType=-1; ChoiceBox* type; Toggle* enabled;
    juce::TextButton earlier{"<"},later{">"};
    std::vector<std::pair<int,juce::Component*>> fields;
    std::vector<juce::Component*> shown;
    void timerCallback() override {refresh();}
public:
    std::function<void(int,int)> onMove;
    RackPanel(AeriformProcessor& p,int slot):ParamPanel(p,"SLOT "+juce::String::charToString((juce::juce_wchar)('A'+slot)),theme::copper),index(slot){
        type=control<ChoiceBox>(p,ids::id(rackTypes[slot]),"Effect");type->setCaptionVisible(false);
        enabled=control<Toggle>(p,ids::id(rackEnables[slot]),"Enabled");
        addAndMakeVisible(earlier);addAndMakeVisible(later);
        earlier.setTooltip("Move this instance earlier in the chain");later.setTooltip("Move this instance later in the chain");
        earlier.onClick=[this]{if(onMove)onMove(index,-1);};later.onClick=[this]{if(onMove)onMove(index,1);};
        for(size_t f=0;f<std::size(originalRackFields);++f){
            const auto id=ids::id(rackFields[slot][f]);const auto original=juce::String(ids::id(originalRackFields[f]));
            const int group=original.startsWith("rd_")?1:original.startsWith("sh_")?2:original.startsWith("sf_")?3:4;
            const auto* info=findParamInfo(id);auto label=findParamInfo(ids::id(originalRackFields[f]))->name;
            for(const auto& prefix:{"Resonant Delay ","Delay Resonator ","Delay ","Shimmer ","Spectral ","Saturation "})if(label.startsWith(prefix)){label=label.substring((int)std::strlen(prefix));break;}
            juce::Component* c=nullptr;
            if(originalRackFields[f]==P::sfCapture || originalRackFields[f]==P::sfRelease){
                const bool capture=originalRackFields[f]==P::sfCapture;
                auto* button=control<juce::TextButton>(capture?"Capture":"Release");
                button->setComponentID(id);
                button->onClick=[this,id,capture]{
                    auto& tools=processor.getPatchTools();
                    tools.perform(capture?"Capture spectrum":"Release spectrum",[&]{
                        const auto prefix="rack"+juce::String(index+1)+"_";
                        if(capture)tools.setParameter(ids::id(rackEnables[index]),1);
                        tools.setParameter(prefix+"sf_freeze",capture?1.0f:0.0f);
                        tools.setParameter(id,processor.getAPVTS().getRawParameterValue(id)->load()>.5f?0.0f:1.0f);
                    });
                };
                button->setTooltip(capture?"Capture a fresh spectrum and enable Hold":"Release Hold and return to live audio");
                c=button;
            }
            else if(info->isBool)c=control<Toggle>(p,id,label);
            else if(info->isChoice)c=control<ChoiceBox>(p,id,label);
            else {const auto dest=advancedDestination(rackFields[slot][f]);c=knob(id,label,{dest,Kind::Additive,info->maxValue-info->minValue},46);}
            fields.emplace_back(group,c);
        }
        refresh();startTimerHz(20);
    }
    ~RackPanel() override {stopTimer();}
    void setPosition(int pos){earlier.setEnabled(pos>0);later.setEnabled(pos<3);}
    void refresh(){int t=(int)processor.getAPVTS().getRawParameterValue(ids::id(rackTypes[index]))->load();if(t==lastType)return;lastType=t;shown.clear();for(auto [group,c]:fields){c->setVisible(group==t);if(group==t)shown.push_back(c);}resized();repaint();}
    void resized() override {
        auto r=getContentArea();auto head=r.removeFromTop(28);
        later.setBounds(head.removeFromRight(28));head.removeFromRight(4);earlier.setBounds(head.removeFromRight(28));head.removeFromRight(8);
        enabled->setBounds(head.removeFromRight(86));head.removeFromRight(8);type->setBounds(head);
        r.removeFromTop(8);const int cols=4;const int rows=((int)shown.size()+cols-1)/cols;const int h=std::min(70,rows>0?r.getHeight()/rows:70);
        for(int row=0;row<rows;++row){auto line=r.removeFromTop(h);int width=line.getWidth()/cols;for(int col=0;col<cols;++col){int f=row*cols+col;if(f<(int)shown.size()){auto bounds=line.removeFromLeft(width).reduced(3,1);if(dynamic_cast<juce::TextButton*>(shown[(size_t)f]))bounds=bounds.withSizeKeepingCentre(bounds.getWidth(),28);shown[(size_t)f]->setBounds(bounds);}}}
    }
    void paint(juce::Graphics& g) override {ParamPanel::paint(g);if(lastType==0){g.setColour(theme::textDim);g.setFont(theme::font(13));g.drawText("Choose an effect for this slot",getContentArea().withTrimmedTop(50),juce::Justification::centred);}}
};
}
