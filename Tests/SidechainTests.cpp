#include "TestFramework.h"
#include "TestHelpers.h"

using namespace aeriform;
using namespace aeriform::test;

AERIFORM_TEST (sidechain_input_excites_the_resonators)
{
    TestHost h (48000.0, 256);
    h.prepare (48000.0, 256, true);   // enable the sidechain bus
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
    h.inputSource = [&] (long) { return rng.next() * 0.5f; };
    h.render (0.3);
    std::vector<float> mono;
    const auto excited = h.render (0.5, &mono);
    CHECK_MSG (excited.rms > 1.0e-3, "sidechain excites the tube: rms=" + std::to_string (excited.rms));
    const double f = estimateFrequency (mono, 48000.0, dsp::midiNoteToHz (60.0f));
    CHECK_MSG (std::fabs (centsBetween (f, dsp::midiNoteToHz (60.0f))) < 15.0, "tube output is tuned to the note: " + std::to_string (f) + " Hz");

    // input with no note held is not passed through (the resonators are note-gated)
    h.noteOff (60);
    h.render (0.6);
    const auto gated = h.render (0.3);
    CHECK_MSG (gated.rms < 1.0e-4, "no note -> no output even with input: rms=" + std::to_string (gated.rms));
    CHECK (excited.finite && gated.finite);
}

AERIFORM_TEST (sidechain_bus_layouts_are_supported)
{
    AeriformProcessor p;
    auto layouts = p.getBusesLayout();
    CHECK (p.getBusCount (true) == 1);
    CHECK (p.getBus (true, 0)->getName() == "Sidechain");
    CHECK (p.checkBusesLayoutSupported (layouts));
    layouts.inputBuses.getReference (0) = juce::AudioChannelSet::mono();
    CHECK (p.checkBusesLayoutSupported (layouts));
    layouts.inputBuses.getReference (0) = juce::AudioChannelSet::disabled();
    CHECK (p.checkBusesLayoutSupported (layouts));
    layouts.outputBuses.getReference (0) = juce::AudioChannelSet::mono();
    CHECK (p.checkBusesLayoutSupported (layouts));
    layouts.outputBuses.getReference (0) = juce::AudioChannelSet::create5point1();
    CHECK (! p.checkBusesLayoutSupported (layouts));
    CHECK (p.getVST3ClientExtensions() != nullptr && ! p.getVST3ClientExtensions()->getPluginHasMainInput());
}
