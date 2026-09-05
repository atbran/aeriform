#include "LookAndFeel.h"
#include "Theme.h"

namespace aeriform
{
using namespace theme;

AeriformLookAndFeel::AeriformLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, background);
    setColour (juce::Slider::rotarySliderFillColourId, copper);
    setColour (juce::Slider::rotarySliderOutlineColourId, knobTrack);
    setColour (juce::Slider::thumbColourId, copperBright);
    setColour (juce::Slider::trackColourId, copper);
    setColour (juce::Slider::backgroundColourId, knobTrack);
    setColour (juce::Slider::textBoxTextColourId, textPrimary);
    setColour (juce::Slider::textBoxBackgroundColourId, inset);
    setColour (juce::Slider::textBoxOutlineColourId, panelBorder);
    setColour (juce::ComboBox::backgroundColourId, inset);
    setColour (juce::ComboBox::textColourId, textPrimary);
    setColour (juce::ComboBox::outlineColourId, panelBorder);
    setColour (juce::ComboBox::arrowColourId, copperBright);
    setColour (juce::PopupMenu::backgroundColourId, panelRaised);
    setColour (juce::PopupMenu::textColourId, textPrimary);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, copperDim);
    setColour (juce::PopupMenu::highlightedTextColourId, textPrimary);
    setColour (juce::PopupMenu::headerTextColourId, brass);
    setColour (juce::TextButton::buttonColourId, panelRaised);
    setColour (juce::TextButton::buttonOnColourId, copperDim);
    setColour (juce::TextButton::textColourOffId, textPrimary);
    setColour (juce::TextButton::textColourOnId, textPrimary);
    setColour (juce::ToggleButton::textColourId, textPrimary);
    setColour (juce::ToggleButton::tickColourId, copperBright);
    setColour (juce::ToggleButton::tickDisabledColourId, textDim);
    setColour (juce::Label::textColourId, textPrimary);
    setColour (juce::TooltipWindow::backgroundColourId, panelRaised);
    setColour (juce::TooltipWindow::textColourId, textPrimary);
    setColour (juce::TooltipWindow::outlineColourId, copperDim);
    setColour (juce::TextEditor::backgroundColourId, inset);
    setColour (juce::TextEditor::textColourId, textPrimary);
    setColour (juce::TextEditor::highlightColourId, copperDim);
    setColour (juce::TextEditor::outlineColourId, panelBorder);
    setColour (juce::TextEditor::focusedOutlineColourId, copper);
    setColour (juce::CaretComponent::caretColourId, copperBright);
    setColour (juce::AlertWindow::backgroundColourId, panel);
    setColour (juce::AlertWindow::textColourId, textPrimary);
    setColour (juce::AlertWindow::outlineColourId, panelBorder);
    setColour (juce::ScrollBar::thumbColourId, knobRim);
    setColour (juce::ListBox::backgroundColourId, inset);
}

// ---------------------------------------------------------------------------
void AeriformLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                                            float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    const float radius = size * 0.5f;
    const float trackRadius = radius - 3.0f;
    const float trackWidth = juce::jmax (2.0f, size * 0.075f);
    const float angle = startAngle + pos * (endAngle - startAngle);
    const bool enabled = slider.isEnabled();
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;

    // track
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, trackRadius, trackRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (knobTrack);
    g.strokePath (track, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc
    const float zeroPos = bipolar ? (float) ((0.0 - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum())) : 0.0f;
    const float zeroAngle = startAngle + zeroPos * (endAngle - startAngle);
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, trackRadius, trackRadius, 0.0f, juce::jmin (zeroAngle, angle), juce::jmax (zeroAngle, angle), true);
    g.setColour (enabled ? slider.findColour (juce::Slider::rotarySliderFillColourId) : copperDim);
    g.strokePath (value, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // body: radial shading
    const float bodyRadius = radius - trackWidth - 5.0f;
    juce::ColourGradient body (knobBody.brighter (0.25f), centre.x - bodyRadius * 0.4f, centre.y - bodyRadius * 0.5f,
                               knobBody.darker (0.45f), centre.x + bodyRadius * 0.6f, centre.y + bodyRadius * 0.7f, true);
    g.setGradientFill (body);
    g.fillEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);
    g.setColour (knobRim);
    g.drawEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.0f);

    // pointer
    juce::Path pointer;
    const float pointerLength = bodyRadius * 0.62f;
    const float pointerWidth = juce::jmax (2.0f, size * 0.055f);
    pointer.addRoundedRectangle (-pointerWidth * 0.5f, -bodyRadius + 2.0f, pointerWidth, pointerLength, pointerWidth * 0.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (enabled ? knobPointer : textDim);
    g.fillPath (pointer);
}

void AeriformLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                            float, float, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    const bool horizontal = style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar;
    const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float trackThickness = 4.0f;
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;

    juce::Rectangle<float> track = horizontal ? juce::Rectangle<float> (area.getX(), area.getCentreY() - trackThickness * 0.5f, area.getWidth(), trackThickness)
                                              : juce::Rectangle<float> (area.getCentreX() - trackThickness * 0.5f, area.getY(), trackThickness, area.getHeight());
    g.setColour (knobTrack);
    g.fillRoundedRectangle (track, 2.0f);

    float zeroPos = horizontal ? area.getX() : area.getBottom();
    if (bipolar)
    {
        const float t = (float) ((0.0 - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum()));
        zeroPos = horizontal ? area.getX() + t * area.getWidth() : area.getBottom() - t * area.getHeight();
        g.setColour (textDim);
        if (horizontal) g.fillRect (juce::Rectangle<float> (zeroPos - 0.5f, area.getY() + 2.0f, 1.0f, area.getHeight() - 4.0f));
        else            g.fillRect (juce::Rectangle<float> (area.getX() + 2.0f, zeroPos - 0.5f, area.getWidth() - 4.0f, 1.0f));
    }

    juce::Rectangle<float> fill = horizontal ? juce::Rectangle<float> (juce::jmin (zeroPos, sliderPos), track.getY(), std::fabs (sliderPos - zeroPos), trackThickness)
                                             : juce::Rectangle<float> (track.getX(), juce::jmin (zeroPos, sliderPos), trackThickness, std::fabs (sliderPos - zeroPos));
    g.setColour (slider.isEnabled() ? slider.findColour (juce::Slider::trackColourId) : copperDim);
    g.fillRoundedRectangle (fill, 2.0f);

    const float r = (float) getSliderThumbRadius (slider);
    const auto thumbCentre = horizontal ? juce::Point<float> (sliderPos, area.getCentreY()) : juce::Point<float> (area.getCentreX(), sliderPos);
    g.setColour (knobBody.brighter (0.3f));
    g.fillEllipse (thumbCentre.x - r, thumbCentre.y - r, r * 2.0f, r * 2.0f);
    g.setColour (slider.isEnabled() ? copperBright : textDim);
    g.drawEllipse (thumbCentre.x - r, thumbCentre.y - r, r * 2.0f, r * 2.0f, 1.5f);
}

// ---------------------------------------------------------------------------
void AeriformLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (box.hasKeyboardFocus (true) ? copper : box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    const float ah = 4.0f, aw = 7.0f;
    const float ax = bounds.getRight() - 10.0f - aw * 0.5f, ay = bounds.getCentreY();
    juce::Path arrow;
    arrow.addTriangle (ax - aw * 0.5f, ay - ah * 0.5f, ax + aw * 0.5f, ay - ah * 0.5f, ax, ay + ah * 0.5f);
    g.setColour (box.isEnabled() ? box.findColour (juce::ComboBox::arrowColourId) : textDim);
    g.fillPath (arrow);
}

juce::Font AeriformLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return font (juce::jmin (12.5f, (float) box.getHeight() * 0.62f));
}

void AeriformLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void AeriformLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (panelRaised);
    g.setColour (panelBorder);
    g.drawRect (0, 0, width, height, 1);
}

juce::Font AeriformLookAndFeel::getPopupMenuFont() { return font (13.0f); }

void AeriformLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                                             bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                                             const juce::String& shortcutKeyText, const juce::Drawable*, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        g.setColour (panelBorder);
        g.fillRect (area.reduced (8, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }
    auto r = area.reduced (1);
    if (isHighlighted && isActive)
    {
        g.setColour (copperDim.withAlpha (0.8f));
        g.fillRoundedRectangle (r.toFloat(), 3.0f);
    }
    g.setColour (textColour != nullptr ? *textColour : (isActive ? textPrimary : textDim));
    g.setFont (getPopupMenuFont());
    auto textArea = r.reduced (10, 0);
    if (isTicked)
    {
        g.setColour (copperBright);
        g.fillEllipse (juce::Rectangle<float> ((float) textArea.getX(), (float) textArea.getCentreY() - 3.0f, 6.0f, 6.0f));
        g.setColour (isActive ? textPrimary : textDim);
    }
    textArea.removeFromLeft (14);
    g.drawFittedText (text, textArea, juce::Justification::centredLeft, 1);
    if (shortcutKeyText.isNotEmpty())
    {
        g.setColour (textSecondary);
        g.drawText (shortcutKeyText, textArea, juce::Justification::centredRight);
    }
    if (hasSubMenu)
    {
        const float ax = (float) r.getRight() - 12.0f, ay = (float) r.getCentreY();
        juce::Path p;
        p.addTriangle (ax - 3.0f, ay - 4.0f, ax - 3.0f, ay + 4.0f, ax + 3.0f, ay);
        g.setColour (copperBright);
        g.fillPath (p);
    }
}

void AeriformLookAndFeel::drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& name)
{
    g.setColour (brass);
    g.setFont (font (11.0f, true));
    g.drawText (name.toUpperCase(), area.reduced (10, 0), juce::Justification::centredLeft);
}

// ---------------------------------------------------------------------------
void AeriformLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& bg, bool isMouseOver, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    juce::Colour c = bg;
    if (button.getToggleState()) c = copperDim;
    if (isMouseOver) c = c.brighter (0.12f);
    if (isButtonDown) c = c.darker (0.2f);
    g.setColour (c);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (button.getToggleState() ? copper : panelBorder.brighter (0.15f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

juce::Font AeriformLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return font (juce::jmin (12.5f, (float) buttonHeight * 0.55f), true);
}

void AeriformLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool isMouseOver, bool)
{
    auto bounds = button.getLocalBounds().toFloat();
    const float h = juce::jmin (16.0f, bounds.getHeight() - 4.0f);
    const float w = h * 1.9f;
    auto sw = juce::Rectangle<float> (bounds.getX() + 2.0f, bounds.getCentreY() - h * 0.5f, w, h);
    const bool on = button.getToggleState();
    g.setColour (on ? copperDim : inset);
    g.fillRoundedRectangle (sw, h * 0.5f);
    g.setColour (on ? copper : (isMouseOver ? knobRim.brighter (0.2f) : knobRim));
    g.drawRoundedRectangle (sw, h * 0.5f, 1.0f);
    const float kr = h * 0.5f - 2.0f;
    const float kx = on ? sw.getRight() - kr - 2.0f : sw.getX() + kr + 2.0f;
    g.setColour (on ? copperBright : textSecondary);
    g.fillEllipse (kx - kr, sw.getCentreY() - kr, kr * 2.0f, kr * 2.0f);

    g.setColour (button.isEnabled() ? textPrimary : textDim);
    g.setFont (font (11.5f));
    g.drawFittedText (button.getButtonText(), bounds.withTrimmedLeft ((int) (w + 8.0f)).toNearestInt(), juce::Justification::centredLeft, 1);
}

// ---------------------------------------------------------------------------
juce::Rectangle<int> AeriformLookAndFeel::getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea)
{
    const juce::TextLayout tl = [&]
    {
        juce::AttributedString s;
        s.setJustification (juce::Justification::centredLeft);
        s.append (tipText, font (12.5f), textPrimary);
        juce::TextLayout l;
        l.createLayout (s, 320.0f);
        return l;
    }();
    const int w = (int) std::ceil (tl.getWidth()) + 20, h = (int) std::ceil (tl.getHeight()) + 14;
    return juce::Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                                 screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6, w, h)
               .constrainedWithin (parentArea);
}

void AeriformLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    g.setColour (panelRaised);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (copperDim);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    juce::AttributedString s;
    s.setJustification (juce::Justification::centredLeft);
    s.append (text, font (12.5f), textPrimary);
    juce::TextLayout tl;
    tl.createLayout (s, 320.0f);
    tl.draw (g, r.reduced (10.0f, 7.0f));
}

juce::Font AeriformLookAndFeel::getLabelFont (juce::Label& label)
{
    return label.getFont();
}

void AeriformLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour (juce::Label::backgroundColourId));
    if (! label.isBeingEdited())
    {
        g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (label.isEnabled() ? 1.0f : 0.5f));
        g.setFont (label.getFont());
        g.drawFittedText (label.getText(), label.getBorderSize().subtractedFrom (label.getLocalBounds()),
                          label.getJustificationType(), juce::jmax (1, (int) ((float) label.getHeight() / label.getFont().getHeight())),
                          label.getMinimumHorizontalScale());
    }
}

void AeriformLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor& editor)
{
    g.setColour (editor.findColour (juce::TextEditor::backgroundColourId));
    g.fillRoundedRectangle (juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height), 3.0f);
}

void AeriformLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& editor)
{
    g.setColour (editor.hasKeyboardFocus (true) ? copper : panelBorder);
    g.drawRoundedRectangle (juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f), 3.0f, 1.0f);
}

// ---------------------------------------------------------------------------
void AeriformLookAndFeel::drawSectionPanel (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title, juce::Colour accent)
{
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, cornerRadius);
    // subtle top highlight and bottom shade for depth
    g.setColour (juce::Colours::white.withAlpha (0.035f));
    g.fillRoundedRectangle (bounds.withHeight (bounds.getHeight() * 0.5f), cornerRadius);
    g.setColour (panelBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerRadius, 1.0f);

    auto strip = bounds.withHeight ((float) sectionTitleHeight);
    g.setColour (accent.withAlpha (0.85f));
    g.fillRect (juce::Rectangle<float> (strip.getX() + 10.0f, strip.getBottom() - 1.0f, 26.0f, 2.0f));
    g.setColour (textPrimary);
    g.setFont (titleFont (12.0f));
    g.drawText (title, strip.reduced (10.0f, 0.0f), juce::Justification::centredLeft);
    g.setColour (panelBorder);
    g.fillRect (juce::Rectangle<float> (strip.getX() + 40.0f, strip.getBottom() - 1.0f, strip.getWidth() - 50.0f, 1.0f));
}
} // namespace aeriform
