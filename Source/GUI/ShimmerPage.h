#pragma once
#include "Pages.h"
namespace aeriform {
class ShimmerPage final:public Page {
public:
    explicit ShimmerPage(AeriformProcessor&);
    void resized() override;void paint(juce::Graphics&) override;
private:
    struct Controls final:ParamPanel {
        explicit Controls(AeriformProcessor&);void resized() override;
        Toggle* enabled;std::array<Knob*,9> knobs;
        juce::TextButton octave{"+12 OCTAVE"},fifth{"+7 FIFTH"},fourth{"+5 FOURTH"};
    };
    Controls* controls;
};
}
