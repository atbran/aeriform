#pragma once

#include <cmath>
#include <algorithm>

namespace aeriform::dsp
{
/**
    Click-free ADSR built from one-pole segments (analogue-style curves).
    Attack overshoots towards a target above 1.0 so it reaches full level in the
    requested time; decay/release approach their targets exponentially.
*/
class ADSR
{
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release, Kill };

    void setSampleRate (float sr) noexcept { sampleRate = sr; }

    void setTimes (float attackMs, float decayMs, float sustainLevel, float releaseMs) noexcept
    {
        attackCoef  = coefForTime (attackMs, 0.7f);   // reach ~99% of the overshoot target
        decayCoef   = coefForTime (decayMs, 1.0f);
        releaseCoef = coefForTime (releaseMs, 1.0f);
        sustain     = std::clamp (sustainLevel, 0.0f, 1.0f);
    }

    void noteOn() noexcept
    {
        stage = Stage::Attack;
        // level keeps its current value: retriggering while active is click-free
    }

    void noteOff() noexcept
    {
        if (stage != Stage::Idle && stage != Stage::Kill)
            stage = Stage::Release;
    }

    /** Very fast fade-out used when a voice is stolen. */
    void kill (float ms = 3.0f) noexcept
    {
        killCoef = coefForTime (ms, 1.0f);
        stage = Stage::Kill;
    }

    void reset() noexcept { level = 0.0f; stage = Stage::Idle; }

    inline float next() noexcept
    {
        switch (stage)
        {
            case Stage::Attack:
                level += attackCoef * (kAttackTarget - level);
                if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
                break;
            case Stage::Decay:
                level += decayCoef * (sustain - level);
                if (std::fabs (level - sustain) < 1.0e-4f) { level = sustain; stage = Stage::Sustain; }
                break;
            case Stage::Sustain:
                level = sustain;
                break;
            case Stage::Release:
                level += releaseCoef * (0.0f - level);
                if (level < kSilence) { level = 0.0f; stage = Stage::Idle; }
                break;
            case Stage::Kill:
                level += killCoef * (0.0f - level);
                if (level < kSilence) { level = 0.0f; stage = Stage::Idle; }
                break;
            case Stage::Idle:
            default:
                level = 0.0f;
                break;
        }
        return level;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }
    bool isReleasing() const noexcept { return stage == Stage::Release || stage == Stage::Kill; }
    Stage getStage() const noexcept { return stage; }
    float getLevel() const noexcept { return level; }
    float getSustain() const noexcept { return sustain; }

private:
    static constexpr float kAttackTarget = 1.25f;
    static constexpr float kSilence = 1.0e-4f;

    float coefForTime (float ms, float scale) const noexcept
    {
        const float samples = std::max (1.0f, ms * 0.001f * sampleRate * scale);
        // time constant so that the segment covers ~99.9% (release) in the given time
        return 1.0f - std::exp (-4.6f / samples);
    }

    float sampleRate = 44100.0f;
    float attackCoef = 0.01f, decayCoef = 0.001f, releaseCoef = 0.001f, killCoef = 0.05f;
    float sustain = 1.0f, level = 0.0f;
    Stage stage = Stage::Idle;
};
} // namespace aeriform::dsp
