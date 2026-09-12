#include "SpectralPage.h"
namespace aeriform {
using namespace theme;
SpectralPage::SpectralPage(AeriformProcessor& p):processor(p){controls=add<Controls>(p);startTimerHz(24);}
SpectralPage::Controls::Controls(AeriformProcessor& p):ParamPanel(p,"SPECTRAL FREEZE / HELD TEXTURE",teal){
    enabled=control<Toggle>(p,ids::sfOn,"ENABLED");hold=control<Toggle>(p,ids::sfFreeze,"HOLD");
    blur=knob(ids::sfBlur,"Blur");shift=knob(ids::sfShift,"Spectral shift");random=knob(ids::sfRandom,"Random phase");decay=knob(ids::sfDecay,"Decay");mix=knob(ids::sfMix,"Mix");
    addAndMakeVisible(capture);addAndMakeVisible(release);
    capture.onClick=[&p]{auto& tools=p.getPatchTools();tools.begin("Capture spectrum");tools.setParameter(ids::sfOn,1);tools.setParameter(ids::sfFreeze,1);tools.setParameter(ids::sfCapture,p.getAPVTS().getRawParameterValue(ids::sfCapture)->load()>.5f?0:1);tools.end();};
    release.onClick=[&p]{auto& tools=p.getPatchTools();tools.begin("Release spectrum");tools.setParameter(ids::sfFreeze,0);tools.setParameter(ids::sfRelease,p.getAPVTS().getRawParameterValue(ids::sfRelease)->load()>.5f?0:1);tools.end();};
    capture.setTooltip("Capture one new spectrum per click. Host Capture Spectrum automation triggers on each change, in either direction.");
    release.setTooltip("Cancel a pending capture and return smoothly to live audio.");
}
void SpectralPage::Controls::resized(){auto r=getContentArea();auto row=r.removeFromTop(26);enabled->setBounds(row.removeFromLeft(row.getWidth()/2));hold->setBounds(row);row=r.removeFromTop(28);capture.setBounds(row.removeFromLeft((row.getWidth()-8)/2));row.removeFromLeft(8);release.setBounds(row);r.removeFromTop(8);knobRow(r.removeFromTop(82),{blur,shift,random},8);knobRow(r.removeFromTop(82),{decay,mix,nullptr},8);}
void SpectralPage::resized(){controls->setBounds(getLocalBounds().withHeight(278));}
void SpectralPage::paint(juce::Graphics& g){auto r=getLocalBounds().withTrimmedTop(286).reduced(8,4);auto& model=processor.getVisualizerModel();const bool held=model.spectralFrozen.load();g.setColour(held?teal:textSecondary);g.setFont(font(11));g.drawText(held?"HELD SPECTRUM":"LIVE / READY",r.removeFromTop(20),juce::Justification::centredLeft);auto graph=r.toFloat();g.setColour(inset);g.fillRoundedRectangle(graph,4);float w=graph.getWidth()/64;for(int b=0;b<64;++b){float h=std::clamp((20*std::log10(std::max(1e-6f,model.spectralEnergy[(size_t)b].load()))+80)/80,0.0f,1.0f)*graph.getHeight();g.setColour(teal);g.fillRect(graph.getX()+b*w,graph.getBottom()-h,std::max(1.0f,w-1),h);}}
}
