#include "SympatheticPage.h"
namespace aeriform {
using namespace theme;
SympatheticPage::SympatheticPage(AeriformProcessor& p):processor(p){controls=add<Controls>(p);intervals=add<Intervals>(p);startTimerHz(20);addAndMakeVisible(audition);audition.setClickingTogglesState(true);
    audition.setTooltip("Hear only this module's actual return before global effects. No extra gain. Click again for the full synth; switching presets or closing the editor ends audition.");
    audition.onClick=[this]{processor.setReturnAudition(audition.getToggleState()?AeriformProcessor::ReturnAudition::Sympathetic:AeriformProcessor::ReturnAudition::Off);};timerCallback();}
SympatheticPage::Controls::Controls(AeriformProcessor& p):ParamPanel(p,"SYMPATHETIC BANK",teal){
    enabled=control<Toggle>(p,ids::symOn,"ENABLED");freeze=control<Toggle>(p,ids::symFreeze,"HOLD ENERGY");tuning=control<ChoiceBox>(p,ids::symTuning,"Tuning");
    send=knob(ids::symSend,"Voice send");level=knob(ids::symReturn,"Return");decay=knob(ids::symDecay,"Decay");damper=knob(ids::symDamper,"Damper");damping=knob(ids::symDamping,"Damping");brightness=knob(ids::symBrightness,"Brightness");detune=knob(ids::symDetune,"Detune");spread=knob(ids::symSpread,"Spread");threshold=knob(ids::symThreshold,"Threshold");root=knob(ids::symRoot,"Root MIDI note");count=knob(ids::symCount,"Active modes");
    addAndMakeVisible(capture);addAndMakeVisible(clear);
    capture.onClick=[&p]{const auto chord=p.getEngine().getHeldChord();if(chord[0]>=0)p.getPatchTools().perform("Capture sympathetic chord",[&]{p.setCapturedChord(chord);});};
    clear.onClick=[&p]{p.getPatchTools().setParameter(ids::symClear,p.getAPVTS().getRawParameterValue(ids::symClear)->load()>.5f?0:1);};
}
void SympatheticPage::Controls::resized()
{
    auto r=getContentArea().reduced(4,2);auto top=r.removeFromTop(26);
    enabled->setBounds(top.removeFromLeft(top.getWidth()/2));freeze->setBounds(top);
    tuning->setBounds(r.removeFromTop(42));r.removeFromTop(4);
    knobRow(r.removeFromTop(70),{send,level,decay},4);
    knobRow(r.removeFromTop(70),{damper,damping,brightness},4);
    knobRow(r.removeFromTop(70),{detune,spread,threshold},4);
    knobRow(r.removeFromTop(70),{root,count,nullptr},4);
    auto buttons=r.removeFromTop(28);capture.setBounds(buttons.removeFromLeft(buttons.getWidth()*3/5));buttons.removeFromLeft(6);clear.setBounds(buttons);
}
SympatheticPage::Intervals::Intervals(AeriformProcessor& p):ParamPanel(p,"CUSTOM INTERVALS / SEMITONES",brass){for(int i=0;i<12;++i)intervals[(size_t)i]=knob(ids::id((P)((int)P::symInterval1+i)),juce::String(i+1),{},48);}
void SympatheticPage::Intervals::resized()
{
    auto r=getContentArea();
    for(int row=0;row<2;++row){auto line=r.removeFromTop(72);int width=line.getWidth()/6;for(int i=0;i<6;++i)intervals[(size_t)(row*6+i)]->setBounds(line.removeFromLeft(width));}
}
void SympatheticPage::resized()
{
    controls->setBounds(0,0,getWidth(),432);
    intervals->setBounds(0,440,getWidth(),182);
    audition.setBounds(8,getHeight()-34,getWidth()-16,28);
}
void SympatheticPage::timerCallback()
{
    audition.setToggleState(processor.getReturnAudition()==AeriformProcessor::ReturnAudition::Sympathetic,juce::dontSendNotification);
    intervals->setVisible((int)processor.getAPVTS().getRawParameterValue(ids::symTuning)->load()==5);
    controls->capture.setEnabled(processor.getEngine().getHeldChord()[0]>=0);
    if(isShowing())repaint();
}
void SympatheticPage::paint(juce::Graphics& g)
{
    auto r=getLocalBounds().withTrimmedTop(440).reduced(10,4);
    const auto& vis=processor.getVisualizerModel();
    if(!intervals->isVisible())
    {
        auto graph=r.removeFromTop(140).toFloat();const float width=graph.getWidth()/12;
        for(int i=0;i<12;++i){const float h=std::min(110.0f,110*std::sqrt(std::max(0.0f,vis.sympatheticEnergy[(size_t)i].load()))*6);float x=graph.getX()+width*i;g.setColour(inset);g.fillRect(x+3,graph.getY(),width-6,110.0f);g.setColour(teal);g.fillRect(x+3,graph.getY()+110-h,width-6,h);}
        juce::String chord="Captured: ";for(int n:processor.getCapturedChord())if(n>=0)chord+=juce::MidiMessage::getMidiNoteName(n,true,true,4)+" ";
        g.setColour(textSecondary);g.setFont(font(11));g.drawFittedText(chord,r.removeFromTop(30),juce::Justification::centredLeft,2);
    }
}
}
