#include "PipePrototype.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace pipe_probe;
struct Control { const char* name; double Settings::*field; double lo,hi; bool logarithmic=false; };
const Control controls[] {
    {"pressure",&Settings::pressure,.03,1}, {"dcnoise",&Settings::dcnoise,0,1},
    {"exc_cut",&Settings::excCut,100,10000,true}, {"exc_res",&Settings::excQ,.5,8},
    {"exc_kt",&Settings::excKt,0,1.5}, {"exc_vt",&Settings::excVt,0,1},
    {"rt",&Settings::rt,.01,30,true}, {"rt_kt",&Settings::rtKt,0,1.5},
    {"damp",&Settings::damp,0,1}, {"lp_0",&Settings::lp0,100,18000,true},
    {"lp_1",&Settings::lp1,100,18000,true}, {"hp_0",&Settings::hp0,20,2000,true},
    {"hp_1",&Settings::hp1,20,2000,true}, {"filt_kt",&Settings::filtKt,0,1.5},
    {"w",&Settings::morph,0,1}, {"sat_drive",&Settings::drive,1,32,true},
    {"sat_knee",&Settings::hardness,0,1}, {"sat_sym",&Settings::asym,-1,1}
};

int main(int argc,char**argv)
{
    if(argc!=2) { std::cerr<<"usage: pipe-render output-directory\n"; return 2; }
    const std::filesystem::path dir=argv[1]; std::filesystem::create_directories(dir);
    std::ofstream manifest(dir/"renders.csv");
    manifest<<"name,kind,fs,note,rt,pressure,cylinder,delay,gain,phase_residual,clamped,guards,peak,rms,phase_only_delay,peak_tuned\n";
    manifest<<std::setprecision(12);
    auto render=[&](const std::string& name,const std::string& kind,Settings p,double fs,
                    double duration,const Control* control=nullptr,bool macro=false)
    {
        Pipe pipe; pipe.prepare(fs,p,12345+static_cast<uint32_t>(p.note));
        const int count=static_cast<int>(fs*duration);
        std::vector<float> data(static_cast<size_t>(count));
        double sum=0,peak=0;
        for(int i=0;i<count;++i)
        {
            const double t=i/fs, sweep=std::clamp((t-.5)/(duration-1.),0.,1.);
            if((i%64)==0 && (control || macro))
            {
                if(control) p.*(control->field)=control->logarithmic ?
                    control->lo*std::pow(control->hi/control->lo,sweep) : std::lerp(control->lo,control->hi,sweep);
                if(macro) {p.pressure=std::lerp(.03,.8,sweep);p.dcnoise=std::lerp(1.,.2,sweep);p.excCut=std::lerp(800.,5000.,sweep);}
                pipe.configure(p);
            }
            // Fixture envelope only, not a replacement for the plugin's shared ADSR.
            const double envelope=std::min(std::clamp(t/.08,0.,1.),std::clamp((duration-t)/.2,0.,1.));
            double value=kind=="impulse" ? pipe.processExcitation(i==0 ? .001 : 0) : pipe.next(envelope);
            if(!std::isfinite(value)) {std::cerr<<"nonfinite "<<name<<'\n';return false;}
            data[static_cast<size_t>(i)]=static_cast<float>(value);
            sum+=value*value; peak=std::max(peak,std::abs(value));
        }
        std::ofstream file(dir/(name+".f32"),std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.data()),static_cast<std::streamsize>(data.size()*sizeof(float)));
        manifest<<name<<','<<kind<<','<<fs<<','<<p.note<<','<<p.rt<<','<<p.pressure<<','<<p.cylinder<<','
            <<pipe.getDelay()<<','<<pipe.getGain()<<','<<pipe.getPhaseResidual()<<','<<pipe.isClamped()<<','
            <<pipe.guards()<<','<<peak<<','<<std::sqrt(sum/count)<<','<<pipe.getPhaseOnlyLength()<<','<<pipe.hasTunedPeak()<<'\n';
        return bool(file) && pipe.guards()==0;
    };
    bool ok=true;
    for(double rt : {.15,3.})
    {
        const std::string tag=rt<1 ? "short" : "long";
        Settings p; p.note=57; p.rt=rt;p.rtKt=0;
        for(double pressure : {.08,.3,.8})
        {p.pressure=pressure;ok &= render("pressure-"+tag+"-"+std::to_string(int(pressure*100)),"pressure",p,48000,3);}
        ok &= render("pressure-sweep-"+tag,"pressure_sweep",p,48000,8,&controls[0]);
        ok &= render("macro-sweep-"+tag,"macro",p,48000,8,nullptr,true);
    }
    for(const auto& c:controls)
    {
        Settings p;p.note=72;p.velocity=.8;
        if(std::string(c.name)=="lp_1" || std::string(c.name)=="hp_1")p.morph=1;
        ok &= render(std::string("control-")+c.name,"control",p,48000,4,&c);
        const std::string controlName=c.name;
        if(controlName=="exc_kt" || controlName=="exc_vt" || controlName=="rt_kt" || controlName=="filt_kt")
            for(int note : {48,72})
                for(double velocity : {.2,.9})
                {
                    p.note=note;p.velocity=velocity;
                    const auto name=std::string("tracking-")+c.name+"-n"+std::to_string(note)+"-v"+std::to_string(int(velocity*100));
                    ok &= render(name,"tracking",p,48000,4,&c);
                }
    }
    for(bool cylinder : {false,true})
    {
        Settings p;p.cylinder=cylinder;
        ok &= render(cylinder ? "bore-cylinder" : "bore-cone","bore",p,48000,4);
        for(double fs : {44100.,48000.,96000.})
            for(int note=33;note<=93;note+=12)
                for(bool closed : {false,true})
                {
                    p={};p.cylinder=cylinder;p.note=note;p.drive=1;p.hardness=1;p.rt=3;p.rtKt=0;
                    p.morph=closed?1:0;
                    const auto name=std::string("impulse-")+(cylinder?"cyl-":"cone-")+std::to_string(int(fs))+"-"+std::to_string(note)+(closed?"-closed":"-open");
                    ok &= render(name,"impulse",p,fs,2);
                }
    }
    std::cout<<"Rendered pressure, macro, 18 continuous controls, both bores and 72 tuning probes to "<<dir<<'\n';
    return ok && manifest ? 0 : 1;
}
