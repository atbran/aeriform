#pragma once
#include "Pages.h"
namespace aeriform {
class SaturationPage final:public Page {
public:
    explicit SaturationPage(AeriformProcessor&);void resized() override;void paint(juce::Graphics&) override;
private:
    struct Global final:ParamPanel {explicit Global(AeriformProcessor&);void resized() override;Toggle* enabled;ChoiceBox* quality;Knob *low,*high,*mix;};
    struct Band final:ParamPanel {Band(AeriformProcessor&,int);void resized() override;ChoiceBox* model;Knob *drive,*mix,*output;};
    Global* global;std::array<Band*,3> bands;
};
}
