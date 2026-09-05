#include "Voice.h"

namespace aeriform::dsp
{
namespace
{
    // unison detune / pan positions (in -1..1) for 1..4 stacked tubes
    const float* unisonOffsets (int count) noexcept
    {
        static const float t1[] = { 0.0f };
        static const float t2[] = { -1.0f, 1.0f };
        static const float t3[] = { -1.0f, 0.0f, 1.0f };
        static const float t4[] = { -1.0f, -0.33f, 0.33f, 1.0f };
        switch (count) { case 2: return t2; case 3: return t3; case 4: return t4; default: return t1; }
    }
}

void Voice::prepare (double sr, int index)
{
    sampleRate = sr;
    voiceIndex = index;
    const uint32_t seed = 0xA5F1C3u + (uint32_t) index * 0x9E3779B9u;
    rng.seed (seed);
    exciter.prepare ((float) sr, seed ^ 0x51ED2Fu);
    resonator.prepare ((float) sr);
    ampEnv.setSampleRate ((float) sr);
    modEnv.setSampleRate ((float) sr);
    fader.setSampleRate ((float) sr);
    for (int i = 0; i < ids::numLFOs; ++i)
    {
        lfos[i].setSampleRate ((float) sr);
        lfos[i].seed (seed + (uint32_t) i * 977u);
    }
    instabilityRnd.seed (seed ^ 0x77u);
    instabilityRnd.setRate (0.7f * (float) kControlInterval, (float) sr);   // stepped once per control block

    // fixed per-voice component tolerances
    varTune = rng.next(); varDamp = rng.next(); varBright = rng.next(); varShape = rng.next();
    reset();
}

void Voice::reset()
{
    exciter.reset();
    resonator.reset();
    ampEnv.reset();
    modEnv.reset();
    active = false;
    noteId = -1;
    lastMono = 0.0f;
    lastPressure = 0.0f;
    gainRampL = gainRampR = 0.0f;
}

void Voice::startNote (int midiNote, float vel, float glideFromNote, bool legato,
                       int uIndex, int uCount, int id, const VoiceParams& p)
{
    if (! active)
    {
        // fresh start: clear the tube so no stale energy from a previous note leaks in
        resonator.reset();
        exciter.reset();
        gainRampL = gainRampR = 0.0f;
    }
    active = true;
    currentNote = midiNote;
    noteId = id;
    unisonIndex = uIndex;
    unisonCount = std::max (1, uCount);
    velocity = clamp01 (vel);
    noteRandom = rng.next();
    bendSemitones = 0.0f;
    notePressure = 0.0f;
    noteSlide = 0.0f;

    ampEnv.setTimes (p.envAttackMs, p.envDecayMs, p.envSustain, p.envReleaseMs);
    modEnv.setTimes (p.menvAttackMs, p.menvDecayMs, p.menvSustain, p.menvReleaseMs);
    ampEnv.noteOn();
    modEnv.noteOn();
    fader.start();

    // glide
    glideTarget = (float) midiNote;
    if (p.glideMs > 0.0f && legato)
    {
        glideNote = glideFromNote;
        glideStepPerSample = (glideTarget - glideNote) / std::max (1.0f, p.glideMs * 0.001f * (float) sampleRate);
    }
    else
    {
        glideNote = glideTarget;
        glideStepPerSample = 0.0f;
    }
    snapNextLength = ! legato;

    for (int i = 0; i < ids::numLFOs; ++i)
    {
        lfos[i].setFadeMs (p.lfo[i].fadeMs);
        if (p.lfo[i].mode == LfoMode::Retrigger)
            lfos[i].retrigger (p.lfo[i].phaseDeg / 360.0f);
        else
        {
            lfos[i].setPhase (p.lfoGlobalPhase[i] + p.lfo[i].phaseDeg / 360.0f);
            lfos[i].retrigger (lfos[i].getPhase());
        }
    }

    exciter.update (p.exciter, midiNoteToHz ((float) midiNote), 0.0f, 0.0f);
    exciter.noteOn (velocity, midiNoteToHz ((float) midiNote));
}

void Voice::changeNote (int midiNote, float vel, bool retriggerEnvelope, const VoiceParams& p)
{
    const float from = glideNote;
    currentNote = midiNote;
    glideTarget = (float) midiNote;
    if (p.glideMs > 0.0f)
    {
        glideNote = from;
        glideStepPerSample = (glideTarget - glideNote) / std::max (1.0f, p.glideMs * 0.001f * (float) sampleRate);
    }
    else
    {
        glideNote = glideTarget;
        glideStepPerSample = 0.0f;
    }
    if (retriggerEnvelope)
    {
        velocity = clamp01 (vel);
        ampEnv.noteOn();
        modEnv.noteOn();
        exciter.noteOn (velocity, midiNoteToHz ((float) midiNote));
    }
    fader.start();
}

void Voice::stopNote (const VoiceParams& p)
{
    if (! active) return;
    ampEnv.noteOff();
    modEnv.noteOff();
    exciter.noteOff();
    fader.release (p.envReleaseMs);
}

void Voice::kill (float ms)
{
    if (! active) return;
    ampEnv.kill (ms);
    modEnv.kill (ms);
    fader.kill (ms);
}

float Voice::lfoRate (const LfoParams& lp, double bpm, float rateMod) const noexcept
{
    float hz = lp.rateHz;
    if (lp.sync)
    {
        const double beats = choices::syncDivisionBeats (lp.division);
        hz = (float) ((bpm > 1.0 ? bpm : 120.0) / 60.0 / beats);
    }
    return std::clamp (hz * std::exp2 (rateMod * 3.0f), 0.0f, 200.0f);
}

void Voice::updateControl (int n, const VoiceParams& p, const ModSources& globalSources)
{
    // ---- glide ---------------------------------------------------------------
    if (std::fabs (glideStepPerSample) > 0.0f)
    {
        glideNote += glideStepPerSample * (float) n;
        if ((glideStepPerSample > 0.0f && glideNote >= glideTarget) || (glideStepPerSample < 0.0f && glideNote <= glideTarget))
        {
            glideNote = glideTarget;
            glideStepPerSample = 0.0f;
        }
    }

    // ---- modulation sources ----------------------------------------------------
    sources = globalSources;
    for (int i = 0; i < ids::numLFOs; ++i)
    {
        lfos[i].setShape (p.lfo[i].shape);
        const float rateMod = modValues[(size_t) ModDest::Lfo1Rate + (size_t) i];
        lfos[i].setRate (lfoRate (p.lfo[i], p.tempoBpm, rateMod));
        sources[(size_t) ModSource::LFO1 + (size_t) i] = lfos[i].advance (n);
    }
    sources[(size_t) ModSource::ModEnv]     = modEnv.getLevel();
    sources[(size_t) ModSource::AmpEnv]     = ampEnv.getLevel();
    sources[(size_t) ModSource::Velocity]   = velocity;
    sources[(size_t) ModSource::Aftertouch] = notePressure;
    sources[(size_t) ModSource::MpeSlide]   = noteSlide;
    sources[(size_t) ModSource::KeyTrack]   = std::clamp (((float) currentNote - 60.0f) / 60.0f, -1.0f, 1.0f);
    sources[(size_t) ModSource::Random]     = noteRandom;
    if (std::fabs (globalSources[(size_t) ModSource::PitchBend]) < 1.0e-9f && std::fabs (bendSemitones) > 1.0e-9f)
        sources[(size_t) ModSource::PitchBend] = std::clamp (bendSemitones / 12.0f, -1.0f, 1.0f);

    ModMatrix::evaluate (p.mod, sources, modValues);
    auto mod = [this] (ModDest d) { return modValues[(size_t) d]; };

    // ---- breath / pressure -----------------------------------------------------------
    const float env = ampEnv.getLevel();
    const float velScale = lerp (1.0f, 0.2f + 0.8f * velocity, p.velToPressure);
    const float pressureParam = clamp01 (p.pressure + mod (ModDest::Pressure));
    pressureScale = pressureParam * velScale;
    breathScale = velScale;
    lastPressure = pressureScale * env;

    // ---- pitch -------------------------------------------------------------------------
    const float trackedNote = 60.0f + (glideNote - 60.0f) * p.keyTrack;
    const float flowCents = p.flowPitch * 60.0f * (env - ampEnv.getSustain());
    const float instabCents = p.instability * 30.0f * instabilityRnd.next();
    const float variationCents = p.variation * 8.0f * varTune;
    const float* uni = unisonOffsets (unisonCount);
    const float unisonCents = unisonCount > 1 ? p.unisonDetuneCents * uni[std::clamp (unisonIndex, 0, unisonCount - 1)] : 0.0f;

    const float semis = trackedNote + p.coarse + p.fine / 100.0f + bendSemitones + mod (ModDest::Pitch) * 24.0f
                        + (flowCents + instabCents + variationCents + unisonCents) / 100.0f;
    lastFreq = midiNoteToHz (std::clamp (semis, -12.0f, 140.0f)) / std::max (0.1f, p.length);

    // ---- exciter --------------------------------------------------------------------------
    exciterParams = p.exciter;
    exciterParams.noise = clamp01 (p.exciter.noise + mod (ModDest::Noise));
    exciterParams.noiseColor = clamp01 (p.exciter.noiseColor + mod (ModDest::NoiseColor));
    exciterParams.lowpassHz = p.exciter.lowpassHz * std::exp2 (mod (ModDest::ExciterLP) * 4.0f);
    exciterParams.highpassHz = p.exciter.highpassHz * std::exp2 (mod (ModDest::ExciterHP) * 4.0f);
    exciter.update (exciterParams, lastFreq, lastPressure, mod (ModDest::Turbulence));

    // ---- resonator ------------------------------------------------------------------------
    resonatorParams = p.resonator;
    resonatorParams.freqHz = lastFreq;
    resonatorParams.feedback = clamp01 (p.resonator.feedback + mod (ModDest::Feedback));
    resonatorParams.damping = clamp01 (p.resonator.damping + mod (ModDest::Damping));
    resonatorParams.brightness = clamp01 (p.resonator.brightness + mod (ModDest::Brightness));
    resonatorParams.dispersion = clamp01 (p.resonator.dispersion + mod (ModDest::Dispersion));
    resonatorParams.shape = clamp01 (p.resonator.shape + mod (ModDest::Shape) + p.variation * 0.05f * varShape);
    resonatorParams.reflection = clamp01 (p.resonator.reflection + mod (ModDest::Reflection));
    resonatorParams.bodyFreqHz = p.resonator.bodyFreqHz * std::exp2 (mod (ModDest::BodyFreq) * 3.0f);
    resonatorParams.bodyMix = clamp01 (p.resonator.bodyMix + mod (ModDest::BodyMix));
    resonatorParams.reed = p.reed;
    resonatorParams.pressure = lastPressure;
    resonatorParams.variationDamping = p.variation * 0.12f * varDamp;
    resonatorParams.variationBright = p.variation * 0.12f * varBright;
    resonator.update (resonatorParams, snapNextLength);
    snapNextLength = false;

    // ---- amplitude / pan (ramped over the control block) ------------------------------------
    const float unisonPan = unisonCount > 1 ? p.unisonSpread * uni[std::clamp (unisonIndex, 0, unisonCount - 1)] : 0.0f;
    const float pan = std::clamp (unisonPan + mod (ModDest::Pan), -1.0f, 1.0f);
    const float angle = (pan + 1.0f) * 0.25f * kPi;
    ampGain = std::max (0.0f, 1.0f + mod (ModDest::Amp)) * 0.5f / std::sqrt ((float) unisonCount);
    panL = std::cos (angle);
    panR = std::sin (angle);
    const float targetL = ampGain * panL, targetR = ampGain * panR;
    gainStepL = (targetL - gainRampL) / (float) n;
    gainStepR = (targetR - gainRampR) / (float) n;
}

void Voice::render (float* left, float* right, int numSamples, const VoiceParams& p,
                    const ModSources& globalSources, const float* externalIn, float couplingIn)
{
    if (! active) return;

    int pos = 0;
    float monoAcc = 0.0f;
    while (pos < numSamples)
    {
        const int n = std::min (kControlInterval, numSamples - pos);
        updateControl (n, p, globalSources);

        for (int i = 0; i < n; ++i)
        {
            const float env = ampEnv.next();
            modEnv.next();
            const float fade = fader.next();
            const float ext = externalIn != nullptr ? externalIn[pos + i] : 0.0f;

            const float excitation = exciter.next (ext + couplingIn, env * breathScale);
            const float tube = resonator.next (excitation, pressureScale * env);
            const float out = tube * fade;

            gainRampL += gainStepL;
            gainRampR += gainStepR;
            left[pos + i]  += out * gainRampL;
            right[pos + i] += out * gainRampR;
            monoAcc = out;
        }
        pos += n;
    }

    lastMono = monoAcc * ampGain;

    // numerical safety: a non-finite state is flushed immediately (never propagates)
    if (! resonator.isFinite() || ! std::isfinite (lastMono))
    {
        resonator.reset();
        exciter.reset();
        lastMono = 0.0f;
    }

    if (! fader.isRunning())
    {
        active = false;
        noteId = -1;
        ampEnv.reset();
        modEnv.reset();
    }
}
} // namespace aeriform::dsp
