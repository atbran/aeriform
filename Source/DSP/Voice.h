#pragma once
#include "RoomCoupling.h"

#include "DspUtils.h"
#include "Envelope.h"
#include "LFO.h"
#include "VoiceParams.h"
#include "Exciters/ExciterSlot.h"
#include "Interaction.h"
#include "PreShaper.h"
#include "Wavefolder.h"
#include "Oversampler.h"
#include "StereoResonatorNetwork.h"
#include "ModMatrix.h"
#include "../Visualization/VisualizerModel.h"

namespace aeriform::dsp
{
/** Output fader: unity while the note plays, exponential release / kill afterwards. */
class Fader
{
public:
    void setSampleRate (float sr) noexcept { sampleRate = sr; }
    void start() noexcept { level = 1.0f; coef = 0.0f; running = true; }
    void release (float ms) noexcept { coef = 1.0f - std::exp (-4.6f / std::max (1.0f, ms * 0.001f * sampleRate)); }
    void kill (float ms) noexcept { coef = 1.0f - std::exp (-4.6f / std::max (1.0f, ms * 0.001f * sampleRate)); }
    inline float next() noexcept
    {
        if (coef > 0.0f)
        {
            level -= coef * level;
            if (level < 1.0e-4f) { level = 0.0f; running = false; }
        }
        return level;
    }
    bool isRunning() const noexcept { return running; }
    bool isReleasing() const noexcept { return coef > 0.0f; }
    float get() const noexcept { return level; }
private:
    float sampleRate = 44100.0f, level = 0.0f, coef = 0.0f;
    bool running = false;
};

/**
    One polyphonic voice:
    Exciter A/B -> Interaction -> Pre-shaper -> Wavefolder (oversampled) -> Dynamics
    -> Resonator network (A/B/C, routing, cross-feedback, energy loop) -> Body -> Fader -> Pan,
    with its own envelopes, LFOs and modulation evaluation. All processing is allocation-free.
*/
class Voice
{
public:
    static constexpr int kMaxControlInterval = 64;

    void prepare (double sampleRate, int voiceIndex);
    void reset();

    void startNote (int midiNote, float velocity, float glideFromNote, bool legato,
                    int unisonIndex, int unisonCount, int noteId, const VoiceParams& p);
    /** Mono / legato: move to a new note without retriggering. */
    void changeNote (int midiNote, float velocity, bool retriggerEnvelope, const VoiceParams& p);
    void stopNote (const VoiceParams& p);
    void kill (float ms = 4.0f);

    void setBend (float semitones) noexcept { bendSemitones = semitones; }
    void setPressure (float p) noexcept { notePressure = clamp01 (p); }
    void setSlide (float s) noexcept { noteSlide = clamp01 (s); }
    void setAlternate (float a) noexcept { alternateNote = a; }
    /** The engine points the newest voice at the visualiser so its exciter / folder scopes are fed. */
    void setScopeTarget (VisualizerModel* target) noexcept { scope = target; }

    /** externalIn: base-rate sidechain samples (or nullptr), sharedNoise: engine noise at the oversampled rate
        (numSamples * osFactor samples), couplingIn: sympathetic coupling input. */
    void render (float* left, float* right, int numSamples, const VoiceParams& p,
                 const ModSources& globalSources, const float* externalIn, const float* sharedNoise, float couplingIn,const float* roomReturn=nullptr);

    bool isActive() const noexcept { return active; }
    bool isReleasing() const noexcept { return fader.isReleasing(); }
    int  getNote() const noexcept { return currentNote; }
    int  getNoteId() const noexcept { return noteId; }
    int  getUnisonIndex() const noexcept { return unisonIndex; }
    void adoptNoteId (int id) noexcept { noteId = id; }
    unsigned getStartOrder() const noexcept { return startOrder; }
    void setStartOrder (unsigned o) noexcept { startOrder = o; }

    float getEnergy() const noexcept { return network.energy (0) + network.energy (1) + network.energy (2); }
    float getResonatorTargetHz(int i) const noexcept {return netParams.res[std::clamp(i,0,2)].freqHz;}
    float getResonatorEnergy (int i) const noexcept { return network.energy (i); }
    float getStereoLeftEnergy() const noexcept {return network.leftEnergy();}
    float getStereoRightEnergy() const noexcept {return network.rightEnergy();}
    float getContactActivity() const noexcept {return network.contactActivity();}
    float getNetworkEnergy() const noexcept { return network.netEnergy(); }
    float getGovernor() const noexcept { return network.governor(); }
    bool  isResonatorRunning (int i) const noexcept { return network.slotRunning (i); }
    float getExciterEnvelope (int slot) const noexcept { return slot == 0 ? exA.getEnvelope() : exB.getEnvelope(); }
    float getPressureLevel() const noexcept { return lastPressure; }
    float getFreqHz() const noexcept { return lastFreq; }
    float getLastMono() const noexcept { return lastMono; }
    float getEnvLevel() const noexcept { return ampEnv.getLevel(); }
    float getLastExciterA() const noexcept { return exA.getLastOutput(); }
    float getLastExciterB() const noexcept { return exB.getLastOutput(); }
    float getLastFolded() const noexcept { return lastFolded; }
    const ModValues& getLastMod() const noexcept { return modValues; }

private:
    double sampleRate = 44100.0;
    int voiceIndex = 0;
    int osFactor = 2;

    // ---- exciter chain (oversampled) ----
    ExciterSlot exA, exB;
    Interaction interaction;
    PreShaper preShaper;
    Wavefolder folder;
    Oversampler decimator, extUp, loopUp, sideDecimator;
    OnePole dynEnv;
    // ---- resonator network + body ----
    StereoResonatorNetwork network;
    ModularFilters filters;
    SVF bodyL, bodyR;
    // ---- modulation ----
    ADSR ampEnv, modEnv;
    Fader fader;
    LFO lfos[ids::numLFOs];
    SlowRandom instabilityRnd, smoothRnd;
    Noise rng;
    ModSources sources {};
    ModValues modValues {};

    bool active = false;
    int currentNote = 60, noteId = -1, unisonIndex = 0, unisonCount = 1;
    unsigned startOrder = 0;
    float velocity = 1.0f;
    float bendSemitones = 0.0f, notePressure = 0.0f, noteSlide = 0.0f;
    float noteRandom = 0.0f, alternateNote = 1.0f, sampleHoldValue = 0.0f, lastLfo1Phase = 0.0f;
    float henonX = 0.1f, henonY = 0.1f;
    long noteAgeSamples = 0;

    float glideNote = 60.0f, glideTarget = 60.0f, glideStepPerSample = 0.0f;
    float varTune = 0.0f, varDamp = 0.0f, varBright = 0.0f, varShape = 0.0f;

    // control-rate state
    float lastFreq = 440.0f, lastPressure = 0.0f, lastMono = 0.0f, lastFolded = 0.0f;
    float pressureScale = 0.0f, breathScale = 1.0f, envAmount = 1.0f, ampGain = 0.5f;
    float gainRampL = 0.0f, gainRampR = 0.0f, gainStepL = 0.0f, gainStepR = 0.0f;
    float dynAmount = 0.0f, loopRet = 0.0f;
    RoomCoupling roomCoupling;
    bool  snapNextLength = true, syncBtoA = false;
    InteractionMode interactionMode = InteractionMode::Crossfade;
    LoopDest loopDest = LoopDest::FolderIn;
    ShaperOrder shaperOrder = ShaperOrder::ShapeThenFold;
    float b2a = 0.0f, interactionAmount = 0.5f;
    float bodyK = 1.0f, bodyGain = 0.0f, bodyMix = 0.0f;

    ExciterSlot::Params exParamsA, exParamsB;
    NetworkParams netParams;
    VisualizerModel* scope = nullptr;

    void updateControl (int numSamples, const VoiceParams& p, const ModSources& globalSources);
    void buildExciterParams (ExciterSlot::Params& out, const VoiceParams& p, bool slotA, float pitchSemis) const;
    void buildNetworkParams (const VoiceParams& p, float baseNote);
    float lfoRate (const LfoParams& lp, double bpm, float rateMod) const noexcept;
    void configureOversampling (int factor);
};
} // namespace aeriform::dsp
