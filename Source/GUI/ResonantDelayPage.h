#pragma once
#include "Pages.h"
namespace aeriform {
class ResonantDelayPage final:public Page {
public:
    explicit ResonantDelayPage(AeriformProcessor&);
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    struct Timing final:ParamPanel,private juce::Timer {
        explicit Timing(AeriformProcessor&);~Timing() override {stopTimer();}
        void resized() override;void timerCallback() override;
        Toggle *enabled,*sync;ChoiceBox* division;
        Knob *time,*feedback,*mix,*offset,*saturation;
    };
    struct Colour final:ParamPanel {
        explicit Colour(AeriformProcessor&);void resized() override;
        ChoiceBox* type;Knob *tuning,*track,*amount,*damping,*dispersion;
    };
    Timing* timing;Colour* colour;
};
}
