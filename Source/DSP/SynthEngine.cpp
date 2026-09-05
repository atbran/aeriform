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
    std::vector<float> sharedNoise;          // engine-wide noise stream at the oversampled rate
    std::vector<float> captureRing;          // last 250 ms of sidechain input for Freeze
    int captureWrite = 0, freezeStart = 0, freezeLen = 0, freezePos = 0;
    bool frozen = false;
    Noise sharedRng;

    VoiceParams params;
    const SynthEngine::EffectiveValues* effective=nullptr;
    bool skipOutputStage=false;
    std::array<std::atomic<float>*, (size_t) kNumParams> atomics {};
    ModSources globalSources {};
    ModValues globalMod {};
    LFO globalLfo[ids::numLFOs];
    Chorus chorus;
    StereoDelay delay;
    FdnReverb reverb;
    OutputStage output;
    ModularFilters globalFilters;
    std::vector<ModularFilters::FrameWeights> filterFrames;
    float filterNote=60,filterEnvelope=0;
    OnePole sidechainFollower;
    float sidechainEnv = 0.0f;

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
        if (effective != nullptr) return (*effective)[(size_t)p];
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
        output.prepare (sr);globalFilters.prepare((float)sr);filterFrames.resize((size_t)maxBlock);
        sidechainFollower.setCutoff (20.0f, (float) sr);
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
        output.reset();globalFilters.reset();
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
            params.v[(size_t) i] = raw((P)i);
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
        if(m.isNoteOn())filterNote=(float)m.getNoteNumber();
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

    void process (juce::AudioBuffer<float>& out, const juce::AudioBuffer<float>* extIn, juce::MidiBuffer& midi,
                  const juce::AudioPlayHead::PositionInfo& pos, MidiLearn* learn, bool, int midiOffset = 0)
    {
        const int numSamples = out.getNumSamples();
        if (! prepared || numSamples <= 0) { out.clear(); return; }
        if (numSamples > maxBlock)
        {
            for (int start = 0; start < numSamples; start += maxBlock)
            {
                const int n = juce::jmin (maxBlock, numSamples - start);
                juce::AudioBuffer<float> sub (out.getArrayOfWritePointers(), out.getNumChannels(), start, n);
                float* inputChannels[2] {};
                const int inputCount = extIn != nullptr ? std::min(2,extIn->getNumChannels()) : 0;
                for (int c=0;c<inputCount;++c) inputChannels[c]=const_cast<float*>(extIn->getReadPointer(c))+start;
                juce::AudioBuffer<float> inputView(inputChannels,inputCount,n);
                process (sub, inputCount > 0 ? &inputView : nullptr, midi, pos, learn, false, midiOffset+start);
            }
            return;
        }

        const double bpm = pos.getBpm().hasValue() ? *pos.getBpm() : 120.0;
        readParams (bpm > 1.0 ? bpm : 120.0);
        noteOnsThisBlock = 0;

        prepareSidechain (extIn, numSamples);

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
            if (meta.samplePosition < midiOffset || meta.samplePosition >= midiOffset+numSamples || meta.numBytes > 3) continue;
            const int t = meta.samplePosition-midiOffset;
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
            filterEnvelope=newest?newest->getEnvLevel():0;
            vis.stereoLeftEnergy.store(newest?newest->getStereoLeftEnergy():0,std::memory_order_relaxed);vis.stereoRightEnergy.store(newest?newest->getStereoRightEnergy():0,std::memory_order_relaxed);
            vis.collisionActivity.store(newest?newest->getContactActivity():0,std::memory_order_relaxed);
            if (newest != nullptr)
            {
                gs[(size_t) ModSource::AmpEnv] = newest->getEnvLevel();
                gs[(size_t) ModSource::Aftertouch] = newest->getPressureLevel();
            }
            ModMatrix::evaluate (params.mod, gs, globalMod);
        }

        float* L = mixBuffer.getWritePointer (0);
        float* R = mixBuffer.getWritePointer (1);

        globalFilters.update(params,filterNote,filterEnvelope,numSamples);
        for(int i=0;i<numSamples;++i){globalFilters.advance();filterFrames[(size_t)i]=globalFilters.weights();L[i]=globalFilters.atWeighted(FilterPosition::PreEffects,L[i],0,filterFrames[(size_t)i]);R[i]=globalFilters.atWeighted(FilterPosition::PreEffects,R[i],3,filterFrames[(size_t)i]);}
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

        for(int i=0;i<numSamples;++i){L[i]=globalFilters.atWeighted(FilterPosition::PostEffects,L[i],0,filterFrames[(size_t)i]);R[i]=globalFilters.atWeighted(FilterPosition::PostEffects,R[i],3,filterFrames[(size_t)i]);}
        if (!skipOutputStage) {
        output.setParams (params.get (P::outGain), params.get (P::outHighpass), params.getb (P::limiterOn));
        output.process (L, R, numSamples);
        const auto& meter=output.getMeter();
        vis.preLimiterPeak.store(meter.prePeak); vis.preLimiterRms.store(meter.preRms); vis.preLimiterMean.store(meter.preMean);
        vis.postLimiterRms.store(meter.postRms); vis.postLimiterMean.store(meter.postMean);
        vis.limiterFraction.store(meter.limitedFraction); vis.ceilingFraction.store(meter.ceilingFraction);
        }

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

void SynthEngine::process (juce::AudioBuffer<float>& output, const juce::AudioBuffer<float>* externalInput, juce::MidiBuffer& midi,
                           const juce::AudioPlayHead::PositionInfo& position, MidiLearn* midiLearn, bool isNonRealtime)
{
    impl->process (output, externalInput, midi, position, midiLearn, isNonRealtime);
}

void SynthEngine::setEffectiveValues(const EffectiveValues* v,bool skip) noexcept {impl->effective=v;impl->skipOutputStage=skip;}
void SynthEngine::processRange(juce::AudioBuffer<float>& out,const juce::AudioBuffer<float>* in,juce::MidiBuffer& midi,
                              const juce::AudioPlayHead::PositionInfo& pos,MidiLearn* learn,bool offline,int offset) {
    impl->process(out,in,midi,pos,learn,offline,offset);
}
int SynthEngine::getActiveVoiceCount() const noexcept { return impl->activeVoiceCount(); }

dsp::ModConfig SynthEngine::getModConfig() const
{
    dsp::ModConfig cfg;
    for (int i = 0; i < ids::numModSlots; ++i)
    {
        auto& s = cfg.slots[(size_t) i];
        s.source = (ModSource) juce::jlimit (0, (int) ModSource::Count - 1, (int) std::lround (impl->atomics[(size_t)ids::modP(i+1,ids::ModField::Src)]->load()));
        s.dest = (ModDest) juce::jlimit (0, (int) ModDest::Count - 1, (int) std::lround (impl->atomics[(size_t)ids::modP(i+1,ids::ModField::Dst)]->load()));
        s.depth = impl->atomics[(size_t)ids::modP(i+1,ids::ModField::Depth)]->load();
    }
    return cfg;
}
} // namespace aeriform
