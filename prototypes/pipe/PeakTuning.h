#pragma once

#include <algorithm>
#include <cmath>
#include <complex>

namespace pipe_probe
{
// Small-signal output transfer: q=z^-1, F=delay(D-1)*ADAA*HP*LP,
// T=F/(1-sign*g*q*F). Includes the numerator, excludes driven saturation.
struct PeakResponse
{
    std::complex<double> transfer;
    double logMagnitudeSlope;
};

inline PeakResponse peakResponse(double length,double w,double aLP,double aHP,
                                 double gain,bool cylinder,bool adaa)
{
    using C=std::complex<double>;
    const C j(0,1),q=std::polar(1.,-w);
    const double integer=std::floor(length), f=length-integer;
    const double c0=-f*(f-1)*(f-2)/6,c1=(f+1)*(f-1)*(f-2)/2;
    const double c2=-(f+1)*f*(f-2)/2,c3=(f+1)*f*(f-1)/6;
    const C interpolation=c0/q+c1+c2*q+c3*q*q;
    const C interpolationDerivative=j*(c0/q-c2*q-2*c3*q*q);
    const C lp=(1-aLP)/(1.-aLP*q),hp=.5*(1+aHP)*(1.-q)/(1.-aHP*q);
    const C aa=adaa ? .5*(1.+q) : C(1,0);
    const C forward=std::polar(1.,-w*(integer-1))*interpolation*aa*hp*lp;
    const C loop=(cylinder ? -gain : gain)*q*forward;
    C logForwardDerivative=-j*(integer-1)+interpolationDerivative/interpolation
        -j*aLP*q/(1.-aLP*q)+j*q/(1.-q)-j*aHP*q/(1.-aHP*q);
    if(adaa)logForwardDerivative-=j*q/(1.+q);
    const C logTransferDerivative=(logForwardDerivative-j*loop)/(1.-loop);
    return {forward/(1.-loop),std::real(logTransferDerivative)};
}

struct PeakTuning
{
    double delay=0;
    double slope=0;
    bool found=false;
};

inline PeakTuning tuneMagnitudePeak(double seed,double w,double aLP,double aHP,
                                    double gain,bool cylinder,bool adaa,double maximum)
{
    // Stay near the phase-aligned fundamental instead of changing registers.
    // A heavily damped/nonresonant setting may have no peak here: report it.
    const double low=std::max(4.,seed*.8), high=std::min(maximum,seed*1.2);
    auto slope=[&](double d) {return peakResponse(d,w,aLP,aHP,gain,cylinder,adaa).logMagnitudeSlope;};
    PeakTuning result{std::clamp(seed,4.,maximum),0,false};
    result.slope=slope(result.delay);
    if(low>=high || !std::isfinite(result.slope))return result;
    double left=0,right=0,bestDistance=maximum;
    double previous=low,previousSlope=slope(low);
    // Bracket before bisection; choose the crossing nearest the phase solution.
    constexpr int intervals=32;
    for(int i=1;i<=intervals;++i)
    {
        const double current=std::lerp(low,high,double(i)/intervals),currentSlope=slope(current);
        if(std::isfinite(previousSlope) && std::isfinite(currentSlope)
           && previousSlope>=0 && currentSlope<=0)
        {
            const double distance=std::abs(.5*(previous+current)-seed);
            if(distance<bestDistance){left=previous;right=current;bestDistance=distance;}
        }
        previous=current;previousSlope=currentSlope;
    }
    if(bestDistance==maximum)return result;
    for(int i=0;i<36;++i)
    {
        const double middle=.5*(left+right);
        if(slope(middle)>0)left=middle;else right=middle;
    }
    const double candidate=.5*(left+right),step=w*1e-4;
    const double below=peakResponse(candidate,w-step,aLP,aHP,gain,cylinder,adaa).logMagnitudeSlope;
    const double above=peakResponse(candidate,w+step,aLP,aHP,gain,cylinder,adaa).logMagnitudeSlope;
    if(std::isfinite(below) && std::isfinite(above) && below>0 && above<0)
        result={candidate,slope(candidate),true};
    return result;
}
}
