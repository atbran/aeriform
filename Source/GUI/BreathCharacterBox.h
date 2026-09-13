#pragma once
#include "ParamControls.h"
#include "../Params/BreathCharacters.h"
namespace aeriform {
class BreathCharacterBox final:public juce::ComboBox,private juce::Timer {
    AeriformProcessor& processor;int previous=0;
    void timerCallback() override {int match=0;for(int c=0;c<5;++c){bool same=true;for(size_t f=0;f<std::size(breathCharacterFields);++f)if(std::abs(processor.getAPVTS().getRawParameterValue(ids::id(breathCharacterFields[f]))->load()-breathCharacters[c][f])>.0001f)same=false;if(same)match=c+1;}if(match){previous=match;setSelectedId(match,juce::dontSendNotification);}else {setSelectedId(0,juce::dontSendNotification);setText(previous?juce::String(breathCharacterNames[previous-1])+" (modified)":"Modified breath",juce::dontSendNotification);}}
public:
    explicit BreathCharacterBox(AeriformProcessor& p):processor(p){for(int i=0;i<5;++i)addItem(breathCharacterNames[i],i+1);setTooltip("Breath character: changes only the shared breath-engine controls");onChange=[this]{int c=getSelectedId()-1;if(c<0)return;processor.getPatchTools().perform("Breath character",[&]{for(size_t f=0;f<std::size(breathCharacterFields);++f)processor.getPatchTools().setParameter(ids::id(breathCharacterFields[f]),breathCharacters[c][f]);});previous=c+1;};timerCallback();startTimerHz(15);}
    ~BreathCharacterBox() override {stopTimer();}
};
}
