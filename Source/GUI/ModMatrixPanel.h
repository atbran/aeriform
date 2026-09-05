#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ParamControls.h"

namespace aeriform
{
/** Modulation matrix rows (source, destination, bipolar depth), laid out in one or more columns. */
class ModMatrixPanel : public juce::Component
{
public:
    /** Shows slots firstSlot .. firstSlot + numSlots - 1 (1-based) in `columns` columns. */
    ModMatrixPanel (AeriformProcessor&, int firstSlot = 1, int numSlots = ids::numModSlots, int columns = 1);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    struct Row
    {
        int slot = 1;
        std::unique_ptr<ChoiceBox> source, dest;
        std::unique_ptr<HSlider> depth;
    };
    std::vector<Row> rows;
    std::vector<std::unique_ptr<juce::Label>> headers;
    int columns = 1;
    static constexpr int headerHeight = 14;

    juce::Rectangle<int> columnArea (int column) const;
};
} // namespace aeriform
