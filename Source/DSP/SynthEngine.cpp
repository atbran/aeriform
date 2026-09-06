#include "SynthEngine.h"
#include "Voice.h"
#include "NetworkParamsBuilder.h"
#include "Effects/Chorus.h"
#include "Effects/Delay.h"
#include "Effects/Reverb.h"
#include "Effects/OutputStage.h"
#include "../MIDI/MidiLearn.h"
#include "../Params/ParameterLayout.h"

#ifndef AERIFORM_FX
 #define AERIFORM_FX 0
#endif

namespace aeriform
{
using namespace dsp;

#if AERIFORM_FX
// -----------------------------------------------------------------------------
// Aeriform FX: the always-on main-input resonator path.
//
// One resonator network + body filter, excited continuously by the (input-gained)
// DAW main input. There is no MIDI note, no amplitude envelope and no per-block
// reset: an input transient rings the network and the energy decays naturally, so
// the tail keeps sounding after the input stops. The resonator maths, tuning and
// routing are exactly the polyphonic voice's - buildNetworkParams() is shared -
// with the FX Root parameter standing in for the played note and the Pressure
// control driving the reed / jet junctions.
// -----------------------------------------------------------------------------
struct FxResonatorPath
{
    dsp::ResonatorNetwork network;
    dsp::SVF bodyL, bodyR;
    dsp::NetworkParams netParams;
    double sampleRate = 44100.0;
    float bodyK = 1.0f, bodyGain = 0.0f, bodyMix = 0.0f;
    float loopRet = 0.0f;
    bool firstUpdate = true;

    void prepare (double sr)
    {
        sampleRate = sr;
        network.prepare ((float) sr);
        bodyL.setSampleRate ((float) sr);
        bodyR.setSampleRate ((float) sr);
        reset();
    }

    void reset()
    {
        network.reset();
        bodyL.reset();
        bodyR.reset();
        loopRet = 0.0f;
        firstUpdate = true;
    }

    void updateControl (const dsp::VoiceParams& p, const dsp::ModValues& mod, float rootNote, float pressure)
    {
        dsp::buildNetworkParams (netParams, p, rootNote, pressure, mod);
        network.update (netParams, firstUpdate);
        firstUpdate = false;

        const float rootHz = dsp::midiNoteToHz (rootNote);
        const float bodyFreq = p.get (P::resBodyFreq) * std::exp2 (mod[(size_t) ModDest::BodyFreq] * 3.0f)
                               * std::pow (rootHz / 261.63f, p.get (P::resBodyTrack));
        const float qv = 0.5f + 12.0f * dsp::clamp01 (p.get (P::resBodyRes));
        const float fc = std::clamp (bodyFreq, 40.0f, (float) sampleRate * 0.4f);
        bodyL.set (fc, qv);
        bodyR.set (fc, qv);
        bodyK = 1.0f / qv;
        bodyMix = dsp::clamp01 (p.get (P::resBodyMix) + mod[(size_t) ModDest::BodyMix]);
        bodyGain = bodyMix * (1.0f + 2.0f * dsp::clamp01 (p.get (P::resBodyRes)));
    }

    /** Adds the wet resonator signal into L / R. exc = mono excitation (already input-gained). */
    void process (float* L, float* R, const float* exc, int numSamples, float pressure)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float l, r;
            const float loopNet = netParams.loopOn ? loopRet : 0.0f;
            network.next (exc[i], loopNet, pressure, l, r);
            loopRet = network.loopReturn();
            if (bodyMix > 0.0005f)
            {
                l = l * (1.0f - 0.7f * bodyMix) + bodyL.bandpass (l) * bodyK * bodyGain;
                r = r * (1.0f - 0.7f * bodyMix) + bodyR.bandpass (r) * bodyK * bodyGain;
            }
            L[i] += l;
            R[i] += r;
        }
        if (! network.isFinite())
        {
            network.reset();
            bodyL.reset();
            bodyR.reset();
            loopRet = 0.0f;
        }
    }
};
#endif // AERIFORM_FX

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
    std::vector<float> sharedNoise;          // engine-wide noise stream at the oversampled rate
    std::vector<float> captureRing;          // last 250 ms of sidechain input for Freeze
    int captureWrite = 0, freezeStart = 0, freezeLen = 0, freezePos = 0;
    bool frozen = false;
    Noise sharedRng;

    VoiceParams params;
    std::array<std::atomic<float>*, (size_t) kNumParams> atomics {};
    ModSources globalSources {};
    ModValues globalMod {};
    LFO globalLfo[ids::numLFOs];
    Chorus chorus;
    StereoDelay delay;
    FdnReverb reverb;
    OutputStage output;
    OnePole sidechainFollower;
    float sidechainEnv = 0.0f;

#if AERIFORM_FX
    FxResonatorPath fxPath;
    juce::AudioBuffer<float> dryBuffer;    // untouched stereo main input, kept for the dry / wet mix
    juce::AudioBuffer<float> fxExcBuffer;  // mono, input-gained main input feeding the FX resonator network
    float fxInGain = 1.0f, fxMix = 1.0f, fxOutGain = 1.0f;   // per-sample-smoothed
#endif

    unsigned startCounter = 0;
    int activeVoiceLimit = 8;
    int unisonCount = 1;
    VoiceMode voiceMode = VoiceMode::Poly;
    bool glideLegatoOnly = true;
    bool mpeMode = false;
    int bendRange = 2;
    bool midiConfigured = false;
    float alternate = 1.0f;

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

    // ---------------------------------------------------------------------
    Impl (juce::AudioProcessorValueTreeState& s, VisualizerModel& v) : apvts (s), vis (v)
    {
        for (int i = 0; i < kNumParams; ++i)
        {
            atomics[(size_t) i] = apvts.getRawParameterValue (ids::all[i]);
            jassert (atomics[(size_t) i] != nullptr);
        }
        sharedRng.seed (0xC0FFEE42u);
        mpe.addListener (this);
    }

    ~Impl() override { mpe.removeListener (this); }

    float raw (P p) const noexcept
    {
        auto* a = atomics[(size_t) p];
        return a != nullptr ? a->load (std::memory_order_relaxed) : 0.0f;
    }

    // ---------------------------------------------------------------------
    void prepare (double sr, int block)
    {
        sampleRate = sr;
        maxBlock = juce::jmax (1, block);
        mixBuffer.setSize (2, maxBlock, false, true, true);
        extBuffer.setSize (1, maxBlock, false, true, true);
        sharedNoise.assign ((size_t) maxBlock * (size_t) Oversampler::kMaxFactor, 0.0f);
        captureRing.assign ((size_t) (sr * 0.25) + 16, 0.0f);
        captureWrite = 0; frozen = false;
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
        sidechainFollower.setCutoff (20.0f, (float) sr);
#if AERIFORM_FX
        fxPath.prepare (sr);
        dryBuffer.setSize (2, maxBlock, false, true, true);
        fxExcBuffer.setSize (1, maxBlock, false, true, true);
        fxInGain = dbToGain (raw (P::fxInputGain));
        fxMix = clamp01 (raw (P::fxMix));
        fxOutGain = dbToGain (raw (P::fxOutputGain));
#endif
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
#if AERIFORM_FX
        fxPath.reset();
#endif
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
        const bool wantMpe = raw (P::mpeEnabled) > 0.5f;
        bendRange = juce::jlimit (1, 24, (int) std::lround (raw (P::bendRange)));
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
        for (int i = 0; i < kNumParams; ++i)
            params.v[(size_t) i] = atomics[(size_t) i]->load (std::memory_order_relaxed);
        params.tempoBpm = bpm;
        params.derive();

        glideLegatoOnly = params.getb (P::glideLegatoOnly);
        voiceMode = params.getEnum (P::voiceMode, VoiceMode::Count);
        activeVoiceLimit = juce::jlimit (1, kMaxVoices, params.geti (P::voiceCount));
        unisonCount = juce::jlimit (1, 4, params.geti (P::unisonVoices));

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
        for (int i = 0; i < activeVoiceLimit; ++i)
            if (! voices[(size_t) i].isActive() && ! isVoicePending (i)) return i;

        int best = -1; unsigned bestOrder = 0xFFFFFFFFu;
        for (int i = 0; i < activeVoiceLimit; ++i)
        {
            auto& v = voices[(size_t) i];
            if (v.isActive() && v.isReleasing() && ! isVoicePending (i) && v.getStartOrder() < bestOrder)
            {
                best = i; bestOrder = v.getStartOrder();
            }
        }
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
        v.setAlternate (alternate);
        v.startNote (note, velocity, glideFrom, legato, uIndex, uCount, noteId, params);
        // only the newest voice feeds the exciter / folder scopes
        for (auto& other : voices) other.setScopeTarget (nullptr);
        v.setScopeTarget (&vis);
    }

    void queueOrStart (int voiceIndex, bool steal, int note, float velocity, int noteId, int uIndex, int uCount, float glideFrom, bool legato)
    {
        if (! steal)
        {
            startVoice (voiceIndex, note, velocity, noteId, uIndex, uCount, glideFrom, legato);
            return;
        }
        voices[(size_t) voiceIndex].kill (3.0f);
        voices[(size_t) voiceIndex].setStartOrder (++startCounter);
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
        alternate = -alternate;
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
        const bool glide = params.get (P::glideTime) > 0.0f && (! glideLegatoOnly || legatoTransition);
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
                    if (! glide) p.v[(size_t) P::glideTime] = 0.0f;
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
            return;

        if (noteStackSize > 0)
        {
            const auto& prev = noteStack[(size_t) noteStackSize - 1];
            const bool glide = params.get (P::glideTime) > 0.0f;
            const int count = juce::jmin (unisonCount, activeVoiceLimit);
            for (int u = 0; u < count; ++u)
            {
                auto& v = voices[(size_t) u];
                if (v.isActive() && v.getNoteId() == monoNoteId)
                {
                    VoiceParams p = params;
                    if (! glide) p.v[(size_t) P::glideTime] = 0.0f;
                    v.changeNote (prev.note, prev.velocity, voiceMode == VoiceMode::Mono, p);
                }
            }
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
                return;

            switch (cc)
            {
                case 1:  modWheel = (float) value / 127.0f; break;
                case 2:  breathCC = (float) value / 127.0f; break;
                case 11: expressionCC = (float) value / 127.0f; break;
                case 120: for (auto& v : voices) v.kill (3.0f); break;
                default: break;
            }
            mpe.processNextMidiEvent (m);
            return;
        }

        if (m.isPitchWheel())
            globalBend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
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
        globalSources[(size_t) ModSource::SidechainEnv] = std::min (1.0f, sidechainEnv * 3.0f);
    }

    void renderSegment (int start, int numSamples)
    {
        if (numSamples <= 0) return;
        startPendingIfReady();
        updateGlobalSources (numSamples);

        float* L = mixBuffer.getWritePointer (0) + start;
        float* R = mixBuffer.getWritePointer (1) + start;
        const float* ext = extBuffer.getReadPointer (0) + start;
        const float* noise = sharedNoise.data() + (size_t) start * (size_t) params.osFactor;

        const float couple = params.get (P::artCoupling) * 0.25f * couplingIn;
        float sum = 0.0f;
        for (auto& v : voices)
        {
            if (! v.isActive()) continue;
            v.render (L, R, numSamples, params, globalSources, ext, noise, couple);
            sum += v.getLastMono();
        }
        couplingIn = fastTanh (sum);
    }

    void prepareSidechain (const juce::AudioBuffer<float>* extIn, int numSamples)
    {
        float* extMono = extBuffer.getWritePointer (0);
        const bool wantFreeze = raw (P::exaScFreeze) > 0.5f || raw (P::exbScFreeze) > 0.5f;
        const int ringSize = (int) captureRing.size();

        if (extIn != nullptr && extIn->getNumChannels() > 0)
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

        if (wantFreeze && ! frozen)
        {
            frozen = true;
            freezeLen = juce::jmax (1, ringSize - 16);
            freezeStart = (captureWrite - freezeLen + ringSize) % ringSize;
            freezePos = 0;
        }
        else if (! wantFreeze)
        {
            frozen = false;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            if (frozen)
            {
                extMono[i] = captureRing[(size_t) ((freezeStart + freezePos) % ringSize)];
                freezePos = (freezePos + 1) % freezeLen;
            }
            else
            {
                captureRing[(size_t) captureWrite] = extMono[i];
                captureWrite = (captureWrite + 1) % ringSize;
            }
            sidechainEnv = sidechainFollower.process (std::fabs (extMono[i]));
        }
    }

#if AERIFORM_FX
    // Aeriform FX: capture the untouched stereo main input for the dry / wet mix and
    // build the input-gained mono excitation that drives the resonator network.
    void prepareFxInput (const juce::AudioBuffer<float>* mainIn, int numSamples)
    {
        float* dryL = dryBuffer.getWritePointer (0);
        float* dryR = dryBuffer.getWritePointer (1);
        float* exc  = fxExcBuffer.getWritePointer (0);

        const float targetGain = dbToGain (raw (P::fxInputGain));
        const float gainStep = (targetGain - fxInGain) / (float) numSamples;

        if (mainIn != nullptr && mainIn->getNumChannels() > 0)
        {
            const int ch = mainIn->getNumChannels();
            const float* inL = mainIn->getReadPointer (0);
            const float* inR = mainIn->getReadPointer (ch > 1 ? 1 : 0);
            for (int i = 0; i < numSamples; ++i)
            {
                dryL[i] = inL[i];
                dryR[i] = inR[i];
                fxInGain += gainStep;
                exc[i] = 0.5f * (inL[i] + inR[i]) * fxInGain;
            }
        }
        else
        {
            juce::FloatVectorOperations::clear (dryL, numSamples);
            juce::FloatVectorOperations::clear (dryR, numSamples);
            juce::FloatVectorOperations::clear (exc, numSamples);
            fxInGain = targetGain;
        }
    }

    // Aeriform FX: linear dry / wet crossfade (unity at both extremes; the dry signal
    // has bypassed the resonators and every non-linear stage) then the FX output gain.
    // Writes back into mixBuffer so the existing mono / stereo copy-out is unchanged.
    void applyFxMixAndOutput (int numSamples)
    {
        float* L = mixBuffer.getWritePointer (0);
        float* R = mixBuffer.getWritePointer (1);
        const float* dryL = dryBuffer.getReadPointer (0);
        const float* dryR = dryBuffer.getReadPointer (1);

        const float targetMix = clamp01 (raw (P::fxMix));
        const float targetOut = dbToGain (raw (P::fxOutputGain));
        const float mixStep = (targetMix - fxMix) / (float) numSamples;
        const float outStep = (targetOut - fxOutGain) / (float) numSamples;

        for (int i = 0; i < numSamples; ++i)
        {
            fxMix += mixStep;
            fxOutGain += outStep;
            const float wetG = fxMix, dryG = 1.0f - fxMix;
            L[i] = (L[i] * wetG + dryL[i] * dryG) * fxOutGain;
            R[i] = (R[i] * wetG + dryR[i] * dryG) * fxOutGain;
        }
    }
#endif // AERIFORM_FX

    void process (juce::AudioBuffer<float>& out, const juce::AudioBuffer<float>* mainIn,
                  const juce::AudioBuffer<float>* scIn, juce::MidiBuffer& midi,
                  const juce::AudioPlayHead::PositionInfo& pos, MidiLearn* learn, bool)
    {
        const int numSamples = out.getNumSamples();
        if (! prepared || numSamples <= 0) { out.clear(); return; }
        if (numSamples > maxBlock)
        {
            // Slice an input view to [start, start+n). const_cast is safe: the engine only
            // reads external input and copies it to internal buffers before writing output.
            auto sliceView = [] (const juce::AudioBuffer<float>* src, int start, int n) -> juce::AudioBuffer<float>
            {
                if (src == nullptr || src->getNumChannels() == 0) return {};
                return juce::AudioBuffer<float> (const_cast<float* const*> (src->getArrayOfReadPointers()),
                                                 src->getNumChannels(), start, n);
            };
            for (int start = 0; start < numSamples; start += maxBlock)
            {
                const int n = juce::jmin (maxBlock, numSamples - start);
                juce::AudioBuffer<float> sub (out.getArrayOfWritePointers(), out.getNumChannels(), start, n);
                const juce::AudioBuffer<float> subMain = sliceView (mainIn, start, n);
                const juce::AudioBuffer<float> subSc   = sliceView (scIn, start, n);

                juce::MidiBuffer subMidi;
                for (const auto meta : midi)
                    if (meta.samplePosition >= start && meta.samplePosition < start + n)
                        subMidi.addEvent (meta.getMessage(), meta.samplePosition - start);
                process (sub, subMain.getNumChannels() > 0 ? &subMain : nullptr,
                              subSc.getNumChannels() > 0 ? &subSc : nullptr, subMidi, pos, learn, false);
            }
            return;
        }

        const double bpm = pos.getBpm().hasValue() ? *pos.getBpm() : 120.0;
        readParams (bpm > 1.0 ? bpm : 120.0);
        noteOnsThisBlock = 0;

        prepareSidechain (scIn, numSamples);
#if AERIFORM_FX
        prepareFxInput (mainIn, numSamples);
#endif

        // shared noise stream (oversampled rate) for the Correlation control
        {
            const int n = numSamples * params.osFactor;
            for (int i = 0; i < n; ++i) sharedNoise[(size_t) i] = sharedRng.next();
        }

        mixBuffer.clear (0, 0, numSamples);
        mixBuffer.clear (1, 0, numSamples);

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
                gs[(size_t) ModSource::AmpEnv] = newest->getEnvLevel();
                gs[(size_t) ModSource::Aftertouch] = newest->getPressureLevel();
            }
            ModMatrix::evaluate (params.mod, gs, globalMod);
        }

        float* L = mixBuffer.getWritePointer (0);
        float* R = mixBuffer.getWritePointer (1);

#if AERIFORM_FX
        // Aeriform FX: the DAW main input drives the resonator network here, so its
        // wet output joins the mix bus *before* the existing effects chain (chorus /
        // delay / reverb / output stage) and the numerical safety net below.
        {
            const float fxPressure = clamp01 (raw (P::excPressure));
            const float fxRoot = (float) juce::jlimit (0, 127, (int) std::lround (raw (P::fxRootNote)));
            fxPath.updateControl (params, globalMod, fxRoot, fxPressure);
            fxPath.process (L, R, fxExcBuffer.getReadPointer (0), numSamples, fxPressure);
        }
#endif

        chorus.setParams (clamp01 (params.get (P::chorusMix) + globalMod[(size_t) ModDest::ChorusMix]), params.get (P::chorusRate),
                          params.get (P::chorusDepth), params.get (P::chorusWidth));
        chorus.process (L, R, numSamples);

        float delayMs = params.get (P::delayTime);
        if (params.getb (P::delaySync))
            delayMs = (float) (60000.0 / (bpm > 1.0 ? bpm : 120.0) * choices::syncDivisionBeats (params.geti (P::delayDiv)));
        delay.setParams (clamp01 (params.get (P::delayMix) + globalMod[(size_t) ModDest::DelayMix]), delayMs, params.get (P::delayFeedback),
                         params.get (P::delayTone), params.getb (P::delayPingPong));
        delay.process (L, R, numSamples);

        reverb.setParams (clamp01 (params.get (P::reverbMix) + globalMod[(size_t) ModDest::ReverbMix]), params.get (P::reverbSize),
                          params.get (P::reverbDecay), params.get (P::reverbDamping), params.get (P::reverbPreDelay),
                          params.get (P::reverbWidth), params.get (P::reverbModulation));
        reverb.process (L, R, numSamples);

        output.setParams (params.get (P::outGain), params.get (P::outHighpass), params.getb (P::limiterOn));
        output.process (L, R, numSamples);

#if AERIFORM_FX
        // Aeriform FX: dry / wet mix (against the untouched main input) and the FX
        // output gain, applied before the safety net so peak / scope / NaN checks
        // see the true plug-in output.
        applyFxMixAndOutput (numSamples);
#endif

        // ---- numerical safety net -------------------------------------------------------
        bool finite = true;
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            const float a = std::fabs (L[i]), b = std::fabs (R[i]);
            if (! (a <= 8.0f && b <= 8.0f)) { finite = false; break; }
            peak = std::max (peak, std::max (a, b));
            vis.pushScopeSample (0.5f * (L[i] + R[i]));
        }
        if (! finite)
        {
            mixBuffer.clear();
            reset();
            peak = 0.0f;
        }

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
        const Voice* newest = nullptr;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            const auto& v = voices[(size_t) i];
            auto& s = vis.voices[(size_t) i];
            const bool on = v.isActive();
            s.active.store (on ? 1 : 0, std::memory_order_relaxed);
            s.energy.store (on ? v.getEnergy() : 0.0f, std::memory_order_relaxed);
            s.pressure.store (on ? v.getPressureLevel() : 0.0f, std::memory_order_relaxed);
            s.pitchHz.store (on ? v.getFreqHz() : 0.0f, std::memory_order_relaxed);
            if (on)
            {
                ++active; pressureSum += v.getPressureLevel(); energySum += v.getEnergy();
                if (newest == nullptr || v.getStartOrder() > newest->getStartOrder()) newest = &v;
            }
        }
        vis.activeVoices.store (active, std::memory_order_relaxed);
        vis.masterPeak.store (peak, std::memory_order_relaxed);
        vis.masterPressure.store (active > 0 ? pressureSum / (float) active : 0.0f, std::memory_order_relaxed);
        vis.masterEnergy.store (active > 0 ? energySum / (float) active : 0.0f, std::memory_order_relaxed);
        vis.limiterGain.store (output.getLimiterGain(), std::memory_order_relaxed);
        vis.sidechainEnv.store (sidechainEnv, std::memory_order_relaxed);
        if (noteOnsThisBlock > 0) vis.midiActivity.fetch_add (noteOnsThisBlock, std::memory_order_relaxed);

        for (int r = 0; r < 3; ++r)
        {
            vis.resonatorEnergy[(size_t) r].store (newest != nullptr ? newest->getResonatorEnergy (r) : 0.0f, std::memory_order_relaxed);
            vis.resonatorRunning[(size_t) r].store (newest != nullptr && newest->isResonatorRunning (r) ? 1 : 0, std::memory_order_relaxed);
        }
        vis.networkEnergy.store (newest != nullptr ? newest->getNetworkEnergy() : 0.0f, std::memory_order_relaxed);
        vis.governorGain.store (newest != nullptr ? newest->getGovernor() : 1.0f, std::memory_order_relaxed);
#if AERIFORM_FX
        // With no voice playing, show the always-on FX resonator path on the meters instead.
        if (newest == nullptr)
        {
            for (int r = 0; r < 3; ++r)
            {
                vis.resonatorEnergy[(size_t) r].store (fxPath.network.energy (r), std::memory_order_relaxed);
                vis.resonatorRunning[(size_t) r].store (fxPath.network.slotRunning (r) ? 1 : 0, std::memory_order_relaxed);
            }
            vis.networkEnergy.store (fxPath.network.netEnergy(), std::memory_order_relaxed);
            vis.governorGain.store (fxPath.network.governor(), std::memory_order_relaxed);
        }
#endif
        vis.exciterAEnv.store (newest != nullptr ? newest->getExciterEnvelope (0) : 0.0f, std::memory_order_relaxed);
        vis.exciterBEnv.store (newest != nullptr ? newest->getExciterEnvelope (1) : 0.0f, std::memory_order_relaxed);

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

void SynthEngine::process (juce::AudioBuffer<float>& output,
                           const juce::AudioBuffer<float>* mainInput,
                           const juce::AudioBuffer<float>* sidechainInput,
                           juce::MidiBuffer& midi,
                           const juce::AudioPlayHead::PositionInfo& position, MidiLearn* midiLearn, bool isNonRealtime)
{
    impl->process (output, mainInput, sidechainInput, midi, position, midiLearn, isNonRealtime);
}

int SynthEngine::getActiveVoiceCount() const noexcept { return impl->activeVoiceCount(); }

dsp::ModConfig SynthEngine::getModConfig() const
{
    dsp::ModConfig cfg;
    for (int i = 0; i < ids::numModSlots; ++i)
    {
        auto& s = cfg.slots[(size_t) i];
        s.source = (ModSource) juce::jlimit (0, (int) ModSource::Count - 1, (int) std::lround (impl->raw (ids::modP (i + 1, ids::ModField::Src))));
        s.dest = (ModDest) juce::jlimit (0, (int) ModDest::Count - 1, (int) std::lround (impl->raw (ids::modP (i + 1, ids::ModField::Dst))));
        s.depth = impl->raw (ids::modP (i + 1, ids::ModField::Depth));
    }
    return cfg;
}
} // namespace aeriform
