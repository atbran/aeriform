#pragma once
#include "Pages.h"
namespace aeriform {
class PerformancePage final : public Page {
public:
    explicit PerformancePage(AeriformProcessor&);
    void resized() override;
private:
    class ToolsPanel final:public ParamPanel,private juce::Timer {
    public:
        explicit ToolsPanel(AeriformProcessor&);
        ~ToolsPanel() override {stopTimer();}
        void resized() override;
        void paint(juce::Graphics&) override;
    private:
        std::array<juce::TextButton,2> capture,load,editEndpoint;
        std::array<juce::Label,2> endpointName;
        juce::TextButton randomize{"RANDOMIZE"},mutate{"MUTATE"},newSeed{"NEW SEED"},lockAll{"LOCK ALL"},unlockAll{"UNLOCK ALL"},lockSection{"LOCK SECTION"},unlockSection{"UNLOCK SECTION"},commit{"COMMIT"};
        juce::TextEditor seed;
        juce::ComboBox scope,section;
        Toggle* enabled;ChoiceBox* engine;HSlider* morph;Toggle* wild;
        Knob *mutation,*repipe,*coupling,*feedback,*folder,*brightness,*width,*room;
        juce::Label help;
        void timerCallback() override;
        void choosePreset(int);
    };
    ToolsPanel* tools;
};
}
