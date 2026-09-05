#pragma once
#include "Pages.h"
#include "../DSP/ModularFilters.h"
namespace aeriform {
class FiltersPage final:public Page,private juce::Timer {
public:
    explicit FiltersPage(AeriformProcessor&);
    ~FiltersPage() override{stopTimer();}
    void resized() override;void paint(juce::Graphics&) override;void mouseDown(const juce::MouseEvent&) override;void mouseUp(const juce::MouseEvent&) override;
private:
    class Block final:public ParamPanel,private juce::Timer {
    public:Block(AeriformProcessor&,int);~Block() override{stopTimer();}void resized() override;
    private:int slot;Toggle* enabled;ChoiceBox *position,*model,*slope,*vowel;Knob *cutoff,*resonance,*drive,*keytrack,*envelope,*morph,*mix;void timerCallback() override;
    };
    AeriformProcessor& processor;std::array<Block*,3> blocks;int selected=0;
    void timerCallback() override{repaint();}
};
}
