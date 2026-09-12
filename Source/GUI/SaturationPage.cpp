#include "SaturationPage.h"
namespace aeriform {
using namespace theme;
SaturationPage::SaturationPage(AeriformProcessor& p){global=add<Global>(p);for(int b=0;b<3;++b)bands[(size_t)b]=add<Band>(p,b);}
SaturationPage::Global::Global(AeriformProcessor& p):ParamPanel(p,"MULTIBAND SATURATION / CROSSOVERS",copper){enabled=control<Toggle>(p,ids::satOn,"ENABLED");quality=control<ChoiceBox>(p,ids::satQuality,"Oversampling");low=knob(ids::satLow,"Low split");high=knob(ids::satHigh,"High split");mix=knob(ids::satMix,"Global mix");}
void SaturationPage::Global::resized(){auto r=getContentArea();auto left=r.removeFromLeft(160);enabled->setBounds(left.removeFromTop(24));quality->setBounds(left.removeFromTop(42));r.removeFromLeft(8);knobRow(r,{low,high,mix},4);}
SaturationPage::Band::Band(AeriformProcessor& p,int b):ParamPanel(p,b==0?"LOW / WEIGHT":b==1?"MID / BODY":"HIGH / EDGE",b==1?teal:copper){const int base=(int)P::satLowDrive+b*4;drive=knob(ids::id((P)base),"Drive");model=control<ChoiceBox>(p,ids::id((P)(base+1)),"Character");mix=knob(ids::id((P)(base+2)),"Band mix");output=knob(ids::id((P)(base+3)),"Output");}
void SaturationPage::Band::resized(){auto r=getContentArea();model->setBounds(r.removeFromTop(42));r.removeFromTop(8);knobRow(r.removeFromTop(84),{drive,output},4);mix->setBounds(r.removeFromTop(84));}
void SaturationPage::resized(){auto r=getLocalBounds();global->setBounds(r.removeFromTop(104));r.removeFromTop(8);const int w=(r.getWidth()-16)/3;for(auto* b:bands){b->setBounds(r.removeFromLeft(w));r.removeFromLeft(8);}}
void SaturationPage::paint(juce::Graphics&){}
}
