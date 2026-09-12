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
void FiltersPage::Block::resized()
{
    auto r=getContentArea();enabled->setBounds(r.removeFromTop(24));
    auto boxes=r.removeFromTop(42);position->setBounds(boxes.removeFromLeft((boxes.getWidth()-6)/2));boxes.removeFromLeft(6);model->setBounds(boxes);
    r.removeFromTop(6);knobRow(r.removeFromTop(80),{cutoff,resonance,drive},4);
    knobRow(r.removeFromTop(80),{keytrack,envelope,morph},4);
    auto bottom=r.removeFromTop(92);mix->setBounds(bottom.removeFromLeft(100));bottom.removeFromLeft(8);
    slope->setBounds(bottom.removeFromTop(40));bottom.removeFromTop(6);vowel->setBounds(bottom.removeFromTop(40));
}
void FiltersPage::Block::timerCallback(){int type=(int)processor.getAPVTS().getRawParameterValue(ids::id(ModularFilters::parameter(slot,2)))->load();morph->setVisible(type==(int)FilterModel::SVFMorph||type==(int)FilterModel::DrivenSVF||type==(int)FilterModel::Comb||type==(int)FilterModel::Tilt);slope->setVisible(type!=(int)FilterModel::Comb&&type!=(int)FilterModel::Formant&&type!=(int)FilterModel::Modal);vowel->setVisible(type==(int)FilterModel::Formant);}
void FiltersPage::resized(){auto r=getLocalBounds();int w=(r.getWidth()-16)/3;for(auto* b:blocks){b->setBounds(r.removeFromLeft(w));r.removeFromLeft(8);}}
void FiltersPage::paint(juce::Graphics&){}
void FiltersPage::mouseDown(const juce::MouseEvent&){}
void FiltersPage::mouseUp(const juce::MouseEvent&){}
}
