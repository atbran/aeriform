#include "FiltersPage.h"
namespace aeriform {
using namespace theme;using namespace dsp;
FiltersPage::FiltersPage(AeriformProcessor& p):processor(p){for(int i=0;i<3;++i)blocks[(size_t)i]=add<Block>(p,i);startTimerHz(15);}
FiltersPage::Block::Block(AeriformProcessor& p,int index):ParamPanel(p,"FILTER "+juce::String(index+1),index==0?copper:index==1?teal:brass),slot(index) {
    auto id=[&](int field){return ids::id(ModularFilters::parameter(slot,field));};
    enabled=control<Toggle>(p,id(0),"ENABLED");position=control<ChoiceBox>(p,id(1),"Insert position");model=control<ChoiceBox>(p,id(2),"Filter type");
    cutoff=knob(id(3),"Cutoff",{},70);resonance=knob(id(4),"Resonance",{},70);drive=knob(id(5),"Drive",{},70);keytrack=knob(id(6),"Key track",{},70);
    envelope=knob(id(7),"Envelope",{},70);morph=knob(id(8),"Morph / tilt",{},70);slope=control<ChoiceBox>(p,id(9),"Slope");vowel=control<ChoiceBox>(p,id(10),"Vowel");mix=knob(id(11),"Mix",{},70);
    startTimerHz(10);timerCallback();
}
void FiltersPage::Block::resized(){auto r=getContentArea().reduced(10,6);enabled->setBounds(r.removeFromTop(25));r.removeFromTop(8);position->setBounds(r.removeFromTop(46));r.removeFromTop(8);model->setBounds(r.removeFromTop(46));r.removeFromTop(18);
    knobRow(r.removeFromTop(102),{cutoff,resonance,drive},8);r.removeFromTop(14);knobRow(r.removeFromTop(102),{keytrack,envelope,morph},8);r.removeFromTop(14);
    auto bottom=r.removeFromTop(102);mix->setBounds(bottom.removeFromLeft(bottom.getWidth()/3));bottom.removeFromLeft(10);slope->setBounds(bottom.removeFromTop(42));bottom.removeFromTop(10);vowel->setBounds(bottom.removeFromTop(42));}
void FiltersPage::Block::timerCallback(){int type=(int)processor.getAPVTS().getRawParameterValue(ids::id(ModularFilters::parameter(slot,2)))->load();morph->setVisible(type==(int)FilterModel::SVFMorph||type==(int)FilterModel::DrivenSVF||type==(int)FilterModel::Comb||type==(int)FilterModel::Tilt);slope->setVisible(type!=(int)FilterModel::Comb&&type!=(int)FilterModel::Formant&&type!=(int)FilterModel::Modal);vowel->setVisible(type==(int)FilterModel::Formant);}
void FiltersPage::resized(){auto r=getLocalBounds();r.removeFromTop(176);int w=(r.getWidth()-16)/3;for(auto* b:blocks){b->setBounds(r.removeFromLeft(w));r.removeFromLeft(8);}}
void FiltersPage::paint(juce::Graphics& g){auto r=getLocalBounds().removeFromTop(164);g.setColour(panel);g.fillRoundedRectangle(r.toFloat(),8);g.setColour(textPrimary);g.setFont(titleFont(15));g.drawText("MODULAR SIGNAL PATH",r.removeFromTop(32).reduced(14,0),juce::Justification::centredLeft);
    const char* labels[]={"EXCITERS","FOLDER","RES A","RES B","RES C","BODY","PRE FX","OUTPUT"};auto nodes=juce::Rectangle<int>(14,44,getWidth()-28,40);int w=nodes.getWidth()/8;
    for(int i=0;i<8;++i){auto node=nodes.removeFromLeft(w).reduced(5,0);g.setColour(inset);g.fillRoundedRectangle(node.toFloat(),5);g.setColour(textSecondary);g.setFont(font(11,true));g.drawText(labels[i],node,juce::Justification::centred);}
    for(int i=0;i<3;++i){auto chip=juce::Rectangle<int>(14+i*(getWidth()-28)/3,100,(getWidth()-40)/3,30);auto* value=processor.getAPVTS().getParameter(ids::id(ModularFilters::parameter(i,1)));auto text=value->getText(value->getValue(),80);g.setColour(i==selected?copper.withAlpha(.22f):inset);g.fillRoundedRectangle(chip.toFloat(),5);g.setColour(i==selected?copperBright:textSecondary);g.drawText("F"+juce::String(i+1)+"  /  "+text,chip.reduced(10,0),juce::Justification::centredLeft);}
    g.setColour(textDim);g.setFont(font(11));g.drawText("Select a filter chip, then click a stage above. Use Insert position for all 20 placements, including feedback and resonator loops.",14,139,getWidth()-28,18,juce::Justification::centredLeft);
}
void FiltersPage::mouseDown(const juce::MouseEvent& e){if(e.y>=100&&e.y<130){selected=std::clamp((e.x-14)/std::max(1,(getWidth()-28)/3),0,2);repaint();}}
void FiltersPage::mouseUp(const juce::MouseEvent& e){if(e.y<44||e.y>=84)return;int node=std::clamp((e.x-14)/std::max(1,(getWidth()-28)/8),0,7);const FilterPosition positions[]={FilterPosition::Combined,FilterPosition::AfterFolder,FilterPosition::ResAInput,FilterPosition::ResBInput,FilterPosition::ResCInput,FilterPosition::PostBody,FilterPosition::PreEffects,FilterPosition::PostEffects};processor.getPatchTools().setParameter(ids::id(ModularFilters::parameter(selected,1)),(float)positions[node]);}
}
