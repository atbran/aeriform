#pragma once

#include "../PanelBase.h"
#include "../Displays.h"
#include <map>

namespace aeriform
{
/**
    One exciter slot (A or B). Shows the model selector, the common controls
    and a context-sensitive set of model parameters (waveform, orbit, noise
    laboratory, physical or sidechain), plus a live waveform scope and level.

    Full mode is the EXCITERS page module; compact mode is the MAIN page
    overview (model, level, tone and the three most characteristic knobs).
*/
class ExciterSlotPanel : public ParamPanel,
                         private juce::AudioProcessorValueTreeState::Listener,
                         private juce::AsyncUpdater
{
public:
    ExciterSlotPanel (AeriformProcessor&, int slot, bool compact);
    ~ExciterSlotPanel() override;

    void resized() override;
    void paint (juce::Graphics&) override;

    /** Knobs shown for a model (ordered), as parameter ids. */
    std::vector<juce::String> idsForModel (ExciterModel, bool compactSet) const;
    ExciterModel currentModel() const;

private:
    int slot;
    bool compact;
    juce::String px;          // "exa" / "exb"
    juce::String modelId;
    ChoiceBox* model = nullptr;
    ChoiceBox* retrig = nullptr;
    Toggle* sync = nullptr;
    Toggle* freeze = nullptr;
    std::unique_ptr<ScopeDisplay> scope;
    juce::Label* commonCaption = nullptr;
    juce::Label* modelCaption = nullptr;
    juce::Label* nameLabel = nullptr;
    std::map<juce::String, Knob*> byId;
    std::vector<Knob*> commonKnobs, modelKnobs;

    juce::String id (const char* suffix) const { return px + suffix; }
    Knob* make (const juce::String& paramId, const juce::String& name, Knob::ModMapping mapping = {}, int diameter = theme::knobSizeSmall);
    void parameterChanged (const juce::String&, float) override
    {
        if (juce::MessageManager::getInstance()->isThisTheMessageThread()) updateVisibility();
        else triggerAsyncUpdate();
    }
    void handleAsyncUpdate() override { updateVisibility(); }
    void updateVisibility();
};
} // namespace aeriform
