#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Visual identity of AERIFORM: dark industrial-scientific, graphite panels,
// blue primary controls, warm resonator accents and sea-glass modulation. Color identifiers remain stable for existing panels.
namespace aeriform::theme
{
inline const juce::Colour background   { 0xff111820 };
inline const juce::Colour panel        { 0xff1b2430 };
inline const juce::Colour panelRaised  { 0xff253140 };
inline const juce::Colour panelBorder  { 0xff344253 };
inline const juce::Colour inset        { 0xff0e141c };
inline const juce::Colour grid         { 0xff273544 };

inline const juce::Colour textPrimary  { 0xffedf0f3 };
inline const juce::Colour textSecondary{ 0xffacb9c8 };
inline const juce::Colour textDim      { 0xff8191a4 };

inline const juce::Colour copper       { 0xff73a8ed };
inline const juce::Colour copperBright { 0xffa6ccff };
inline const juce::Colour copperDim    { 0xff304d70 };
inline const juce::Colour brass        { 0xffcbb895 };

inline const juce::Colour teal         { 0xff73c9ba };
inline const juce::Colour tealBright   { 0xffa2e3d5 };
inline const juce::Colour tealDim      { 0xff315f5a };

inline const juce::Colour amber        { 0xffe6a23c };
inline const juce::Colour danger       { 0xffd9534f };

inline const juce::Colour knobBody     { 0xff2d3a49 };
inline const juce::Colour knobRim      { 0xff4b5c70 };
inline const juce::Colour knobTrack    { 0xff3b4b5e };
inline const juce::Colour knobPointer  { 0xfff1f3f7 };

// v2.1 accents: exciter slots and resonator nodes
inline const juce::Colour exciterA     { 0xffa6ccff };
inline const juce::Colour exciterB     { 0xffb6a2df };
inline const juce::Colour nodeA        { 0xffcbb895 };
inline const juce::Colour nodeB        { 0xff8ebce3 };
inline const juce::Colour nodeC        { 0xffb0a1d6 };
inline const juce::Colour folder       { 0xffb6a2df };

// Logical layout constants (the editor scales everything uniformly)
inline constexpr int editorWidth  = 1280;
inline constexpr int editorHeight = 900;
inline constexpr int knobSize     = 58;
inline constexpr int knobSizeLarge= 72;
inline constexpr int knobSizeSmall= 46;
inline constexpr int sectionTitleHeight = 22;
inline constexpr int tabBarHeight = 28;
inline constexpr float cornerRadius = 6.0f;

inline juce::Font font (float size, bool bold = false)
{
    return juce::Font (juce::Font::getDefaultSansSerifFontName(), size, bold ? juce::Font::bold : juce::Font::plain);
}

inline juce::Font titleFont (float size) { return juce::Font (juce::Font::getDefaultSansSerifFontName(), size, juce::Font::bold); }
inline juce::Font monoFont (float size)  { return juce::Font (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain); }
} // namespace aeriform::theme
