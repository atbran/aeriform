#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Plugin/PluginProcessor.h"
#include "Theme.h"
#include "GuiDiagnostics.h"

namespace aeriform
{
inline void showParameterLock(juce::Component& target,AeriformProcessor& p,const juce::String& id) {
    juce::PopupMenu menu;menu.addItem(1,"Lock for randomization",true,p.getPatchTools().isLocked(id));
    juce::Component::SafePointer<juce::Component> safe(&target);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&target),[safe,&p,id](int result){if(safe&&result==1)p.getPatchTools().setLocked(id,!p.getPatchTools().isLocked(id));});
}
class LockableCombo : public juce::ComboBox {
public:std::function<void()> onLock;
    void mouseDown(const juce::MouseEvent& e) override {if(e.mods.isPopupMenu()){if(onLock)onLock();}else juce::ComboBox::mouseDown(e);}
};
class LockableToggle : public juce::ToggleButton {
public:std::function<void()> onLock;
    void mouseDown(const juce::MouseEvent& e) override {if(e.mods.isPopupMenu()){if(onLock)onLock();}else juce::ToggleButton::mouseDown(e);}
};
/** Combo box bound to a choice parameter, with a small caption above it. */
class ChoiceBox : public juce::Component
{
public:
    ChoiceBox (AeriformProcessor& p, const juce::String& paramID, const juce::String& caption = {})
    {
        box.onLock=[this,&p,paramID]{showParameterLock(box,p,paramID);};
        auto* param = dynamic_cast<juce::AudioParameterChoice*> (p.getAPVTS().getParameter (paramID));
        jassert (param != nullptr);
        if (param == nullptr) ++gui::unboundControlCount();
        if (param != nullptr)
        {
            box.addItemList (param->choices, 1);
            box.setTooltip (findParamInfo (paramID) != nullptr ? findParamInfo (paramID)->tooltip : juce::String());
            attachment=std::make_unique<juce::ParameterAttachment>(*param,[this](float value){box.setSelectedId((int)std::lround(value)+1,juce::dontSendNotification);});
            attachment->sendInitialUpdate();
            box.onChange=[this,&p,paramID]{p.getPatchTools().setParameter(paramID,(float)(box.getSelectedId()-1));};
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
    LockableCombo box;
    juce::Label label;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

/** Toggle switch bound to a bool parameter. */
class Toggle : public juce::Component
{
public:
    Toggle (AeriformProcessor& p, const juce::String& paramID, const juce::String& caption = {})
    {
        button.onLock=[this,&p,paramID]{showParameterLock(button,p,paramID);};
        button.setButtonText (caption.isNotEmpty() ? caption : (findParamInfo (paramID) != nullptr ? findParamInfo (paramID)->name : paramID));
        button.setTooltip (findParamInfo (paramID) != nullptr ? findParamInfo (paramID)->tooltip : juce::String());
        if (p.getAPVTS().getParameter (paramID) == nullptr) { ++gui::unboundControlCount(); }
        else {attachment=std::make_unique<juce::ParameterAttachment>(*p.getAPVTS().getParameter(paramID),[this](float value){button.setToggleState(value>0.5f,juce::dontSendNotification);});attachment->sendInitialUpdate();
            button.onClick=[this,&p,paramID]{p.getPatchTools().setParameter(paramID,button.getToggleState()?1.0f:0.0f);};}
        addAndMakeVisible (button);
    }
    void resized() override { button.setBounds (getLocalBounds()); }
    juce::ToggleButton& getButton() { return button; }

private:
    LockableToggle button;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

/** Horizontal slider bound to a parameter (used for modulation depths and integer values). */
class HSlider : public juce::Component
{
public:
    HSlider (AeriformProcessor& p, const juce::String& paramID, bool showValueBox = false)
    {
        auto* param = p.getAPVTS().getParameter (paramID);
        jassert (param != nullptr);
        if (param == nullptr) ++gui::unboundControlCount();
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
        slider.onDragStart=[&p,paramID]{p.getPatchTools().begin("Edit "+paramID);};
        slider.onDragEnd=[&p]{p.getPatchTools().end();};
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
