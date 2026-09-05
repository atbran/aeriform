#pragma once

#include "ExciterSlotPanel.h"

namespace aeriform
{
/**
    EXCITERS (MAIN page): the two exciter slots in compact form, the interaction
    mode and amount, the wavefolder essentials and the breath envelope.
    Replaces the v0.1 BREATH panel; every v0.1 control is still reachable here
    or on the EXCITERS page.
*/
class ExcitersOverviewPanel : public ParamPanel
{
public:
    explicit ExcitersOverviewPanel (AeriformProcessor&);
    void resized() override;
    void collectPanels (std::vector<ParamPanel*>& out) override;

private:
    ExciterSlotPanel *slotA, *slotB;
    juce::Label *mixCaption, *foldCaption, *envCaption;
    ChoiceBox *mixMode, *foldMode;
    Toggle* foldOn;
    Knob *interaction, *depth, *balance;
    Knob *fold, *drive, *shape;
    Knob *attack, *decay, *sustain, *release, *velPress, *pressBright;
};
} // namespace aeriform
