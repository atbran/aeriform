#pragma once

#include "../PanelBase.h"
#include "../ModMatrixPanel.h"

namespace aeriform
{
/** MOTION: three LFOs, modulation envelope and the routing matrix.
    Compact (MAIN page column): matrix slots 1-8. Full (MOTION page): LFOs side by side, all 16 slots. */
class MotionPanel : public ParamPanel
{
public:
    MotionPanel (AeriformProcessor&, bool full);
    void resized() override;

private:
    struct LfoRow
    {
        juce::Label* caption;
        ChoiceBox *shape, *division, *mode;
        Toggle* sync;
        Knob *rate, *fade, *phase;
    };
    bool full;
    LfoRow lfos[ids::numLFOs];
    juce::Label* envCaption;
    Knob *menvA, *menvD, *menvS, *menvR;
    juce::Label* matrixCaption;
    ModMatrixPanel* matrix;
};
} // namespace aeriform
