#include "ResonatorNetwork.h"

namespace aeriform::dsp
{
namespace
{
    inline float smoothstep (float a, float b, float x) noexcept
    {
        const float t = clamp01 ((x - a) / (b - a));
        return t * t * (3.0f - 2.0f * t);
    }
}

void ResonatorNetwork::prepare (float sr)
{
    sampleRate = sr;
    gSmooth = 1.0f - std::exp (-1.0f / (0.004f * sr));
    for (auto& s : slots) s.prepare (sr);
    const int maxRoute = (int) (0.05f * sr) + 16;
    for (auto& d : routeDelay) d.prepare (maxRoute);
    for (auto& f : routeLP) f.setCutoff (6000.0f, sr);
    loopDelay.prepare ((int) (0.1f * sr) + 16);
    loopLP.setCutoff (3000.0f, sr);
    for (int i = 0; i < kNumGains; ++i) g[i] = gTarget[i] = 0.0f;
    reset();
}

void ResonatorNetwork::reset()
{
    for (auto& s : slots) s.reset();
    for (auto& d : routeDelay) d.clear();
    for (auto& f : routeLP) f.reset();
    loopDelay.clear();
    loopLP.reset();
    fb[0] = fb[1] = fb[2] = 0.0f;
    loopReturnValue = 0.0f;
    netEnergyValue = 0.0f;
    governorGain = 1.0f;
    for (int i = 0; i < kNumGains; ++i) g[i] = gTarget[i];
    fbDelaySamples = fbDelayTarget;
    loopDelaySamples = loopDelayTarget;
}

void ResonatorNetwork::update (const NetworkParams& p, bool snapLength)
{
    const float r = clamp01 (p.repipe);
    const bool repipe = r > 0.001f;
    hybrid = p.mode == NetMode::Hybrid;

    // ---- which slots run -------------------------------------------------------
    bool wantRunning[3] = { p.on[0], p.on[1] || repipe, p.on[2] || repipe };
    if (p.mode == NetMode::Single && ! repipe) { wantRunning[1] = false; wantRunning[2] = false; }
    for (int i = 0; i < 3; ++i)
    {
        if (wantRunning[i] && ! running[i]) { slots[(size_t) i].reset(); running[i] = true; g[G_gateA + i] = 0.0f; }
        gTarget[G_gateA + i] = wantRunning[i] ? 1.0f : 0.0f;
        // keep processing until the gate has faded out, then stop
        if (! wantRunning[i] && running[i] && g[G_gateA + i] < 1.0e-3f) { running[i] = false; fb[i] = 0.0f; }
    }

    // ---- injection / serial sends by mode -----------------------------------------
    float injA = 0.0f, injB = 0.0f, injC = 0.0f, sendAB = 0.0f, sendBC = 0.0f;
    switch (p.mode)
    {
        case NetMode::Parallel:
            injA = p.in[0]; injB = p.in[1]; injC = p.in[2];
            break;
        case NetMode::Serial:
            sendAB = p.sendAB; sendBC = p.sendBC;
            injB = p.injectB; injC = p.injectC;
            injA = p.in[0];
            break;
        case NetMode::Hybrid:
            sendAB = p.sendAB; sendBC = p.sendBC;   // both from A (hybrid flag)
            injB = p.injectB; injC = p.injectC;
            injA = p.in[0];
            break;
        case NetMode::Single:
        default:
            injA = p.in[0];
            break;
    }
    if (p.mode != NetMode::Parallel)
    {
        switch (p.inject)
        {
            case InjectPoint::B:   injA = 0.0f; injB = p.in[1]; break;
            case InjectPoint::C:   injA = 0.0f; injC = p.in[2]; break;
            case InjectPoint::All: injA = p.in[0]; injB = std::max (injB, p.in[1]); injC = std::max (injC, p.in[2]); break;
            case InjectPoint::A:
            default: break;
        }
    }

    // ---- Repipe macro: single tube -> serial, cross-fed network ---------------------------
    float ab = p.ab, ba = p.ba, bc = p.bc, cb = p.cb, ca = p.ca, ac = p.ac;
    float fbScale = p.feedback, fbDrive = p.fbDrive;
    float outW[3] = { p.out[0], p.out[1], p.out[2] };
    if (repipe)
    {
        sendAB = std::max (sendAB, smoothstep (0.0f, 0.45f, r));
        sendBC = std::max (sendBC, smoothstep (0.25f, 0.75f, r));
        ba = std::max (ba, 0.5f * smoothstep (0.4f, 1.0f, r));
        ca = std::max (ca, 0.35f * smoothstep (0.55f, 1.0f, r));
        cb = std::max (cb, 0.25f * smoothstep (0.7f, 1.0f, r));
        ac = std::max (ac, 0.2f * smoothstep (0.8f, 1.0f, r));
        fbScale = std::max (fbScale, 0.5f + 0.5f * smoothstep (0.4f, 1.0f, r));
        fbDrive = std::max (fbDrive, 0.4f * r);
        outW[0] = p.out[0] * lerp (1.0f, 0.6f, smoothstep (0.0f, 0.5f, r));
        outW[1] = std::max (outW[1] * smoothstep (0.1f, 0.6f, r), p.on[1] ? p.out[1] : 0.0f);
        outW[2] = std::max (outW[2] * smoothstep (0.4f, 0.9f, r), p.on[2] ? p.out[2] : 0.0f);
    }

    // ---- output tap ------------------------------------------------------------------------
    float tapW[3] = { outW[0], outW[1], outW[2] };
    switch (p.tap)
    {
        case OutputTap::A: tapW[1] = tapW[2] = 0.0f; break;
        case OutputTap::B: tapW[0] = tapW[2] = 0.0f; break;
        case OutputTap::C: tapW[0] = tapW[1] = 0.0f; break;
        case OutputTap::Last:
        {
            const int last = wantRunning[2] ? 2 : (wantRunning[1] ? 1 : 0);
            for (int i = 0; i < 3; ++i) if (i != last) tapW[i] = 0.0f;
            break;
        }
        case OutputTap::Mix:
        default: break;
    }

    // ---- gain targets ------------------------------------------------------------------------
    gTarget[G_injA] = injA; gTarget[G_injB] = injB; gTarget[G_injC] = injC;
    gTarget[G_sendAB] = sendAB; gTarget[G_sendBC] = sendBC;
    gTarget[G_ab] = ab; gTarget[G_ba] = ba; gTarget[G_bc] = bc; gTarget[G_cb] = cb; gTarget[G_ca] = ca; gTarget[G_ac] = ac;
    gTarget[G_fbScale] = clamp01 (fbScale) * 0.98f;
    for (int i = 0; i < 3; ++i)
    {
        gTarget[G_outA + i] = tapW[i];
        gTarget[G_widthA + i] = clamp01 (p.width3[i]);
        // pans: network width spreads B / C in parallel and hybrid modes
        float pan = p.pan[i];
        if ((p.mode == NetMode::Parallel || p.mode == NetMode::Hybrid || repipe) && i > 0)
            pan = std::clamp (pan + (i == 1 ? -1.0f : 1.0f) * 0.6f * p.width, -1.0f, 1.0f);
        const float angle = (pan + 1.0f) * 0.25f * kPi;
        gTarget[G_panLA + i] = std::cos (angle);
        gTarget[G_panRA + i] = std::sin (angle);
    }
    gTarget[G_mix] = clamp01 (p.mix);
    gTarget[G_loop] = p.loopOn ? clamp01 (p.loopAmount) : 0.0f;

    // ---- route processing ------------------------------------------------------------------------
    fbDelayTarget = std::clamp (p.fbDelayMs * 0.001f * sampleRate, 0.0f, 0.05f * sampleRate);
    for (auto& f : routeLP) f.setCutoff (std::clamp (p.fbFilterHz, 100.0f, sampleRate * 0.45f), sampleRate);
    fbDriveGain = 1.0f + 5.0f * clamp01 (fbDrive);
    fbDriveNorm = 1.0f / std::sqrt (fbDriveGain);
    fbDampGain = 1.0f - 0.85f * clamp01 (p.damping);
    fbPolarity = p.polarity == Polarity::Negative ? -1.0f : 1.0f;

    // ---- energy loop --------------------------------------------------------------------------------
    loopSrc = p.loopSource;
    loopDelayTarget = std::clamp (p.loopDelayMs * 0.001f * sampleRate, 1.0f, 0.1f * sampleRate);
    loopLP.setCutoff (std::clamp (p.loopFilterHz, 50.0f, sampleRate * 0.45f), sampleRate);
    loopDriveGain = (1.0f + 4.0f * clamp01 (p.loopSat)) * (p.loopPolarity == Polarity::Negative ? -1.0f : 1.0f);
    loopDriveNorm = 1.0f / (1.0f + clamp01 (p.loopSat));

    if (snapLength)
    {
        for (int i = 0; i < kNumGains; ++i) g[i] = gTarget[i];
        fbDelaySamples = fbDelayTarget;
        loopDelaySamples = loopDelayTarget;
    }

    // ---- resonators ------------------------------------------------------------------------------------
    for (int i = 0; i < 3; ++i)
        if (running[i]) slots[(size_t) i].update (p.res[i], snapLength);

    // safety: a slot that produced a non-finite value is flushed
    for (int i = 0; i < 3; ++i)
        if (running[i] && ! slots[(size_t) i].isFinite()) { slots[(size_t) i].reset(); fb[i] = 0.0f; }
    if (! std::isfinite (fb[0]) || ! std::isfinite (fb[1]) || ! std::isfinite (fb[2]) || ! std::isfinite (loopReturnValue))
    {
        fb[0] = fb[1] = fb[2] = 0.0f;
        loopReturnValue = 0.0f;
        for (auto& d : routeDelay) d.clear();
        loopDelay.clear();
    }
}
} // namespace aeriform::dsp
