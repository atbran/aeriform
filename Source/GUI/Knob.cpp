#include "Knob.h"
#include "Theme.h"
#include "GuiDiagnostics.h"

namespace aeriform
{
using namespace theme;

Knob::Knob (AeriformProcessor& p, const juce::String& id, ModMapping m, int diam)
    : processor (p), paramID (id), mapping (m), diameter (diam), slider (*this)
{
    param = processor.getAPVTS().getParameter (paramID);
    info = findParamInfo (paramID);
    jassert (param != nullptr && info != nullptr);
    if (param == nullptr || info == nullptr) ++gui::unboundControlCount();

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    slider.setMouseDragSensitivity (220);
    slider.setVelocityModeParameters (0.6, 1, 0.08, true, juce::ModifierKeys::ctrlModifier);
    slider.setScrollWheelEnabled (true);
    slider.setPopupDisplayEnabled (false, false, nullptr);
    if (param != nullptr)
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    if (info != nullptr)
        slider.setTooltip (info->tooltip);
    slider.addListener (this);
    addAndMakeVisible (slider);

    label.setJustificationType (juce::Justification::centred);
    label.setFont (font (11.0f));
    label.setColour (juce::Label::textColourId, textSecondary);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);
    showName();

    if (param != nullptr)
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.getAPVTS(), paramID, slider);
}

Knob::~Knob()
{
    slider.removeListener (this);
    attachment.reset();
}

void Knob::setDisplayName (const juce::String& name)
{
    displayName = name;
    showName();
}

void Knob::showName()
{
    showingValue = false;
    label.setText (displayName.isNotEmpty() ? displayName : (info != nullptr ? info->name : paramID), juce::dontSendNotification);
    label.setColour (juce::Label::textColourId, textSecondary);
}

void Knob::showValue()
{
    showingValue = true;
    if (param != nullptr)
        label.setText (param->getText (param->getValue(), 24), juce::dontSendNotification);
    label.setColour (juce::Label::textColourId, copperBright);
}

void Knob::sliderValueChanged (juce::Slider*)
{
    if (showingValue) showValue();
    else { showValue(); startTimer (1100); }
}

void Knob::sliderDragStarted (juce::Slider*) { stopTimer(); showValue(); }
void Knob::sliderDragEnded (juce::Slider*)   { startTimer (900); }
void Knob::timerCallback() { stopTimer(); showName(); repaint(); }

// ---------------------------------------------------------------------------
void Knob::KnobSlider::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        owner.showContextMenu();
        return;
    }
    // Shift = fine adjustment (8x more travel per value change). Ctrl toggles velocity mode (JUCE built-in).
    setMouseDragSensitivity (e.mods.isShiftDown() ? 1800 : 220);
    juce::Slider::mouseDown (e);
}

void Knob::KnobSlider::mouseDrag (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    juce::Slider::mouseDrag (e);
}

void Knob::KnobSlider::mouseUp (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    juce::Slider::mouseUp (e);
}

void Knob::showContextMenu()
{
    auto& learn = processor.getMidiLearn();
    const int mappedCC = learn.getMappedCC (paramID);
    const bool learning = learn.isLearning() && learn.getLearningParam() == paramID;

    juce::PopupMenu menu;
    menu.addSectionHeader (info != nullptr ? info->name : paramID);
    menu.addItem (1, learning ? "Learning... (move a MIDI controller)" : "MIDI Learn", ! learning);
    menu.addItem (2, mappedCC >= 0 ? "Clear MIDI mapping (CC " + juce::String (mappedCC) + ")" : "No MIDI mapping", mappedCC >= 0);
    menu.addSeparator();
    menu.addItem (3, "Reset to default");
    if (learning) menu.addItem (4, "Cancel learn");

    juce::Component::SafePointer<Knob> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int result)
    {
        if (safe == nullptr) return;
        auto& l = safe->processor.getMidiLearn();
        switch (result)
        {
            case 1: l.armLearn (safe->paramID); safe->repaint(); break;
            case 2: l.clearMapping (safe->paramID); safe->repaint(); break;
            case 3:
                if (safe->param != nullptr)
                {
                    safe->param->beginChangeGesture();
                    safe->param->setValueNotifyingHost (safe->param->getDefaultValue());
                    safe->param->endChangeGesture();
                }
                break;
            case 4: l.cancelLearn(); safe->repaint(); break;
            default: break;
        }
    });
}

// ---------------------------------------------------------------------------
float Knob::normFromDspValue (float dsp) const
{
    return param != nullptr ? param->convertTo0to1 (juce::jlimit (param->getNormalisableRange().start, param->getNormalisableRange().end, dsp)) : 0.0f;
}

float Knob::modulatedNorm (float amount) const
{
    if (param == nullptr) return 0.0f;
    const float norm = param->getValue();
    const float dsp = param->convertFrom0to1 (norm);
    switch (mapping.kind)
    {
        case ModMapping::Kind::Additive:
        {
            // destinations that add directly in 0..1 units
            const auto& r = param->getNormalisableRange();
            return normFromDspValue (dsp + amount * (r.end - r.start));
        }
        case ModMapping::Kind::Exponential:   return normFromDspValue (dsp * std::exp2 (amount * mapping.scale));
        case ModMapping::Kind::Semitones:     return normFromDspValue (dsp + amount * mapping.scale);
        case ModMapping::Kind::None:
        default: return norm;
    }
}

void Knob::updateModRing (const dsp::ModConfig& config, const std::array<float, (size_t) ModDest::Count>& live)
{
    if (mapping.dest == ModDest::None || param == nullptr)
        return;
    const float depth = dsp::ModMatrix::maxDepth (config, mapping.dest);
    const bool nowHasMod = depth > 0.0005f;
    const float norm = param->getValue();
    const float newDepthNorm = nowHasMod ? juce::jmax (std::fabs (modulatedNorm (depth) - norm), std::fabs (norm - modulatedNorm (-depth))) : 0.0f;
    const float liveValue = live[(size_t) mapping.dest];
    const float newLiveNorm = nowHasMod ? modulatedNorm (liveValue) - norm : 0.0f;

    if (nowHasMod != hasMod || std::fabs (newDepthNorm - modDepthNorm) > 0.002f || std::fabs (newLiveNorm - modLiveNorm) > 0.004f)
    {
        hasMod = nowHasMod;
        modDepthNorm = newDepthNorm;
        modLiveNorm = newLiveNorm;
        repaint();
    }
}

void Knob::resized()
{
    auto r = getLocalBounds();
    const int labelH = 15;
    auto knobArea = r.withTrimmedBottom (labelH);
    const int d = juce::jmin (diameter, juce::jmin (knobArea.getWidth(), knobArea.getHeight()));
    slider.setBounds (knobArea.withSizeKeepingCentre (d, d));
    label.setBounds (r.removeFromBottom (labelH));
}

void Knob::paint (juce::Graphics& g)
{
    // modulation ring around the knob + MIDI learn indicator
    auto& learn = processor.getMidiLearn();
    const bool learning = learn.isLearning() && learn.getLearningParam() == paramID;
    const int mappedCC = learn.getMappedCC (paramID);

    const auto sb = slider.getBounds().toFloat();
    const auto centre = sb.getCentre();
    const float radius = sb.getWidth() * 0.5f + 1.5f;
    const float start = juce::MathConstants<float>::pi * 1.2f, end = juce::MathConstants<float>::pi * 2.8f;

    if (hasMod && param != nullptr)
    {
        const float norm = param->getValue();
        const float a0 = start + juce::jlimit (0.0f, 1.0f, norm - modDepthNorm) * (end - start);
        const float a1 = start + juce::jlimit (0.0f, 1.0f, norm + modDepthNorm) * (end - start);
        juce::Path ring;
        ring.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, a0, a1, true);
        g.setColour (teal.withAlpha (0.55f));
        g.strokePath (ring, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float aLive = start + juce::jlimit (0.0f, 1.0f, norm + modLiveNorm) * (end - start);
        const auto dot = centre.getPointOnCircumference (radius, aLive);
        g.setColour (tealBright);
        g.fillEllipse (dot.x - 2.5f, dot.y - 2.5f, 5.0f, 5.0f);
    }

    if (learning)
    {
        g.setColour (amber.withAlpha (0.9f));
        g.drawEllipse (sb.expanded (2.0f), 2.0f);
        g.setFont (font (9.5f, true));
        g.drawText ("LEARN", getLocalBounds().removeFromTop (12), juce::Justification::centred);
    }
    else if (mappedCC >= 0)
    {
        g.setColour (teal.withAlpha (0.9f));
        g.setFont (font (9.0f, true));
        g.drawText ("CC" + juce::String (mappedCC), getLocalBounds().removeFromTop (11), juce::Justification::centredRight);
    }
}
} // namespace aeriform
