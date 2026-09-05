#include "ModMatrixPanel.h"
#include "Theme.h"

namespace aeriform
{
using namespace theme;

ModMatrixPanel::ModMatrixPanel (AeriformProcessor& p)
{
    for (int i = 1; i <= ids::numModSlots; ++i)
    {
        Row r;
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
    header.setText ("SOURCE                        DESTINATION                     DEPTH", juce::dontSendNotification);
    header.setFont (font (9.5f, true));
    header.setColour (juce::Label::textColourId, textDim);
    addAndMakeVisible (header);
}

void ModMatrixPanel::resized()
{
    auto r = getLocalBounds();
    header.setBounds (r.removeFromTop (14));
    const int rowH = juce::jmax (18, r.getHeight() / (int) rows.size());
    for (auto& row : rows)
    {
        auto line = r.removeFromTop (rowH).reduced (0, 2);
        const int w = line.getWidth();
        row.source->setBounds (line.removeFromLeft ((int) (w * 0.34f)));
        line.removeFromLeft (4);
        row.dest->setBounds (line.removeFromLeft ((int) (w * 0.34f)));
        line.removeFromLeft (4);
        row.depth->setBounds (line);
    }
}

void ModMatrixPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().withTrimmedTop (14);
    const int rowH = juce::jmax (18, r.getHeight() / (int) rows.size());
    for (size_t i = 0; i < rows.size(); ++i)
    {
        if (i % 2 == 1)
        {
            g.setColour (juce::Colours::white.withAlpha (0.025f));
            g.fillRect (r.getX(), r.getY() + (int) i * rowH, r.getWidth(), rowH);
        }
    }
}
} // namespace aeriform
