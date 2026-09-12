#include "ShimmerPage.h"
namespace aeriform {
using namespace theme;
ShimmerPage::ShimmerPage(AeriformProcessor& p){controls=add<Controls>(p);}
ShimmerPage::Controls::Controls(AeriformProcessor& p):ParamPanel(p,"SHIMMER / SHIFTED REVERB",teal){
    enabled=control<Toggle>(p,ids::shOn,"ENABLED");knobs={knob(ids::shInterval,"Shift interval"),knob(ids::shFeedback,"Feedback"),knob(ids::shMix,"Mix"),knob(ids::shSize,"Size"),knob(ids::shDiffusion,"Diffusion"),knob(ids::shDamping,"Damping"),knob(ids::shSpread,"Stereo spread"),knob(ids::shLowCut,"Low cut"),knob(ids::shHighCut,"High cut")};
    for(auto* b:{&octave,&fifth,&fourth})addAndMakeVisible(b);octave.onClick=[&p]{p.getPatchTools().setParameter(ids::shInterval,12);};fifth.onClick=[&p]{p.getPatchTools().setParameter(ids::shInterval,7);};fourth.onClick=[&p]{p.getPatchTools().setParameter(ids::shInterval,5);};
}
void ShimmerPage::Controls::resized(){auto r=getContentArea();enabled->setBounds(r.removeFromTop(24));auto buttons=r.removeFromTop(26);int w=(buttons.getWidth()-12)/3;for(auto* b:{&octave,&fifth,&fourth}){b->setBounds(buttons.removeFromLeft(w));buttons.removeFromLeft(6);}r.removeFromTop(8);for(int i=0;i<3;++i)knobRow(r.removeFromTop(84),{knobs[(size_t)i*3],knobs[(size_t)i*3+1],knobs[(size_t)i*3+2]},8);}
void ShimmerPage::resized(){controls->setBounds(getLocalBounds());}
void ShimmerPage::paint(juce::Graphics&){}
}
