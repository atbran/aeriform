#pragma once
#include "Pages.h"
namespace aeriform {
class RoomPage final : public Page, private juce::Timer {
public:
    explicit RoomPage(AeriformProcessor&);
    ~RoomPage() override { stopTimer(); }
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    struct Controls final : ParamPanel {
        explicit Controls(AeriformProcessor&);
        void resized() override;
        Toggle *enabled, *freeze;
        std::array<Knob*,12> knobs;
        juce::TextButton clear{"CLEAR ROOM ENERGY"};
    };
    AeriformProcessor& processor;
    Controls* controls;
    juce::TextButton audition{"AUDITION RETURN"};
    void timerCallback() override { audition.setToggleState(processor.getReturnAudition()==AeriformProcessor::ReturnAudition::Room,juce::dontSendNotification);if(isShowing()) repaint(); }
};
}
