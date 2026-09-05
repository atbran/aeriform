#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeel.h"
#include "Theme.h"

namespace aeriform
{
/** Base for the five conceptual regions: draws the titled panel, exposes the content area. */
class SectionPanel : public juce::Component
{
public:
    SectionPanel (juce::String title, juce::Colour accent) : sectionTitle (std::move (title)), accentColour (accent) {}

    void paint (juce::Graphics& g) override
    {
        AeriformLookAndFeel::drawSectionPanel (g, getLocalBounds().toFloat(), sectionTitle, accentColour);
    }

    juce::Rectangle<int> getContentArea() const
    {
        return getLocalBounds().withTrimmedTop (theme::sectionTitleHeight).reduced (8, 6);
    }

    /** Lays out a row of equally-spaced components inside `area`. */
    static void layoutRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> items, int gap = 4)
    {
        const int n = (int) items.size();
        if (n == 0) return;
        const int w = (area.getWidth() - gap * (n - 1)) / n;
        int x = area.getX();
        for (auto* c : items)
        {
            if (c != nullptr) c->setBounds (x, area.getY(), w, area.getHeight());
            x += w + gap;
        }
    }

private:
    juce::String sectionTitle;
    juce::Colour accentColour;
};
} // namespace aeriform
