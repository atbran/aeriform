#pragma once
#include "Pages.h"
namespace aeriform {
/** A small section strip keeps the six primary workspaces uncluttered. */
class WorkspacePage final:public Page {
public:
    WorkspacePage(AeriformProcessor& p,int workspace):processor(p),workspaceIndex(workspace),selected(p.getEditorSection(workspace)){}
    void addSection(const juce::String& name,std::unique_ptr<Page> page){
        const int index=(int)sections.size();auto button=std::make_unique<juce::TextButton>(name);button->onClick=[this,index]{showSection(index);};addAndMakeVisible(*button);addChildComponent(*page);buttons.push_back(std::move(button));sections.push_back(std::move(page));sections.back()->setVisible(index==selected);buttons.back()->setToggleState(index==selected,juce::dontSendNotification);
    }
    void showSection(int index){selected=std::clamp(index,0,(int)sections.size()-1);for(size_t i=0;i<sections.size();++i){sections[i]->setVisible((int)i==selected);buttons[i]->setToggleState((int)i==selected,juce::dontSendNotification);}processor.setEditorSection(workspaceIndex,selected);}
    void resized() override {auto r=getLocalBounds();auto bar=r.removeFromTop(30);r.removeFromTop(10);int width=std::min(200,bar.getWidth()/std::max(1,(int)buttons.size()));for(auto& b:buttons)b->setBounds(bar.removeFromLeft(width).reduced(2,0));for(auto& s:sections)s->setBounds(r);}
    std::vector<ParamPanel*> getPanels() override {std::vector<ParamPanel*> panels;for(auto& section:sections){auto children=section->getPanels();panels.insert(panels.end(),children.begin(),children.end());}return panels;}
private:
    AeriformProcessor& processor;int workspaceIndex,selected=0;
    std::vector<std::unique_ptr<Page>> sections;
    std::vector<std::unique_ptr<juce::TextButton>> buttons;
};
}
