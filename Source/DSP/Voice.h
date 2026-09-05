#pragma once

#include "DspUtils.h"
#include "Envelope.h"
#include "LFO.h"
#include "Exciter.h"
#include "Resonator.h"
#include "ModMatrix.h"

namespace aeriform::dsp
{
struct LfoParams
{
    LfoShape shape = LfoShape::Sine;
    float rateHz = 1.0f;
    bool sync = false;
    int division = 7;
    LfoMode mode = LfoMode::Free;
    float fadeMs = 0.0f;
    float phaseDeg = 0.0f;
};

/** Block-rate snapshot of every parameter a voice needs (plain floats, copied
    from the atomics once per block by the engine). */
struct VoiceParams
{
    ExciterParams exciter;
    ResonatorParams resonator;
    float pressure = 0.5f;
    float reed = 0.0f;
    float envAttackMs = 25.0f, envDecayMs = 300.0f, envSustain = 0.8f, envReleaseMs = 250.0f;
    float velToPressure = 0.6f;
    float flowPitch = 0.15f, instability = 0.1f, variation = 0.2f, coupling = 0.0f;
    float menvAttackMs = 100.0f, menvDecayMs = 600.0f, menvSustain = 0.2f, menvReleaseMs = 400.0f;
    LfoParams lfo[ids::numLFOs];
    ModConfig mod;
    float coarse = 0.0f, fine = 0.0f, length = 1.0f, keyTrack = 1.0f;
    float unisonDetuneCents = 12.0f, unisonSpread = 0.6f;
    float glideMs = 0.0f;
    double tempoBpm = 120.0;
    float lfoGlobalPhase[ids::numLFOs] { 0.0f, 0.0f, 0.0f };
};

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
    One polyphonic voice: exciter -> tube -> body -> VCA -> pan, with its own
    envelopes, LFOs and modulation evaluation. All processing is allocation-free.
*/
class Voice
{
public:
    static constexpr int kControlInterval = 32;

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

    void render (float* left, float* right, int numSamples, const VoiceParams& p,
                 const ModSources& globalSources, const float* externalIn, float couplingIn);

    bool isActive() const noexcept { return active; }
    bool isReleasing() const noexcept { return fader.isReleasing(); }
    int  getNote() const noexcept { return currentNote; }
    int  getNoteId() const noexcept { return noteId; }
    int  getUnisonIndex() const noexcept { return unisonIndex; }
    /** Mono/legato: the voice now sounds a different held note (id follows it). */
    void adoptNoteId (int id) noexcept { noteId = id; }
    unsigned getStartOrder() const noexcept { return startOrder; }
    void setStartOrder (unsigned o) noexcept { startOrder = o; }

    float getEnergy() const noexcept { return resonator.getEnergy(); }
    float getPressureLevel() const noexcept { return lastPressure; }
    float getFreqHz() const noexcept { return lastFreq; }
    float getLastMono() const noexcept { return lastMono; }
    float getEnvLevel() const noexcept { return ampEnv.getLevel(); }
    const ModValues& getLastMod() const noexcept { return modValues; }

private:
    double sampleRate = 44100.0;
    int voiceIndex = 0;
    Exciter exciter;
    Resonator resonator;
    ADSR ampEnv, modEnv;
    Fader fader;
    LFO lfos[ids::numLFOs];
    SlowRandom instabilityRnd;
    Noise rng;
    ModSources sources {};
    ModValues modValues {};

    bool active = false;
    int currentNote = 60, noteId = -1, unisonIndex = 0, unisonCount = 1;
    unsigned startOrder = 0;
    float velocity = 1.0f;
    float bendSemitones = 0.0f, notePressure = 0.0f, noteSlide = 0.0f;
    float noteRandom = 0.0f;

    // glide (in semitones, control-rate linear ramp)
    float glideNote = 60.0f, glideTarget = 60.0f, glideStepPerSample = 0.0f;

    // per-voice component variation (fixed random offsets in [-1, 1])
    float varTune = 0.0f, varDamp = 0.0f, varBright = 0.0f, varShape = 0.0f;

    // control-rate state
    float lastFreq = 440.0f, lastPressure = 0.0f, lastMono = 0.0f;
    float pressureScale = 0.0f, breathScale = 1.0f, ampGain = 0.5f, panL = 0.707f, panR = 0.707f;
    float gainRampL = 0.0f, gainRampR = 0.0f, gainStepL = 0.0f, gainStepR = 0.0f;
    bool  snapNextLength = true;
    ExciterParams exciterParams;
    ResonatorParams resonatorParams;

    void updateControl (int numSamples, const VoiceParams& p, const ModSources& globalSources);
    float lfoRate (const LfoParams& lp, double bpm, float rateMod) const noexcept;
};
} // namespace aeriform::dsp
