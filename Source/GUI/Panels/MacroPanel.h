#pragma once

#include "../PanelBase.h"

namespace aeriform
{
/** Panel hosting the 4 assignable macro knobs with in-place editable labels. */
class MacroPanel : public ParamPanel
{
public:
    explicit MacroPanel (AeriformProcessor&);
    void resized() override;

private:
    std::array<Knob*, 4> macroKnobs;
};
} // namespace aeriform
