#pragma once
#include "Pages.h"
namespace aeriform {
class ContactPage final:public Page,private juce::Timer {
public:
    explicit ContactPage(AeriformProcessor&);
    ~ContactPage() override {stopTimer();}
    void resized() override;
    void paint(juce::Graphics&) override;
    void showStereo(bool);
private:
    struct Controls final:ParamPanel {
        explicit Controls(AeriformProcessor&);
        void resized() override;
        Toggle* enabled;ChoiceBox *source,*destination,*quality,*polarity;
        Knob *gap,*stiffness,*hardness,*damping,*friction,*asymmetry,*amount;
    };
    struct StereoControls final:ParamPanel {
        explicit StereoControls(AeriformProcessor&);void resized() override;
        ChoiceBox* mode;Knob *divergence,*coupling,*exciter,*pickup,*damping,*rotation,*width,*bass;
    };
    Controls* controls;StereoControls* stereoControls;bool stereoVisible=false;
    juce::TextButton switchView{"EDIT PHYSICAL STEREO"};
    AeriformProcessor& processor;
    void timerCallback() override;
};
}
