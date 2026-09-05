#include "TestFramework.h"
#include "TestHelpers.h"
#include "Params/ParameterLayout.h"
#include <set>

using namespace aeriform;
using namespace aeriform::test;

AERIFORM_TEST (parameter_ids_are_unique_and_all_present)
{
    TestHost host;
    auto& apvts = host.processor.getAPVTS();
    std::set<juce::String> seen;
    for (auto* p : host.processor.getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p);
        CHECK (rp != nullptr);
        if (rp == nullptr) continue;
        CHECK_MSG (seen.insert (rp->paramID).second, ("duplicate id " + rp->paramID).toStdString());
    }
    const char* required[] = { ids::excNoise, ids::excPressure, ids::excReed, ids::envAttack, ids::resFeedback, ids::resMode,
                               ids::chorusMix, ids::delayMix, ids::reverbMix, ids::voiceMode, ids::voiceCount, ids::outGain,
                               ids::mpeEnabled, ids::limiterOn, ids::artCoupling, ids::resBodyTrack };
    for (auto* id : required)
        CHECK_MSG (apvts.getParameter (id) != nullptr, std::string ("missing ") + id);
    for (int i = 1; i <= ids::numLFOs; ++i)
        CHECK (apvts.getParameter (ids::lfoParam (i, ids::lfoRateSuffix)) != nullptr);
    for (int i = 1; i <= ids::numModSlots; ++i)
    {
        CHECK (apvts.getParameter (ids::modParam (i, ids::modSrcSuffix)) != nullptr);
        CHECK (apvts.getParameter (ids::modParam (i, ids::modDstSuffix)) != nullptr);
        CHECK (apvts.getParameter (ids::modParam (i, ids::modDepthSuffix)) != nullptr);
    }
    CHECK (parameterInfos().size() == seen.size());
    CHECK (seen.size() > 100);
}

AERIFORM_TEST (parameter_defaults_round_trip_through_normalisation)
{
    TestHost host;
    for (auto* p : host.processor.getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p);
        if (rp == nullptr) continue;
        const float def = rp->getDefaultValue();
        const float dsp = rp->convertFrom0to1 (def);
        const float back = rp->convertTo0to1 (dsp);
        CHECK_MSG (std::fabs (back - def) < 1.0e-4f, rp->paramID.toStdString());
        const auto* info = findParamInfo (rp->paramID);
        CHECK (info != nullptr);
        if (info != nullptr && ! info->isChoice && ! info->isBool)
            CHECK_MSG (std::fabs (info->defaultValue - dsp) < 1.0e-3f * std::max (1.0f, std::fabs (dsp)), rp->paramID.toStdString());
    }
}

AERIFORM_TEST (parameter_text_formatting_uses_units)
{
    TestHost host;
    auto& apvts = host.processor.getAPVTS();
    auto text = [&] (const char* id, float dsp)
    {
        auto* p = apvts.getParameter (id);
        return p->getText (p->convertTo0to1 (dsp), 64).toStdString();
    };
    CHECK (text (ids::excLowpass, 7000.0f) == "7.00 kHz");
    CHECK (text (ids::excHighpass, 40.0f) == "40.0 Hz");
    CHECK (text (ids::envAttack, 25.0f) == "25.0 ms");
    CHECK (text (ids::envRelease, 2500.0f) == "2.50 s");
    CHECK (text (ids::resFeedback, 0.9f) == "90 %");
    CHECK (text (ids::resCoarse, -12.0f) == "-12 st");
    CHECK (text (ids::resFine, 25.0f) == "+25 ct");
    CHECK (text (ids::outGain, -6.0f) == "-6.0 dB");
    CHECK (text (ids::resMode, 1.0f) == "Closed Pipe");
}

AERIFORM_TEST (log_ranges_place_centre_at_half)
{
    TestHost host;
    auto* p = host.processor.getAPVTS().getParameter (ids::excLowpass);
    const float centre = p->convertFrom0to1 (0.5f);
    CHECK_NEAR (centre, 3000.0, 30.0);
    auto* t = host.processor.getAPVTS().getParameter (ids::envAttack);
    CHECK_NEAR (t->convertFrom0to1 (0.5f), 200.0, 2.0);
}

AERIFORM_TEST (choice_lists_match_enums)
{
    CHECK (choices::lfoShapes().size() == (int) LfoShape::Count);
    CHECK (choices::lfoModes().size() == (int) LfoMode::Count);
    CHECK (choices::resModes().size() == (int) ResMode::Count);
    CHECK (choices::voiceModes().size() == (int) VoiceMode::Count);
    CHECK (choices::modSources().size() == (int) ModSource::Count);
    CHECK (choices::modDests().size() == (int) ModDest::Count);
    CHECK (choices::syncDivisions().size() == 17);
    CHECK_NEAR (choices::syncDivisionBeats (7), 1.0, 1.0e-9);        // 1/4 = one beat
    CHECK_NEAR (choices::syncDivisionBeats (11), 0.75, 1.0e-9);      // 1/8 dotted
}

AERIFORM_TEST (parameters_reach_the_dsp)
{
    // A parameter change must audibly change the output: output gain -60 dB vs 0 dB.
    TestHost host;
    host.set (ids::reverbMix, 0.0f);
    host.noteOn (60);
    const auto loud = host.render (0.5);
    host.set (ids::outGain, -60.0f);
    host.render (0.3);   // let the gain smoother settle
    const auto quiet = host.render (0.5);
    CHECK (loud.rms > 1.0e-3);
    CHECK (quiet.rms < loud.rms * 0.01);
}
