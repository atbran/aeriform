#include "ModMatrixPanel.h"
#include "Theme.h"

namespace aeriform
{
using namespace theme;

ModMatrixPanel::ModMatrixPanel (AeriformProcessor& p, int firstSlot, int numSlots, int cols) : columns (juce::jmax (1, cols))
{
    for (int i = firstSlot; i < firstSlot + numSlots && i <= ids::numModSlots; ++i)
    {
        Row r;
        r.slot = i;
        r.source = std::make_unique<ChoiceBox> (p, ids::modParam (i, ids::modSrcSuffix));
        r.dest = std::make_unique<ChoiceBox> (p, ids::modParam (i, ids::modDstSuffix));
        r.depth = std::make_unique<HSlider> (p, ids::modParam (i, ids::modDepthSuffix));
        r.source->setCaptionVisible (false);
        r.dest->setCaptionVisible (false);
        r.depth->getSlider().setColour (juce::Slider::trackColourId, teal);
        addAndMakeVisible (*r.source);
        addAndMakeVisible (*r.dest);
        addAndMakeVisible (*r.depth);
        rows.push_back (std::move (r));
    }
    for (int c = 0; c < columns; ++c)
    {
        auto h = std::make_unique<juce::Label>();
        h->setText ("SOURCE                        DESTINATION                     DEPTH", juce::dontSendNotification);
        h->setFont (font (9.5f, true));
        h->setColour (juce::Label::textColourId, textDim);
        addAndMakeVisible (*h);
        headers.push_back (std::move (h));
    }
}

juce::Rectangle<int> ModMatrixPanel::columnArea (int column) const
{
    auto r = getLocalBounds();
    const int gap = 12;
    const int w = (r.getWidth() - gap * (columns - 1)) / columns;
    return juce::Rectangle<int> (r.getX() + column * (w + gap), r.getY(), w, r.getHeight());
}

void ModMatrixPanel::resized()
{
    const int perColumn = ((int) rows.size() + columns - 1) / columns;
    for (int c = 0; c < columns; ++c)
    {
        auto area = columnArea (c);
        headers[(size_t) c]->setBounds (area.removeFromTop (headerHeight));
        const int rowH = juce::jmax (18, area.getHeight() / juce::jmax (1, perColumn));
        for (int i = c * perColumn; i < juce::jmin ((int) rows.size(), (c + 1) * perColumn); ++i)
        {
            auto& row = rows[(size_t) i];
            auto line = area.removeFromTop (rowH).reduced (0, 2);
            const int w = line.getWidth();
            line.removeFromLeft (16);   // slot number
            row.source->setBounds (line.removeFromLeft ((int) (w * 0.32f)));
            line.removeFromLeft (4);
            row.dest->setBounds (line.removeFromLeft ((int) (w * 0.32f)));
            line.removeFromLeft (4);
            row.depth->setBounds (line);
        }
    }
}

void ModMatrixPanel::paint (juce::Graphics& g)
{
    const int perColumn = ((int) rows.size() + columns - 1) / columns;
    for (int c = 0; c < columns; ++c)
    {
        auto area = columnArea (c).withTrimmedTop (headerHeight);
        const int rowH = juce::jmax (18, area.getHeight() / juce::jmax (1, perColumn));
        for (int i = c * perColumn; i < juce::jmin ((int) rows.size(), (c + 1) * perColumn); ++i)
        {
            const int local = i - c * perColumn;
            auto line = juce::Rectangle<int> (area.getX(), area.getY() + local * rowH, area.getWidth(), rowH);
            if (local % 2 == 1)
            {
                g.setColour (juce::Colours::white.withAlpha (0.025f));
                g.fillRect (line);
            }
            g.setColour (textDim);
            g.setFont (monoFont (9.5f));
            g.drawText (juce::String (rows[(size_t) i].slot), line.removeFromLeft (14), juce::Justification::centred);
        }
    }
}
} // namespace aeriform
