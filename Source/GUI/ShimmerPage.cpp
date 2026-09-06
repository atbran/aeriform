#include "ShimmerPage.h"
namespace aeriform {
using namespace theme;
ShimmerPage::ShimmerPage(AeriformProcessor& p){controls=add<Controls>(p);}
ShimmerPage::Controls::Controls(AeriformProcessor& p):ParamPanel(p,"SHIMMER / SHIFTED REVERB",teal){
    enabled=control<Toggle>(p,ids::shOn,"ENABLED");knobs={knob(ids::shInterval,"Shift interval"),knob(ids::shFeedback,"Feedback"),knob(ids::shMix,"Mix"),knob(ids::shSize,"Size"),knob(ids::shDiffusion,"Diffusion"),knob(ids::shDamping,"Damping"),knob(ids::shSpread,"Stereo spread"),knob(ids::shLowCut,"Low cut"),knob(ids::shHighCut,"High cut")};
    for(auto* b:{&octave,&fifth,&fourth})addAndMakeVisible(b);octave.onClick=[&p]{p.getPatchTools().setParameter(ids::shInterval,12);};fifth.onClick=[&p]{p.getPatchTools().setParameter(ids::shInterval,7);};fourth.onClick=[&p]{p.getPatchTools().setParameter(ids::shInterval,5);};
}
void ShimmerPage::Controls::resized(){auto r=getContentArea().reduced(12,8);enabled->setBounds(r.removeFromTop(26));r.removeFromTop(12);auto buttons=r.removeFromTop(28);int w=(buttons.getWidth()-12)/3;for(auto* b:{&octave,&fifth,&fourth}){b->setBounds(buttons.removeFromLeft(w));buttons.removeFromLeft(6);}r.removeFromTop(24);for(int i=0;i<3;++i){knobRow(r.removeFromTop(110),{knobs[(size_t)i*3],knobs[(size_t)i*3+1],knobs[(size_t)i*3+2]},12);r.removeFromTop(20);}}
void ShimmerPage::resized(){controls->setBounds(0,0,getWidth()/2-6,getHeight());}
void ShimmerPage::paint(juce::Graphics& g){auto r=getLocalBounds().withTrimmedLeft(getWidth()/2+24).reduced(10,16);g.setColour(textPrimary);g.setFont(titleFont(18));g.drawText("REFLECTIONS WITH A SECOND VOICE",r.removeFromTop(44),juce::Justification::centredLeft);
    auto graph=r.removeFromTop(230).toFloat().reduced(15,12);for(int i=0;i<7;++i){float x=graph.getX()+i*graph.getWidth()/8,level=(float)(i+1)/7;g.setColour(teal.withAlpha(.18f+level*.5f));g.fillRoundedRectangle(x,graph.getBottom()-level*graph.getHeight(),graph.getWidth()/10,level*graph.getHeight(),5);}
    g.setColour(textSecondary);g.setFont(font(14));r.removeFromTop(20);g.drawFittedText("The tail passes through a pitch shifter and returns to the reverb. An octave adds a rising halo; a fifth or fourth builds a different harmonic cloud. Shift interval also accepts custom semitones and downward shifts.",r.removeFromTop(130),juce::Justification::centredLeft,6,1.0f);
    r.removeFromTop(20);g.drawFittedText("Feedback extends the reflections and strengthens their shifted return. Low cut keeps the feedback clear of bass, while High cut and Damping soften the upper layers. Spread controls the stereo field.",r.removeFromTop(130),juce::Justification::centredLeft,6,1.0f);
    r.removeFromTop(12);g.setColour(textDim);g.drawFittedText("The granular pitch shifter adds a soft, moving texture to the wet tail. Your dry signal remains immediate. Turn the effect off to fade and clear its energy.",r.removeFromTop(110),juce::Justification::centredLeft,5,1.0f);
}
}
