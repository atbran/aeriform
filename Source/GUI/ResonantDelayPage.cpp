#include "ResonantDelayPage.h"
namespace aeriform {
using namespace theme;
ResonantDelayPage::ResonantDelayPage(AeriformProcessor& p){timing=add<Timing>(p);colour=add<Colour>(p);}
ResonantDelayPage::Timing::Timing(AeriformProcessor& p):ParamPanel(p,"RESONANT DELAY / REPEATS",copper){
    enabled=control<Toggle>(p,ids::rdOn,"ENABLED");sync=control<Toggle>(p,ids::rdSync,"TEMPO SYNC");division=control<ChoiceBox>(p,ids::rdDiv,"Division");
    time=knob(ids::rdTime,"Time");feedback=knob(ids::rdFeedback,"Feedback");mix=knob(ids::rdMix,"Mix");offset=knob(ids::rdOffset,"Stereo offset");saturation=knob(ids::rdSaturation,"Saturation");startTimerHz(10);timerCallback();
}
void ResonantDelayPage::Timing::timerCallback(){bool on=processor.getAPVTS().getRawParameterValue(ids::rdSync)->load()>.5f;time->setEnabled(!on);division->setEnabled(on);}
void ResonantDelayPage::Timing::resized(){auto r=getContentArea().reduced(16,8);auto row=r.removeFromTop(26);enabled->setBounds(row.removeFromLeft(row.getWidth()/2));sync->setBounds(row);r.removeFromTop(14);division->setBounds(r.removeFromTop(46));r.removeFromTop(30);knobRow(r.removeFromTop(110),{time,feedback,mix},18);r.removeFromTop(22);knobRow(r.removeFromTop(110),{offset,saturation},32);}
ResonantDelayPage::Colour::Colour(AeriformProcessor& p):ParamPanel(p,"PITCHED FEEDBACK / MODAL COLOUR",teal){
    type=control<ChoiceBox>(p,ids::rdType,"Resonator type");tuning=knob(ids::rdTuning,"Tuning");track=knob(ids::rdTrack,"Pitch tracking");amount=knob(ids::rdAmount,"Resonator amount");damping=knob(ids::rdDamping,"Damping");dispersion=knob(ids::rdDispersion,"Dispersion");
}
void ResonantDelayPage::Colour::resized(){auto r=getContentArea().reduced(16,8);type->setBounds(r.removeFromTop(46));r.removeFromTop(50);knobRow(r.removeFromTop(110),{tuning,track,amount},18);r.removeFromTop(22);knobRow(r.removeFromTop(110),{damping,dispersion},32);}
void ResonantDelayPage::resized(){auto r=getLocalBounds();r.removeFromTop(150);r.removeFromBottom(116);int w=(r.getWidth()-12)/2;timing->setBounds(r.removeFromLeft(w));r.removeFromLeft(12);colour->setBounds(r);}
void ResonantDelayPage::paint(juce::Graphics& g){
    auto top=getLocalBounds().removeFromTop(138);g.setColour(panel);g.fillRoundedRectangle(top.toFloat(),8);g.setColour(textPrimary);g.setFont(titleFont(17));g.drawText("REPEATS THAT BECOME AN INSTRUMENT",top.removeFromTop(42).reduced(16,0),juce::Justification::centredLeft);
    auto line=top.removeFromTop(46).reduced(16,0);const char* names[]={"INPUT","STEREO DELAY","MODAL COLOUR","FEEDBACK"};int width=line.getWidth()/4;
    for(int i=0;i<4;++i){auto box=line.removeFromLeft(width).reduced(6,2);g.setColour(inset);g.fillRoundedRectangle(box.toFloat(),6);g.setColour(i>1?teal:textSecondary);g.setFont(font(12,true));g.drawText(names[i],box,juce::Justification::centred);}
    g.setColour(textSecondary);g.setFont(font(12));g.drawText("First repeat preserves the source. Each trip around the feedback path adds modal colour.",top.reduced(22,0),juce::Justification::centredLeft);
    auto bottom=getLocalBounds().removeFromBottom(104).reduced(18,12);g.setFont(font(13));g.drawFittedText("Choose Harmonic for tuned echoes, Metallic bar or Membrane for physical colour, or Vowel body for formant-like repeats. Dispersion stretches the upper modes. Pitch tracking follows the last MIDI note relative to A3; Tuning sets the base frequency.",bottom.removeFromTop(56),juce::Justification::centredLeft,3,1.0f);
    g.setColour(textDim);g.drawFittedText("Feedback reduces fresh input as repeats accumulate. Turning the effect off fades its output, then clears the stored tail.",bottom,juce::Justification::centredLeft,2,1.0f);
}
}
