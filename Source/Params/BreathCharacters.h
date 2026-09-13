#pragma once
#include "ParameterLayout.h"
namespace aeriform {
inline constexpr P breathCharacterFields[]{P::excNoise,P::excNoiseColor,P::excTurbulence,P::breathMouth,P::breathSwell,P::breathSettle,P::breathContour,P::breathEdge,P::excAttackClick,P::excReleaseNoise,P::excBreathRandom};
inline constexpr const char* breathCharacterNames[]{"Soft Exhale","Focused Jet","Whisper","Rough Air","Flute Air"};
inline constexpr float breathCharacters[5][11]{
 {.6f,.35f,.12f,.35f,65,280,.4f,0,.02f,.025f,.15f},
 {.65f,.1f,.2f,.7f,25,150,.3f,.4f,.035f,.02f,.08f},
 {.65f,.15f,.08f,.6f,100,400,.2f,.25f,.005f,.015f,.12f},
 {.6f,.45f,.75f,.5f,45,250,.55f,.15f,.025f,.04f,.35f},
 {.65f,.15f,.18f,.55f,40,220,.5f,.85f,.025f,.02f,.1f}
};
}
