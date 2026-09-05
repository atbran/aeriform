#pragma once

#include "../PanelBase.h"
#include "../ModMatrixPanel.h"

namespace aeriform
{
/** MOTION: three LFOs, modulation envelope and the routing matrix. */
class MotionPanel : public ParamPanel
{
public:
    explicit MotionPanel (AeriformProcessor&);
    void resized() override;

private:
    struct LfoRow
    {
        juce::Label* caption;
        ChoiceBox *shape, *division, *mode;
        Toggle* sync;
        Knob *rate, *fade, *phase;
    };
    LfoRow lfos[ids::numLFOs];
    juce::Label* envCaption;
    Knob *menvA, *menvD, *menvS, *menvR;
    juce::Label* matrixCaption;
    ModMatrixPanel* matrix;
};
} // namespace aeriform
