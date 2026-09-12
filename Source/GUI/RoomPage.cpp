#include "RoomPage.h"
namespace aeriform {
using namespace theme;
RoomPage::RoomPage(AeriformProcessor& p):processor(p) { controls=add<Controls>(p);startTimerHz(20);addAndMakeVisible(audition);audition.setClickingTogglesState(true);
    audition.setTooltip("Hear only this module's actual return before global effects. No extra gain. Click again for the full synth; switching presets or closing the editor ends audition.");
    audition.onClick=[this]{processor.setReturnAudition(audition.getToggleState()?AeriformProcessor::ReturnAudition::Room:AeriformProcessor::ReturnAudition::Off);}; }
RoomPage::Controls::Controls(AeriformProcessor& p):ParamPanel(p,"COUPLED ROOM",teal) {
    enabled=control<Toggle>(p,ids::roomOn,"ENABLED");freeze=control<Toggle>(p,ids::roomFreeze,"FREEZE ROOM");
    knobs={knob(ids::roomSize,"Size"),knob(ids::roomShape,"Shape"),knob(ids::roomDiffusion,"Diffusion"),
        knob(ids::roomWallDamping,"Wall damping"),knob(ids::roomAir,"Air absorption"),knob(ids::roomFeedback,"Feedback"),
        knob(ids::roomSend,"Voice send"),knob(ids::roomLevel,"Output level"),knob(ids::roomWidth,"Width"),
        knob(ids::roomNetworkReturn,"Network return"),knob(ids::roomReturnDelay,"Return delay"),knob(ids::roomReturnFilter,"Return filter")};
    addAndMakeVisible(clear);
    clear.onClick=[&p]{p.getPatchTools().setParameter(ids::roomClear,p.getAPVTS().getRawParameterValue(ids::roomClear)->load()>.5f?0:1);};
}
void RoomPage::Controls::resized()
{
    auto r=getContentArea().reduced(4,2);auto top=r.removeFromTop(26);
    enabled->setBounds(top.removeFromLeft(top.getWidth()/2));freeze->setBounds(top);r.removeFromTop(8);
    for(int row=0;row<4;++row)knobRow(r.removeFromTop(82),{knobs[(size_t)row*3],knobs[(size_t)row*3+1],knobs[(size_t)row*3+2]},6);
    clear.setBounds(r.removeFromTop(28));
}
void RoomPage::resized()
{
    controls->setBounds(0,0,getWidth(),432);
    audition.setBounds(8,getHeight()-34,getWidth()-16,28);
}
void RoomPage::paint(juce::Graphics& g)
{
    auto r=getLocalBounds().withTrimmedTop(444).reduced(12,8);
    const auto& v=processor.getVisualizerModel();
    g.setColour(textSecondary);g.setFont(font(11));
    g.drawText(v.roomEnergy.load()<1e-8f?"ROOM EMPTY":"STORED ROOM ENERGY",r.removeFromTop(26),juce::Justification::centredLeft);
    auto meter=[&](const juce::String& name,float value){auto line=r.removeFromTop(36);g.setColour(textSecondary);g.drawText(name,line.removeFromLeft(90),juce::Justification::centredLeft);auto bar=line.reduced(0,12).toFloat();g.setColour(inset);g.fillRoundedRectangle(bar,3);g.setColour(teal);bar.setWidth(bar.getWidth()*std::clamp((juce::Decibels::gainToDecibels(std::max(0.0f,value),-60.0f)+60)/60,0.0f,1.0f));g.fillRoundedRectangle(bar,3);};
    meter("Input",v.roomInputRms.load());meter("Return",v.roomOutputRms.load());
}
}
