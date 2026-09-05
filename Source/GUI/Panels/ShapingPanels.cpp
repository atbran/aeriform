#include "ShapingPanels.h"

namespace aeriform
{
// ---------------------------------------------------------------------------
InteractionPanel::InteractionPanel (AeriformProcessor& p) : ParamPanel (p, "INTERACTION", theme::copper)
{
    using namespace ids;
    mode        = control<ChoiceBox> (processor, mixMode, "Mode");
    dcBlock     = control<Toggle> (processor, mixDcBlock, "DC Block");
    sync        = control<Toggle> (processor, exbSync, "Sync B>A");
    interaction = knob (mixInteraction, "Interaction", additive (ModDest::Interaction), theme::knobSizeLarge);
    depth       = knob (mixDepth, "Depth");
    balance     = knob (mixBalance, "Balance", additive (ModDest::Balance));
    b2a         = knob (mixB2A, "B > A");
    a2b         = knob (mixA2B, "A > B");
    normalize   = knob (mixNormalize, "Normalize");
    drive       = knob (mixDrive, "Drive", additive (ModDest::PreDrive));
    hint        = caption ("");
    hint->setFont (theme::font (10.0f));
    hint->setColour (juce::Label::textColourId, theme::textDim);
    processor.getAPVTS().addParameterListener (mixMode, this);
    handleAsyncUpdate();
}

InteractionPanel::~InteractionPanel()
{
    processor.getAPVTS().removeParameterListener (ids::mixMode, this);
    cancelPendingUpdate();
}

void InteractionPanel::handleAsyncUpdate()
{
    static const char* hints[] = {
        "Crossfade: Interaction pans between A (0 %) and B (100 %).",
        "Add: A + B. Interaction sets the level of B relative to A.",
        "Subtract: A - B. Interaction sets the level of B.",
        "Ring: A x B four-quadrant multiplication. Interaction blends towards the product.",
        "AM: B modulates A's amplitude. Interaction = modulation depth.",
        "FM: B modulates A's frequency at audio rate. Interaction = index (also B > A).",
        "PM: B modulates A's phase. Interaction = index (also B > A).",
        "Sync: B's cycle hard-syncs A. Interaction = sync pitch offset.",
        "XOR: bitwise-style combination of A and B (digital, harsh). Interaction = bit depth.",
        "Min / Max: Interaction morphs from the minimum to the maximum of A and B.",
        "Rectified Diff: |A - B| with polarity restored. Interaction = rectification amount.",
        "Sample & Hold: B samples A on its zero crossings. Interaction = hold blend.",
        "Audio-rate Crossfade: B's waveform steers the A / B crossfade. Interaction = steering depth."
    };
    auto* v = processor.getAPVTS().getRawParameterValue (ids::mixMode);
    const int m = juce::jlimit (0, (int) InteractionMode::Count - 1, v != nullptr ? (int) v->load() : 0);
    hint->setText (hints[m], juce::dontSendNotification);
}

void InteractionPanel::resized()
{
    auto r = getContentArea();
    auto head = r.removeFromTop (40);
    mode->setBounds (head.removeFromLeft (140));
    head.removeFromLeft (8);
    dcBlock->setBounds (head.removeFromLeft (92).withTrimmedTop (14));
    sync->setBounds (head.withTrimmedTop (14));
    r.removeFromTop (2);
    hint->setBounds (r.removeFromTop (16));
    r.removeFromTop (2);
    knobRow (r.removeFromTop (80), { interaction, depth, balance, b2a, a2b, normalize, drive });
}

// ---------------------------------------------------------------------------
PreShaperPanel::PreShaperPanel (AeriformProcessor& p) : ParamPanel (p, "PRE-SHAPER", theme::copper)
{
    using namespace ids;
    type      = control<ChoiceBox> (processor, preType, "Filter");
    order     = control<ChoiceBox> (processor, preOrder, "Order");
    lp        = knob (excLowpass, "Low-Pass", exponential (ModDest::ExciterLP, 4.0f));
    hp        = knob (excHighpass, "High-Pass", exponential (ModDest::ExciterHP, 4.0f));
    keytrack  = knob (excKeyTrack, "Key Track");
    res       = knob (preRes, "Resonance");
    drive     = knob (preDrive, "Drive", additive (ModDest::PreDrive));
    bias      = knob (preBias, "Bias");
    slew      = knob (preSlew, "Slew");
    transient = knob (preTransient, "Transient");
    env       = knob (preEnv, "Envelope");
}

void PreShaperPanel::resized()
{
    auto r = getContentArea();
    auto head = r.removeFromTop (40);
    type->setBounds (head.removeFromLeft (170));
    head.removeFromLeft (10);
    order->setBounds (head.removeFromLeft (190));
    r.removeFromTop (20);
    knobRow (r.removeFromTop (80), { lp, hp, keytrack, res, drive, bias, slew, transient, env });
}

// ---------------------------------------------------------------------------
WavefolderPanel::WavefolderPanel (AeriformProcessor& p) : ParamPanel (p, "WAVEFOLDER", theme::folder)
{
    using namespace ids;
    on       = control<Toggle> (processor, wfOn, "Folder On");
    mode     = control<ChoiceBox> (processor, wfMode, "Mode");
    fold     = knob (wfFold, "Fold", additive (ModDest::Fold), theme::knobSizeLarge);
    drive    = knob (wfDrive, "Drive", additive (ModDest::FoldDrive));
    symmetry = knob (wfSymmetry, "Symmetry", additive (ModDest::FoldSymmetry));
    bias     = knob (wfBias, "Bias", additive (ModDest::FoldBias));
    stages   = knob (wfStages, "Stages");
    shape    = knob (wfShape, "Shape");
    mix      = knob (wfMix, "Mix");
    comp     = knob (wfComp, "Comp");
    postLp   = knob (wfLp, "Post LP");
    dynamics = knob (dynAmount, "Dynamics");
    for (auto& k : knobs) k->setAccentColour (theme::folder);
    display = std::make_unique<FoldCurveDisplay> (processor);
    addAndMakeVisible (*display);
}

void WavefolderPanel::resized()
{
    auto r = getContentArea();
    display->setBounds (r.removeFromRight (300));
    r.removeFromRight (10);
    auto head = r.removeFromTop (40);
    on->setBounds (head.removeFromLeft (110).withTrimmedTop (14));
    head.removeFromLeft (10);
    mode->setBounds (head.removeFromLeft (190));
    r.removeFromTop (8);
    knobRow (r.removeFromTop (84), { fold, drive, symmetry, bias, stages, shape, mix, comp, postLp, dynamics });
}
} // namespace aeriform
