#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeel.h"
#include "Theme.h"

namespace aeriform
{
/** Base for the titled panels: draws the panel frame and title strip, exposes the content area.
    An unframed panel draws nothing and uses its whole bounds (for sub-panels nested in a frame). */
class SectionPanel : public juce::Component
{
public:
    SectionPanel (juce::String title, juce::Colour accent, bool framed = true)
        : sectionTitle (std::move (title)), accentColour (accent), isFramed (framed) {}

    void paint (juce::Graphics& g) override
    {
        if (isFramed)
            AeriformLookAndFeel::drawSectionPanel (g, getLocalBounds().toFloat(), sectionTitle, accentColour);
    }

    void setTitle (juce::String t) { sectionTitle = std::move (t); repaint(); }
    const juce::String& getTitle() const noexcept { return sectionTitle; }
    juce::Colour getAccent() const noexcept { return accentColour; }

    juce::Rectangle<int> getContentArea() const
    {
        if (! isFramed) return getLocalBounds();
        return getLocalBounds().withTrimmedTop (theme::sectionTitleHeight).reduced (8, 6);
    }

    /** Lays out a row of equally-spaced components inside `area`. */
    static void layoutRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> items, int gap = 4)
    {
        std::vector<juce::Component*> v (items);
        layoutRowV (area, v, gap);
    }

    static void layoutRowV (juce::Rectangle<int> area, const std::vector<juce::Component*>& items, int gap = 4)
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

    /** Lays out items left-to-right with a fixed width each (the last row entry is not stretched). */
    static void layoutFixed (juce::Rectangle<int> area, const std::vector<juce::Component*>& items, int itemWidth, int gap = 2)
    {
        int x = area.getX();
        for (auto* c : items)
        {
            if (c != nullptr) c->setBounds (x, area.getY(), itemWidth, area.getHeight());
            x += itemWidth + gap;
        }
    }

private:
    juce::String sectionTitle;
    juce::Colour accentColour;
    bool isFramed = true;
};
} // namespace aeriform
