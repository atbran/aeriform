#pragma once

#include "DspUtils.h"
#include "FractionalDelay.h"
#include "Resonator.h"
#include "CollisionRoute.h"
#include "../Params/ParameterLayout.h"
#include <array>

namespace aeriform::dsp
{
struct NetworkParams
{
    bool bypass=false;
    ContactParams contact;
    NetMode mode = NetMode::Single;
    float feedback = 0.5f;
    float ab = 0.0f, ba = 0.0f, bc = 0.0f, cb = 0.0f, ca = 0.0f, ac = 0.0f;
    float sendAB = 1.0f, sendBC = 1.0f, injectB = 0.0f, injectC = 0.0f;
    Polarity polarity = Polarity::Positive;
    float fbDelayMs = 0.0f, fbFilterHz = 6000.0f, fbDrive = 0.3f, damping = 0.2f, width = 0.5f;
    InjectPoint inject = InjectPoint::A;
    OutputTap tap = OutputTap::Mix;
    float mix = 1.0f, repipe = 0.0f;

    bool on[3] = { true, false, false };
    float in[3] = { 1.0f, 1.0f, 1.0f }, out[3] = { 1.0f, 1.0f, 1.0f }, pan[3] = { 0.0f, -0.3f, 0.3f }, width3[3] = { 0.0f, 0.0f, 0.0f };
    ResonatorParams res[3];

    bool loopOn = false;
    float loopAmount = 0.3f;
    LoopSource loopSource = LoopSource::Mix;
    float loopFilterHz = 3000.0f, loopDelayMs = 5.0f, loopSat = 0.5f;
    Polarity loopPolarity = Polarity::Positive;
};

/**
    Three resonator slots with Single / Serial / Parallel / Hybrid routing, six
    cross-feedback routes (shared delay, filter, drive, polarity, damping), an
    injection point, output tap, wet/dry mix, the Repipe macro and the energy
    loop return path. Every feedback path is tanh-bounded and an energy governor
    reduces feedback when the network runs hot. All gains are smoothed per
    sample so topology changes never click.
*/
class ResonatorNetwork
{
public:
    /** The Repipe macro's route curves (shared with the GUI diagram so it shows what is really active).
        r = 0..1; outputs are the minimum values the macro imposes on the corresponding routes. */
    struct RepipeRoutes { float sendAB = 0.0f, sendBC = 0.0f, ba = 0.0f, ca = 0.0f, cb = 0.0f, ac = 0.0f, fbScale = 0.0f, fbDrive = 0.0f; };
    static RepipeRoutes repipeRoutes (float r) noexcept
    {
        auto smoothstep = [] (float a, float b, float x) { const float t = clamp01 ((x - a) / (b - a)); return t * t * (3.0f - 2.0f * t); };
        RepipeRoutes o;
        o.sendAB = smoothstep (0.0f, 0.45f, r);
        o.sendBC = smoothstep (0.25f, 0.75f, r);
        o.ba = 0.5f * smoothstep (0.4f, 1.0f, r);
        o.ca = 0.35f * smoothstep (0.55f, 1.0f, r);
        o.cb = 0.25f * smoothstep (0.7f, 1.0f, r);
        o.ac = 0.2f * smoothstep (0.8f, 1.0f, r);
        o.fbScale = 0.5f + 0.5f * smoothstep (0.4f, 1.0f, r);
        o.fbDrive = 0.4f * r;
        return o;
    }

    void setFilters(ModularFilters* f,int lane=0) noexcept {filters=f;filterLane=lane;for(int i=0;i<3;++i)slots[(size_t)i].setLoopFilter(f,(FilterPosition)((int)FilterPosition::ResALoop+i),lane);}
    void prepare (float sampleRate);
    void reset();
    void update (const NetworkParams& p, bool snapLength);

    /** One sample. excitation = mono excitation, loopNetIn = energy-loop return injected at the network input
        (0 unless the loop destination is Network In), pressureNow = breath pressure for the reed junctions. */
    inline void next (float excitation, float loopNetIn, float pressureNow, float& outL, float& outR) noexcept
    {
        // ---- smooth all gains ---------------------------------------------------
        for (int i = 0; i < kNumGains; ++i) g[i] += (gTarget[i] - g[i]) * gSmooth;
        fbDelaySamples += (fbDelayTarget - fbDelaySamples) * gSmooth;
        loopDelaySamples += (loopDelayTarget - loopDelaySamples) * gSmooth;

        const float ex = filters?filters->at(FilterPosition::NetworkInput,excitation+loopNetIn,filterLane):excitation+loopNetIn;
        float o[3] = { 0.0f, 0.0f, 0.0f }, t2[3] = { 0.0f, 0.0f, 0.0f };

        // ---- resonators in A -> B -> C order (feed-forward sends use current samples) ----
        if (running[0])
        {
            float inA = ex * g[G_injA] + fb[0] + contactInjection[0]*contactLoss[0];
            if(filters)inA=filters->at(FilterPosition::ResAInput,inA,filterLane);
            o[0] = slots[0].next (inA, pressureNow, t2[0]) * g[G_gateA];
            if(filters){o[0]=filters->at(FilterPosition::ResAOutput,o[0],filterLane);t2[0]=filters->at(FilterPosition::ResAOutput,t2[0],filterLane+1);}
        }
        if (running[1])
        {
            float inB = ex * g[G_injB] + o[0] * g[G_sendAB] + fb[1] + contactInjection[1]*contactLoss[1];
            if(filters)inB=filters->at(FilterPosition::ResBInput,inB,filterLane);
            o[1] = slots[1].next (inB, pressureNow, t2[1]) * g[G_gateB];
            if(filters){o[1]=filters->at(FilterPosition::ResBOutput,o[1],filterLane);t2[1]=filters->at(FilterPosition::ResBOutput,t2[1],filterLane+1);}
        }
        if (running[2])
        {
            const float serialIn = hybrid ? o[0] : o[1];
            float inC = ex * g[G_injC] + serialIn * g[G_sendBC] + fb[2] + contactInjection[2]*contactLoss[2];
            if(filters)inC=filters->at(FilterPosition::ResCInput,inC,filterLane);
            o[2] = slots[2].next (inC, pressureNow, t2[2]) * g[G_gateC];
            if(filters){o[2]=filters->at(FilterPosition::ResCOutput,o[2],filterLane);t2[2]=filters->at(FilterPosition::ResCOutput,t2[2],filterLane+1);}
        }

        contact.next(o,contactInjection);

        // ---- cross-feedback for the next sample --------------------------------------
        {
            const float scale = g[G_fbScale] * governorGain;
            const float rawA = (o[1] * g[G_ba] + o[2] * g[G_ca]) * scale;
            const float rawB = (o[0] * g[G_ab] + o[2] * g[G_cb]) * scale;
            const float rawC = (o[1] * g[G_bc] + o[0] * g[G_ac]) * scale;
            fb[0] = routeProcess (0, rawA);
            fb[1] = routeProcess (1, rawB);
            fb[2] = routeProcess (2, rawC);
            netEnergyValue += 0.001f * (std::fabs (fb[0]) + std::fabs (fb[1]) + std::fabs (fb[2]) - netEnergyValue);
        }

        // ---- stereo sum --------------------------------------------------------------------
        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < 3; ++i)
        {
            if (! running[i]) continue;
            const float lvl = g[G_outA + i];
            if (lvl <= 1.0e-5f) continue;
            const float side = (t2[i] - o[i]) * g[G_widthA + i];
            l += (o[i] + side) * lvl * g[G_panLA + i];
            r += (o[i] - side) * lvl * g[G_panRA + i];
        }
        const float wet = g[G_mix];
        const float dryGain = (1.0f - wet) * 0.707f;
        outL = l * wet + ex * dryGain;
        outR = r * wet + ex * dryGain;

        // ---- energy loop return ----------------------------------------------------------------
        float src;
        switch (loopSrc)
        {
            case LoopSource::A: src = o[0]; break;
            case LoopSource::B: src = o[1]; break;
            case LoopSource::C: src = o[2]; break;
            case LoopSource::Mix:
            default: src = 0.5f * (l + r); break;
        }
        loopDelay.push (src);
        float lr = loopLP.process (loopDelay.readLinear (std::max (1.0f, loopDelaySamples)));
        lr = fastTanh (lr * loopDriveGain) * loopDriveNorm;
        if(filters)lr=filters->at(FilterPosition::EnergyLoop,lr,filterLane);
        loopReturnValue = lr * g[G_loop] * governorGain;

        // ---- governor: a loop limiter on the feedback paths -----------------------------------
        // Self-oscillation (cross routes / energy loop with loop gain > 1) settles at the reference level
        // instead of at the saturators' ceiling. A peak follower with instant attack sees a feedback burst
        // within a millisecond; the gain recovers slowly so the network breathes rather than pumps. Driven
        // playing below the reference is untouched and the direct signal path is never attenuated.
        const float inst = std::fabs (o[0]) + std::fabs (o[1]) + std::fabs (o[2]);
        peakEnv = inst > peakEnv ? inst : peakEnv * 0.9998f;
        const float target = peakEnv > kGovernorRef ? std::max (0.01f, kGovernorRef / peakEnv) : 1.0f;
        governorGain += (target - governorGain) * (target < governorGain ? 0.02f : 0.0001f);
    }

    float contactActivity() const noexcept {return contact.getActivity();}
    float loopReturn() const noexcept { return loopReturnValue; }
    float energy (int i) const noexcept { return slots[(size_t) juce::jlimit (0, 2, i)].getEnergy(); }
    float netEnergy() const noexcept { return netEnergyValue; }
    float governor() const noexcept { return governorGain; }
    bool  slotRunning (int i) const noexcept { return running[juce::jlimit (0, 2, i)]; }
    bool  isFinite() const noexcept
    {
        return std::isfinite (fb[0]) && std::isfinite (fb[1]) && std::isfinite (fb[2]) && std::isfinite (loopReturnValue)
               && slots[0].isFinite() && slots[1].isFinite() && slots[2].isFinite();
    }

private:
    static constexpr float kGovernorRef = 0.6f;

    enum Gain
    {
        G_injA, G_injB, G_injC, G_sendAB, G_sendBC, G_ab, G_ba, G_bc, G_cb, G_ca, G_ac, G_fbScale,
        G_gateA, G_gateB, G_gateC, G_outA, G_outB, G_outC, G_widthA, G_widthB, G_widthC,
        G_panLA, G_panLB, G_panLC, G_panRA, G_panRB, G_panRC, G_mix, G_loop, kNumGains
    };

    inline float routeProcess (int i, float raw) noexcept
    {
        auto& d = routeDelay[(size_t) i];
        d.push (raw);
        float x = fbDelaySamples > 1.0f ? d.readLinear (fbDelaySamples) : raw;
        x = routeLP[(size_t) i].process (x);
        x = fastTanh (x * fbDriveGain) * fbDriveNorm;
        if(filters)x=filters->at(FilterPosition::CrossFeedback,x,filterLane+i);
        return x * fbDampGain * fbPolarity;
    }

    CollisionRoute contact;
    float contactInjection[3]{},contactLoss[3]{};
    ModularFilters* filters=nullptr;int filterLane=0;
    float sampleRate = 44100.0f, gSmooth = 0.005f;
    std::array<ResonatorSlot, 3> slots;
    std::array<FractionalDelay, 3> routeDelay;
    std::array<OnePole, 3> routeLP;
    FractionalDelay loopDelay;
    OnePole loopLP;
    float g[kNumGains] {}, gTarget[kNumGains] {};
    float fb[3] = { 0.0f, 0.0f, 0.0f };
    bool running[3] = { true, false, false };
    bool hybrid = false;
    float fbDelaySamples = 0.0f, fbDelayTarget = 0.0f, fbDriveGain = 1.0f, fbDriveNorm = 1.0f, fbDampGain = 1.0f, fbPolarity = 1.0f;
    float loopDelaySamples = 1.0f, loopDelayTarget = 1.0f, loopDriveGain = 1.0f, loopDriveNorm = 1.0f, loopReturnValue = 0.0f;
    LoopSource loopSrc = LoopSource::Mix;
    float governorGain = 1.0f, netEnergyValue = 0.0f, peakEnv = 0.0f;
    int releaseCountdown[3] = { 0, 0, 0 };
};
} // namespace aeriform::dsp
