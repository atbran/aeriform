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

    // exciter slot parameter block offsets (exa_* and exb_* are laid out identically and contiguously)
    enum ExOff
    {
        exModel, exLevel, exCoarse, exFine, exKeytrack, exRetrig, exVariation, exVel, exPress, exDrift, exPhase, exTone,
        exWaveShape, exWavePw, exWaveSub, exWavePd,
        exCxComplexity, exCxSymmetry, exCxBend, exCxInstab, exCxSpread, exCxWarp, exCxFeedback, exCxChaos, exCxRatio,
        exNzColor, exNzDensity, exNzGrain, exNzBandwidth, exNzCenter, exNzCorrelation, exNzSeed, exNzWidth, exNzBurst, exNzBurstEnv, exNzTurb, exNzGust,
        exPhStiffness, exPhOpening, exPhPosition, exPhSpeed, exPhTurb, exPhHardness, exPhBright,
        exScLp, exScHp, exScFollow, exScTransient, exScFreeze,
        exCount
    };
    static_assert ((int) P::exbModel - (int) P::exaModel == (int) exCount, "exciter slot layout changed");

    // resonator B / C parameter block offsets (rb_* and rc_* contiguous)
    enum RbOff
    {
        rbOn, rbType, rbInput, rbOutput, rbCoarse, rbFine, rbRatio, rbKeytrack, rbFeedback, rbDamping, rbBrightness, rbDispersion,
        rbInharm, rbShape, rbReflect, rbSaturation, rbReed, rbSize, rbPickup, rbPan, rbWidth, rbCount
    };
    static_assert ((int) P::rcOn - (int) P::rbOn == (int) rbCount, "resonator slot layout changed");

    inline P offsetP (P base, int offset) noexcept { return (P) ((int) base + offset); }
}

// ---------------------------------------------------------------------------
void Voice::prepare (double sr, int index)
{
    sampleRate = sr;
    voiceIndex = index;
    const uint32_t seed = 0xA5F1C3u + (uint32_t) index * 0x9E3779B9u;
    rng.seed (seed);
    network.prepare ((float) sr);filters.prepare((float)sr);network.setFilters(&filters);
    bodyL.setSampleRate ((float) sr);
    bodyR.setSampleRate ((float) sr);
    dynEnv.setCutoff (12.0f, (float) sr);
    ampEnv.setSampleRate ((float) sr);
    modEnv.setSampleRate ((float) sr);
    fader.setSampleRate ((float) sr);
    for (int i = 0; i < ids::numLFOs; ++i)
    {
        lfos[i].setSampleRate ((float) sr);
        lfos[i].seed (seed + (uint32_t) i * 977u);
    }
    instabilityRnd.seed (seed ^ 0x77u);
    instabilityRnd.setRate (0.7f * 32.0f, (float) sr);   // stepped once per control block
    smoothRnd.seed (seed ^ 0x99u);
    smoothRnd.setRate (0.8f * 32.0f, (float) sr);
    varTune = rng.next(); varDamp = rng.next(); varBright = rng.next(); varShape = rng.next();
    osFactor = 0;
    configureOversampling (2);
    reset();
}

void Voice::configureOversampling (int factor)
{
    if (factor == osFactor) return;
    osFactor = factor;
    const float osRate = (float) sampleRate * (float) factor;
    exA.prepare (osRate, factor, voiceIndex, 0);
    exB.prepare (osRate, factor, voiceIndex, 1);
    interaction.prepare (osRate);
    preShaper.prepare (osRate);
    folder.prepare (osRate);
    decimator.setFactor (factor);
    extUp.setFactor (factor);
    loopUp.setFactor (factor);sideDecimator.setFactor(factor);
}

void Voice::reset()
{
    exA.reset(); exB.reset();
    interaction.reset(); preShaper.reset(); folder.reset();
    decimator.reset(); extUp.reset(); loopUp.reset();sideDecimator.reset();
    dynEnv.reset (0.3f);
    network.reset();filters.reset();
    bodyL.reset(); bodyR.reset();
    ampEnv.reset(); modEnv.reset();
    active = false;
    noteId = -1;
    lastMono = lastFolded = lastPressure = 0.0f;
    gainRampL = gainRampR = 0.0f;
    loopRet = 0.0f;
    scope = nullptr;
}

// ---------------------------------------------------------------------------
void Voice::startNote (int midiNote, float vel, float glideFromNote, bool legato,
                       int uIndex, int uCount, int id, const VoiceParams& p)
{
    if (! active)
    {
        // fresh start: clear the tube network so no stale energy from a previous note leaks in
        network.reset();filters.reset();
        exA.reset(); exB.reset();
        interaction.reset(); preShaper.reset(); folder.reset();
        decimator.reset(); extUp.reset(); loopUp.reset();sideDecimator.reset();
        bodyL.reset(); bodyR.reset();
        gainRampL = gainRampR = 0.0f;
        loopRet = 0.0f;
    }
    active = true;
    currentNote = midiNote;
    noteId = id;
    unisonIndex = uIndex;
    unisonCount = std::max (1, uCount);
    velocity = clamp01 (vel);
    noteRandom = rng.next();
    sampleHoldValue = rng.next();
    bendSemitones = 0.0f;
    notePressure = 0.0f;
    noteSlide = 0.0f;
    noteAgeSamples = 0;
    henonX = 0.1f + 0.2f * rng.next01(); henonY = 0.1f;

    ampEnv.setTimes (p.get (P::envAttack), p.get (P::envDecay), p.get (P::envSustain), p.get (P::envRelease));
    modEnv.setTimes (p.get (P::menvAttack), p.get (P::menvDecay), p.get (P::menvSustain), p.get (P::menvRelease));
    ampEnv.noteOn();
    modEnv.noteOn();
    fader.start();

    glideTarget = (float) midiNote;
    if (p.get (P::glideTime) > 0.0f && legato)
    {
        glideNote = glideFromNote;
        glideStepPerSample = (glideTarget - glideNote) / std::max (1.0f, p.get (P::glideTime) * 0.001f * (float) sampleRate);
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

    buildExciterParams (exParamsA, p, true, 0.0f);
    buildExciterParams (exParamsB, p, false, 0.0f);
    exA.noteOn (velocity, (float) midiNote, exParamsA);
    exB.noteOn (velocity, (float) midiNote, exParamsB);
}

void Voice::changeNote (int midiNote, float vel, bool retriggerEnvelope, const VoiceParams& p)
{
    const float from = glideNote;
    currentNote = midiNote;
    glideTarget = (float) midiNote;
    if (p.get (P::glideTime) > 0.0f)
    {
        glideNote = from;
        glideStepPerSample = (glideTarget - glideNote) / std::max (1.0f, p.get (P::glideTime) * 0.001f * (float) sampleRate);
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
        exA.noteOn (velocity, (float) midiNote, exParamsA);
        exB.noteOn (velocity, (float) midiNote, exParamsB);
    }
    fader.start();
}

void Voice::stopNote (const VoiceParams& p)
{
    if (! active) return;
    ampEnv.noteOff();
    modEnv.noteOff();
    exA.noteOff(); exB.noteOff();
    fader.release (p.get (P::envRelease));
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

// ---------------------------------------------------------------------------
void Voice::buildExciterParams (ExciterSlot::Params& out, const VoiceParams& p, bool slotA, float pitchSemis) const
{
    const P base = slotA ? P::exaModel : P::exbModel;
    auto g = [&] (int off) { return p.get (offsetP (base, off)); };
    auto mod = [this] (ModDest d) { return modValues[(size_t) d]; };
    const ModDest dLevel = slotA ? ModDest::ExALevel : ModDest::ExBLevel, dPitch = slotA ? ModDest::ExAPitch : ModDest::ExBPitch;
    const ModDest dTone = slotA ? ModDest::ExATone : ModDest::ExBTone, dShape = slotA ? ModDest::ExAShape : ModDest::ExBShape;
    const ModDest dChaos = slotA ? ModDest::ExAChaos : ModDest::ExBChaos;

    out.model = p.getEnum (offsetP (base, exModel), ExciterModel::Count);
    out.level = std::max (0.0f, g (exLevel) * (1.0f + mod (dLevel)));
    out.coarse = g (exCoarse);
    out.fine = g (exFine);
    out.keyTrack = g (exKeytrack);
    out.retrig = p.getEnum (offsetP (base, exRetrig), RetrigMode::Count);
    out.variation = g (exVariation);
    out.velAmount = g (exVel);
    out.pressAmount = g (exPress);
    out.drift = g (exDrift);
    out.phaseDeg = g (exPhase);
    out.tone = std::clamp (g (exTone) + mod (dTone), -1.0f, 1.0f);
    out.pitchModSemis = pitchSemis + mod (dPitch) * 24.0f;

    const float shapeMod = mod (dShape), chaosMod = mod (dChaos);
    out.waveShape = clamp01 (g (exWaveShape) + shapeMod);
    out.wavePw = std::clamp (g (exWavePw), 0.05f, 0.95f);
    out.waveSub = g (exWaveSub);
    out.wavePd = clamp01 (g (exWavePd) + chaosMod);
    out.sync = ! slotA && p.getb (P::exbSync);

    out.cx.complexity = clamp01 (g (exCxComplexity) + shapeMod);
    out.cx.symmetry = g (exCxSymmetry);
    out.cx.bend = g (exCxBend);
    out.cx.instability = g (exCxInstab);
    out.cx.spread = g (exCxSpread);
    out.cx.warp = g (exCxWarp);
    out.cx.feedback = g (exCxFeedback);
    out.cx.chaos = clamp01 (g (exCxChaos) + chaosMod);
    out.cx.ratio = g (exCxRatio);

    out.nz.color = std::clamp (g (exNzColor) + shapeMod, -1.0f, 1.0f);
    out.nz.density = g (exNzDensity);
    out.nz.grainMs = g (exNzGrain);
    out.nz.bandwidth = g (exNzBandwidth);
    out.nz.centerHz = g (exNzCenter);
    out.nz.correlation = g (exNzCorrelation);
    out.nz.seed = (int) std::lround (g (exNzSeed));
    out.nz.burstMs = g (exNzBurst);
    out.nz.burstEnv = g (exNzBurstEnv);
    out.nz.turbulence = clamp01 (g (exNzTurb) + chaosMod);
    out.nz.gustHz = g (exNzGust);

    out.ph.stiffness = g (exPhStiffness);
    out.ph.opening = g (exPhOpening);
    out.ph.position = g (exPhPosition);
    out.ph.speed = clamp01 (g (exPhSpeed) + shapeMod);
    out.ph.turbulence = clamp01 (g (exPhTurb) + chaosMod);
    out.ph.hardness = g (exPhHardness);
    out.ph.brightness = g (exPhBright);

    out.scLp = g (exScLp);
    out.scHp = g (exScHp);
    out.scFollow = g (exScFollow);
    out.scTransient = g (exScTransient);

    // shared legacy breath parameters (the Breath model)
    auto& b = out.breath;
    b.noise = clamp01 (p.get (P::excNoise) + mod (ModDest::Noise) + (out.model == ExciterModel::Breath ? shapeMod : 0.0f));
    b.noiseColor = clamp01 (p.get (P::excNoiseColor) + mod (ModDest::NoiseColor));
    b.pluck = p.get (P::excPluck);
    b.pluckLengthMs = p.get (P::excPluckLength);
    b.turbulence = clamp01 (p.get (P::excTurbulence) + mod (ModDest::Turbulence) + (out.model == ExciterModel::Breath ? chaosMod : 0.0f));
    b.velocityAmount = p.get (P::excVelocity);
    b.externalIn = p.get (P::excExternalIn);
    b.keyTrack = p.get (P::excKeyTrack);
    b.attackClick = p.get (P::excAttackClick);
    b.releaseNoise = p.get (P::excReleaseNoise);
    b.breathRandom = p.get (P::excBreathRandom);
    b.pressureBright = p.get (P::artPressBright);
}

void Voice::buildNetworkParams (const VoiceParams& p, float baseNote)
{
    auto mod = [this] (ModDest d) { return modValues[(size_t) d]; };
    auto& n = netParams;
    n.bypass=p.getb(P::netBypass);
    n.mode = p.getEnum (P::netMode, NetMode::Count);
    n.feedback = clamp01 (p.get (P::netFeedback) + mod (ModDest::NetFeedback));
    n.ab = p.get (P::netAB); n.ba = p.get (P::netBA); n.bc = p.get (P::netBC);
    n.cb = p.get (P::netCB); n.ca = p.get (P::netCA); n.ac = p.get (P::netAC);
    n.sendAB = p.get (P::netSendAB); n.sendBC = p.get (P::netSendBC);
    n.injectB = p.get (P::netInjectB); n.injectC = p.get (P::netInjectC);
    n.polarity = p.getEnum (P::netPolarity, Polarity::Count);
    n.fbDelayMs = p.get (P::netFbDelay); n.fbFilterHz = p.get (P::netFbFilter); n.fbDrive = p.get (P::netFbDrive);
    n.damping = p.get (P::netDamping);
    n.width = clamp01 (p.get (P::netWidth) + mod (ModDest::NetWidth));
    n.inject = p.getEnum (P::netInject, InjectPoint::Count);
    n.tap = p.getEnum (P::netTap, OutputTap::Count);
    n.mix = p.get (P::netMix);
    n.repipe = clamp01 (p.get (P::netRepipe) + mod (ModDest::Repipe));

    n.loopOn = p.getb (P::loopOn);
    n.loopAmount = clamp01 (p.get (P::loopAmount) + mod (ModDest::LoopAmount));
    n.loopSource = p.getEnum (P::loopSource, LoopSource::Count);
    n.loopFilterHz = p.get (P::loopFilter); n.loopDelayMs = p.get (P::loopDelay);
    n.loopPolarity = p.getEnum (P::loopPolarity, Polarity::Count);
    n.loopSat = p.get (P::loopSat);

    // ---- resonator A (legacy res_* parameters) --------------------------------
    n.on[0] = p.getb (P::resOn);
    n.in[0] = p.get (P::resInput); n.out[0] = p.get (P::resOutput);
    n.pan[0] = std::clamp (p.get (P::resPan) + mod (ModDest::ResAPan), -1.0f, 1.0f);
    n.width3[0] = p.get (P::resWidth);
    {
        auto& r = n.res[0];
        r.type = p.getEnum (P::resMode, ResMode::Count);
        const float tracked = 60.0f + (baseNote - 60.0f) * p.get (P::resKeyTrack);
        const float semis = tracked + p.get (P::resCoarse) + p.get (P::resFine) / 100.0f;
        r.freqHz = midiNoteToHz (std::clamp (semis, -12.0f, 140.0f)) / std::max (0.1f, p.get (P::resLength));
        r.feedback = clamp01 (p.get (P::resFeedback) + mod (ModDest::Feedback));
        r.damping = clamp01 (p.get (P::resDamping) + mod (ModDest::Damping));
        r.brightness = clamp01 (p.get (P::resBrightness) + mod (ModDest::Brightness));
        r.dispersion = clamp01 (p.get (P::resDispersion) + mod (ModDest::Dispersion));
        r.inharm = p.get (P::resInharm);
        r.shape = clamp01 (p.get (P::resShape) + mod (ModDest::Shape) + p.get (P::artVariation) * 0.05f * varShape);
        r.reflection = clamp01 (p.get (P::resReflection) + mod (ModDest::Reflection));
        r.saturation = p.get (P::resSaturation);
        r.reed = p.get (P::excReed);
        r.pressure = lastPressure;
        r.size = p.get (P::resSize);
        r.pickup = p.get (P::resPickup);
        r.variationDamping = p.get (P::artVariation) * 0.12f * varDamp;
        r.variationBright = p.get (P::artVariation) * 0.12f * varBright;
    }

    // ---- resonators B and C ---------------------------------------------------------
    for (int slot = 1; slot <= 2; ++slot)
    {
        const P base = slot == 1 ? P::rbOn : P::rcOn;
        auto g = [&] (int off) { return p.get (offsetP (base, off)); };
        const ModDest dPitch = slot == 1 ? ModDest::ResBPitch : ModDest::ResCPitch;
        const ModDest dFb = slot == 1 ? ModDest::ResBFeedback : ModDest::ResCFeedback;
        const ModDest dDamp = slot == 1 ? ModDest::ResBDamping : ModDest::ResCDamping;
        const ModDest dBright = slot == 1 ? ModDest::ResBBrightness : ModDest::ResCBrightness;

        n.on[slot] = g (rbOn) > 0.5f;
        n.in[slot] = g (rbInput); n.out[slot] = g (rbOutput);
        n.pan[slot] = g (rbPan); n.width3[slot] = g (rbWidth);
        auto& r = n.res[slot];
        r.type = p.getEnum (offsetP (base, rbType), ResMode::Count);
        const float tracked = 60.0f + (baseNote - 60.0f) * g (rbKeytrack);
        const float semis = tracked + g (rbCoarse) + g (rbFine) / 100.0f + 12.0f * std::log2 (std::max (0.01f, g (rbRatio))) + mod (dPitch) * 24.0f;
        r.freqHz = midiNoteToHz (std::clamp (semis, -12.0f, 140.0f));
        r.feedback = clamp01 (g (rbFeedback) + mod (dFb));
        r.damping = clamp01 (g (rbDamping) + mod (dDamp));
        r.brightness = clamp01 (g (rbBrightness) + mod (dBright));
        r.dispersion = g (rbDispersion);
        r.inharm = g (rbInharm);
        r.shape = clamp01 (g (rbShape) + p.get (P::artVariation) * 0.05f * varShape);
        r.reflection = g (rbReflect);
        r.saturation = g (rbSaturation);
        r.reed = g (rbReed);
        r.pressure = lastPressure;
        r.size = g (rbSize);
        r.pickup = g (rbPickup);
        r.variationDamping = p.get (P::artVariation) * 0.12f * varDamp;
        r.variationBright = p.get (P::artVariation) * 0.12f * varBright;
    }
}

// ---------------------------------------------------------------------------
void Voice::updateControl (int n, const VoiceParams& p, const ModSources& globalSources)
{
    if (p.osFactor != osFactor) configureOversampling (p.osFactor);

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
    // sample & hold: new random value at every LFO 1 cycle
    if (lfos[0].getPhase() < lastLfo1Phase) sampleHoldValue = rng.next();
    lastLfo1Phase = lfos[0].getPhase();
    // bounded Henon map for the chaos sources when no complex oscillator is running
    {
        const float nx = 1.0f - 1.4f * henonX * henonX + henonY;
        henonY = 0.3f * henonX;
        henonX = std::clamp (nx, -1.5f, 1.5f);
        if (! std::isfinite (henonX) || ! std::isfinite (henonY)) { henonX = 0.1f; henonY = 0.1f; }
    }
    const bool cxA = exA.getModel() == ExciterModel::Complex, cxB = exB.getModel() == ExciterModel::Complex;
    const float chaosX = cxA ? exA.chaosX() * 2.0f - 1.0f : (cxB ? exB.chaosX() * 2.0f - 1.0f : henonX * 0.7f);
    const float chaosY = cxA ? exA.chaosY() * 2.0f - 1.0f : (cxB ? exB.chaosY() * 2.0f - 1.0f : henonY * 3.0f);

    sources[(size_t) ModSource::ModEnv]       = modEnv.getLevel();
    sources[(size_t) ModSource::AmpEnv]       = ampEnv.getLevel();
    sources[(size_t) ModSource::Velocity]     = velocity;
    sources[(size_t) ModSource::Aftertouch]   = notePressure;
    sources[(size_t) ModSource::MpeSlide]     = noteSlide;
    sources[(size_t) ModSource::KeyTrack]     = std::clamp (((float) currentNote - 60.0f) / 60.0f, -1.0f, 1.0f);
    sources[(size_t) ModSource::Random]       = noteRandom;
    sources[(size_t) ModSource::ExAEnv]       = std::min (1.0f, exA.getEnvelope() * 2.0f);
    sources[(size_t) ModSource::ExBEnv]       = std::min (1.0f, exB.getEnvelope() * 2.0f);
    sources[(size_t) ModSource::ResAEnergy]   = std::min (1.0f, network.energy (0) * 2.0f);
    sources[(size_t) ModSource::ResBEnergy]   = std::min (1.0f, network.energy (1) * 2.0f);
    sources[(size_t) ModSource::ResCEnergy]   = std::min (1.0f, network.energy (2) * 2.0f);
    sources[(size_t) ModSource::NetEnergy]    = std::min (1.0f, network.netEnergy() * 3.0f);
    sources[(size_t) ModSource::SampleHold]   = sampleHoldValue;
    sources[(size_t) ModSource::SmoothRandom] = smoothRnd.next();
    sources[(size_t) ModSource::ChaosX]       = std::clamp (chaosX, -1.0f, 1.0f);
    sources[(size_t) ModSource::ChaosY]       = std::clamp (chaosY, -1.0f, 1.0f);
    sources[(size_t) ModSource::NoteAge]      = std::min (1.0f, (float) noteAgeSamples / (8.0f * (float) sampleRate));
    sources[(size_t) ModSource::KeyPosition]  = (float) currentNote / 127.0f;
    sources[(size_t) ModSource::VoiceNumber]  = (float) voiceIndex / 15.0f;
    sources[(size_t) ModSource::AlternateNote]= alternateNote;
    if (std::fabs (globalSources[(size_t) ModSource::PitchBend]) < 1.0e-9f && std::fabs (bendSemitones) > 1.0e-9f)
        sources[(size_t) ModSource::PitchBend] = std::clamp (bendSemitones / 12.0f, -1.0f, 1.0f);

    ModMatrix::evaluate (p.mod, sources, modValues);
    auto mod = [this] (ModDest d) { return modValues[(size_t) d]; };

    // ---- breath / pressure -----------------------------------------------------------
    const float env = ampEnv.getLevel();
    const float velScale = lerp (1.0f, 0.2f + 0.8f * velocity, p.get (P::envVelToPressure));
    const float pressureParam = clamp01 (p.get (P::excPressure) + mod (ModDest::Pressure));
    pressureScale = pressureParam * velScale;
    breathScale = velScale;
    envAmount = p.get (P::preEnv);
    lastPressure = pressureScale * env;

    // ---- pitch (shared by exciters and resonators) ------------------------------------
    const float flowCents = p.get (P::artFlowPitch) * 60.0f * (env - ampEnv.getSustain());
    const float instabCents = p.get (P::artInstability) * 30.0f * instabilityRnd.next();
    const float variationCents = p.get (P::artVariation) * 8.0f * varTune;
    const float* uni = unisonOffsets (unisonCount);
    const float unisonCents = unisonCount > 1 ? p.get (P::unisonDetune) * uni[std::clamp (unisonIndex, 0, unisonCount - 1)] : 0.0f;
    const float baseNote = glideNote + bendSemitones + mod (ModDest::Pitch) * 24.0f
                           + (flowCents + instabCents + variationCents + unisonCents) / 100.0f;
    lastFreq = midiNoteToHz (std::clamp (baseNote, -12.0f, 140.0f));

    // ---- exciters -----------------------------------------------------------------------
    buildExciterParams (exParamsA, p, true, 0.0f);
    buildExciterParams (exParamsB, p, false, 0.0f);
    exA.update (exParamsA, baseNote, env, lastPressure, notePressure, velocity, (float) p.tempoBpm);
    exB.update (exParamsB, baseNote, env, lastPressure, notePressure, velocity, (float) p.tempoBpm);
    syncBtoA = exParamsB.sync && exB.isActive();

    // ---- interaction -----------------------------------------------------------------------
    {
        Interaction::Params ip;
        ip.mode = p.getEnum (P::mixMode, InteractionMode::Count);
        ip.interaction = clamp01 (p.get (P::mixInteraction) + mod (ModDest::Interaction));
        ip.balance = std::clamp (p.get (P::mixBalance) + mod (ModDest::Balance), -1.0f, 1.0f);
        ip.depth = p.get (P::mixDepth);
        ip.a2b = p.get (P::mixA2B);
        ip.normalize = p.get (P::mixNormalize);
        ip.drive = p.get (P::mixDrive);
        ip.dcBlock = p.getb (P::mixDcBlock);
        ip.aActive = exA.isActive();
        ip.bActive = exB.isActive();
        interaction.update (ip);
        interactionMode = ip.mode;
        interactionAmount = ip.interaction;
        b2a = p.get (P::mixB2A);
    }

    // ---- pre-shaper ---------------------------------------------------------------------------
    {
        PreShaper::Params sp;
        sp.type = p.getEnum (P::preType, PreFilterType::Count);
        sp.lowpassHz = p.get (P::excLowpass) * std::exp2 (mod (ModDest::ExciterLP) * 4.0f);
        sp.highpassHz = p.get (P::excHighpass) * std::exp2 (mod (ModDest::ExciterHP) * 4.0f);
        sp.keyTrack = p.get (P::excKeyTrack);
        sp.resonance = p.get (P::preRes);
        sp.drive = clamp01 (p.get (P::preDrive) + mod (ModDest::PreDrive));
        sp.bias = p.get (P::preBias);
        sp.slew = p.get (P::preSlew);
        sp.transient = p.get (P::preTransient);
        sp.pressureBright = p.get (P::artPressBright);
        preShaper.update (sp, lastFreq, lastPressure);
        shaperOrder = p.getEnum (P::preOrder, ShaperOrder::Count);
    }

    // ---- wavefolder -----------------------------------------------------------------------------
    {
        Wavefolder::Params wp;
        wp.on = p.getb (P::wfOn);
        wp.mode = p.getEnum (P::wfMode, FoldMode::Count);
        wp.fold = clamp01 (p.get (P::wfFold) + mod (ModDest::Fold));
        wp.drive = clamp01 (p.get (P::wfDrive) + mod (ModDest::FoldDrive));
        wp.symmetry = std::clamp (p.get (P::wfSymmetry) + mod (ModDest::FoldSymmetry), -1.0f, 1.0f);
        wp.bias = std::clamp (p.get (P::wfBias) + mod (ModDest::FoldBias), -1.0f, 1.0f);
        wp.stages = p.geti (P::wfStages);
        wp.shape = p.get (P::wfShape);
        wp.mix = p.get (P::wfMix);
        wp.comp = p.get (P::wfComp);
        wp.postLpHz = p.get (P::wfLp);
        folder.update (wp);
        dynAmount = p.get (P::dynAmount);
        loopDest = p.getEnum (P::loopDest, LoopDest::Count);
    }

    // ---- network + body ----------------------------------------------------------------------------
    filters.update(p,baseNote,ampEnv.getLevel(),n);
    buildNetworkParams (p, baseNote);
    auto& cp=netParams.contact;cp.enabled=p.getb(P::contactOn);cp.source=p.geti(P::contactSource);cp.destination=p.geti(P::contactDestination);
    cp.gap=p.get(P::contactGap);cp.stiffness=p.get(P::contactStiffness);cp.hardness=p.get(P::contactHardness);cp.damping=p.get(P::contactDamping);cp.friction=p.get(P::contactFriction);cp.asymmetry=p.get(P::contactAsymmetry);cp.amount=p.get(P::contactAmount);cp.polarity=p.geti(P::contactPolarity)?-1.0f:1.0f;cp.quality=p.geti(P::contactQuality);
    StereoNetworkParams stereo;stereo.enabled=p.geti(P::stereoMode)>0;stereo.divergence=p.get(P::stereoDivergence);stereo.coupling=p.get(P::stereoCoupling);stereo.exciterSpread=p.get(P::stereoExciterSpread);stereo.pickupSpread=p.get(P::stereoPickupSpread);stereo.dampingDivergence=p.get(P::stereoDamping);stereo.rotation=p.get(P::stereoRotation);stereo.width=p.get(P::stereoWidth);stereo.monoBass=p.get(P::stereoMonoBass);network.setStereo(stereo,n);
    network.update (netParams, snapNextLength);
    snapNextLength = false;
    {
        const float bodyFreq = p.get (P::resBodyFreq) * std::exp2 (mod (ModDest::BodyFreq) * 3.0f)
                               * std::pow (lastFreq / 261.63f, p.get (P::resBodyTrack));
        const float q = 0.5f + 12.0f * clamp01 (p.get (P::resBodyRes));
        bodyL.set (std::clamp (bodyFreq, 40.0f, (float) sampleRate * 0.4f), q);
        bodyR.set (std::clamp (bodyFreq, 40.0f, (float) sampleRate * 0.4f), q);
        bodyK = 1.0f / q;
        bodyMix = clamp01 (p.get (P::resBodyMix) + mod (ModDest::BodyMix));
        bodyGain = bodyMix * (1.0f + 2.0f * clamp01 (p.get (P::resBodyRes)));
    }

    // ---- amplitude / pan (ramped over the control block) -----------------------------------------------
    const float unisonPan = unisonCount > 1 ? p.get (P::unisonSpread) * uni[std::clamp (unisonIndex, 0, unisonCount - 1)] : 0.0f;
    const float pan = std::clamp (unisonPan + mod (ModDest::Pan), -1.0f, 1.0f);
    const float angle = (pan + 1.0f) * 0.25f * kPi;
    ampGain = std::max (0.0f, 1.0f + mod (ModDest::Amp)) * 0.5f / std::sqrt ((float) unisonCount);
    const float targetL = ampGain * std::cos (angle) * 1.414f, targetR = ampGain * std::sin (angle) * 1.414f;
    gainStepL = (targetL - gainRampL) / (float) n;
    gainStepR = (targetR - gainRampR) / (float) n;
}

// ---------------------------------------------------------------------------
void Voice::render (float* left, float* right, int numSamples, const VoiceParams& p,
                    const ModSources& globalSources, const float* externalIn, const float* sharedNoise, float couplingIn)
{
    if (! active) return;

    const int interval = std::clamp (p.controlInterval, 8, kMaxControlInterval);
    int pos = 0;
    float monoAcc = 0.0f;
    float osSide[Oversampler::kMaxFactor];
    float osExt[Oversampler::kMaxFactor], osLoop[Oversampler::kMaxFactor], osOut[Oversampler::kMaxFactor];
    static const float zeroNoise[Oversampler::kMaxFactor] = { 0.0f, 0.0f, 0.0f, 0.0f };

    while (pos < numSamples)
    {
        const int n = std::min (interval, numSamples - pos);
        updateControl (n, p, globalSources);
        const int os = osFactor;
        const bool loopToChain = netParams.loopOn && loopDest != LoopDest::NetworkIn;

        for (int i = 0; i < n; ++i)
        {
            filters.advance();
            const float env = ampEnv.next();
            modEnv.next();
            const float fade = fader.next();
            const float breath = lerp (1.0f, env, envAmount) * breathScale;
            const float pressureNow = pressureScale * env;

            const float ext = externalIn != nullptr ? externalIn[pos + i] : 0.0f;
            extUp.upsample (ext, osExt);
            if (loopToChain) loopUp.upsample (loopRet, osLoop);
            const float* shared = sharedNoise != nullptr ? sharedNoise + (size_t) (pos + i) * (size_t) os : zeroNoise;

            // ---- oversampled exciter chain ----------------------------------------
            for (int k = 0; k < os; ++k)
            {
                const bool syncPulseB = syncBtoA && exA.wrapped();
                const float b = filters.at(FilterPosition::ExciterB,exB.next (osExt[k], shared[k], breath, 0.0f, 0.0f, syncPulseB));
                float fm = 0.0f, pm = 0.0f;
                bool syncA = false;
                if (exB.isActive())
                {
                    if (interactionMode == InteractionMode::FM)        fm = b * b2a * interactionAmount * 3.0f;
                    else if (interactionMode == InteractionMode::PM)   pm = b * b2a * interactionAmount * 0.5f;
                    else if (interactionMode == InteractionMode::Sync) syncA = exB.wrapped() && interactionAmount > 0.01f;
                }
                const float a = filters.at(FilterPosition::ExciterA,exA.next (osExt[k], shared[k], breath, fm, pm, syncA));
                osSide[k]=.5f*(a-b);
                float m = filters.at(FilterPosition::Combined,interaction.next (a, b));
                if (loopToChain && loopDest == LoopDest::ShaperIn) m += osLoop[k];
                float f;
                if (shaperOrder == ShaperOrder::ShapeThenFold)
                {
                    f = preShaper.next (m);
                    if (loopToChain && loopDest == LoopDest::FolderIn) f += osLoop[k];
                    f = filters.at(FilterPosition::AfterFolder,folder.next(filters.at(FilterPosition::BeforeFolder,f)));
                }
                else
                {
                    if (loopToChain && loopDest == LoopDest::FolderIn) m += osLoop[k];
                    f = filters.at(FilterPosition::AfterFolder,folder.next(filters.at(FilterPosition::BeforeFolder,m)));
                    f = preShaper.next (f);
                }
                osOut[k] = f;
            }
            float x = decimator.downsample (osOut);
            if(network.stereoActive())network.setExciterSide(sideDecimator.downsample(osSide));

            // ---- dynamics normaliser ----------------------------------------------------
            if (dynAmount > 0.0005f)
            {
                const float e = dynEnv.process (std::fabs (x));
                x *= lerp (1.0f, std::min (4.0f, 0.3f / std::max (e, 0.02f)), dynAmount);
            }
            lastFolded = x;
            if (scope != nullptr)
            {
                scope->exciterAScope.push (exA.getLastOutput(), VisualizerModel::kDecimation);
                scope->exciterBScope.push (exB.getLastOutput(), VisualizerModel::kDecimation);
                scope->foldScope.push (x, VisualizerModel::kDecimation);
            }

            // ---- resonator network -------------------------------------------------------
            float l, r;
            const float loopNet = (netParams.loopOn && loopDest == LoopDest::NetworkIn) ? loopRet : 0.0f;
            network.next (x + couplingIn, loopNet, pressureNow, l, r);
            loopRet = network.loopReturn();

            // ---- body / formant filter ------------------------------------------------------
            if (bodyMix > 0.0005f)
            {
                l = l * (1.0f - 0.7f * bodyMix) + bodyL.bandpass (l) * bodyK * bodyGain;
                r = r * (1.0f - 0.7f * bodyMix) + bodyR.bandpass (r) * bodyK * bodyGain;
            }

            l=filters.at(FilterPosition::PostBody,l,0);r=filters.at(FilterPosition::PostBody,r,3);
            gainRampL += gainStepL;
            gainRampR += gainStepR;
            const float outL = l * fade * gainRampL, outR = r * fade * gainRampR;
            left[pos + i]  += outL;
            right[pos + i] += outR;
            monoAcc = 0.5f * (l + r) * fade;
        }
        pos += n;
        noteAgeSamples += n;
    }

    lastMono = monoAcc * ampGain;

    // numerical safety: a non-finite state is flushed immediately (never propagates)
    if (! network.isFinite() || ! std::isfinite (lastMono) || ! std::isfinite (lastFolded) || ! std::isfinite (loopRet))
    {
        network.reset();filters.reset();
        exA.reset(); exB.reset();
        interaction.reset(); preShaper.reset(); folder.reset();
        decimator.reset(); extUp.reset(); loopUp.reset();sideDecimator.reset();
        lastMono = lastFolded = loopRet = 0.0f;
    }

    if (! fader.isRunning())
    {
        active = false;
        noteId = -1;
        ampEnv.reset();
        modEnv.reset();
        scope = nullptr;
    }
}
} // namespace aeriform::dsp
