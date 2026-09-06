#include "RoomPage.h"
namespace aeriform {
using namespace theme;
RoomPage::RoomPage(AeriformProcessor& p):processor(p) { controls=add<Controls>(p);startTimerHz(20);addAndMakeVisible(audition);audition.setClickingTogglesState(true);
    audition.setTooltip("Hear only this module's actual return before global effects. No extra gain. Click again for the full synth; switching presets or closing the editor ends audition.");
    audition.onClick=[this]{processor.setReturnAudition(audition.getToggleState()?AeriformProcessor::ReturnAudition::Room:AeriformProcessor::ReturnAudition::Off);}; }
RoomPage::Controls::Controls(AeriformProcessor& p):ParamPanel(p,"SHARED ROOM / PHYSICAL RETURN",teal) {
    enabled=control<Toggle>(p,ids::roomOn,"ENABLED");freeze=control<Toggle>(p,ids::roomFreeze,"FREEZE ROOM");
    knobs={knob(ids::roomSize,"Size"),knob(ids::roomShape,"Shape"),knob(ids::roomDiffusion,"Diffusion"),
        knob(ids::roomWallDamping,"Wall damping"),knob(ids::roomAir,"Air absorption"),knob(ids::roomFeedback,"Feedback"),
        knob(ids::roomSend,"Voice send"),knob(ids::roomLevel,"Output level"),knob(ids::roomWidth,"Width"),
        knob(ids::roomNetworkReturn,"Network return"),knob(ids::roomReturnDelay,"Return delay"),knob(ids::roomReturnFilter,"Return filter")};
    addAndMakeVisible(clear);
    clear.onClick=[&p]{p.getPatchTools().setParameter(ids::roomClear,p.getAPVTS().getRawParameterValue(ids::roomClear)->load()>.5f?0:1);};
}
void RoomPage::Controls::resized() {
    auto r=getContentArea().reduced(12,8);auto top=r.removeFromTop(26);
    enabled->setBounds(top.removeFromLeft(top.getWidth()/2));freeze->setBounds(top);r.removeFromTop(12);
    for(int row=0;row<4;++row){knobRow(r.removeFromTop(98),{knobs[(size_t)row*3],knobs[(size_t)row*3+1],knobs[(size_t)row*3+2]},12);r.removeFromTop(8);}
    clear.setBounds(r.removeFromTop(30));
}
void RoomPage::resized(){audition.setBounds(getWidth()/2+24,getHeight()-38,getWidth()/2-48,30);controls->setBounds(0,0,getWidth()/2-6,getHeight());}
void RoomPage::paint(juce::Graphics& g) {
    auto r=getLocalBounds().withTrimmedLeft(getWidth()/2+24).reduced(10,16);
    g.setColour(textPrimary);g.setFont(titleFont(18));g.drawText("A ROOM THAT PLAYS BACK",r.removeFromTop(36),juce::Justification::centredLeft);
    auto graph=r.removeFromTop(220).toFloat().reduced(14,12);
    const float size=processor.getAPVTS().getRawParameterValue(ids::roomSize)->load();
    const float shape=processor.getAPVTS().getRawParameterValue(ids::roomShape)->load();
    auto room=graph.reduced(24*(1-size),20*(1-size));room.setWidth(room.getWidth()*(1-.25f*shape));room.setCentre(graph.getCentre());
    g.setColour(inset);g.fillRoundedRectangle(room,12);g.setColour(brass);g.drawRoundedRectangle(room,12,2);
    const auto& v=processor.getVisualizerModel();const float energy=std::clamp(std::sqrt(std::max(0.0f,v.roomEnergy.load()))*5,0.0f,1.0f);
    juce::Path rays;auto c=room.getCentre();rays.startNewSubPath(c);rays.lineTo(room.getX()+8,room.getY()+28);rays.lineTo(room.getRight()-8,room.getBottom()-35);rays.lineTo(room.getX()+room.getWidth()*.3f,room.getY()+8);rays.lineTo(c);
    g.setColour(teal.withAlpha(.2f+.8f*energy));g.strokePath(rays,juce::PathStrokeType(1+energy*3));g.fillEllipse(c.x-7,c.y-7,14,14);
    g.setColour(textSecondary);g.setFont(font(12));g.drawText(v.roomEnergy.load()<1e-8f?"Room empty / play to excite it":"Shared reflections / live stored energy",r.removeFromTop(28),juce::Justification::centredLeft);
    auto meter=[&](const juce::String& name,float value){auto line=r.removeFromTop(32);g.setColour(textSecondary);g.drawText(name,line.removeFromLeft(130),juce::Justification::centredLeft);auto bar=line.reduced(0,9).toFloat();g.setColour(inset);g.fillRoundedRectangle(bar,3);g.setColour(teal);bar.setWidth(bar.getWidth()*std::clamp(std::sqrt(std::max(0.0f,value))*5,0.0f,1.0f));g.fillRoundedRectangle(bar,3);};
    meter("Input RMS",v.roomInputRms.load());meter("Output RMS",v.roomOutputRms.load());r.removeFromTop(18);
    g.setColour(textSecondary);g.setFont(font(14));g.drawFittedText("Every voice excites the same space. Output level adds the audible room; Network return feeds delayed room energy back into the resonators, with its strength bounded by your played excitation. These two paths work independently.",r.removeFromTop(104),juce::Justification::centredLeft,5,1.0f);
    r.removeFromTop(8);g.drawFittedText("Freeze holds the reflections and stops new excitation. Clear empties the room and its return delay. Size and shape change the reflection paths; damping and air absorption soften the tail.",r.removeFromTop(108),juce::Justification::centredLeft,5,1.0f);
}
}
