#include "MacroPanel.h"

namespace aeriform
{
MacroPanel::MacroPanel (AeriformProcessor& p) : ParamPanel (p, "MACROS", theme::copper)
{
    const char* macroIds[4] = { ids::macro1, ids::macro2, ids::macro3, ids::macro4 };
    const ModDest macroDests[4] = { ModDest::Macro1, ModDest::Macro2, ModDest::Macro3, ModDest::Macro4 };

    for (int i = 0; i < 4; ++i)
    {
        macroKnobs[(size_t) i] = knob (macroIds[i], p.getMacroName (i), additive (macroDests[i]), theme::knobSizeSmall);
        macroKnobs[(size_t) i]->setAccentColour (theme::copper);
        macroKnobs[(size_t) i]->makeNameEditable ([&p, i] (const juce::String& newName)
        {
            p.setMacroName (i, newName);
        });
    }

    processor.onMacroNameChanged = [this] (int idx, const juce::String& name)
    {
        if (idx >= 0 && idx < 4 && macroKnobs[(size_t) idx] != nullptr)
            macroKnobs[(size_t) idx]->setDisplayName (name);
    };
}

void MacroPanel::resized()
{
    auto r = getContentArea();
    const int rowH = r.getHeight() / 2;
    auto topRow = r.removeFromTop (rowH);
    auto bottomRow = r;

    const int colW = topRow.getWidth() / 2;
    macroKnobs[0]->setBounds (topRow.removeFromLeft (colW).reduced (2));
    macroKnobs[1]->setBounds (topRow.reduced (2));

    macroKnobs[2]->setBounds (bottomRow.removeFromLeft (colW).reduced (2));
    macroKnobs[3]->setBounds (bottomRow.reduced (2));
}
} // namespace aeriform
