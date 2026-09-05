#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ParamControls.h"

namespace aeriform
{
/** Eight-slot modulation matrix: source, destination and bipolar depth per row. */
class ModMatrixPanel : public juce::Component
{
public:
    explicit ModMatrixPanel (AeriformProcessor&);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    struct Row
    {
        std::unique_ptr<ChoiceBox> source, dest;
        std::unique_ptr<HSlider> depth;
    };
    std::vector<Row> rows;
    juce::Label header;
};
} // namespace aeriform
