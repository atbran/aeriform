#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Plugin/PluginProcessor.h"
#include "../DSP/ModMatrix.h"

namespace aeriform
{
/** How a modulation destination maps onto a knob's range (for the modulation ring). */
struct KnobModMapping
{
    enum class Kind { None, Additive, Exponential, Semitones };
    KnobModMapping() = default;
    KnobModMapping (ModDest d, Kind k, float s) : dest (d), kind (k), scale (s) {}
    ModDest dest = ModDest::None;
    Kind kind = Kind::None;
    float scale = 1.0f;     // octaves per unit (Exponential) or semitones per unit (Semitones)
};

/**
    Rotary parameter control: name label, value readout while editing,
    double-click reset, shift/ctrl fine adjustment, right-click MIDI learn menu,
    tooltip, and a teal modulation ring showing matrix depth and live value.
*/
class Knob : public juce::Component,
             private juce::Slider::Listener,
             private juce::Timer
{
public:
    using ModMapping = KnobModMapping;

    Knob (AeriformProcessor& processor, const juce::String& paramID, ModMapping mapping = ModMapping(), int diameter = 58);
    ~Knob() override;

    void setDisplayName (const juce::String& name);
    void setAccentColour (juce::Colour c) { slider.setColour (juce::Slider::rotarySliderFillColourId, c); }
    void setKnobDiameter (int d) { diameter = d; resized(); }

    /** Updates the modulation ring from the current matrix configuration and live values. */
    void updateModRing (const dsp::ModConfig& config, const std::array<float, (size_t) ModDest::Count>& live);

    const juce::String& getParamID() const noexcept { return paramID; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class KnobSlider : public juce::Slider
    {
    public:
        explicit KnobSlider (Knob& k) : owner (k) {}
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
    private:
        Knob& owner;
    };

    AeriformProcessor& processor;
    juce::String paramID, displayName;
    juce::RangedAudioParameter* param = nullptr;
    const ParamInfo* info = nullptr;
    ModMapping mapping;
    int diameter = 58;

    KnobSlider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    bool showingValue = false;
    float modDepthNorm = 0.0f;   // normalised half-width of the ring
    float modLiveNorm = 0.0f;    // normalised live offset from the knob value
    bool  hasMod = false;

    void sliderValueChanged (juce::Slider*) override;
    void sliderDragStarted (juce::Slider*) override;
    void sliderDragEnded (juce::Slider*) override;
    void timerCallback() override;
    void showValue();
    void showName();
    void showContextMenu();
    float normFromDspValue (float dsp) const;
    float modulatedNorm (float amount) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
};
} // namespace aeriform
