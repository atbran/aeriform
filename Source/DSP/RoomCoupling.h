#pragma once
#include "DspUtils.h"
namespace aeriform::dsp {
/** A source-funded room injection budget, independent of resonator gain, body,
    filters, and the master limiter. Power must be measured BEFORE any network/
    energy-loop audio is mixed into the exciter chain. This is an engineering
    signal-energy bound, not a claim of physical passivity for the whole synth. */
class RoomCoupling {
public:
    void prepare(float sr) noexcept {dt=1.0/sr;release=std::exp(-1/(.25f*sr));leak=std::exp(-1/(.5*sr));reset();}
    void reset() noexcept {budget=credited=spent=0;envelope=0;gain=1;}
    float next(float returned,float sourcePower) noexcept {
        const double power=std::clamp((double)sanitize(sourcePower),0.0,4.0);
        const double credit=.25*power*dt;
        credited+=credit;budget=std::min(.05,budget*leak+credit);
        envelope=std::max((float)std::sqrt(power),envelope*release);
        const float peakBound=std::min(.25f,.75f*envelope);
        if(peakBound<1e-12f){gain=1;return 0;}
        const double desired=peakBound*std::tanh(sanitize(returned)/peakBound);
        const double request=desired*desired*dt;
        // Smooth knee with an exact energy budget: emitted energy never exceeds
        // current credit plus stored budget, even on an abrupt full-scale return.
        gain=request>0?(float)std::sqrt(budget/(budget+request)):1.0f;
        const float output=(float)(desired*gain);
        const double used=(double)output*output*dt;
        budget=std::max(0.0,budget-used);spent+=used;
        return output;
    }
    double inputCredit() const noexcept {return credited;}
    double outputEnergy() const noexcept {return spent;}
    double storedBudget() const noexcept {return budget;}
    float inputGain() const noexcept {return gain;}
private:
    double dt=1.0/48000,leak=1,budget=0,credited=0,spent=0;
    float envelope=0,release=1,gain=1;
};
}
