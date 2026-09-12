#pragma once

#include "Pages.h"
#include "ContactPage.h"
#include "SympatheticPage.h"
#include "RoomPage.h"
#include "FiltersPage.h"
#include "ResonantDelayPage.h"
#include "ShimmerPage.h"
#include "SpectralPage.h"
#include "SaturationPage.h"

namespace aeriform
{
/** The existing processors, grouped into a continuous control surface. */
class EffectsCollection final : public Page
{
public:
    explicit EffectsCollection (AeriformProcessor& p)
    {
        fixed = add<SpacePage> (p);
        filters = add<FiltersPage> (p);
        delay = add<ResonantDelayPage> (p);
        shimmer = add<ShimmerPage> (p);
        spectral = add<SpectralPage> (p);
        saturation = add<SaturationPage> (p);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        fixed->setBounds (r.removeFromTop (244)); r.removeFromTop (12);
        filters->setBounds (r.removeFromTop (366)); r.removeFromTop (12);
        auto row = r.removeFromTop (360);
        delay->setBounds (row.removeFromLeft ((row.getWidth() - 12) / 2)); row.removeFromLeft (12);
        shimmer->setBounds (row); r.removeFromTop (12);
        row = r.removeFromTop (400);
        spectral->setBounds (row.removeFromLeft ((row.getWidth() - 12) / 2)); row.removeFromLeft (12);
        saturation->setBounds (row);
    }

    std::vector<ParamPanel*> getPanels() override
    {
        std::vector<ParamPanel*> result;
        for (auto* page : { fixed, filters, delay, shimmer, spectral, saturation })
        {
            auto list = page->getPanels(); result.insert (result.end(), list.begin(), list.end());
        }
        return result;
    }

private:
    Page *fixed, *filters, *delay, *shimmer, *spectral, *saturation;
};

class EffectsPage final : public Page
{
public:
    explicit EffectsPage (AeriformProcessor& p) : collection (p)
    {
        addAndMakeVisible (viewport);
        viewport.setViewedComponent (&collection, false);
        viewport.setScrollBarsShown (true, false);
    }
    ~EffectsPage() override { viewport.setViewedComponent (nullptr, false); }
    void resized() override
    {
        viewport.setBounds (getLocalBounds());
        collection.setSize (juce::jmax (1, getWidth() - viewport.getScrollBarThickness() - 4), 1406);
    }
    std::vector<ParamPanel*> getPanels() override { return collection.getPanels(); }
private:
    EffectsCollection collection;
    juce::Viewport viewport;
};

/** Three acoustic modules together, with every existing parameter retained. */
class AcousticPage final : public Page
{
public:
    explicit AcousticPage (AeriformProcessor& p)
    {
        contact = add<ContactPage> (p); bank = add<SympatheticPage> (p); room = add<RoomPage> (p);
    }
    void resized() override
    {
        auto r = getLocalBounds();
        const int w = (r.getWidth() - 24) / 3;
        contact->setBounds (r.removeFromLeft (w)); r.removeFromLeft (12);
        bank->setBounds (r.removeFromLeft (w)); r.removeFromLeft (12);
        room->setBounds (r);
    }
    std::vector<ParamPanel*> getPanels() override
    {
        std::vector<ParamPanel*> result;
        for (auto* page : { contact, bank, room })
        {
            auto list = page->getPanels(); result.insert (result.end(), list.begin(), list.end());
        }
        return result;
    }
private:
    Page *contact, *bank, *room;
};
} // namespace aeriform
