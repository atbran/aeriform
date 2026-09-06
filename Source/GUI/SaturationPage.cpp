#include "SaturationPage.h"
namespace aeriform {
using namespace theme;
SaturationPage::SaturationPage(AeriformProcessor& p){global=add<Global>(p);for(int b=0;b<3;++b)bands[(size_t)b]=add<Band>(p,b);}
SaturationPage::Global::Global(AeriformProcessor& p):ParamPanel(p,"MULTIBAND SATURATION / CROSSOVERS",copper){enabled=control<Toggle>(p,ids::satOn,"ENABLED");quality=control<ChoiceBox>(p,ids::satQuality,"Oversampling");low=knob(ids::satLow,"Low split");high=knob(ids::satHigh,"High split");mix=knob(ids::satMix,"Global mix");}
void SaturationPage::Global::resized(){auto r=getContentArea().reduced(14,8);auto left=r.removeFromLeft(r.getWidth()/3);enabled->setBounds(left.removeFromTop(28));left.removeFromTop(14);quality->setBounds(left.removeFromTop(46));r.removeFromLeft(24);knobRow(r,{low,high,mix},18);}
SaturationPage::Band::Band(AeriformProcessor& p,int b):ParamPanel(p,b==0?"LOW / WEIGHT":b==1?"MID / BODY":"HIGH / EDGE",b==1?teal:copper){const int base=(int)P::satLowDrive+b*4;drive=knob(ids::id((P)base),"Drive");model=control<ChoiceBox>(p,ids::id((P)(base+1)),"Character");mix=knob(ids::id((P)(base+2)),"Band mix");output=knob(ids::id((P)(base+3)),"Output");}
void SaturationPage::Band::resized(){auto r=getContentArea().reduced(14,10);model->setBounds(r.removeFromTop(46));r.removeFromTop(26);knobRow(r.removeFromTop(110),{drive,output},16);r.removeFromTop(14);mix->setBounds(r.removeFromTop(110).withSizeKeepingCentre(110,110));}
void SaturationPage::resized(){auto r=getLocalBounds();global->setBounds(r.removeFromTop(174));r.removeFromTop(12);r.removeFromBottom(58);const int width=(r.getWidth()-24)/3;for(auto* b:bands){b->setBounds(r.removeFromLeft(width));r.removeFromLeft(12);}}
void SaturationPage::paint(juce::Graphics& g){auto r=getLocalBounds().removeFromBottom(48).reduced(10,4);g.setColour(textSecondary);g.setFont(font(12));g.drawFittedText("Soft rounds the peaks. Warm adds asymmetric colour. Clip brings grit; Fold adds metallic overtones. Zero drive leaves a band undistorted. Adjust Output to balance the driven bands. The neutral active crossover has flat magnitude; disable the effect for raw bypass.",r,juce::Justification::centredLeft,3);}
}
