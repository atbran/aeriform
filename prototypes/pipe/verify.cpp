// Engineering diagnostics, not an acceptance suite. No musical thresholds.
#include "PipePrototype.h"
#include <iostream>
#include <iomanip>
#include <complex>
#include <fstream>

int main(int argc,char** argv)
{
    using namespace pipe_probe;
    if(argc>2){std::cerr<<"usage: pipe-verify [extreme-grid.csv]\n";return 2;}
    std::ofstream grid;
    if(argc==2)
    {
        grid.open(argv[1]);
        if(!grid)return 2;
        grid<<"fs,note,cylinder,adaa,lp,hp,rt,phase_delay,tuned_delay,peak_found\n"<<std::setprecision(12);
    }
    std::cout << std::setprecision(12) << "measurement,value\n";
    double derivativeError = 0, contractionExcess = 0, hpPeak = 0, apError = 0;
    for (double h : {0.0, 0.4, 1.0})
        for (double s : {-1.0, -0.3, 0.0, 0.7, 1.0})
            for (int i = -2000; i <= 2000; ++i)
            {
                const double x = i / 1000.0, e = 1e-6;
                const double numerical = (primitive(x+e,h,s)-primitive(x-e,h,s))/(2*e);
                derivativeError = std::max(derivativeError, std::abs(numerical-curve(x,h,s)));
                contractionExcess = std::max(contractionExcess, std::abs(curve(x,h,s))-std::abs(x));
            }
    for (double c : {0.0, 0.2, 0.5, 0.99, 1.0})
        for (int i = 1; i < 1000; ++i)
        {
            const double w = pi*i/1000;
            const auto z = std::polar(1.0,-w);
            apError = std::max(apError, std::abs(std::arg((c+z)/(1.0+c*z))-allpassPhase(c,w)));
        }
    for (double fs : {44100.,48000.,96000.})
        for (double fc : {20.,300.,2000.})
            for (int i=0; i<=1000; ++i)
                hpPeak=std::max(hpPeak,std::abs(hpResponse(fs,fc,pi*i/1000)));
    std::cout << "primitive_derivative_max_error," << derivativeError << '\n'
              << "curve_magnitude_bound_excess," << contractionExcess << '\n'
              << "allpass_phase_max_error_rad," << apError << '\n'
              << "hp_peak_magnitude," << hpPeak << '\n';

    // Actual repeated multiplication, with integer round trips and no other DSP.
    for (int period : {27,109,436,873})
        for (double rt : {0.05,0.5,3.0})
        {
            const double g=nominalGain(period,48000,rt);
            double x=1; int trips=0;
            while (x>0.001) { x*=g; ++trips; }
            const double fraction=std::log(0.001/(x/g))/std::log(g);
            const double measured=(trips-1+fraction)*period/48000.;
            std::cout << "isolated_rt_error_s_D" << period << "_RT" << rt << ',' << measured-rt << '\n';
        }
    // Constant input under changing curve must be evaluated in one current curve.
    Saturator sat;
    double automationError=0;
    for (int i=0; i<10000; ++i)
    {
        const double drive=1+31.*i/9999, h=double(i%100)/99, s=std::sin(i*.01);
        const double y=sat.process(.2,drive,h,s,true);
        if(i>0) automationError=std::max(automationError,std::abs(y-curve(drive*.2,h,s)/drive));
    }
    std::cout << "adaa_constant_input_automation_error," << automationError << '\n';

    double lpPhaseError=0,hpPhaseError=0,lagrangePeak=0,linearAdaaError=0;
    for(double fs : {44100.,48000.,96000.})
        for(double fc : {20.,300.,2000.,10000.})
            for(int i=1;i<1000;++i)
            {
                const double w=pi*i/1000, a=pole(fs,fc);
                const double denominator=std::atan2(a*std::sin(w),1-a*std::cos(w));
                lpPhaseError=std::max(lpPhaseError,std::abs(std::arg(lpResponse(fs,fc,w))+denominator));
                hpPhaseError=std::max(hpPhaseError,std::abs(std::arg(hpResponse(fs,fc,w))-((pi-w)*.5-denominator)));
            }
    for(int j=0;j<=100;++j)
        for(int i=0;i<=1000;++i)
            lagrangePeak=std::max(lagrangePeak,std::abs(lagrangeFraction(j/100.,pi*i/1000.)));
    Saturator linear;
    double previous=0;
    for(int i=0;i<10000;++i)
    {
        const double x=.01*std::sin(i*.4);
        linearAdaaError=std::max(linearAdaaError,std::abs(linear.process(x,1,1,0,true)-.5*(x+previous)));
        previous=x;
    }
    std::cout<<"lp_phase_max_error_rad,"<<lpPhaseError<<'\n'
             <<"hp_phase_max_error_rad,"<<hpPhaseError<<'\n'
             <<"lagrange_sampled_peak_magnitude,"<<lagrangePeak<<'\n'
             <<"adaa_linear_average_max_error,"<<linearAdaaError<<'\n';
    // Directly exercise read/write timing, independent of the phase equations.
    aeriform::dsp::FractionalDelay delay;delay.prepare(128);
    int first=-1;
    for(int i=0;i<30;++i)
    {
        delay.push(i==0 ? 1.f : 0.f);
        if(delay.readLagrange(10.f)!=0 && first<0)first=i;
    }
    std::cout<<"push_before_read_D10_impulse_sample,"<<first<<'\n';
    Settings dc;dc.dcnoise=0;
    Pipe dcPipe;dcPipe.prepare(48000,dc);
    double dcEnergy=0;
    for(int i=0;i<480000;++i)
    {
        const double y=dcPipe.next(1);
        if(i>=432000)dcEnergy+=y*y;
    }
    std::cout<<"pure_dc_last_second_rms,"<<std::sqrt(dcEnergy/48000)<<'\n';
    Saturator changing;
    double previousInput=0,quadratureError=0;
    for(int i=0;i<1000;++i)
    {
        const double x=.6*std::sin(i*.13),d=1+double(i%32),h=double(i%17)/16,s=std::sin(i*.031);
        double integral=0;
        constexpr int steps=256;
        for(int k=0;k<=steps;++k)
        {
            const double u=d*std::lerp(previousInput,x,double(k)/steps);
            integral+=(k==0 || k==steps ? 1 : (k%2==0 ? 2 : 4))*curve(u,h,s);
        }
        const double reference=integral/(3*steps*d);
        quadratureError=std::max(quadratureError,std::abs(changing.process(x,d,h,s,true)-reference));
        previousInput=x;
    }
    Saturator frequencyProbe;
    Complex inputBin=0,outputBin=0;
    const double omega=2*pi*137/4096;
    for(int i=0;i<8192;++i)
    {
        const double x=.01*std::sin(omega*i),y=frequencyProbe.process(x,1,1,0,true);
        if(i>=4096){inputBin+=x*std::polar(1.,-omega*i);outputBin+=y*std::polar(1.,-omega*i);}
    }
    const auto measured=outputBin/inputBin,expected=.5*(1.+std::polar(1.,-omega));
    std::cout<<"adaa_changing_curve_quadrature_max_error,"<<quadratureError<<'\n'
             <<"adaa_linear_magnitude_error,"<<std::abs(measured)-std::abs(expected)<<'\n'
             <<"adaa_linear_phase_delay_samples,"<<-std::arg(measured)/omega<<'\n';
    // Compare the new analytic derivative against direct transfer differences.
    // Relative scaling avoids labeling large near-resonance slopes as errors.
    double slopeError=0;
    for(double fs : {44100.,48000.,96000.})
        for(double hz : {55.,440.,1760.})
            for(bool cylinder : {false,true})
                for(bool adaa : {false,true})
                    for(double fraction : {0.,.1,.5,.99})
                    {
                        const double w=2*pi*hz/fs,d=std::floor(fs/(hz*(cylinder?2:1)))+fraction;
                        const double a=pole(fs,800),b=pole(fs,300),g=nominalGain(d,fs,3),dw=w*1e-6;
                        const auto center=peakResponse(d,w,a,b,g,cylinder,adaa);
                        const double lower=std::log(std::abs(peakResponse(d,w-dw,a,b,g,cylinder,adaa).transfer));
                        const double upper=std::log(std::abs(peakResponse(d,w+dw,a,b,g,cylinder,adaa).transfer));
                        const double numeric=(upper-lower)/(2*dw);
                        slopeError=std::max(slopeError,std::abs(numeric-center.logMagnitudeSlope)/(1+std::abs(numeric)));
                    }
    int found=0,unavailable=0;
    double largestSlope=0;
    for(double fs : {44100.,48000.,96000.})
        for(double note : {33.,60.,93.})
            for(bool cylinder : {false,true})
                for(bool adaa : {false,true})
                    for(double lp : {20.,800.,20000.})
                        for(double hp : {20.,300.,2000.})
                            for(double rt : {.001,.5,30.})
                            {
                                Settings s;s.note=note;s.cylinder=cylinder;s.adaa=adaa;
                                s.lp0=lp;s.hp0=hp;s.filtKt=0;s.rt=rt;s.rtKt=0;
                                Pipe probe;probe.prepare(fs,s);
                                if(grid.is_open())grid<<fs<<','<<note<<','<<cylinder<<','<<adaa<<','<<lp<<','<<hp<<','<<rt<<','
                                    <<probe.getPhaseOnlyLength()<<','<<probe.getDelay()<<','<<probe.hasTunedPeak()<<'\n';
                                if(probe.hasTunedPeak())
                                {
                                    ++found;
                                    const double w=2*pi*440*std::exp2((note-69)/12)/fs;
                                    largestSlope=std::max(largestSlope,std::abs(peakResponse(probe.getDelay(),w,pole(fs,lp),pole(fs,hp),probe.getGain(),cylinder,adaa).logMagnitudeSlope));
                                }
                                else ++unavailable;
                            }
    std::cout<<"peak_slope_scaled_derivative_error,"<<slopeError<<'\n'
             <<"extreme_grid_local_peaks_found,"<<found<<'\n'
             <<"extreme_grid_local_peaks_unavailable,"<<unavailable<<'\n'
             <<"extreme_grid_float_delay_max_slope,"<<largestSlope<<'\n';
    return !(std::isfinite(derivativeError) && std::isfinite(automationError) && (!grid.is_open() || bool(grid)));
}
