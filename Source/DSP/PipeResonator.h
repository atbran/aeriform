#pragma once


#include "FractionalDelay.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>

namespace aeriform::dsp
{
/** PIPE model settings. Physical units; percentages are already divided by 100. */
struct PipeParams
{
    float pressure = 0.5f, dcNoise = 0.5f;          // 0..1
    float excCutHz = 2000.0f, excQ = 0.7f;
    float excKeyTrack = 0.5f, excVelTrack = 0.5f;   // 0..1.5, 0..1
    float rtSec = 0.5f, rtKeyTrack = 0.5f, damp = 0.0f;
    float lpHz = 4000.0f, hpHz = 40.0f, filterKeyTrack = 1.0f;
    float drive = 2.0f, hardness = 0.4f, asymmetry = 0.0f; // 1..32, 0..1, -1..1
    bool cylinder = false, adaa = true;
    float velocity = 0.8f;
    /** How pressure drives the pipe. Linear adds pressure and noise into the loop (the original
        PIPE); Jet and Reed are self-oscillating mouthpieces that turn steady pressure into tone. */
    enum class Blow { Linear, Jet, Reed };
    Blow blow = Blow::Linear;
    float jet = 0.5f;                                // Jet: jet length; Reed: reed opening (0..1)
};

/**
    PIPE resonator (model index 9): production port of prototypes/pipe/PipePrototype.h.

    Continuous blown excitation pressure * env * ((1 - m) + m * noise) through a two-pole
    exciter lowpass, into a fractional delay loop with an asymmetric cubic-knee saturator
    (first-order ADAA), normalised one-pole highpass then lowpass, and a loop gain derived
    from the nominal decay time (always below one). Cone = non-inverting full-period loop,
    cylinder = inverting half-period loop. The delay length is tuned to the peak of the
    small-signal output response (see the P1 tuning findings).

    The DC part of the excitation is deliberately not blocked before the saturator: it
    biases the knee into asymmetric operation. The loop highpass follows the saturator.

    Jet and Reed replace the additive excitation with a mouthpiece at the loop input:
    - Jet (flue pipe): pressure sets the jet speed U = sqrt(P). The wave returning to the mouth
      deflects the jet, which crosses the mouth in tau (shorter at higher pressure, so hard blowing
      overblows) and splits on the edge through a saturating profile (Drive = jet gain, Hardness =
      profile, Asymmetry = jet offset from the edge). The resulting flow, proportional to U, feeds the
      bore. Turbulence deflects the jet and is radiated at the mouth, so breath noise is modulated by
      the tone. A two-pole jet lowpass at 1.5 f0 keeps the fundamental regime stable.
    - Reed: a pressure-controlled valve (reflection table offset - slope * pressure difference).
    In both, the loop gain stays below one and the injected flow is bounded, so the loop energy is
    bounded. The output is the radiated wave (bore end plus mouth) through a gentle one-zero
    radiation highpass, normalised by the round-trip loss so decay time does not set loudness.
    Tuning adds the steady-state phase of the jet path to the filter compensation.
*/
class PipeResonator
{
public:
    struct Telemetry { float loopGainDb = 0.0f, phaseCompSamples = 0.0f, effectiveDecaySec = 0.0f; bool peakFound = false; };

    void prepare (float sampleRate, uint32_t seed)
    {
        fs = sampleRate;
        delay.prepare ((int) std::ceil (fs / kMinFreq * 1.5) + 64);
        jetLine.prepare ((int) std::ceil (fs / kMinFreq * 1.6) + 64);
        lenSmooth = 1.0f - std::exp (-1.0f / (0.0025f * sampleRate));
        seedValue = seed != 0 ? seed : 1u;
        tuningCached = false;
        reset();
    }

    void reset() noexcept
    {
        delay.clear();
        sat = {};
        hpX = hpY = lpY = excZ1 = excZ2 = feedback = 0.0;
        rng = seedValue;
        energy = 0.0f; lastOut = 0.0f; loopMeanSquare = 0.0;
        delayLen = targetLen;
        jetLine.clear();
        jet1 = jet2 = radPrev = slowTurb = 0.0;
        jetLen = jetBaseLen;
    }

    /** Control-rate update. freqHz is the slot's fundamental (tuning, bend and glide included). */
    void update (const PipeParams& p, float freqHz, bool snapLength) noexcept
    {
        params = p;
        const double f0 = std::clamp ((double) freqHz, (double) kMinFreq, fs * 0.1);
        const double note = 69.0 + 12.0 * std::log2 (f0 / 440.0);
        const double track = std::exp2 (p.filterKeyTrack * (note - 60.0) / 12.0);
        const double lpCut = std::clamp (p.lpHz * track, 20.0, fs * 0.45);
        const double hpCut = std::clamp (p.hpHz * track, 20.0, fs * 0.45);
        aLP = pole (lpCut); aHP = pole (hpCut); bHP = 0.5 * (1.0 + aHP);
        if (p.blow != PipeParams::Blow::Linear)
        {
            // A mouthpiece locks the harmonics together, so a highpass leading the fundamental much more
            // than the overtones makes the tone multiphonic on low notes: keep it below f0 / 8.
            aHP = std::exp (-2.0 * kPiD * std::clamp (std::min (hpCut, f0 / 8.0), 2.0, fs * 0.45) / fs);
            bHP = 0.5 * (1.0 + aHP);
        }

        const double rt = std::clamp ((double) p.rtSec * std::exp2 (-p.rtKeyTrack * (note - 60.0) / 12.0) * (1.0 - std::clamp (p.damp, 0.0f, 1.0f)), 0.001, 30.0);
        roundTrip = fs / (f0 * (p.cylinder ? 2.0 : 1.0));
        gain = std::pow (10.0, -3.0 * roundTrip / (fs * rt));

        const double w = 2.0 * kPiD * f0 / fs;
        const double filters = -(std::arg (lpResponse (aLP, w)) + std::arg (hpResponse (aHP, w))) / w;
        const bool mouthpiece = p.blow != PipeParams::Blow::Linear;
        // loss per round trip at the fundamental: sets the output normalisation and the jet's steady state
        const double loopMag = gain * std::abs (lpResponse (aLP, w) * hpResponse (aHP, w));
        outNorm = std::clamp (1.0 - loopMag, 0.004, 1.0);
        double jetDelay = 0.0;
        if (p.blow == PipeParams::Blow::Jet)
        {
            const double period = fs / f0;
            const double target = std::max (0.04, (double) p.pressure);   // the embouchure follows the target pressure
            jetRatio = std::clamp ((0.42 + 0.5 * ((double) p.jet - 0.5)) * std::pow (0.5 / target, 0.3), 0.08, 1.2);
            jetBaseLen = std::clamp (jetRatio * period, 1.0, (double) jetLine.getMaxDelay() / 1.3 - 2.0) + 1.0;
            jetA = 1.0 - std::exp (-2.0 * kPiD * std::min (1.5 * f0, fs * 0.4) / fs);
            // steady state: the jet path gain settles where |R| * loopMag = 1, with R = 1 + A e^{j theta}
            const Complex lp1 = jetA / (1.0 - (1.0 - jetA) * std::polar (1.0, -w));
            const double theta = kPiD - w * (jetBaseLen - 1.0) + 2.0 * std::arg (lp1);
            const double M = 1.0 / std::max (loopMag, 1.0e-6), c = std::cos (theta);
            const double A = -c + std::sqrt (std::max (0.0, c * c + M * M - 1.0));
            jetDelay = -std::atan2 (A * std::sin (theta), 1.0 + A * c) / w + harmonicPull (0.04, w, filters, p.cylinder);
            jetOffset = 0.3 + 0.6 * std::clamp ((double) p.asymmetry, -1.0, 1.0);
            jetRest = jetCurve (jetOffset, std::clamp ((double) p.hardness, 0.0, 1.0));
        }
        else if (p.blow == PipeParams::Blow::Reed)
        {
            reedOffset = 0.45 + 0.45 * std::clamp ((double) p.jet, 0.0, 1.0);
            reedSlope = 0.1 + 0.1 * std::clamp ((double) p.drive, 1.0, 32.0);
            reedClose = (1.0 - reedOffset) / reedSlope;
            jetDelay = harmonicPull (0.1, w, filters, p.cylinder);
        }
        const double base = roundTrip - filters - (mouthpiece ? jetDelay : (p.adaa ? 0.5 : 0.0));
        double length = base;
        for (int i = 0; i < 12; ++i)
        {
            const double fraction = length - std::floor (length);
            length = base - (-std::arg (lagrangeFraction (fraction, w)) / w - fraction);
        }
        const double maxDelay = (double) delay.getMaxDelay();
        if (mouthpiece)
        {
            // the small-signal peak search models the linear loop only
            tuned = { std::clamp (length, 4.0, maxDelay), false };
            tuningCached = false;
        }
        else if (! tuningCached || w != cachedW || aLP != cachedLP || aHP != cachedHP || gain != cachedGain
            || p.cylinder != cachedCylinder || p.adaa != cachedAdaa)
        {
            tuned = tuneMagnitudePeak (length, w, maxDelay);
            cachedW = w; cachedLP = aLP; cachedHP = aHP; cachedGain = gain;
            cachedCylinder = p.cylinder; cachedAdaa = p.adaa; tuningCached = true;
        }
        targetLen = (float) std::clamp (tuned.delay, 4.0, maxDelay);
        if (snapLength) delayLen = targetLen;

        // two-pole RBJ lowpass on the excitation, outside the loop
        const double fc = std::clamp (p.excCutHz * std::exp2 ((p.excKeyTrack * (note - 60.0) + p.excVelTrack * 24.0 * p.velocity) / 12.0), 20.0, fs * 0.45);
        const double omega = 2.0 * kPiD * fc / fs, co = std::cos (omega);
        const double alpha = std::sin (omega) / (2.0 * std::clamp ((double) p.excQ, 0.5, 8.0));
        const double den = 1.0 + alpha;
        b0 = (1.0 - co) * 0.5 / den; b1 = (1.0 - co) / den; b2 = b0;
        a1 = -2.0 * co / den; a2 = (1.0 - alpha) / den;

        // telemetry: small-signal loss per round trip at the fundamental
        const double fraction = (double) targetLen - std::floor ((double) targetLen);
        std::complex<double> loop = gain * lagrangeFraction (fraction, w) * lpResponse (aLP, w) * hpResponse (aHP, w);
        if (p.adaa) loop *= 0.5 * (1.0 + std::polar (1.0, -w));
        const double mag = std::abs (loop);
        telemetry.loopGainDb = (float) (20.0 * std::log10 (std::max (gain, 1.0e-12)));
        telemetry.phaseCompSamples = (float) (roundTrip - targetLen);
        telemetry.effectiveDecaySec = mag > 0.0 && mag < 1.0 ? (float) (-3.0 * roundTrip / (fs * std::log10 (mag))) : 0.0f;
        telemetry.peakFound = tuned.found;
    }

    /** One sample. in = excitation arriving from the network (exciters, sends, cross-feedback),
        env = shared voice amp envelope level. Returns the loop output. */
    inline float next (float in, float env) noexcept
    {
        delayLen += (targetLen - delayLen) * lenSmooth;

        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        const double noise = 2.0 * ((double) rng / 4294967295.0) - 1.0;
        if (params.blow != PipeParams::Blow::Linear)
            return nextMouthpiece (in, env, noise);
        const double m = std::clamp ((double) params.dcNoise, 0.0, 1.0);
        const double blow = params.pressure * env * ((1.0 - m) + m * noise);
        const double exc = b0 * blow + excZ1;
        excZ1 = b1 * blow - a1 * exc + excZ2;
        excZ2 = b2 * blow - a2 * exc;

        const double x = exc + in + (params.cylinder ? -feedback : feedback);
        delay.push ((float) x);
        const double delayed = delay.readLagrange (delayLen);
        const double shaped = sat.process (delayed, params.drive, params.hardness, params.asymmetry, params.adaa);
        const double high = bHP * (shaped - hpX) + aHP * hpY; hpX = shaped; hpY = high;
        const double low = (1.0 - aLP) * high + aLP * lpY; lpY = low;
        feedback = gain * low;
        if (! std::isfinite (low))
        {
            delay.clear(); sat = {};
            feedback = hpX = hpY = lpY = excZ1 = excZ2 = 0.0;
            lastOut = 0.0f;
            return 0.0f;
        }
        loopMeanSquare = low * low;
        lastOut = (float) low;
        energy += 0.002f * (std::fabs (lastOut) - energy);
        return lastOut;
    }

    float getEnergy() const noexcept { return energy; }
    float getJetRatio() const noexcept { return (float) jetRatio; }
    double getLoopMeanSquare() const noexcept { return loopMeanSquare; }
    bool isFinite() const noexcept { return std::isfinite (lastOut) && std::isfinite (delayLen); }
    const Telemetry& getTelemetry() const noexcept { return telemetry; }
    float getLoopGain() const noexcept { return (float) gain; }
    float getDelayLength() const noexcept { return delayLen; }

    /** Nominal loop gain for a given setting, for network coupling normalisation. */
    static float nominalLoopGain (const PipeParams& p, float freqHz) noexcept
    {
        const double f0 = std::max ((double) freqHz, (double) kMinFreq);
        const double note = 69.0 + 12.0 * std::log2 (f0 / 440.0);
        const double rt = std::clamp ((double) p.rtSec * std::exp2 (-p.rtKeyTrack * (note - 60.0) / 12.0) * (1.0 - std::clamp (p.damp, 0.0f, 1.0f)), 0.001, 30.0);
        // g = 10^(-3 T / RT) with T in seconds
        return (float) std::pow (10.0, -3.0 / (f0 * (p.cylinder ? 2.0 : 1.0) * rt));
    }

    // ---- shared maths (also used by tests) -----------------------------------------------
    static double knee (double u, double h, double s) noexcept { return std::clamp (0.9 * h * (1.0 + (u >= 0.0 ? s : -s)), 0.0, 0.95); }
    static double curve (double u, double h, double s) noexcept
    {
        const double k = knee (u, h, s), a = std::abs (u);
        if (a <= k) return u;
        const double t = (a - k) / (1.0 - k);
        const double f = t <= 1.0 ? t - t * t * t / 3.0 : 2.0 / 3.0;
        return std::copysign (k + (1.0 - k) * f, u);
    }
    static double primitive (double u, double h, double s) noexcept
    {
        const double k = knee (u, h, s), a = std::abs (u);
        if (a <= k) return 0.5 * u * u;
        const double t = (a - k) / (1.0 - k);
        const double f = t <= 1.0 ? 0.5 * t * t - t * t * t * t / 12.0 : (2.0 / 3.0) * t - 0.25;
        return 0.5 * k * k + (1.0 - k) * (k * t + (1.0 - k) * f);
    }

private:
    using Complex = std::complex<double>;
    static constexpr double kPiD = 3.14159265358979323846;
    static constexpr float kMinFreq = 16.0f;
    static constexpr double kRadiation = 0.65;      // one-zero radiation highpass, unity gain at DC
    static constexpr double kMouthpieceLevel = 1.0, kReedLevel = 0.45;

    /** A mouthpiece locks its harmonics to the fundamental, and the loop filters leave the upper
        resonances flat, so the tone is pulled flat. The correction tunes against a phase delay weighted
        towards the first overtone (second harmonic for the cone, third for the cylinder). */
    double harmonicPull (double weight, double w, double filters, bool cylinder) const noexcept
    {
        const double wh = w * (cylinder ? 3.0 : 2.0);
        if (wh >= kPiD * 0.9) return 0.0;
        const double upper = -(std::arg (lpResponse (aLP, wh)) + std::arg (hpResponse (aHP, wh))) / wh;
        return weight * (upper - filters);
    }

    /** Jet velocity profile at the edge: algebraic (soft) -> tanh (hardness 0.4) -> hard clip (1). */
    static double jetCurve (double u, double h) noexcept
    {
        const double t = std::tanh (u);
        if (h < 0.4) { const double a = u / std::sqrt (1.0 + u * u); return a + (t - a) * (h / 0.4); }
        return t + (std::clamp (u, -1.0, 1.0) - t) * ((h - 0.4) / 0.6);
    }

    inline float nextMouthpiece (float in, float env, double noise) noexcept
    {
        // turbulence, coloured by the exciter lowpass; never fully silent so the jet can start
        const double turb = b0 * noise + excZ1;
        excZ1 = b1 * noise - a1 * turb + excZ2;
        excZ2 = b2 * noise - a2 * turb;
        slowTurb += 0.0005 * (noise - slowTurb);
        const double m = std::min (1.0, 0.03 + std::clamp ((double) params.dcNoise, 0.0, 1.0));
        const double e = std::clamp ((double) env, 0.0, 1.5);
        const double P = std::max (0.0, (double) params.pressure * e);
        const double rootP = std::sqrt (P);
        const double pm = params.cylinder ? -feedback : feedback;   // wave arriving at the mouth

        double x, mouth = 0.0;
        if (params.blow == PipeParams::Blow::Jet)
        {
            // the jet is slower while the pressure is still building: a softer, airier onset
            const double target = jetBaseLen + (jetBaseLen - 1.0) * 0.25 * (1.0 - std::min (e, 1.0));
            jetLen += (target - jetLen) * 0.002;
            jetLine.push ((float) (-pm + m * 0.6 * turb * rootP));
            const double jr = jetLine.readLagrange ((float) jetLen);
            jet1 += jetA * (jr - jet1);
            jet2 += jetA * (jet1 - jet2);
            const double h = std::clamp ((double) params.hardness, 0.0, 1.0);
            const double U = rootP * (1.0 - 0.6 * m);
            const double flow = U * (jetCurve (std::clamp ((double) params.drive, 1.0, 32.0) * jet2 + jetOffset + 0.03 * slowTurb, h) - jetRest);
            const double breath = m * 0.2 * rootP * turb;
            x = pm + 0.6 * flow + breath + in;
            mouth = 0.6 * flow + 2.0 * breath;
        }
        else
        {
            // Pressure spans 45 % .. 95 % of the pressure that shuts the reed, whatever its stiffness and
            // opening: from just below the speaking threshold to just short of choking.
            const double breath = std::min (e, 1.0) * reedClose * (0.45 + 0.5 * (double) params.pressure) * (1.0 + m * 0.5 * turb);
            const double pd = pm - breath;
            const double r = std::clamp (reedOffset - reedSlope * pd, -1.0, 1.0);
            x = breath + pd * r + in;
        }

        delay.push ((float) x);
        const double d = delay.readLagrange (delayLen);
        const double high = bHP * (d - hpX) + aHP * hpY; hpX = d; hpY = high;
        const double low = (1.0 - aLP) * high + aLP * lpY; lpY = low;
        feedback = gain * low;
        if (! std::isfinite (low) || ! std::isfinite (jet2))
        {
            delay.clear(); jetLine.clear(); sat = {};
            feedback = hpX = hpY = lpY = excZ1 = excZ2 = jet1 = jet2 = radPrev = 0.0;
            lastOut = 0.0f;
            return 0.0f;
        }
        loopMeanSquare = low * low;
        // a jet's amplitude grows with the loop's resonance (1 / loss); a reed valve's is set by the breath
        const double radiated = params.blow == PipeParams::Blow::Jet ? (low + 1.2 * mouth) * outNorm : low * kReedLevel;
        const double y = (radiated - kRadiation * radPrev) / (1.0 - kRadiation);
        radPrev = radiated;
        lastOut = (float) (y * kMouthpieceLevel);
        energy += 0.002f * (std::fabs (lastOut) - energy);
        return lastOut;
    }

    struct Saturator
    {
        double process (double x, double d, double h, double s, bool adaa) noexcept
        {
            d = std::clamp (d, 1.0, 32.0); h = std::clamp (h, 0.0, 1.0); s = std::clamp (s, -1.0, 1.0);
            const double u = d * x, prev = d * previousX;
            const double nowF = primitive (u, h, s);
            const double oldF = (d == drive && h == hard && s == asym) ? previousF : primitive (prev, h, s);
            const double delta = u - prev;
            const double out = ! adaa ? curve (u, h, s)
                             : (std::abs (delta) < 1.0e-5 ? curve (0.5 * (u + prev), h, s) : (nowF - oldF) / delta);
            previousX = x; previousF = nowF; drive = d; hard = h; asym = s;
            return out / d;
        }
        double previousX = 0.0, previousF = 0.0, drive = 1.0, hard = 0.4, asym = 0.0;
    };

    struct PeakTuning { double delay = 0.0; bool found = false; };

    double pole (double fc) const noexcept { return std::exp (-2.0 * kPiD * std::clamp (fc, 20.0, fs * 0.45) / fs); }
    static Complex lpResponse (double a, double w) noexcept { return (1.0 - a) / (1.0 - a * std::polar (1.0, -w)); }
    static Complex hpResponse (double a, double w) noexcept { const auto z = std::polar (1.0, -w); return 0.5 * (1.0 + a) * (1.0 - z) / (1.0 - a * z); }
    static Complex lagrangeFraction (double f, double w) noexcept
    {
        const double c0 = -f * (f - 1) * (f - 2) / 6, c1 = (f + 1) * (f - 1) * (f - 2) / 2;
        const double c2 = -(f + 1) * f * (f - 2) / 2, c3 = (f + 1) * f * (f - 1) / 6;
        return c0 * std::polar (1.0, w) + c1 + c2 * std::polar (1.0, -w) + c3 * std::polar (1.0, -2 * w);
    }

    // Slope of log|T| with respect to frequency, T = F / (1 - sign g q F), F = delay(D-1) * ADAA * HP * LP.
    double logMagnitudeSlope (double length, double w) const noexcept
    {
        const Complex j (0, 1), q = std::polar (1.0, -w);
        const double integer = std::floor (length), f = length - integer;
        const double c0 = -f * (f - 1) * (f - 2) / 6, c1 = (f + 1) * (f - 1) * (f - 2) / 2;
        const double c2 = -(f + 1) * f * (f - 2) / 2, c3 = (f + 1) * f * (f - 1) / 6;
        const Complex interpolation = c0 / q + c1 + c2 * q + c3 * q * q;
        const Complex interpolationDerivative = j * (c0 / q - c2 * q - 2 * c3 * q * q);
        const Complex lp = (1 - aLP) / (1.0 - aLP * q), hp = 0.5 * (1 + aHP) * (1.0 - q) / (1.0 - aHP * q);
        const bool adaa = params.adaa;
        const Complex aa = adaa ? 0.5 * (1.0 + q) : Complex (1, 0);
        const Complex forward = std::polar (1.0, -w * (integer - 1)) * interpolation * aa * hp * lp;
        const Complex loop = (params.cylinder ? -gain : gain) * q * forward;
        Complex logForwardDerivative = -j * (integer - 1) + interpolationDerivative / interpolation
            - j * aLP * q / (1.0 - aLP * q) + j * q / (1.0 - q) - j * aHP * q / (1.0 - aHP * q);
        if (adaa) logForwardDerivative -= j * q / (1.0 + q);
        return std::real ((logForwardDerivative - j * loop) / (1.0 - loop));
    }

    // Refine the phase-aligned length to the local peak of the small-signal output response.
    // A heavily damped setting may have no peak near the fundamental: keep the phase solution.
    PeakTuning tuneMagnitudePeak (double seed, double w, double maximum) const noexcept
    {
        const double low = std::max (4.0, seed * 0.8), high = std::min (maximum, seed * 1.2);
        PeakTuning result { std::clamp (seed, 4.0, maximum), false };
        if (low >= high) return result;
        double left = 0, right = 0, bestDistance = maximum;
        double previous = low, previousSlope = logMagnitudeSlope (low, w);
        constexpr int intervals = 32;
        for (int i = 1; i <= intervals; ++i)
        {
            const double current = low + (high - low) * double (i) / intervals, currentSlope = logMagnitudeSlope (current, w);
            if (std::isfinite (previousSlope) && std::isfinite (currentSlope) && previousSlope >= 0 && currentSlope <= 0)
            {
                const double distance = std::abs (0.5 * (previous + current) - seed);
                if (distance < bestDistance) { left = previous; right = current; bestDistance = distance; }
            }
            previous = current; previousSlope = currentSlope;
        }
        if (bestDistance == maximum) return result;
        for (int i = 0; i < 36; ++i)
        {
            const double middle = 0.5 * (left + right);
            if (logMagnitudeSlope (middle, w) > 0) left = middle; else right = middle;
        }
        const double candidate = 0.5 * (left + right), step = w * 1e-4;
        const double below = logMagnitudeSlope (candidate, w - step), above = logMagnitudeSlope (candidate, w + step);
        if (std::isfinite (below) && std::isfinite (above) && below > 0 && above < 0)
            result = { candidate, true };
        return result;
    }

    FractionalDelay delay, jetLine;
    Saturator sat;
    PipeParams params;
    Telemetry telemetry;
    PeakTuning tuned;
    uint32_t rng = 1, seedValue = 1;
    double fs = 48000.0, gain = 0.0, roundTrip = 100.0, feedback = 0.0;
    double aLP = 0.0, aHP = 0.0, bHP = 0.0, hpX = 0.0, hpY = 0.0, lpY = 0.0;
    double b0 = 0.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0, excZ1 = 0.0, excZ2 = 0.0;
    double cachedW = 0.0, cachedLP = 0.0, cachedHP = 0.0, cachedGain = 0.0;
    bool tuningCached = false, cachedCylinder = false, cachedAdaa = false;
    float delayLen = 100.0f, targetLen = 100.0f, lenSmooth = 0.01f;
    float energy = 0.0f, lastOut = 0.0f;
    double loopMeanSquare = 0.0;
    // mouthpiece state
    double jetLen = 50.0, jetBaseLen = 50.0, jetRatio = 0.45, jetA = 0.1, jet1 = 0.0, jet2 = 0.0;
    double jetOffset = 0.3, jetRest = 0.0, reedOffset = 0.7, reedSlope = 0.3, reedClose = 1.0;
    double outNorm = 1.0, radPrev = 0.0, slowTurb = 0.0;
};
} // namespace aeriform::dsp
