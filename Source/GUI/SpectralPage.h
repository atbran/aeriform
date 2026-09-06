#pragma once
#include "Pages.h"
namespace aeriform {
class SpectralPage final:public Page,private juce::Timer {
public:
    explicit SpectralPage(AeriformProcessor&);
    ~SpectralPage() override {stopTimer();}
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    struct Controls final:ParamPanel {
        explicit Controls(AeriformProcessor&);void resized() override;
        Toggle *enabled,*hold;Knob *blur,*shift,*random,*decay,*mix;
        juce::TextButton capture{"CAPTURE"},release{"RELEASE"};
    };
    Controls* controls;AeriformProcessor& processor;
    void timerCallback() override {repaint();}
};
}
