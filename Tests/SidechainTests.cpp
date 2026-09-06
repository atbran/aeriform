#include "TestFramework.h"
#include "TestHelpers.h"

using namespace aeriform;
using namespace aeriform::test;

AERIFORM_TEST (sidechain_input_excites_the_resonators)
{
    TestHost h (48000.0, 256);
    h.enableSidechain();               // aux Sidechain bus (bus 1), separate from the FX main input
    h.set (ids::fxMix, 1.0f);          // fully wet: the FX main input is silent here, so only the note path sounds
    h.set (ids::reverbMix, 0.0f);
    h.set (ids::delayMix, 0.0f);
    h.set (ids::chorusMix, 0.0f);
    h.set (ids::excReleaseNoise, 0.0f);
    h.set (ids::envRelease, 60.0f);
    h.set (ids::excNoise, 0.0f);
    h.set (ids::excPluck, 0.0f);
    h.set (ids::excAttackClick, 0.0f);
    h.set (ids::excPressure, 0.0f);
    h.set (ids::excExternalIn, 1.0f);
    h.set (ids::resFeedback, 0.95f);

    // no input, note held -> (almost) silence: nothing excites the tube
    h.noteOn (60);
    const auto silent = h.render (0.5);
    CHECK_MSG (silent.rms < 1.0e-4, "silent input gives silent tube: rms=" + std::to_string (silent.rms));

    // white-noise sidechain -> the tube rings at the played note
    dsp::Noise rng; rng.seed (99);
    h.sidechainSource = [&] (long) { return rng.next() * 0.5f; };
    h.render (0.3);
    std::vector<float> mono;
    const auto excited = h.render (0.5, &mono);
    CHECK_MSG (excited.rms > 1.0e-3, "sidechain excites the tube: rms=" + std::to_string (excited.rms));
    const double f = estimateFrequency (mono, 48000.0, dsp::midiNoteToHz (60.0f));
    CHECK_MSG (std::fabs (centsBetween (f, dsp::midiNoteToHz (60.0f))) < 15.0, "tube output is tuned to the note: " + std::to_string (f) + " Hz");

    // input with no note held is not passed through (the instrument resonators are note-gated)
    h.noteOff (60);
    h.render (0.6);
    const auto gated = h.render (0.3);
    CHECK_MSG (gated.rms < 1.0e-4, "no note -> no output even with input: rms=" + std::to_string (gated.rms));
    CHECK (excited.finite && gated.finite);
}

AERIFORM_TEST (fx_and_sidechain_bus_layouts_are_supported)
{
    AeriformProcessor p;
    // Aeriform FX exposes a real main input (bus 0) plus the aux Sidechain (bus 1).
    CHECK (p.getBusCount (true) == 2);
    CHECK (p.getBus (true, 0)->getName() == "Input");
    CHECK (p.getBus (true, 1)->getName() == "Sidechain");
    CHECK (p.getBus (false, 0)->getName() == "Output");

    auto layouts = p.getBusesLayout();
    CHECK (p.checkBusesLayoutSupported (layouts));

    // main input mono / stereo
    layouts.inputBuses.getReference (0) = juce::AudioChannelSet::mono();
    CHECK (p.checkBusesLayoutSupported (layouts));
    layouts.inputBuses.getReference (0) = juce::AudioChannelSet::stereo();

    // sidechain may be disabled, mono or stereo
    layouts.inputBuses.getReference (1) = juce::AudioChannelSet::disabled();
    CHECK (p.checkBusesLayoutSupported (layouts));
    layouts.inputBuses.getReference (1) = juce::AudioChannelSet::mono();
    CHECK (p.checkBusesLayoutSupported (layouts));
    layouts.inputBuses.getReference (1) = juce::AudioChannelSet::stereo();

    // output mono is fine, surround is not
    layouts.outputBuses.getReference (0) = juce::AudioChannelSet::mono();
    CHECK (p.checkBusesLayoutSupported (layouts));
    layouts.outputBuses.getReference (0) = juce::AudioChannelSet::create5point1();
    CHECK (! p.checkBusesLayoutSupported (layouts));

    // the host must see a true main input
    CHECK (p.getVST3ClientExtensions() != nullptr && p.getVST3ClientExtensions()->getPluginHasMainInput());
}

AERIFORM_TEST (fx_main_input_is_processed_without_any_midi)
{
    TestHost h (48000.0, 256, true);   // main input enabled, NO sidechain, NO MIDI
    h.set (ids::fxMix, 1.0f);
    h.set (ids::reverbMix, 0.0f);
    h.set (ids::delayMix, 0.0f);
    h.set (ids::chorusMix, 0.0f);
    h.set (ids::resFeedback, 0.9f);
    h.set (ids::resOn, 1.0f);

    // silence in -> silence out (no unexpected self-noise)
    const auto quiet = h.render (0.4);
    CHECK_MSG (quiet.rms < 1.0e-4, "silence in -> silence out: rms=" + std::to_string (quiet.rms));

    // a burst of noise, then silence: the resonators ring and then decay on their own
    dsp::Noise rng; rng.seed (7);
    int burst = 0;
    h.inputSource = [&] (long) { return (burst++ < 4800) ? rng.next() * 0.4f : 0.0f; };

    std::vector<float> mono;
    const auto excited = h.render (0.1, &mono);   // during the burst
    CHECK_MSG (excited.finite && excited.rms > 1.0e-3, "main input drives the resonators: rms=" + std::to_string (excited.rms));

    h.inputSource = [] (long) { return 0.0f; };
    const auto tailNear = h.render (0.05);
    const auto tailFar  = h.render (2.0);
    CHECK_MSG (tailNear.rms > 1.0e-4, "resonator tail keeps sounding after the input stops: " + std::to_string (tailNear.rms));
    CHECK_MSG (tailFar.rms < tailNear.rms, "the tail decays: near=" + std::to_string (tailNear.rms) + " far=" + std::to_string (tailFar.rms));
    CHECK (tailNear.finite && tailFar.finite);
}

AERIFORM_TEST (fx_resonator_parameters_reshape_the_input)
{
    // One noise burst, then silence. Measure the wet signal during the burst and the
    // resonator tail 0.5 s after it stops. Deterministic input across renders.
    auto renderBurstAndTail = [] (std::function<void (TestHost&)> setup, RenderStats& duringBurst, RenderStats& tail)
    {
        TestHost h (48000.0, 256, true);
        h.set (ids::fxMix, 1.0f);
        h.set (ids::reverbMix, 0.0f); h.set (ids::delayMix, 0.0f); h.set (ids::chorusMix, 0.0f);
        h.set (ids::resOn, 1.0f);
        setup (h);
        dsp::Noise rng; rng.seed (5);
        long t = 0;
        h.inputSource = [rng, &t] (long) mutable { return (t++ < 4800) ? rng.next() * 0.4f : 0.0f; };
        duringBurst = h.render (0.1);          // 4800 samples of noise
        h.inputSource = [] (long) { return 0.0f; };
        h.render (0.5);                        // let the tail develop / decay
        tail = h.render (0.2);
    };

    RenderStats burstHi, tailHi, burstLo, tailLo, burstRoot, tailRoot;
    renderBurstAndTail ([] (TestHost& h) { h.set (ids::resFeedback, 0.94f); h.set (ids::fxRootNote, 48.0f); h.set (ids::resDamping, 0.15f); }, burstHi, tailHi);
    renderBurstAndTail ([] (TestHost& h) { h.set (ids::resFeedback, 0.2f);  h.set (ids::fxRootNote, 48.0f); h.set (ids::resDamping, 0.15f); }, burstLo, tailLo);
    renderBurstAndTail ([] (TestHost& h) { h.set (ids::resFeedback, 0.94f); h.set (ids::fxRootNote, 72.0f); h.set (ids::resDamping, 0.15f); }, burstRoot, tailRoot);

    CHECK (burstHi.finite && tailHi.finite && tailLo.finite && tailRoot.finite);
    CHECK_MSG (burstHi.rms > 1.0e-3, "the resonators transform the input during the burst: " + std::to_string (burstHi.rms));

    // Feedback controls the tail: a high-feedback resonator keeps ringing, a low-feedback one dies quickly.
    CHECK_MSG (tailHi.rms > 5.0 * tailLo.rms + 1.0e-5,
               "resonator Feedback controls the tail: fb0.94 tail=" + std::to_string (tailHi.rms)
               + " vs fb0.2 tail=" + std::to_string (tailLo.rms));
    // FX Root retunes the resonators: the transformed signal is materially different.
    CHECK_MSG (std::fabs (burstRoot.rms - burstHi.rms) > 1.0e-3 || std::fabs (burstRoot.peak - burstHi.peak) > 1.0e-2,
               "FX Root retunes the resonators (burst rms " + std::to_string (burstHi.rms) + " -> " + std::to_string (burstRoot.rms) + ")");
}

AERIFORM_TEST (fx_stereo_input_is_processed_on_both_channels)
{
    TestHost h (48000.0, 256, true);
    h.set (ids::fxMix, 1.0f);
    h.set (ids::reverbMix, 0.0f); h.set (ids::delayMix, 0.0f); h.set (ids::chorusMix, 0.0f);
    // resonator with a hard left pan so the two output channels differ
    h.set (ids::resPan, -0.8f);
    h.set (ids::resWidth, 0.6f);

    dsp::Noise rng; rng.seed (21);
    h.inputSource = [&] (long) { return rng.next() * 0.3f; };
    h.render (0.3);

    double sumL = 0.0, sumR = 0.0, diff = 0.0; int nn = 0;
    for (int blk = 0; blk < 60; ++blk)
    {
        h.renderBlock();
        for (int i = 0; i < h.blockSize; ++i)
        {
            const float l = h.buffer.getSample (0, i), r = h.buffer.getSample (1, i);
            sumL += (double) l * l; sumR += (double) r * r; diff += std::fabs (l - r); ++nn;
        }
    }
    CHECK_MSG (std::isfinite (sumL) && std::isfinite (sumR) && sumL > 1.0e-6 && sumR > 1.0e-6, "both channels carry audio");
    CHECK_MSG (diff / nn > 1.0e-4, "panned resonator makes L and R differ: " + std::to_string (diff / nn));
}

AERIFORM_TEST (fx_dry_wet_input_and_output_gain)
{
    TestHost h (48000.0, 256, true);
    h.set (ids::reverbMix, 0.0f);
    h.set (ids::delayMix, 0.0f);
    h.set (ids::chorusMix, 0.0f);

    dsp::Noise rng; rng.seed (11);
    h.inputSource = [&] (long) { return rng.next() * 0.25f; };

    // 0 % wet -> output is the untouched dry input (uniform white * 0.25 -> rms ~= 0.25/sqrt(3))
    h.set (ids::fxMix, 0.0f);
    h.set (ids::fxInputGain, 0.0f);
    h.set (ids::fxOutputGain, 0.0f);
    h.render (0.2);
    const auto dry = h.render (0.3);
    CHECK_MSG (std::fabs (dry.rms - 0.25 / std::sqrt (3.0)) < 0.03, "dry passes through at unity: " + std::to_string (dry.rms));

    // 100 % wet -> resonator output, different from the dry signal
    h.set (ids::fxMix, 1.0f);
    h.render (0.2);
    const auto wet = h.render (0.3);
    CHECK_MSG (wet.finite && wet.rms > 1.0e-3, "wet path audible: " + std::to_string (wet.rms));

    // output gain: -12 dB roughly quarters the power
    h.set (ids::fxOutputGain, -12.0f);
    h.render (0.2);
    const auto quieter = h.render (0.3);
    CHECK_MSG (quieter.rms < 0.35 * wet.rms + 1.0e-4, "output gain attenuates: " + std::to_string (quieter.rms) + " vs " + std::to_string (wet.rms));
    CHECK (quieter.finite);

    // input gain: driving the resonators harder raises the wet level (and, on the
    // non-linear model, is not a pure linear scaling of it).
    h.set (ids::fxOutputGain, 0.0f);
    h.set (ids::fxInputGain, -18.0f);
    h.render (0.2);
    const auto softDrive = h.render (0.3);
    h.set (ids::fxInputGain, 6.0f);
    h.render (0.2);
    const auto hardDrive = h.render (0.3);
    CHECK_MSG (hardDrive.rms > 2.0 * softDrive.rms + 1.0e-5,
               "input gain drives the resonators harder: soft=" + std::to_string (softDrive.rms)
               + " hard=" + std::to_string (hardDrive.rms));
    CHECK (softDrive.finite && hardDrive.finite);
}
