#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Visual identity of AERIFORM: dark industrial-scientific, graphite panels,
// muted copper/brass controls, cool teal for modulation and airflow.
namespace aeriform::theme
{
inline const juce::Colour background   { 0xff17191d };
inline const juce::Colour panel        { 0xff1f2227 };
inline const juce::Colour panelRaised  { 0xff262a30 };
inline const juce::Colour panelBorder  { 0xff2f343b };
inline const juce::Colour inset        { 0xff121417 };
inline const juce::Colour grid         { 0xff2a2e34 };

inline const juce::Colour textPrimary  { 0xffd9dce0 };
inline const juce::Colour textSecondary{ 0xff8f959d };
inline const juce::Colour textDim      { 0xff5d636b };

inline const juce::Colour copper       { 0xffc27a3f };
inline const juce::Colour copperBright { 0xffe0a35a };
inline const juce::Colour copperDim    { 0xff7a4d29 };
inline const juce::Colour brass        { 0xffd8b46a };

inline const juce::Colour teal         { 0xff3fb8c4 };
inline const juce::Colour tealBright   { 0xff6fe0ea };
inline const juce::Colour tealDim      { 0xff23636a };

inline const juce::Colour amber        { 0xffe6a23c };
inline const juce::Colour danger       { 0xffd9534f };

inline const juce::Colour knobBody     { 0xff2c3036 };
inline const juce::Colour knobRim      { 0xff3d424a };
inline const juce::Colour knobTrack    { 0xff35393f };
inline const juce::Colour knobPointer  { 0xffe8e2d6 };

// v2.1 accents: exciter slots and resonator nodes
inline const juce::Colour exciterA     { 0xffe0a35a };
inline const juce::Colour exciterB     { 0xffd87a6a };
inline const juce::Colour nodeA        { 0xffd8b46a };
inline const juce::Colour nodeB        { 0xff8fbf7f };
inline const juce::Colour nodeC        { 0xff6fb8e0 };
inline const juce::Colour folder       { 0xffc98ad6 };

// Logical layout constants (the editor scales everything uniformly)
inline constexpr int editorWidth  = 1180;
inline constexpr int editorHeight = 820;
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
