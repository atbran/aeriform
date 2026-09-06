#include "ContactPage.h"
#include "../DSP/CollisionRoute.h"
namespace aeriform {
using namespace theme;
ContactPage::ContactPage(AeriformProcessor& p):processor(p){controls=add<Controls>(p);stereoControls=add<StereoControls>(p);stereoControls->setVisible(false);addAndMakeVisible(switchView);switchView.onClick=[this]{showStereo(!stereoVisible);};startTimerHz(30);}
ContactPage::Controls::Controls(AeriformProcessor& p):ParamPanel(p,"CONTACT / COLLISION",copper){
    enabled=control<Toggle>(p,ids::contactOn,"ENABLED");source=control<ChoiceBox>(p,ids::contactSource,"Source");destination=control<ChoiceBox>(p,ids::contactDestination,"Destination");
    gap=knob(ids::contactGap,"Gap");stiffness=knob(ids::contactStiffness,"Stiffness");hardness=knob(ids::contactHardness,"Hardness");damping=knob(ids::contactDamping,"Damping");friction=knob(ids::contactFriction,"Friction / buzz");asymmetry=knob(ids::contactAsymmetry,"Asymmetry");amount=knob(ids::contactAmount,"Route amount");
    quality=control<ChoiceBox>(p,ids::contactQuality,"Oversampling");polarity=control<ChoiceBox>(p,ids::contactPolarity,"Polarity");
}
void ContactPage::Controls::resized(){auto r=getContentArea().reduced(12);enabled->setBounds(r.removeFromTop(28));r.removeFromTop(14);auto route=r.removeFromTop(46);source->setBounds(route.removeFromLeft(route.getWidth()/2).reduced(0,0));route.removeFromLeft(12);destination->setBounds(route);r.removeFromTop(28);
    knobRow(r.removeFromTop(108),{gap,stiffness,hardness},12);r.removeFromTop(24);knobRow(r.removeFromTop(108),{damping,friction,asymmetry},12);r.removeFromTop(24);
    auto bottom=r.removeFromTop(112);amount->setBounds(bottom.removeFromLeft(bottom.getWidth()/3));bottom.removeFromLeft(16);quality->setBounds(bottom.removeFromTop(46));bottom.removeFromTop(12);polarity->setBounds(bottom.removeFromTop(46));
}
void ContactPage::timerCallback(){
    const bool physical=processor.getAPVTS().getRawParameterValue(ids::stereoMode)->load()>.5f;
    for(auto* knob:{stereoControls->divergence,stereoControls->coupling,stereoControls->exciter,stereoControls->pickup,stereoControls->damping,stereoControls->rotation,stereoControls->width,stereoControls->bass})knob->setEnabled(physical);
    repaint();
}
void ContactPage::showStereo(bool on){stereoVisible=on;controls->setVisible(!on);stereoControls->setVisible(on);switchView.setButtonText(on?"EDIT CONTACT / COLLISION":"EDIT PHYSICAL STEREO");repaint();}
ContactPage::StereoControls::StereoControls(AeriformProcessor& p):ParamPanel(p,"TRUE STEREO NETWORK",teal){
    mode=control<ChoiceBox>(p,ids::stereoMode,"Network mode");divergence=knob(ids::stereoDivergence,"Length divergence");coupling=knob(ids::stereoCoupling,"Cross coupling");exciter=knob(ids::stereoExciterSpread,"Exciter spread");pickup=knob(ids::stereoPickupSpread,"Pickup spread");damping=knob(ids::stereoDamping,"Damping divergence");rotation=knob(ids::stereoRotation,"Rotation");width=knob(ids::stereoWidth,"Width");bass=knob(ids::stereoMonoBass,"Mono bass");
}
void ContactPage::StereoControls::resized(){auto r=getContentArea().reduced(12);mode->setBounds(r.removeFromTop(48));r.removeFromTop(38);knobRow(r.removeFromTop(110),{divergence,coupling,exciter},12);r.removeFromTop(34);knobRow(r.removeFromTop(110),{pickup,damping,rotation},12);r.removeFromTop(34);knobRow(r.removeFromTop(110),{width,bass},24);}
void ContactPage::resized(){auto left=getLocalBounds().removeFromLeft(getWidth()/2-6);controls->setBounds(left);stereoControls->setBounds(left);switchView.setBounds(getWidth()/2+26,getHeight()-56,getWidth()/2-52,32);}
void ContactPage::paint(juce::Graphics& g){auto r=getLocalBounds().withTrimmedLeft(getWidth()/2+6).reduced(20);g.setColour(textPrimary);g.setFont(titleFont(19));g.drawText(stereoVisible?"TWO PHYSICAL NETWORKS":"PREPARED RESONATORS",r.removeFromTop(40),juce::Justification::centredLeft);
    if(stereoVisible){
        auto& vis=processor.getVisualizerModel();float energy[2]={vis.stereoLeftEnergy.load(),vis.stereoRightEnergy.load()};
        auto field=r.removeFromTop(230);for(int i=0;i<2;++i){float cx=field.getX()+field.getWidth()*(i?.75f:.25f),cy=(float)field.getCentreY();auto colour=i?teal:copper;g.setColour(colour.withAlpha(.08f+std::min(.3f,energy[i])));g.fillEllipse(cx-80,cy-80,160,160);g.setColour(colour);g.drawEllipse(cx-52,cy-52,104,104,2);g.setColour(textPrimary);g.drawText(i?"RIGHT":"LEFT",(int)cx-50,(int)cy-18,100,36,juce::Justification::centred);g.setColour(colour);g.fillRect(cx-70,cy+90,std::min(140.0f,energy[i]*280),5.0f);}
        g.setColour(textSecondary);g.setFont(font(14));g.drawFittedText("Independent resonators carry each side. Length, pickup and damping differences change their physical response. Cross coupling exchanges a bounded amount of motion.",r.removeFromTop(110),juce::Justification::centredLeft,5,1.0f);
        r.removeFromTop(20);g.setColour(textPrimary);g.drawText("MONO COMPATIBILITY",r.removeFromTop(30),juce::Justification::centredLeft);g.setColour(textSecondary);g.setFont(font(13));g.drawFittedText("Width changes the side signal while preserving the mid signal. Mono bass converges low frequencies. At zero width the network channels match. Effects may add stereo afterward. Rotation changes the balance before bass convergence.",r.removeFromTop(110),juce::Justification::centredLeft,5,1.0f);
        r.removeFromTop(16);g.setColour(textDim);g.drawFittedText("Economy uses the original network and stops the second network after a short fade. Physical stereo adds resonator processing; it does not change the selected exciter oversampling or voice count.",r.removeFromTop(110),juce::Justification::centredLeft,5,1.0f);return;
    }
    auto value=[&](const char* id){return processor.getAPVTS().getRawParameterValue(id)->load();};int src=(int)value(ids::contactSource),dst=(int)value(ids::contactDestination);
    float activity=processor.getVisualizerModel().collisionActivity.load();auto nodes=r.removeFromTop(130);const float cy=(float)nodes.getCentreY(),spacing=nodes.getWidth()/3.0f;float x[3];
    for(int i=0;i<3;++i)x[i]=nodes.getX()+spacing*(i+.5f);
    g.setColour(copper.withAlpha(std::clamp(.2f+activity*10,0.0f,1.0f)));g.drawLine(x[std::clamp(src,0,2)],cy,x[std::clamp(dst,0,2)],cy,2+std::min(6.0f,activity*20));
    for(int i=0;i<3;++i){g.setColour(panel);g.fillEllipse(x[i]-28,cy-28,56,56);g.setColour(i==src?copperBright:i==dst?teal:textDim);g.drawEllipse(x[i]-28,cy-28,56,56,2);g.setFont(titleFont(18));g.drawText(juce::String::charToString((juce::juce_wchar)('A'+i)),(int)x[i]-20,(int)cy-18,40,36,juce::Justification::centred);}
    juce::String status;
    auto& model=processor.getVisualizerModel();
    if(value(ids::contactOn)<.5f)status="CONTACT OFF";
    else if(src==dst)status="INACTIVE: choose two different resonators";
    else if(!model.resonatorRunning[(size_t)src].load()||!model.resonatorRunning[(size_t)dst].load())status="WAITING: play a note with both route slots running";
    else if(value(ids::contactAmount)<.0001f||value(ids::contactStiffness)<.0001f)status="INACTIVE: raise Amount and Stiffness";
    else status=activity>.00005f?"CONTACT ACTIVE":"BELOW GAP: lower Gap or increase excitation";
    g.setColour(activity>.00005f?teal:textSecondary);g.setFont(font(12));g.drawFittedText(status,r.removeFromTop(36),juce::Justification::centredLeft,2);
    r.removeFromTop(5);g.setColour(textSecondary);g.setFont(font(12));g.drawFittedText("Contact adds a nonlinear stop at the pickup and a bounded physical reaction between two resonators. Both slots must be running in NETWORK.",r.removeFromTop(65),juce::Justification::centredLeft,3);r.removeFromTop(18);
    auto graph=r.removeFromTop(190).toFloat();g.setColour(inset);g.fillRoundedRectangle(graph,6);g.setColour(textDim.withAlpha(.5f));g.drawHorizontalLine((int)graph.getCentreY(),graph.getX()+10,graph.getRight()-10);g.drawVerticalLine((int)graph.getCentreX(),graph.getY()+10,graph.getBottom()-10);
    dsp::ContactParams p;p.gap=value(ids::contactGap);p.stiffness=value(ids::contactStiffness);p.hardness=value(ids::contactHardness);p.damping=value(ids::contactDamping);p.friction=value(ids::contactFriction);p.asymmetry=value(ids::contactAsymmetry);p.amount=value(ids::contactAmount);
    juce::Path path;for(int i=0;i<160;++i){float displacement=2*i/159.0f-1;float sa,sb;dsp::CollisionRoute::scatter(displacement,0,p,sa,sb);float force=p.amount*(2-p.amount)*(sa-displacement);float px=graph.getX()+10+(graph.getWidth()-20)*i/159.0f,py=graph.getCentreY()-force*graph.getHeight()*.24f;if(i==0)path.startNewSubPath(px,py);else path.lineTo(px,py);}g.setColour(copperBright);g.strokePath(path,juce::PathStrokeType(2));
    r.removeFromTop(12);g.setColour(textSecondary);g.setFont(font(11));g.drawFittedText("PICKUP CHANGE / SOURCE DISPLACEMENT\nThe curve shows the audible source correction. Route brightness follows the measured pickup change; physical injection remains loss-bounded.",r.removeFromTop(64),juce::Justification::centredLeft,4);
    g.setColour(textDim);g.drawFittedText("Gap has a squared response for finer control of quiet signals. Try friction for buzz, damping for a muted stop, or a wider gap for occasional impacts.",r.removeFromTop(80),juce::Justification::centredLeft,4);
}
}
