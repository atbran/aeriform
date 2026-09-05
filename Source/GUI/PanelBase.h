#pragma once

#include "Section.h"
#include "Knob.h"
#include "ParamControls.h"

namespace aeriform
{
/** Section panel that owns parameter controls and exposes its knobs for modulation-ring updates. */
class ParamPanel : public SectionPanel
{
public:
    ParamPanel (AeriformProcessor& p, juce::String title, juce::Colour accent) : SectionPanel (std::move (title), accent), processor (p) {}

    const std::vector<std::unique_ptr<Knob>>& getKnobs() const noexcept { return knobs; }

protected:
    using Kind = Knob::ModMapping::Kind;

    AeriformProcessor& processor;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<juce::Component>> controls;

    Knob* knob (const juce::String& id, Knob::ModMapping mapping = {}, int diameter = theme::knobSize)
    {
        auto k = std::make_unique<Knob> (processor, id, mapping, diameter);
        addAndMakeVisible (*k);
        knobs.push_back (std::move (k));
        return knobs.back().get();
    }

    static Knob::ModMapping additive (ModDest d)               { return { d, Kind::Additive, 1.0f }; }
    static Knob::ModMapping exponential (ModDest d, float oct) { return { d, Kind::Exponential, oct }; }
    static Knob::ModMapping semitones (ModDest d, float st)    { return { d, Kind::Semitones, st }; }

    template <typename T, typename... Args>
    T* control (Args&&... args)
    {
        auto c = std::make_unique<T> (std::forward<Args> (args)...);
        addAndMakeVisible (*c);
        T* raw = c.get();
        controls.push_back (std::move (c));
        return raw;
    }

    juce::Label* caption (const juce::String& text, juce::Justification just = juce::Justification::centredLeft)
    {
        auto l = makeCaption (text, just);
        addAndMakeVisible (*l);
        juce::Label* raw = l.get();
        controls.push_back (std::move (l));
        return raw;
    }

    /** Places knobs in a grid row: equal widths, given height. */
    static void knobRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> items, int gap = 2)
    {
        SectionPanel::layoutRow (area, items, gap);
    }
};
} // namespace aeriform
