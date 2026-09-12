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
void ContactPage::Controls::resized()
{
    auto r=getContentArea().reduced(4,2);
    enabled->setBounds(r.removeFromTop(26));
    auto route=r.removeFromTop(44);
    source->setBounds(route.removeFromLeft((route.getWidth()-8)/2)); route.removeFromLeft(8); destination->setBounds(route);
    r.removeFromTop(8);
    knobRow(r.removeFromTop(82),{gap,stiffness,hardness},6);
    knobRow(r.removeFromTop(82),{damping,friction,asymmetry},6);
    auto bottom=r.removeFromTop(96);
    amount->setBounds(bottom.removeFromLeft(100)); bottom.removeFromLeft(8);
    quality->setBounds(bottom.removeFromTop(42)); bottom.removeFromTop(8); polarity->setBounds(bottom.removeFromTop(42));
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
void ContactPage::StereoControls::resized()
{
    auto r=getContentArea().reduced(20,12);
    mode->setBounds(r.removeFromTop(44).withWidth(280)); r.removeFromTop(20);
    knobRow(r.removeFromTop(100),{divergence,coupling,exciter,pickup},20);
    knobRow(r.removeFromTop(100),{damping,rotation,width,bass},20);
}
void ContactPage::resized()
{
    controls->setBounds(getLocalBounds().withHeight(432));
    stereoControls->setBounds(getLocalBounds());
    switchView.setVisible(false);
}
void ContactPage::paint(juce::Graphics& g)
{
    if(stereoVisible)return;
    auto r=getLocalBounds().withTrimmedTop(444).reduced(12,8);
    if(r.getHeight()<40)return;
    auto value=[&](const char* id){return processor.getAPVTS().getRawParameterValue(id)->load();};
    const int src=std::clamp((int)value(ids::contactSource),0,2),dst=std::clamp((int)value(ids::contactDestination),0,2);
    const auto& model=processor.getVisualizerModel();
    juce::String status;
    if(value(ids::contactOn)<.5f)status="CONTACT OFF";
    else if(src==dst)status="Choose different source and destination";
    else if(!model.resonatorRunning[(size_t)src].load()||!model.resonatorRunning[(size_t)dst].load())status="Waiting for both resonators";
    else if(value(ids::contactAmount)<.0001f||value(ids::contactStiffness)<.0001f)status="Raise Amount and Stiffness";
    else status=model.collisionActivity.load()>.00005f?"CONTACT ACTIVE":"Below contact gap";
    g.setColour(textSecondary);g.setFont(font(11));g.drawText(status,r.removeFromTop(26),juce::Justification::centredLeft);
    auto graph=r.removeFromTop(160).toFloat();g.setColour(inset);g.fillRoundedRectangle(graph,6);
    g.setColour(grid);g.drawHorizontalLine((int)graph.getCentreY(),graph.getX()+10,graph.getRight()-10);
    dsp::ContactParams p;p.gap=value(ids::contactGap);p.stiffness=value(ids::contactStiffness);p.hardness=value(ids::contactHardness);p.damping=value(ids::contactDamping);p.friction=value(ids::contactFriction);p.asymmetry=value(ids::contactAsymmetry);p.amount=value(ids::contactAmount);
    juce::Path path;
    for(int i=0;i<160;++i){float displacement=2*i/159.0f-1,sa,sb;dsp::CollisionRoute::scatter(displacement,0,p,sa,sb);float force=p.amount*(2-p.amount)*(sa-displacement);float x=graph.getX()+10+(graph.getWidth()-20)*i/159.0f,y=graph.getCentreY()-force*graph.getHeight()*.24f;if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
    g.setColour(copperBright);g.strokePath(path,juce::PathStrokeType(1.5f));
    g.setColour(textDim);g.drawText("CONTACT RESPONSE",r.removeFromTop(24),juce::Justification::centredLeft);
}
}
