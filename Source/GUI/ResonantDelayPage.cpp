#include "ResonantDelayPage.h"
namespace aeriform {
using namespace theme;
ResonantDelayPage::ResonantDelayPage(AeriformProcessor& p){timing=add<Timing>(p);colour=add<Colour>(p);}
ResonantDelayPage::Timing::Timing(AeriformProcessor& p):ParamPanel(p,"RESONANT DELAY / REPEATS",copper){
    enabled=control<Toggle>(p,ids::rdOn,"ENABLED");sync=control<Toggle>(p,ids::rdSync,"TEMPO SYNC");division=control<ChoiceBox>(p,ids::rdDiv,"Division");
    time=knob(ids::rdTime,"Time");feedback=knob(ids::rdFeedback,"Feedback");mix=knob(ids::rdMix,"Mix");offset=knob(ids::rdOffset,"Stereo offset");saturation=knob(ids::rdSaturation,"Saturation");startTimerHz(10);timerCallback();
}
void ResonantDelayPage::Timing::timerCallback(){bool on=processor.getAPVTS().getRawParameterValue(ids::rdSync)->load()>.5f;time->setEnabled(!on);division->setEnabled(on);}
void ResonantDelayPage::Timing::resized(){auto r=getContentArea();auto row=r.removeFromTop(24);enabled->setBounds(row.removeFromLeft(row.getWidth()/2));sync->setBounds(row);division->setBounds(r.removeFromTop(42));r.removeFromTop(8);knobRow(r.removeFromTop(84),{time,feedback,mix},4);knobRow(r.removeFromTop(84),{offset,saturation},8);}
ResonantDelayPage::Colour::Colour(AeriformProcessor& p):ParamPanel(p,"PITCHED FEEDBACK / MODAL COLOUR",teal){
    type=control<ChoiceBox>(p,ids::rdType,"Resonator type");tuning=knob(ids::rdTuning,"Tuning");track=knob(ids::rdTrack,"Pitch tracking");amount=knob(ids::rdAmount,"Resonator amount");damping=knob(ids::rdDamping,"Damping");dispersion=knob(ids::rdDispersion,"Dispersion");
}
void ResonantDelayPage::Colour::resized(){auto r=getContentArea();r.removeFromTop(24);type->setBounds(r.removeFromTop(42));r.removeFromTop(8);knobRow(r.removeFromTop(84),{tuning,track,amount},4);knobRow(r.removeFromTop(84),{damping,dispersion},8);}
void ResonantDelayPage::resized(){auto r=getLocalBounds();int w=(r.getWidth()-8)/2;timing->setBounds(r.removeFromLeft(w));r.removeFromLeft(8);colour->setBounds(r);}
void ResonantDelayPage::paint(juce::Graphics&){}
}
