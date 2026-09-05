#pragma once
#include "Pages.h"
namespace aeriform {
class ContactPage final:public Page,private juce::Timer {
public:
    explicit ContactPage(AeriformProcessor&);
    ~ContactPage() override {stopTimer();}
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    struct Controls final:ParamPanel {
        explicit Controls(AeriformProcessor&);
        void resized() override;
        Toggle* enabled;ChoiceBox *source,*destination,*quality,*polarity;
        Knob *gap,*stiffness,*hardness,*damping,*friction,*asymmetry,*amount;
    };
    Controls* controls;AeriformProcessor& processor;
    void timerCallback() override {repaint();}
};
}
