#pragma once
#include "Pages.h"
namespace aeriform {
class SympatheticPage final:public Page,private juce::Timer {
public:
    explicit SympatheticPage(AeriformProcessor&);~SympatheticPage() override {stopTimer();}
    void resized() override;void paint(juce::Graphics&) override;
private:
    struct Controls final:ParamPanel {
        explicit Controls(AeriformProcessor&);void resized() override;
        Toggle *enabled,*freeze;ChoiceBox* tuning;
        Knob *send,*level,*decay,*damper,*damping,*brightness,*detune,*spread,*threshold,*root,*count;
        juce::TextButton capture{"CAPTURE HELD CHORD"},clear{"CLEAR ENERGY"};
    };
    struct Intervals final:ParamPanel {
        explicit Intervals(AeriformProcessor&);void resized() override;
        std::array<Knob*,12> intervals;
    };
    AeriformProcessor& processor;Controls* controls;Intervals* intervals;
    void timerCallback() override;
};
}
