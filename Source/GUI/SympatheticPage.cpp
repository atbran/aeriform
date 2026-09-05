#include "SympatheticPage.h"
namespace aeriform {
using namespace theme;
SympatheticPage::SympatheticPage(AeriformProcessor& p):processor(p){controls=add<Controls>(p);intervals=add<Intervals>(p);startTimerHz(20);timerCallback();}
SympatheticPage::Controls::Controls(AeriformProcessor& p):ParamPanel(p,"SHARED SYMPATHETIC RESONATORS",teal){
    enabled=control<Toggle>(p,ids::symOn,"ENABLED");freeze=control<Toggle>(p,ids::symFreeze,"HOLD ENERGY");tuning=control<ChoiceBox>(p,ids::symTuning,"Tuning");
    send=knob(ids::symSend,"Voice send");level=knob(ids::symReturn,"Return");decay=knob(ids::symDecay,"Decay");damper=knob(ids::symDamper,"Damper");damping=knob(ids::symDamping,"Damping");brightness=knob(ids::symBrightness,"Brightness");detune=knob(ids::symDetune,"Detune");spread=knob(ids::symSpread,"Spread");threshold=knob(ids::symThreshold,"Threshold");root=knob(ids::symRoot,"Root MIDI note");count=knob(ids::symCount,"Active modes");
    addAndMakeVisible(capture);addAndMakeVisible(clear);
    capture.onClick=[&p]{const auto chord=p.getEngine().getHeldChord();if(chord[0]>=0)p.getPatchTools().perform("Capture sympathetic chord",[&]{p.setCapturedChord(chord);});};
    clear.onClick=[&p]{p.getPatchTools().setParameter(ids::symClear,p.getAPVTS().getRawParameterValue(ids::symClear)->load()>.5f?0:1);};
}
void SympatheticPage::Controls::resized(){auto r=getContentArea().reduced(12,8);auto top=r.removeFromTop(26);enabled->setBounds(top.removeFromLeft(top.getWidth()/2));freeze->setBounds(top);r.removeFromTop(8);tuning->setBounds(r.removeFromTop(44));r.removeFromTop(10);
    knobRow(r.removeFromTop(98),{send,level,decay},10);r.removeFromTop(8);knobRow(r.removeFromTop(98),{damper,damping,brightness},10);r.removeFromTop(8);knobRow(r.removeFromTop(98),{detune,spread,threshold},10);r.removeFromTop(8);knobRow(r.removeFromTop(98),{root,count},20);r.removeFromTop(10);auto buttons=r.removeFromTop(30);capture.setBounds(buttons.removeFromLeft(buttons.getWidth()*2/3).reduced(0,0));buttons.removeFromLeft(8);clear.setBounds(buttons);}
SympatheticPage::Intervals::Intervals(AeriformProcessor& p):ParamPanel(p,"CUSTOM INTERVALS / SEMITONES",brass){for(int i=0;i<12;++i)intervals[(size_t)i]=knob(ids::id((P)((int)P::symInterval1+i)),juce::String(i+1),{},48);}
void SympatheticPage::Intervals::resized(){auto r=getContentArea().reduced(8);for(int row=0;row<2;++row){auto line=r.removeFromTop(93);int width=line.getWidth()/6;for(int i=0;i<6;++i)intervals[(size_t)(row*6+i)]->setBounds(line.removeFromLeft(width));}}
void SympatheticPage::resized(){controls->setBounds(0,0,getWidth()/2-6,getHeight());intervals->setBounds(getWidth()/2+6,300,getWidth()/2-6,230);}
void SympatheticPage::timerCallback(){int tuning=(int)processor.getAPVTS().getRawParameterValue(ids::symTuning)->load();intervals->setVisible(tuning==5);controls->capture.setEnabled(processor.getEngine().getHeldChord()[0]>=0);repaint();}
void SympatheticPage::paint(juce::Graphics& g){auto r=getLocalBounds().withTrimmedLeft(getWidth()/2+24).reduced(10,16);g.setColour(textPrimary);g.setFont(titleFont(18));g.drawText("TWELVE STRINGS, ONE SHARED BODY",r.removeFromTop(36),juce::Justification::centredLeft);
    auto graph=r.removeFromTop(154).toFloat();const auto& vis=processor.getVisualizerModel();const float width=graph.getWidth()/12;
    for(int i=0;i<12;++i){float e=vis.sympatheticEnergy[(size_t)i].load(),hz=vis.sympatheticFrequency[(size_t)i].load();float x=graph.getX()+width*i;g.setColour(inset);g.fillRoundedRectangle(x+5,graph.getY()+8,width-10,110,4);float h=std::min(106.0f,106*std::sqrt(std::max(0.0f,e))*6);g.setColour(i%2?teal:copper);g.fillRoundedRectangle(x+7,graph.getY()+116-h,width-14,h,3);g.setColour(textSecondary);g.setFont(font(9));g.drawText(hz>0?juce::String((int)std::round(hz)):"-",(int)x,(int)graph.getBottom()-24,(int)width,20,juce::Justification::centred);}
    g.setColour(textSecondary);g.setFont(font(12));g.drawText("Live mode energy / tuning in Hz",r.removeFromTop(24),juce::Justification::centredLeft);
    juce::String chord="Captured: ";for(int n:processor.getCapturedChord())if(n>=0)chord+=juce::MidiMessage::getMidiNoteName(n,true,true,4)+"  ";g.drawFittedText(chord,r.removeFromTop(54),juce::Justification::centredLeft,2,1.0f);
    if(!intervals->isVisible()){r.removeFromTop(20);g.setFont(font(14));g.drawFittedText("All voices excite this shared bank. Scale modes follow the root; Held notes follows the current chord. Capture keeps a chord available after you release the keys. Captured notes are saved with your patch.",r.removeFromTop(145),juce::Justification::centredLeft,7,1.0f);r.removeFromTop(18);g.drawFittedText("The damper shortens ringing. Hold stops both decay and new excitation. Clear empties the bank. With Held notes selected, releasing the last key leaves the previous tuning ringing naturally.",r.removeFromTop(135),juce::Justification::centredLeft,6,1.0f);}
}
}
