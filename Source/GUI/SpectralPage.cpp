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
void SpectralPage::Controls::resized(){auto r=getContentArea().reduced(14,10);auto row=r.removeFromTop(28);enabled->setBounds(row.removeFromLeft(row.getWidth()/2));hold->setBounds(row);r.removeFromTop(18);row=r.removeFromTop(36);capture.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(0,0));row.removeFromLeft(10);release.setBounds(row);r.removeFromTop(46);knobRow(r.removeFromTop(116),{blur,shift,random},12);r.removeFromTop(32);knobRow(r.removeFromTop(116),{decay,mix},32);}
void SpectralPage::resized(){controls->setBounds(0,0,getWidth()/2-6,getHeight());}
void SpectralPage::paint(juce::Graphics& g){auto r=getLocalBounds().withTrimmedLeft(getWidth()/2+24).reduced(10,16);auto& model=processor.getVisualizerModel();const bool on=processor.getAPVTS().getRawParameterValue(ids::sfOn)->load()>.5f;const bool held=model.spectralFrozen.load();
    g.setColour(textPrimary);g.setFont(titleFont(19));g.drawText("A MOMENT, SUSPENDED",r.removeFromTop(40),juce::Justification::centredLeft);
    g.setColour(held?teal:textSecondary);g.setFont(font(13));g.drawText(!on?"OFF":held?"HELD SPECTRUM":"LIVE / READY TO CAPTURE",r.removeFromTop(30),juce::Justification::centredLeft);
    auto graph=r.removeFromTop(190).toFloat().reduced(0,8);g.setColour(inset);g.fillRoundedRectangle(graph,6);const float width=(graph.getWidth()-20)/64;
    for(int b=0;b<64;++b){const float energy=model.spectralEnergy[(size_t)b].load();const float height=std::clamp((20*std::log10(std::max(1e-6f,energy))+80)/80,0.0f,1.0f)*(graph.getHeight()-20);g.setColour((held?teal:copper).withAlpha(on?.8f:.25f));g.fillRect(graph.getX()+10+b*width,graph.getBottom()-10-height,std::max(1.0f,width-1),height);}
    r.removeFromTop(14);g.setFont(font(14));g.setColour(textSecondary);g.drawFittedText("Play a sound, then Capture. The held spectrum continues after you release the keys. Capture again to replace it; Release returns to the live instrument.",r.removeFromTop(100),juce::Justification::centredLeft,5);
    r.removeFromTop(12);g.drawFittedText("Blur spreads the partials. Shift moves them up or down two octaves. Random phase adds movement; Decay sets how long the captured texture lasts. A zero decay holds indefinitely.",r.removeFromTop(115),juce::Justification::centredLeft,6);
    r.removeFromTop(12);g.setColour(textDim);g.setFont(font(12));g.drawFittedText("Capture needs a short frame of audio. The bars show the captured spectrum. The audio capture is temporary: saving a patch stores the controls, and reloading captures fresh audio when Hold is on.",r.removeFromTop(95),juce::Justification::centredLeft,5);
}
}
