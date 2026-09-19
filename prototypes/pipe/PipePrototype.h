#pragma once

// Standalone P1 experiment; not included by any plugin target.
#include "../../Source/DSP/FractionalDelay.h"
#include "PeakTuning.h"
#include <complex>
#include <cstdint>
#include <stdexcept>

namespace pipe_probe
{
constexpr double pi=3.14159265358979323846;
using Complex=std::complex<double>;
inline double nominalGain(double period, double fs, double rt)
{ return std::pow(10., -3.*period/(fs*std::clamp(rt,.001,30.))); }
inline double pole(double fs,double fc)
{ return std::exp(-2*pi*std::clamp(fc,20.,.45*fs)/fs); }
inline Complex lpResponse(double fs,double fc,double w)
{ const double a=pole(fs,fc); return (1-a)/(1.-a*std::polar(1.,-w)); }
inline Complex hpResponse(double fs,double fc,double w)
{ const double a=pole(fs,fc); const auto z=std::polar(1.,-w); return .5*(1+a)*(1.-z)/(1.-a*z); }
inline double allpassPhase(double c,double w)
{ return -w+2*std::atan2(c*std::sin(w),1+c*std::cos(w)); }
inline double knee(double u,double h,double s)
{ return std::clamp(.9*h*(1+(u>=0 ? s : -s)),0.,.95); }
inline double curve(double u,double h,double s)
{
    const double k=knee(u,h,s), a=std::abs(u);
    if(a<=k) return u;
    const double t=(a-k)/(1-k);
    const double f=t<=1 ? t-t*t*t/3 : 2./3;
    return std::copysign(k+(1-k)*f,u);
}
inline double primitive(double u,double h,double s)
{
    const double k=knee(u,h,s), a=std::abs(u);
    if(a<=k) return .5*u*u;
    const double t=(a-k)/(1-k);
    const double f=t<=1 ? .5*t*t-t*t*t*t/12 : (2./3)*t-.25;
    return .5*k*k+(1-k)*(k*t+(1-k)*f);
}

class Saturator
{
public:
    double process(double x,double d,double h,double s,bool adaa)
    {
        const double u=d*x, prev=d*previousX;
        const double nowF=primitive(u,h,s);
        const double oldF=(d==drive && h==hard && s==asym) ? previousF : primitive(prev,h,s);
        const double delta=u-prev;
        const double out=!adaa ? curve(u,h,s) :
            (std::abs(delta)<1e-5 ? curve(.5*(u+prev),h,s) : (nowF-oldF)/delta);
        previousX=x; previousF=nowF; drive=d; hard=h; asym=s;
        return out/d;
    }
private:
    double previousX=0,previousF=0,drive=1,hard=.4,asym=0;
};

struct Settings
{
    double note=60, velocity=.8;
    double pressure=.5, dcnoise=.5, excCut=2000, excQ=.7, excKt=.5, excVt=.5;
    double rt=.5, rtKt=.5, damp=0;
    double lp0=4000,lp1=800,hp0=40,hp1=300,filtKt=1,morph=0;
    double drive=2,hardness=.4,asym=0;
    bool cylinder=false, adaa=true;
};

inline Complex lagrangeFraction(double f,double w)
{
    const double c0=-f*(f-1)*(f-2)/6, c1=(f+1)*(f-1)*(f-2)/2;
    const double c2=-(f+1)*f*(f-2)/2, c3=(f+1)*f*(f-1)/6;
    return c0*std::polar(1.,w)+c1+c2*std::polar(1.,-w)+c3*std::polar(1.,-2*w);
}

class Pipe
{
public:
    void prepare(double sampleRate,const Settings& settings,uint32_t seed=12345)
    {
        if(!std::isfinite(sampleRate) || sampleRate<8000 || sampleRate>192000)
            throw std::invalid_argument("prototype sample rate outside 8–192 kHz");
        fs=sampleRate;
        delay.prepare(static_cast<int>(std::ceil(fs/16*1.5))+64);
        sat={}; hpX=hpY=lpY=excZ1=excZ2=feedback=0;
        rng=seed ? seed : 1; guardCount=0;tuningCached=false;
        configure(settings);
    }
    // Prototype renderer supplies smoothly moving controls at 64-sample intervals.
    // This is not production-grade arbitrary host-automation handling.
    void configure(const Settings& settings)
    {
        p=settings;
        const double f0=std::clamp(440*std::exp2((p.note-69)/12),16.,fs*.1);
        const double track=std::exp2(p.filtKt*(p.note-60)/12);
        lpCut=std::clamp(std::lerp(p.lp0,p.lp1,p.morph)*track,20.,fs*.45);
        hpCut=std::clamp(std::lerp(p.hp0,p.hp1,p.morph)*track,20.,fs*.45);
        aLP=pole(fs,lpCut); aHP=pole(fs,hpCut); bHP=.5*(1+aHP);
        const double rt=std::clamp(p.rt*std::exp2(-p.rtKt*(p.note-60)/12)*(1-p.damp),.001,30.);
        const double total=fs/(f0*(p.cylinder ? 2 : 1));
        gain=nominalGain(total,fs,rt);
        const double w=2*pi*f0/fs;
        const double filters=-(std::arg(lpResponse(fs,lpCut,w))+std::arg(hpResponse(fs,hpCut,w)))/w;
        const double base=total-filters-(p.adaa ? .5 : 0.);
        double length=base;
        for(int i=0;i<12;++i)
        {
            const double fraction=length-std::floor(length);
            const double extra=-std::arg(lagrangeFraction(fraction,w))/w-fraction;
            length=base-extra;
        }
        phaseOnlyLength=length;
        if(w!=cachedW || aLP!=cachedLP || aHP!=cachedHP || gain!=cachedGain
           || p.cylinder!=cachedCylinder || p.adaa!=cachedAdaa || !tuningCached)
        {
            peakTuning=tuneMagnitudePeak(length,w,aLP,aHP,gain,p.cylinder,p.adaa,delay.getMaxDelay());
            cachedW=w;cachedLP=aLP;cachedHP=aHP;cachedGain=gain;
            cachedCylinder=p.cylinder;cachedAdaa=p.adaa;tuningCached=true;
        }
        length=peakTuning.delay;
        clamped=phaseOnlyLength<4 || phaseOnlyLength>delay.getMaxDelay();
        delayLength=static_cast<float>(std::clamp(length,4.,double(delay.getMaxDelay())));
        const double fraction=delayLength-std::floor(delayLength);
        phaseResidual=w*(double(delayLength)-base-fraction)-std::arg(lagrangeFraction(fraction,w));

        // RBJ/TDF-II two-pole lowpass, outside the feedback loop.
        const double fc=std::clamp(p.excCut*std::exp2((p.excKt*(p.note-60)+p.excVt*24*p.velocity)/12),20.,fs*.45);
        const double omega=2*pi*fc/fs, co=std::cos(omega), alpha=std::sin(omega)/(2*p.excQ);
        const double den=1+alpha;
        b0=(1-co)*.5/den; b1=(1-co)/den; b2=b0;
        a1=-2*co/den; a2=(1-alpha)/den;
    }
    double next(double envelope)
    {
        rng^=rng<<13; rng^=rng>>17; rng^=rng<<5;
        const double noise=2*(double(rng)/4294967295.)-1;
        const double input=p.pressure*envelope*((1-p.dcnoise)+p.dcnoise*noise);
        const double exc=b0*input+excZ1;
        excZ1=b1*input-a1*exc+excZ2; excZ2=b2*input-a2*exc;
        return processExcitation(exc);
    }
    double processExcitation(double excitation)
    {
        delay.push(static_cast<float>(excitation+(p.cylinder ? -feedback : feedback)));
        const double delayed=delay.readLagrange(delayLength);
        const double shaped=sat.process(delayed,p.drive,p.hardness,p.asym,p.adaa);
        const double high=bHP*(shaped-hpX)+aHP*hpY; hpX=shaped; hpY=high;
        const double low=(1-aLP)*high+aLP*lpY; lpY=low;
        feedback=gain*low;
        if(!std::isfinite(low))
        {
            ++guardCount; delay.clear(); sat={}; feedback=hpX=hpY=lpY=excZ1=excZ2=0;
            return 0;
        }
        return low;
    }
    double getDelay() const { return delayLength; }
    double getGain() const { return gain; }
    double getPhaseResidual() const { return phaseResidual; }
    double getPhaseOnlyLength() const { return phaseOnlyLength; }
    bool hasTunedPeak() const { return peakTuning.found; }
    bool isClamped() const { return clamped; }
    unsigned guards() const { return guardCount; }
private:
    Settings p;
    aeriform::dsp::FractionalDelay delay;
    Saturator sat;
    uint32_t rng=1;
    double fs=48000,feedback=0,gain=0,phaseResidual=0,lpCut=0,hpCut=0;
    double aLP=0,aHP=0,bHP=0,hpX=0,hpY=0,lpY=0;
    double b0=0,b1=0,b2=0,a1=0,a2=0,excZ1=0,excZ2=0;
    float delayLength=100;
    PeakTuning peakTuning;
    double phaseOnlyLength=100,cachedW=0,cachedLP=0,cachedHP=0,cachedGain=0;
    bool tuningCached=false,cachedCylinder=false,cachedAdaa=false;
    bool clamped=false;
    unsigned guardCount=0;
};
}
