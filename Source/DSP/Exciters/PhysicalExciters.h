#pragma once

#include "../DspUtils.h"
#include "../FractionalDelay.h"
#include "../../Params/ParameterLayout.h"

namespace aeriform::dsp
{
/**
    Physically inspired excitation models (exciters, not complete instruments).
    Reed, lip, bow and jet contain a small internal "virtual bore" loop so they
    can speak on their own at the note pitch; mallet, pluck, scrape and impact
    are one-shot / granular strikes. Every path is bounded (tanh / clamps) and
    all state is allocated once in prepare().
*/
class PhysicalExciter
{
public:
    struct Params
    {
        float stiffness = 0.5f, opening = 0.5f, position = 0.3f, speed = 0.6f, turbulence = 0.2f, hardness = 0.5f, brightness = 0.5f;
    };

    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        const int maxDelay = (int) (sr / 30.0f) + 8;
        bore.prepare (maxDelay);
        jetDelay.prepare (maxDelay);
        comb.prepare (maxDelay);
        loopLP.setCutoff (4000.0f, sr);
        brightLP.setCutoff (3000.0f, sr);
        dc.setCutoff (5.0f, sr);
        noise.seed (0x51ED1u);
        rough.setRate (40.0f, sr);
        reset();
    }

    void seed (uint32_t s) noexcept { noise.seed (s); rough.seed (s ^ 0x77u); }

    void reset() noexcept
    {
        bore.clear(); jetDelay.clear(); comb.clear();
        loopLP.reset(); brightLP.reset(); dc.reset();
        lipX = lipV = 0.0f; stringV = 0.0f; hitRemaining = 0; hitPos = 0; rollCounter = 0; scrapeCounter = 0; bounce = 0;
        lastOut = 0.0f;
    }

    /** Note-on: (re)starts one-shot models and sets the loop lengths. */
    void trigger (float velocity) noexcept
    {
        vel = clamp01 (velocity);
        hitRemaining = hitLength; hitPos = 0; rollCounter = 0; bounce = 0; bounceGain = 1.0f;
        scrapeCounter = 0;
        lipX = 0.0f; lipV = 0.0f;
    }

    void update (ExciterModel m, const Params& p, float freqHz, float envelope) noexcept
    {
        model = m; params = p; env = envelope;
        const float f0 = std::clamp (freqHz, 20.0f, sr * 0.2f);
        period = sr / f0;
        loopLP.setCutoff (std::clamp (1500.0f + 9000.0f * p.brightness, 300.0f, sr * 0.45f), sr);
        brightLP.setCutoff (std::clamp (800.0f + 12000.0f * p.brightness * p.brightness, 200.0f, sr * 0.45f), sr);
        hitLength = std::max (4, (int) (sr * (0.0008f + 0.03f * (1.0f - p.hardness) * (1.0f - p.hardness))));
        rough.setRate (10.0f + 200.0f * p.hardness, sr);
        // lip: mass-spring tuned around the note (tension raises it)
        const float lipHz = f0 * (0.85f + 0.5f * p.stiffness);
        lipW = kTwoPi * lipHz / sr;
        lipDamp = 0.02f + 0.1f * (1.0f - p.opening);
    }

    inline float next() noexcept
    {
        float out = 0.0f;
        const float breath = env * params.speed;
        const float turb = params.turbulence * noise.next();

        switch (model)
        {
            case ExciterModel::Reed:
            {
                // closed-pipe bore, pressure-driven reed table (STK-style), bounded by clamps
                const float refl = -0.95f * loopLP.process (bore.readLagrange (std::max (2.0f, period * 0.5f - 1.0f)));
                const float pMouth = breath * 1.3f * (1.0f + 0.4f * turb);
                const float dp = pMouth - refl;
                const float slope = 0.15f + 0.5f * params.stiffness;
                const float offset = 0.4f + 0.55f * params.opening;
                const float r = std::clamp (offset - slope * dp, -1.0f, 1.0f);
                const float in = refl + dp * r;
                bore.push (in);
                out = in;
                break;
            }
            case ExciterModel::Lip:
            {
                // mass-spring lip driven by the pressure difference; flow ~ opening^2
                const float refl = 0.9f * loopLP.process (bore.readLagrange (std::max (2.0f, period - 1.0f)));
                const float pMouth = breath * 1.4f * (1.0f + 0.3f * turb);
                const float dp = pMouth - refl;
                const float acc = -lipW * lipW * lipX - 2.0f * lipDamp * lipW * lipV + lipW * lipW * 0.6f * dp;
                lipV += acc; lipX += lipV;
                lipX = std::clamp (lipX, -2.0f, 2.0f); lipV = std::clamp (lipV, -1.0f, 1.0f);
                const float open = std::clamp (lipX + 0.2f + 0.6f * params.opening, 0.0f, 1.5f);
                const float flow = open * open * dp * 0.8f;
                const float in = fastTanh (refl + flow);
                bore.push (in);
                out = in;
                break;
            }
            case ExciterModel::Bow:
            {
                // string velocity loop with bow friction; roughness = velocity noise
                const float sv = 0.97f * loopLP.process (bore.readLagrange (std::max (2.0f, period - 1.0f)));
                const float bowV = params.speed * env * (1.0f + 0.5f * params.turbulence * rough.next());
                const float vRel = bowV - sv;
                const float pressure = 0.2f + 1.2f * params.stiffness;
                const float friction = fastTanh (vRel * 25.0f) / (1.0f + 6.0f * std::fabs (vRel));
                const float force = pressure * friction * 0.5f;
                const float in = fastTanh (sv + force);
                // bow position comb
                comb.push (in);
                const float pos = 0.05f + 0.45f * params.position;
                const float x = in - 0.8f * comb.readLinear (std::max (1.0f, period * pos));
                bore.push (in);
                out = x;
                break;
            }
            case ExciterModel::Jet:
            {
                // air jet edge tone: cubic jet non-linearity with a jet delay against an open bore
                const float boreOut = 0.93f * loopLP.process (bore.readLagrange (std::max (2.0f, period - 1.0f)));
                const float pMouth = breath * 0.8f * (1.0f + 0.5f * turb);
                jetDelay.push (pMouth - 0.5f * boreOut);
                const float jetLen = std::max (2.0f, period * (0.25f + 0.5f * (1.0f - params.speed)));
                float x = jetDelay.readLinear (jetLen);
                x = std::clamp (x, -1.0f, 1.0f);
                const float jet = x - x * x * x;
                const float in = fastTanh (jet + 0.5f * boreOut);
                bore.push (in);
                out = in;
                break;
            }
            case ExciterModel::Mallet:
            {
                // raised-cosine strike; Speed > 0 rolls at 1..20 Hz
                if (hitRemaining <= 0 && params.speed > 0.05f)
                {
                    const int rollPeriod = (int) (sr / (0.5f + 20.0f * params.speed));
                    if (++rollCounter >= rollPeriod) { rollCounter = 0; hitRemaining = hitLength; hitPos = 0; }
                }
                if (hitRemaining > 0)
                {
                    const float ph = (float) hitPos / (float) hitLength;
                    const float pulse = 0.5f - 0.5f * std::cos (kTwoPi * ph);
                    const float contact = params.turbulence * noise.next() * pulse;
                    out = (pulse * 2.0f + contact) * (0.3f + 0.7f * vel);
                    ++hitPos; --hitRemaining;
                }
                comb.push (out);
                out -= 0.7f * params.position * comb.readLinear (std::max (1.0f, period * (0.05f + 0.45f * params.position)));
                break;
            }
            case ExciterModel::Pluck:
            {
                // displacement ramp (plucked shape) released over the burst, filtered by hardness
                if (hitRemaining > 0)
                {
                    const float ph = (float) hitPos / (float) hitLength;
                    const float shape = ph < 0.5f ? ph * 2.0f : 2.0f - ph * 2.0f;
                    out = (shape * 2.0f - 0.5f) * (0.3f + 0.7f * vel) * 1.5f + params.turbulence * 0.3f * noise.next() * (1.0f - ph);
                    ++hitPos; --hitRemaining;
                }
                comb.push (out);
                out -= 0.85f * comb.readLinear (std::max (1.0f, period * (0.05f + 0.45f * params.position)));
                break;
            }
            case ExciterModel::Scrape:
            {
                // continuous stream of short random bursts; speed = rate, hardness = burst sharpness
                if (scrapeCounter <= 0)
                {
                    const float rate = 20.0f + 900.0f * params.speed * (0.5f + 0.5f * env);
                    scrapeCounter = std::max (2, (int) (sr / rate * (1.0f + 0.5f * rough.next())));
                    scrapeAmp = (0.4f + 0.6f * noise.next01()) * env;
                    scrapeLen = std::max (2, (int) (sr * (0.0003f + 0.004f * (1.0f - params.hardness))));
                    scrapePos = 0;
                }
                --scrapeCounter;
                if (scrapePos < scrapeLen)
                {
                    const float ph = (float) scrapePos / (float) scrapeLen;
                    out = noise.next() * scrapeAmp * (1.0f - ph) * 2.5f;
                    ++scrapePos;
                }
                out += params.turbulence * 0.2f * noise.next() * env;
                break;
            }
            case ExciterModel::Impact:
            {
                // decaying noise burst, Speed adds decaying bounces
                if (hitRemaining > 0)
                {
                    const float ph = (float) hitPos / (float) hitLength;
                    out = noise.next() * std::exp (-5.0f * ph) * (0.3f + 0.7f * vel) * 3.0f * bounceGain;
                    ++hitPos; --hitRemaining;
                    if (hitRemaining == 0 && params.speed > 0.05f && bounce < 6)
                    {
                        ++bounce;
                        bounceGain *= 0.4f + 0.5f * params.stiffness;
                        rollCounter = (int) (sr * (0.02f + 0.2f * (1.0f - params.speed)));
                    }
                }
                else if (rollCounter > 0)
                {
                    if (--rollCounter == 0) { hitRemaining = hitLength; hitPos = 0; }
                }
                break;
            }
            default: break;
        }

        out = brightLP.process (out) + (1.0f - params.brightness) * 0.0f;
        out = dc.process (out);
        lastOut = std::clamp (out, -4.0f, 4.0f);
        return lastOut;
    }

    bool isSelfSustaining() const noexcept
    {
        return model == ExciterModel::Reed || model == ExciterModel::Lip || model == ExciterModel::Bow || model == ExciterModel::Jet;
    }

private:
    float sr = 88200.0f, period = 100.0f, env = 0.0f, vel = 1.0f;
    ExciterModel model = ExciterModel::Reed;
    Params params;
    FractionalDelay bore, jetDelay, comb;
    OnePole loopLP, brightLP;
    DcBlocker dc;
    Noise noise;
    SlowRandom rough;
    float lipX = 0.0f, lipV = 0.0f, lipW = 0.01f, lipDamp = 0.05f, stringV = 0.0f;
    int hitRemaining = 0, hitPos = 0, hitLength = 100, rollCounter = 0, bounce = 0; float bounceGain = 1.0f;
    int scrapeCounter = 0, scrapeLen = 10, scrapePos = 0; float scrapeAmp = 0.0f;
    float lastOut = 0.0f;
};
} // namespace aeriform::dsp
