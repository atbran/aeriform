#pragma once

#include "../PanelBase.h"
#include "../Displays.h"

namespace aeriform
{
/** INTERACTION: how exciters A and B combine (mode, amount, depth, balance, cross-modulation). */
class InteractionPanel : public ParamPanel,
                         private juce::AudioProcessorValueTreeState::Listener,
                         private juce::AsyncUpdater
{
public:
    explicit InteractionPanel (AeriformProcessor&);
    ~InteractionPanel() override;
    void resized() override;

private:
    ChoiceBox* mode;
    Toggle *dcBlock, *sync;
    Knob *interaction, *depth, *balance, *b2a, *a2b, *normalize, *drive;
    juce::Label* hint;
    void parameterChanged (const juce::String&, float) override
    {
        if (juce::MessageManager::getInstance()->isThisTheMessageThread()) handleAsyncUpdate();
        else triggerAsyncUpdate();
    }
    void handleAsyncUpdate() override;
};

/** PRE-SHAPER: filters (the classic exciter low / high-pass), resonance, drive, bias, slew, transient emphasis, envelope amount. */
class PreShaperPanel : public ParamPanel
{
public:
    explicit PreShaperPanel (AeriformProcessor&);
    void resized() override;

private:
    ChoiceBox *type, *order;
    Knob *lp, *hp, *keytrack, *res, *drive, *bias, *slew, *transient, *env;
};

/** WAVEFOLDER: the oversampled folder with its transfer-curve display, plus the post-fold dynamics normaliser. */
class WavefolderPanel : public ParamPanel
{
public:
    explicit WavefolderPanel (AeriformProcessor&);
    void resized() override;

private:
    Toggle* on;
    ChoiceBox* mode;
    Knob *fold, *drive, *symmetry, *bias, *stages, *shape, *mix, *comp, *postLp, *dynamics;
    std::unique_ptr<FoldCurveDisplay> display;
};
} // namespace aeriform
