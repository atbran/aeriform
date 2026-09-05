#include "SynthEngine.h"
#include "Voice.h"
#include "Effects/Chorus.h"
#include "Effects/Delay.h"
#include "Effects/Reverb.h"
#include "Effects/OutputStage.h"
#include "../MIDI/MidiLearn.h"
#include "../Params/ParameterLayout.h"

namespace aeriform
{
using namespace dsp;

struct SynthEngine::Impl : private juce::MPEInstrument::Listener
{
    // ---------------------------------------------------------------------
    juce::AudioProcessorValueTreeState& apvts;
    VisualizerModel& vis;
    double sampleRate = 44100.0;
    int maxBlock = 512;
    bool prepared = false;

    std::array<Voice, kMaxVoices> voices;
    juce::MPEInstrument mpe;
    juce::AudioBuffer<float> mixBuffer, extBuffer;
    VoiceParams params;
    ModSources globalSources {};
    ModValues globalMod {};
    ModConfig modConfig;
    LFO globalLfo[ids::numLFOs];
    Chorus chorus;
    StereoDelay delay;
    FdnReverb reverb;
    OutputStage output;

    unsigned startCounter = 0;
    int activeVoiceLimit = 8;
    int unisonCount = 1;
    VoiceMode voiceMode = VoiceMode::Poly;
    bool glideLegatoOnly = true;
    bool mpeMode = false;
    int bendRange = 2;
    bool midiConfigured = false;

    // performance controllers
    float modWheel = 0.0f, breathCC = 0.0f, expressionCC = 1.0f, globalBend = 0.0f;
    float couplingIn = 0.0f;
    int noteOnsThisBlock = 0;

    // mono / legato note stack (last-note priority)
    struct HeldNote { int note = 0; float velocity = 0.0f; int noteId = -1; };
    std::array<HeldNote, 64> noteStack {};
    int noteStackSize = 0;
    int monoNoteId = -1;
    float lastMonoNote = 60.0f;

    // notes waiting for a stolen voice to finish its 3 ms fade
    struct Pending
    {
        bool active = false;
        int voice = -1, note = 60, noteId = -1, unisonIndex = 0, unisonCount = 1;
        float velocity = 1.0f, glideFrom = 60.0f;
        bool legato = false;
    };
    std::array<Pending, kMaxVoices> pending {};

    // ---- parameter atomics -------------------------------------------------
    struct AtomicRef
    {
        std::atomic<float>* p = nullptr;
        float get() const noexcept { return p != nullptr ? p->load (std::memory_order_relaxed) : 0.0f; }
    };
    AtomicRef excNoise, excNoiseColor, excPressure, excPluck, excPluckLen, excLP, excHP, excTurb, excVel, excExt, excKeyTrack,
              excClick, excRelNoise, excBreathRnd, excReed;
    AtomicRef envA, envD, envS, envR, envVelPress, artPressBright, artFlowPitch, artInstab, artVariation, artCoupling;
    AtomicRef resCoarse, resFine, resLength, resKeyTrack, resFeedback, resDamping, resBrightness, resDispersion, resShape,
              resReflect, resSat, resMode, resBodyFreq, resBodyRes, resBodyMix, resBodyTrack;
    AtomicRef lfoShape[ids::numLFOs], lfoRate[ids::numLFOs], lfoSync[ids::numLFOs], lfoDiv[ids::numLFOs],
              lfoMode[ids::numLFOs], lfoFade[ids::numLFOs], lfoPhase[ids::numLFOs];
    AtomicRef menvA, menvD, menvS, menvR;
    AtomicRef modSrc[ids::numModSlots], modDst[ids::numModSlots], modDepth[ids::numModSlots];
    AtomicRef chorusMix, chorusRate, chorusDepth, chorusWidth;
    AtomicRef delayMix, delayTime, delaySync, delayDiv, delayFeedback, delayTone, delayPingPong;
    AtomicRef revMix, revSize, revDecay, revDamp, revPre, revWidth, revMod;
    AtomicRef voiceModeP, voiceCountP, glideTimeP, glideLegatoP, unisonVoicesP, unisonDetuneP, unisonSpreadP, bendRangeP,
              mpeEnabledP, outGainP, outHpP, limiterOnP;

    AtomicRef ref (const juce::String& id)
    {
        AtomicRef r;
        r.p = apvts.getRawParameterValue (id);
        jassert (r.p != nullptr);
        return r;
    }

    // ---------------------------------------------------------------------
    Impl (juce::AudioProcessorValueTreeState& s, VisualizerModel& v) : apvts (s), vis (v)
    {
        using namespace ids;
        excNoise = ref (ids::excNoise); excNoiseColor = ref (ids::excNoiseColor); excPressure = ref (ids::excPressure);
        excPluck = ref (ids::excPluck); excPluckLen = ref (excPluckLength); excLP = ref (excLowpass); excHP = ref (excHighpass);
        excTurb = ref (excTurbulence); excVel = ref (excVelocity); excExt = ref (excExternalIn); excKeyTrack = ref (ids::excKeyTrack);
        excClick = ref (excAttackClick); excRelNoise = ref (excReleaseNoise); excBreathRnd = ref (excBreathRandom); excReed = ref (ids::excReed);
        envA = ref (envAttack); envD = ref (envDecay); envS = ref (envSustain); envR = ref (envRelease); envVelPress = ref (envVelToPressure);
        artPressBright = ref (ids::artPressBright); artFlowPitch = ref (ids::artFlowPitch); artInstab = ref (artInstability);
        artVariation = ref (ids::artVariation); artCoupling = ref (ids::artCoupling);
        resCoarse = ref (ids::resCoarse); resFine = ref (ids::resFine); resLength = ref (ids::resLength); resKeyTrack = ref (ids::resKeyTrack);
        resFeedback = ref (ids::resFeedback); resDamping = ref (ids::resDamping); resBrightness = ref (ids::resBrightness);
        resDispersion = ref (ids::resDispersion); resShape = ref (ids::resShape); resReflect = ref (resReflection); resSat = ref (resSaturation);
        resMode = ref (ids::resMode); resBodyFreq = ref (ids::resBodyFreq); resBodyRes = ref (ids::resBodyRes); resBodyMix = ref (ids::resBodyMix);
        resBodyTrack = ref (ids::resBodyTrack);
        for (int i = 0; i < numLFOs; ++i)
        {
            lfoShape[i] = ref (lfoParam (i + 1, lfoShapeSuffix)); lfoRate[i] = ref (lfoParam (i + 1, lfoRateSuffix));
            lfoSync[i] = ref (lfoParam (i + 1, lfoSyncSuffix)); lfoDiv[i] = ref (lfoParam (i + 1, lfoDivSuffix));
            lfoMode[i] = ref (lfoParam (i + 1, lfoModeSuffix)); lfoFade[i] = ref (lfoParam (i + 1, lfoFadeSuffix));
            lfoPhase[i] = ref (lfoParam (i + 1, lfoPhaseSuffix));
        }
        menvA = ref (menvAttack); menvD = ref (menvDecay); menvS = ref (menvSustain); menvR = ref (menvRelease);
        for (int i = 0; i < numModSlots; ++i)
        {
            modSrc[i] = ref (modParam (i + 1, modSrcSuffix)); modDst[i] = ref (modParam (i + 1, modDstSuffix));
            modDepth[i] = ref (modParam (i + 1, modDepthSuffix));
        }
        chorusMix = ref (ids::chorusMix); chorusRate = ref (ids::chorusRate); chorusDepth = ref (ids::chorusDepth); chorusWidth = ref (ids::chorusWidth);
        delayMix = ref (ids::delayMix); delayTime = ref (ids::delayTime); delaySync = ref (ids::delaySync); delayDiv = ref (ids::delayDiv);
        delayFeedback = ref (ids::delayFeedback); delayTone = ref (ids::delayTone); delayPingPong = ref (ids::delayPingPong);
        revMix = ref (reverbMix); revSize = ref (reverbSize); revDecay = ref (reverbDecay); revDamp = ref (reverbDamping);
        revPre = ref (reverbPreDelay); revWidth = ref (reverbWidth); revMod = ref (reverbModulation);
        voiceModeP = ref (ids::voiceMode); voiceCountP = ref (voiceCount); glideTimeP = ref (glideTime); glideLegatoP = ref (ids::glideLegatoOnly);
        unisonVoicesP = ref (unisonVoices); unisonDetuneP = ref (unisonDetune); unisonSpreadP = ref (unisonSpread); bendRangeP = ref (ids::bendRange);
        mpeEnabledP = ref (mpeEnabled); outGainP = ref (outGain); outHpP = ref (outHighpass); limiterOnP = ref (limiterOn);

        mpe.addListener (this);
    }

    ~Impl() override { mpe.removeListener (this); }

    // ---------------------------------------------------------------------
    void prepare (double sr, int block)
    {
        sampleRate = sr;
        maxBlock = juce::jmax (1, block);
        mixBuffer.setSize (2, maxBlock, false, true, true);
        extBuffer.setSize (1, maxBlock, false, true, true);
        for (int i = 0; i < kMaxVoices; ++i)
            voices[(size_t) i].prepare (sr, i);
        for (int i = 0; i < ids::numLFOs; ++i)
        {
            globalLfo[i].setSampleRate ((float) sr);
            globalLfo[i].seed (0x1234u + (uint32_t) i);
            globalLfo[i].resetFade();
        }
        chorus.prepare (sr);
        delay.prepare (sr);
        reverb.prepare (sr);
        output.prepare (sr);
        for (auto& p : pending) p.active = false;
        noteStackSize = 0;
        monoNoteId = -1;
        midiConfigured = false;
        prepared = true;
        readParams (120.0);
    }

    void reset()
    {
        mpe.releaseAllNotes();
        for (auto& v : voices) v.reset();
        for (auto& p : pending) p.active = false;
        noteStackSize = 0;
        monoNoteId = -1;
        chorus.reset();
        delay.reset();
        reverb.reset();
        output.reset();
        couplingIn = 0.0f;
    }

    void allNotesOff()
    {
        mpe.releaseAllNotes();   // triggers noteReleased for every note -> voices go into release
        for (auto& p : pending) p.active = false;
        noteStackSize = 0;
        monoNoteId = -1;
    }

    // ---------------------------------------------------------------------
    // The MPEInstrument releases every note when its bend range changes, so it is configured
    // once per mode with a fixed internal range and the user bend range is applied by scaling.
    static constexpr int kInternalBendRange = 48;

    void configureMidi()
    {
        const bool wantMpe = mpeEnabledP.get() > 0.5f;
        bendRange = juce::jlimit (1, 24, (int) std::lround (bendRangeP.get()));
        if (midiConfigured && wantMpe == mpeMode) return;

        mpeMode = wantMpe;
        midiConfigured = true;
        if (mpeMode)
        {
            juce::MPEZoneLayout layout;
            layout.setLowerZone (15, kInternalBendRange, 2);
            mpe.setZoneLayout (layout);
        }
        else
        {
            mpe.enableLegacyMode (kInternalBendRange, juce::Range<int> (1, 17));
        }
    }

    float scaledBend (const juce::MPENote& note) const noexcept
    {
        const float total = (float) note.totalPitchbendInSemitones;
        return mpeMode ? total : total * (float) bendRange / (float) kInternalBendRange;
    }

    void readParams (double bpm)
    {
        auto& e = params.exciter;
        e.noise = excNoise.get(); e.noiseColor = excNoiseColor.get(); e.pluck = excPluck.get(); e.pluckLengthMs = excPluckLen.get();
        e.lowpassHz = excLP.get(); e.highpassHz = excHP.get(); e.turbulence = excTurb.get(); e.velocityAmount = excVel.get();
        e.externalIn = excExt.get(); e.keyTrack = excKeyTrack.get(); e.attackClick = excClick.get(); e.releaseNoise = excRelNoise.get();
        e.breathRandom = excBreathRnd.get(); e.pressureBright = artPressBright.get();
        params.pressure = excPressure.get();
        params.reed = excReed.get();

        params.envAttackMs = envA.get(); params.envDecayMs = envD.get(); params.envSustain = envS.get(); params.envReleaseMs = envR.get();
        params.velToPressure = envVelPress.get();
        params.flowPitch = artFlowPitch.get(); params.instability = artInstab.get(); params.variation = artVariation.get();
        params.coupling = artCoupling.get();

        auto& r = params.resonator;
        r.feedback = resFeedback.get(); r.damping = resDamping.get(); r.brightness = resBrightness.get(); r.dispersion = resDispersion.get();
        r.shape = resShape.get(); r.reflection = resReflect.get(); r.saturation = resSat.get();
        r.mode = (ResMode) juce::jlimit (0, (int) ResMode::Count - 1, (int) std::lround (resMode.get()));
        r.bodyFreqHz = resBodyFreq.get(); r.bodyRes = resBodyRes.get(); r.bodyMix = resBodyMix.get();
        params.coarse = resCoarse.get(); params.fine = resFine.get(); params.length = resLength.get(); params.keyTrack = resKeyTrack.get();

        for (int i = 0; i < ids::numLFOs; ++i)
        {
            auto& l = params.lfo[i];
            l.shape = (LfoShape) juce::jlimit (0, (int) LfoShape::Count - 1, (int) std::lround (lfoShape[i].get()));
            l.rateHz = lfoRate[i].get(); l.sync = lfoSync[i].get() > 0.5f; l.division = (int) std::lround (lfoDiv[i].get());
            l.mode = (LfoMode) juce::jlimit (0, (int) LfoMode::Count - 1, (int) std::lround (lfoMode[i].get()));
            l.fadeMs = lfoFade[i].get(); l.phaseDeg = lfoPhase[i].get();
        }
        params.menvAttackMs = menvA.get(); params.menvDecayMs = menvD.get(); params.menvSustain = menvS.get(); params.menvReleaseMs = menvR.get();

        for (int i = 0; i < ids::numModSlots; ++i)
        {
            auto& s = modConfig.slots[(size_t) i];
            s.source = (ModSource) juce::jlimit (0, (int) ModSource::Count - 1, (int) std::lround (modSrc[i].get()));
            s.dest = (ModDest) juce::jlimit (0, (int) ModDest::Count - 1, (int) std::lround (modDst[i].get()));
            s.depth = modDepth[i].get();
        }
        params.mod = modConfig;

        params.unisonDetuneCents = unisonDetuneP.get(); params.unisonSpread = unisonSpreadP.get();
        params.glideMs = glideTimeP.get();
        params.tempoBpm = bpm;
        glideLegatoOnly = glideLegatoP.get() > 0.5f;
        voiceMode = (VoiceMode) juce::jlimit (0, (int) VoiceMode::Count - 1, (int) std::lround (voiceModeP.get()));
        activeVoiceLimit = juce::jlimit (1, kMaxVoices, (int) std::lround (voiceCountP.get()));
        unisonCount = juce::jlimit (1, 4, (int) std::lround (unisonVoicesP.get()));

        // keep global (free-running) LFOs aligned with the voice LFO settings
        for (int i = 0; i < ids::numLFOs; ++i)
        {
            globalLfo[i].setShape (params.lfo[i].shape);
            float hz = params.lfo[i].rateHz;
            if (params.lfo[i].sync)
                hz = (float) ((bpm > 1.0 ? bpm : 120.0) / 60.0 / choices::syncDivisionBeats (params.lfo[i].division));
            globalLfo[i].setRate (hz);
        }

        configureMidi();
    }

    // ---------------------------------------------------------------------
    // voice allocation
    int findVoiceToUse (bool& needsSteal)
    {
        needsSteal = false;
        // 1) free voice
        for (int i = 0; i < activeVoiceLimit; ++i)
            if (! voices[(size_t) i].isActive() && ! isVoicePending (i)) return i;

        // 2) oldest releasing voice
        int best = -1; unsigned bestOrder = 0xFFFFFFFFu;
        for (int i = 0; i < activeVoiceLimit; ++i)
        {
            auto& v = voices[(size_t) i];
            if (v.isActive() && v.isReleasing() && ! isVoicePending (i) && v.getStartOrder() < bestOrder)
            {
                best = i; bestOrder = v.getStartOrder();
            }
        }
        // 3) oldest playing voice
        if (best < 0)
            for (int i = 0; i < activeVoiceLimit; ++i)
            {
                auto& v = voices[(size_t) i];
                if (! isVoicePending (i) && v.getStartOrder() < bestOrder) { best = i; bestOrder = v.getStartOrder(); }
            }
        if (best < 0) best = 0;
        needsSteal = true;
        return best;
    }

    bool isVoicePending (int voiceIndex) const noexcept
    {
        for (const auto& p : pending)
            if (p.active && p.voice == voiceIndex) return true;
        return false;
    }

    void startVoice (int voiceIndex, int note, float velocity, int noteId, int uIndex, int uCount, float glideFrom, bool legato)
    {
        auto& v = voices[(size_t) voiceIndex];
        v.setStartOrder (++startCounter);
        v.startNote (note, velocity, glideFrom, legato, uIndex, uCount, noteId, params);
    }

    void queueOrStart (int voiceIndex, bool steal, int note, float velocity, int noteId, int uIndex, int uCount, float glideFrom, bool legato)
    {
        if (! steal)
        {
            startVoice (voiceIndex, note, velocity, noteId, uIndex, uCount, glideFrom, legato);
            return;
        }
        voices[(size_t) voiceIndex].kill (3.0f);
        voices[(size_t) voiceIndex].setStartOrder (++startCounter);   // stolen voice counts as newest
        for (auto& p : pending)
        {
            if (p.active) continue;
            p.active = true; p.voice = voiceIndex; p.note = note; p.noteId = noteId; p.unisonIndex = uIndex; p.unisonCount = uCount;
            p.velocity = velocity; p.glideFrom = glideFrom; p.legato = legato;
            return;
        }
    }

    void startPendingIfReady()
    {
        for (auto& p : pending)
        {
            if (! p.active) continue;
            if (voices[(size_t) p.voice].isActive()) continue;
            p.active = false;
            startVoice (p.voice, p.note, p.velocity, p.noteId, p.unisonIndex, p.unisonCount, p.glideFrom, p.legato);
        }
    }

    // ---- MPEInstrument::Listener --------------------------------------------------------
    void noteAdded (juce::MPENote note) override
    {
        ++noteOnsThisBlock;
        const float velocity = note.noteOnVelocity.asUnsignedFloat();
        const int midiNote = note.initialNote;

        if (voiceMode == VoiceMode::Poly)
        {
            const int count = juce::jmin (unisonCount, activeVoiceLimit);
            for (int u = 0; u < count; ++u)
            {
                bool steal = false;
                const int vi = findVoiceToUse (steal);
                queueOrStart (vi, steal, midiNote, velocity, note.noteID, u, count, (float) midiNote, false);
                applyNoteExpression (vi, note);
            }
            return;
        }

        // ---- mono / legato ---------------------------------------------------------------
        const bool anyHeld = noteStackSize > 0;
        pushHeld (midiNote, velocity, note.noteID);
        const bool legatoTransition = anyHeld;
        const bool retrigger = voiceMode == VoiceMode::Mono || ! legatoTransition;
        const bool glide = params.glideMs > 0.0f && (! glideLegatoOnly || legatoTransition);
        const int count = juce::jmin (unisonCount, activeVoiceLimit);

        bool monoVoicesActive = false;
        for (int u = 0; u < count; ++u)
            if (voices[(size_t) u].isActive() && voices[(size_t) u].getNoteId() == monoNoteId) monoVoicesActive = true;

        if (monoVoicesActive && (legatoTransition || voices[0].isReleasing()))
        {
            for (int u = 0; u < count; ++u)
            {
                auto& v = voices[(size_t) u];
                if (v.isActive() && v.getNoteId() == monoNoteId)
                {
                    VoiceParams p = params;
                    if (! glide) p.glideMs = 0.0f;
                    v.changeNote (midiNote, velocity, retrigger, p);
                    v.adoptNoteId (note.noteID);
                    v.setStartOrder (++startCounter);
                }
            }
        }
        else
        {
            for (int u = 0; u < count; ++u)
            {
                auto& v = voices[(size_t) u];
                if (v.isActive()) v.kill (3.0f);
                const bool steal = v.isActive();
                queueOrStart (u, steal, midiNote, velocity, note.noteID, u, count, lastMonoNote, glide);
            }
        }
        monoNoteId = note.noteID;
        lastMonoNote = (float) midiNote;
        for (int u = 0; u < count; ++u) applyNoteExpression (u, note);
    }

    void applyNoteExpression (int voiceIndex, const juce::MPENote& note)
    {
        auto& v = voices[(size_t) voiceIndex];
        v.setBend (scaledBend (note));
        v.setPressure (note.pressure.asUnsignedFloat());
        v.setSlide (note.timbre.asUnsignedFloat());
    }

    void noteReleased (juce::MPENote note) override
    {
        if (voiceMode == VoiceMode::Poly)
        {
            for (auto& v : voices)
                if (v.isActive() && v.getNoteId() == note.noteID) v.stopNote (params);
            for (auto& p : pending)
                if (p.active && p.noteId == note.noteID) p.active = false;
            return;
        }

        removeHeld (note.noteID);
        if (note.noteID != monoNoteId)
            return;   // a note that is not the sounding one was released: nothing audible changes

        if (noteStackSize > 0)
        {
            // return to the most recent still-held note
            const auto& prev = noteStack[(size_t) noteStackSize - 1];
            const bool glide = params.glideMs > 0.0f;
            const int count = juce::jmin (unisonCount, activeVoiceLimit);
            for (int u = 0; u < count; ++u)
            {
                auto& v = voices[(size_t) u];
                if (v.isActive() && v.getNoteId() == monoNoteId)
                {
                    VoiceParams p = params;
                    if (! glide) p.glideMs = 0.0f;
                    v.changeNote (prev.note, prev.velocity, voiceMode == VoiceMode::Mono, p);
                }
            }
            // the returning note takes over the id so later releases match
            monoNoteId = prev.noteId;
            lastMonoNote = (float) prev.note;
            for (auto& v : voices)
                if (v.isActive() && v.getUnisonIndex() < count) v.setStartOrder (++startCounter);
            retargetMonoVoiceIds (prev.noteId);
        }
        else
        {
            for (auto& v : voices)
                if (v.isActive() && v.getNoteId() == monoNoteId) v.stopNote (params);
            for (auto& p : pending) p.active = false;
            monoNoteId = -1;
        }
    }

    void retargetMonoVoiceIds (int newId)
    {
        const int count = juce::jmin (unisonCount, activeVoiceLimit);
        for (int u = 0; u < count; ++u)
            voices[(size_t) u].adoptNoteId (newId);
    }

    void notePressureChanged (juce::MPENote note) override
    {
        for (auto& v : voices)
            if (v.isActive() && v.getNoteId() == note.noteID) v.setPressure (note.pressure.asUnsignedFloat());
    }
    void notePitchbendChanged (juce::MPENote note) override
    {
        for (auto& v : voices)
            if (v.isActive() && v.getNoteId() == note.noteID) v.setBend (scaledBend (note));
    }
    void noteTimbreChanged (juce::MPENote note) override
    {
        for (auto& v : voices)
            if (v.isActive() && v.getNoteId() == note.noteID) v.setSlide (note.timbre.asUnsignedFloat());
    }
    void noteKeyStateChanged (juce::MPENote) override {}
    void zoneLayoutChanged() override {}

    void pushHeld (int note, float velocity, int noteId)
    {
        if (noteStackSize >= (int) noteStack.size())
        {
            for (int i = 1; i < noteStackSize; ++i) noteStack[(size_t) i - 1] = noteStack[(size_t) i];
            --noteStackSize;
        }
        noteStack[(size_t) noteStackSize++] = { note, velocity, noteId };
    }

    void removeHeld (int noteId)
    {
        for (int i = 0; i < noteStackSize; ++i)
        {
            if (noteStack[(size_t) i].noteId == noteId)
            {
                for (int j = i + 1; j < noteStackSize; ++j) noteStack[(size_t) j - 1] = noteStack[(size_t) j];
                --noteStackSize;
                return;
            }
        }
    }

    // ---------------------------------------------------------------------
    void handleMidi (const juce::MidiMessage& m, MidiLearn* learn)
    {
        if (m.isController())
        {
            const int cc = m.getControllerNumber();
            const int value = m.getControllerValue();
            if (learn != nullptr && learn->handleController (cc, value))
                return;   // consumed by a MIDI-learn mapping (or captured while learning)

            switch (cc)
            {
                case 1:  modWheel = (float) value / 127.0f; break;
                case 2:  breathCC = (float) value / 127.0f; break;
                case 11: expressionCC = (float) value / 127.0f; break;
                case 120: for (auto& v : voices) v.kill (3.0f); break;   // all sound off
                default: break;
            }
            mpe.processNextMidiEvent (m);   // sustain, sostenuto, timbre (74), all notes off (123), MPE config
            return;
        }

        if (m.isPitchWheel())
        {
            // remember the master bend as a modulation source (per-note bends are applied via MPENote)
            globalBend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
        }
        mpe.processNextMidiEvent (m);
    }

    // ---------------------------------------------------------------------
    void updateGlobalSources (int numSamples)
    {
        globalSources.fill (0.0f);
        for (int i = 0; i < ids::numLFOs; ++i)
        {
            globalSources[(size_t) ModSource::LFO1 + (size_t) i] = globalLfo[i].advance (numSamples);
            params.lfoGlobalPhase[i] = globalLfo[i].getPhase();
        }
        globalSources[(size_t) ModSource::ModWheel]     = modWheel;
        globalSources[(size_t) ModSource::BreathCC]     = breathCC;
        globalSources[(size_t) ModSource::ExpressionCC] = expressionCC;
        globalSources[(size_t) ModSource::PitchBend]    = globalBend;
    }

    void renderSegment (int start, int numSamples)
    {
        if (numSamples <= 0) return;
        startPendingIfReady();
        updateGlobalSources (numSamples);

        float* L = mixBuffer.getWritePointer (0) + start;
        float* R = mixBuffer.getWritePointer (1) + start;
        const float* ext = extBuffer.getReadPointer (0) + start;

        // sympathetic coupling: a little of the previous segment's total goes into every tube
        const float couple = params.coupling * 0.25f * couplingIn;
        float sum = 0.0f;
        for (auto& v : voices)
        {
            if (! v.isActive()) continue;
            v.render (L, R, numSamples, params, globalSources, ext, couple);
            sum += v.getLastMono();
        }
        couplingIn = fastTanh (sum);
    }

    void process (juce::AudioBuffer<float>& out, const juce::AudioBuffer<float>* extIn, juce::MidiBuffer& midi,
                  const juce::AudioPlayHead::PositionInfo& pos, MidiLearn* learn, bool)
    {
        const int numSamples = out.getNumSamples();
        if (! prepared || numSamples <= 0) { out.clear(); return; }
        if (numSamples > maxBlock)
        {
            // hosts should never exceed the prepared size; render in chunks if one does
            for (int start = 0; start < numSamples; start += maxBlock)
            {
                const int n = juce::jmin (maxBlock, numSamples - start);
                juce::AudioBuffer<float> sub (out.getArrayOfWritePointers(), out.getNumChannels(), start, n);
                juce::MidiBuffer subMidi;
                for (const auto meta : midi)
                    if (meta.samplePosition >= start && meta.samplePosition < start + n)
                        subMidi.addEvent (meta.getMessage(), meta.samplePosition - start);
                process (sub, nullptr, subMidi, pos, learn, false);
            }
            return;
        }

        const double bpm = pos.getBpm().hasValue() ? *pos.getBpm() : 120.0;
        readParams (bpm > 1.0 ? bpm : 120.0);
        noteOnsThisBlock = 0;

        // external input -> mono excitation buffer (copied first: hosts may process in place)
        float* extMono = extBuffer.getWritePointer (0);
        if (extIn != nullptr && extIn->getNumChannels() > 0 && params.exciter.externalIn > 0.0001f)
        {
            const int chans = extIn->getNumChannels();
            for (int i = 0; i < numSamples; ++i)
            {
                float s = 0.0f;
                for (int c = 0; c < chans; ++c) s += extIn->getReadPointer (c)[i];
                extMono[i] = s / (float) chans;
            }
        }
        else
        {
            juce::FloatVectorOperations::clear (extMono, numSamples);
        }

        mixBuffer.clear (0, 0, numSamples);
        mixBuffer.clear (1, 0, numSamples);

        // sample-accurate MIDI: render up to each event, then apply it
        int cursor = 0;
        for (const auto meta : midi)
        {
            const int t = juce::jlimit (0, numSamples, meta.samplePosition);
            renderSegment (cursor, t - cursor);
            cursor = t;
            handleMidi (meta.getMessage(), learn);
        }
        renderSegment (cursor, numSamples - cursor);

        // ---- global modulation of effect mixes ----------------------------------------
        {
            ModSources gs = globalSources;
            const Voice* newest = nullptr;
            for (const auto& v : voices)
                if (v.isActive() && (newest == nullptr || v.getStartOrder() > newest->getStartOrder())) newest = &v;
            if (newest != nullptr)
            {
                const auto& vs = newest->getLastMod();
                juce::ignoreUnused (vs);
                gs[(size_t) ModSource::AmpEnv] = newest->getEnvLevel();
                gs[(size_t) ModSource::Aftertouch] = newest->getPressureLevel();
            }
            ModMatrix::evaluate (modConfig, gs, globalMod);
        }

        float* L = mixBuffer.getWritePointer (0);
        float* R = mixBuffer.getWritePointer (1);

        chorus.setParams (clamp01 (chorusMix.get() + globalMod[(size_t) ModDest::ChorusMix]), chorusRate.get(), chorusDepth.get(), chorusWidth.get());
        chorus.process (L, R, numSamples);

        float delayMs = delayTime.get();
        if (delaySync.get() > 0.5f)
            delayMs = (float) (60000.0 / (bpm > 1.0 ? bpm : 120.0) * choices::syncDivisionBeats ((int) std::lround (delayDiv.get())));
        delay.setParams (clamp01 (delayMix.get() + globalMod[(size_t) ModDest::DelayMix]), delayMs, delayFeedback.get(), delayTone.get(), delayPingPong.get() > 0.5f);
        delay.process (L, R, numSamples);

        reverb.setParams (clamp01 (revMix.get() + globalMod[(size_t) ModDest::ReverbMix]), revSize.get(), revDecay.get(), revDamp.get(),
                          revPre.get(), revWidth.get(), revMod.get());
        reverb.process (L, R, numSamples);

        output.setParams (outGainP.get(), outHpP.get(), limiterOnP.get() > 0.5f);
        output.process (L, R, numSamples);

        // ---- numerical safety net -------------------------------------------------------
        bool finite = true;
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            const float a = std::fabs (L[i]), b = std::fabs (R[i]);
            if (! (a <= 8.0f && b <= 8.0f)) { finite = false; break; }   // NaN fails the comparison too
            peak = std::max (peak, std::max (a, b));
            vis.pushScopeSample (0.5f * (L[i] + R[i]));
        }
        if (! finite)
        {
            mixBuffer.clear();
            reset();
            peak = 0.0f;
        }

        // ---- write to the host buffer --------------------------------------------------
        const int outChans = out.getNumChannels();
        if (outChans >= 2)
        {
            out.copyFrom (0, 0, mixBuffer, 0, 0, numSamples);
            out.copyFrom (1, 0, mixBuffer, 1, 0, numSamples);
            for (int c = 2; c < outChans; ++c) out.clear (c, 0, numSamples);
        }
        else if (outChans == 1)
        {
            out.copyFrom (0, 0, mixBuffer, 0, 0, numSamples);
            out.addFrom (0, 0, mixBuffer, 1, 0, numSamples);
            out.applyGain (0, 0, numSamples, 0.5f);
        }

        updateVisualizer (peak);
    }

    void updateVisualizer (float peak)
    {
        int active = 0;
        float pressureSum = 0.0f, energySum = 0.0f;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            const auto& v = voices[(size_t) i];
            auto& s = vis.voices[(size_t) i];
            const bool on = v.isActive();
            s.active.store (on ? 1 : 0, std::memory_order_relaxed);
            s.energy.store (on ? v.getEnergy() : 0.0f, std::memory_order_relaxed);
            s.pressure.store (on ? v.getPressureLevel() : 0.0f, std::memory_order_relaxed);
            s.pitchHz.store (on ? v.getFreqHz() : 0.0f, std::memory_order_relaxed);
            if (on) { ++active; pressureSum += v.getPressureLevel(); energySum += v.getEnergy(); }
        }
        vis.activeVoices.store (active, std::memory_order_relaxed);
        vis.masterPeak.store (peak, std::memory_order_relaxed);
        vis.masterPressure.store (active > 0 ? pressureSum / (float) active : 0.0f, std::memory_order_relaxed);
        vis.masterEnergy.store (active > 0 ? energySum / (float) active : 0.0f, std::memory_order_relaxed);
        vis.limiterGain.store (output.getLimiterGain(), std::memory_order_relaxed);
        if (noteOnsThisBlock > 0) vis.midiActivity.fetch_add (noteOnsThisBlock, std::memory_order_relaxed);

        // live modulation of the newest voice (global-only destinations come from the global evaluation)
        const Voice* newest = nullptr;
        for (const auto& v : voices)
            if (v.isActive() && (newest == nullptr || v.getStartOrder() > newest->getStartOrder())) newest = &v;
        for (size_t d = 0; d < (size_t) ModDest::Count; ++d)
        {
            float value = globalMod[d];
            if (newest != nullptr && d != (size_t) ModDest::ChorusMix && d != (size_t) ModDest::DelayMix && d != (size_t) ModDest::ReverbMix)
                value = newest->getLastMod()[d];
            vis.liveMod[d].store (value, std::memory_order_relaxed);
        }
    }

    int activeVoiceCount() const noexcept
    {
        int n = 0;
        for (const auto& v : voices) if (v.isActive()) ++n;
        return n;
    }
};

// ---------------------------------------------------------------------------
SynthEngine::SynthEngine (juce::AudioProcessorValueTreeState& state, VisualizerModel& visualizer)
    : impl (std::make_unique<Impl> (state, visualizer)) {}

SynthEngine::~SynthEngine() = default;

void SynthEngine::prepare (double sampleRate, int maxBlockSize) { impl->prepare (sampleRate, maxBlockSize); }
void SynthEngine::reset() { impl->reset(); }
void SynthEngine::allNotesOff() { impl->allNotesOff(); }

void SynthEngine::process (juce::AudioBuffer<float>& output, const juce::AudioBuffer<float>* externalInput, juce::MidiBuffer& midi,
                           const juce::AudioPlayHead::PositionInfo& position, MidiLearn* midiLearn, bool isNonRealtime)
{
    impl->process (output, externalInput, midi, position, midiLearn, isNonRealtime);
}

int SynthEngine::getActiveVoiceCount() const noexcept { return impl->activeVoiceCount(); }

dsp::ModConfig SynthEngine::getModConfig() const
{
    dsp::ModConfig cfg;
    for (int i = 0; i < ids::numModSlots; ++i)
    {
        auto& s = cfg.slots[(size_t) i];
        s.source = (ModSource) juce::jlimit (0, (int) ModSource::Count - 1, (int) std::lround (impl->modSrc[i].get()));
        s.dest = (ModDest) juce::jlimit (0, (int) ModDest::Count - 1, (int) std::lround (impl->modDst[i].get()));
        s.depth = impl->modDepth[i].get();
    }
    return cfg;
}
} // namespace aeriform
