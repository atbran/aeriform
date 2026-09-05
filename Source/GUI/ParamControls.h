#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Plugin/PluginProcessor.h"
#include "Theme.h"

namespace aeriform
{
/** Combo box bound to a choice parameter, with a small caption above it. */
class ChoiceBox : public juce::Component
{
public:
    ChoiceBox (AeriformProcessor& p, const juce::String& paramID, const juce::String& caption = {})
    {
        auto* param = dynamic_cast<juce::AudioParameterChoice*> (p.getAPVTS().getParameter (paramID));
        jassert (param != nullptr);
        if (param != nullptr)
        {
            box.addItemList (param->choices, 1);
            box.setTooltip (findParamInfo (paramID) != nullptr ? findParamInfo (paramID)->tooltip : juce::String());
            attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.getAPVTS(), paramID, box);
        }
        label.setText (caption.isNotEmpty() ? caption : (findParamInfo (paramID) != nullptr ? findParamInfo (paramID)->name : paramID), juce::dontSendNotification);
        label.setFont (theme::font (10.5f));
        label.setColour (juce::Label::textColourId, theme::textSecondary);
        label.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (label);
        addAndMakeVisible (box);
    }

    void setCaptionVisible (bool v) { label.setVisible (v); resized(); }
    juce::ComboBox& getBox() { return box; }

    void resized() override
    {
        auto r = getLocalBounds();
        if (label.isVisible()) label.setBounds (r.removeFromTop (14));
        box.setBounds (r.reduced (0, 1));
    }

private:
    juce::ComboBox box;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

/** Toggle switch bound to a bool parameter. */
class Toggle : public juce::Component
{
public:
    Toggle (AeriformProcessor& p, const juce::String& paramID, const juce::String& caption = {})
    {
        button.setButtonText (caption.isNotEmpty() ? caption : (findParamInfo (paramID) != nullptr ? findParamInfo (paramID)->name : paramID));
        button.setTooltip (findParamInfo (paramID) != nullptr ? findParamInfo (paramID)->tooltip : juce::String());
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.getAPVTS(), paramID, button);
        addAndMakeVisible (button);
    }
    void resized() override { button.setBounds (getLocalBounds()); }
    juce::ToggleButton& getButton() { return button; }

private:
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

/** Horizontal slider bound to a parameter (used for modulation depths and integer values). */
class HSlider : public juce::Component
{
public:
    HSlider (AeriformProcessor& p, const juce::String& paramID, bool showValueBox = false)
    {
        auto* param = p.getAPVTS().getParameter (paramID);
        jassert (param != nullptr);
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (showValueBox ? juce::Slider::TextBoxRight : juce::Slider::NoTextBox, false, 54, 16);
        slider.setPopupDisplayEnabled (true, false, nullptr);
        slider.setScrollWheelEnabled (true);
        if (param != nullptr)
        {
            slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
            slider.setTooltip (findParamInfo (paramID) != nullptr ? findParamInfo (paramID)->tooltip : juce::String());
            attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.getAPVTS(), paramID, slider);
        }
        addAndMakeVisible (slider);
    }
    void resized() override { slider.setBounds (getLocalBounds()); }
    juce::Slider& getSlider() { return slider; }

private:
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

/** Small caption label helper. */
inline std::unique_ptr<juce::Label> makeCaption (const juce::String& text, juce::Justification just = juce::Justification::centredLeft)
{
    auto l = std::make_unique<juce::Label> (juce::String(), text);
    l->setFont (theme::font (10.5f, true));
    l->setColour (juce::Label::textColourId, theme::textSecondary);
    l->setJustificationType (just);
    l->setInterceptsMouseClicks (false, false);
    return l;
}
} // namespace aeriform
