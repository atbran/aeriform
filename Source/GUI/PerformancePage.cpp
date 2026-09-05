#include "PerformancePage.h"
namespace aeriform {
using namespace theme;
PerformancePage::PerformancePage(AeriformProcessor& p){tools=add<ToolsPanel>(p);}
void PerformancePage::resized(){tools->setBounds(getLocalBounds());}
PerformancePage::ToolsPanel::ToolsPanel(AeriformProcessor& p):ParamPanel(p,"PLAY / SNAPSHOT LAB",copper) {
    for(int i=0;i<2;++i) {
        capture[(size_t)i].setButtonText("CAPTURE");load[(size_t)i].setButtonText("LOAD PRESET");editEndpoint[(size_t)i].setButtonText(i==0?"EDIT A":"EDIT B");
        for(auto* c:std::initializer_list<juce::Component*>{&capture[(size_t)i],&load[(size_t)i],&editEndpoint[(size_t)i],&endpointName[(size_t)i]})addAndMakeVisible(c);
        endpointName[(size_t)i].setFont(titleFont(20));endpointName[(size_t)i].setColour(juce::Label::textColourId,i==0?copperBright:teal);
        capture[(size_t)i].onClick=[this,i]{processor.getPatchTools().capture(i);};
        load[(size_t)i].onClick=[this,i]{choosePreset(i);};editEndpoint[(size_t)i].onClick=[this,i]{processor.getPatchTools().selectEndpoint(i);};
    }
    enabled=control<Toggle>(p,ids::morphOn,"A/B MORPH");engine=control<ChoiceBox>(p,ids::morphMode,"Engine");morph=control<HSlider>(p,ids::morphPosition,true);
    wild=control<Toggle>(p,ids::randomWild,"WILD");mutation=knob(ids::randomMutation,"Mutation",{},72);
    repipe=knob(ids::netRepipe,"Repipe",additive(ModDest::Repipe),72);coupling=knob(ids::artCoupling,"Coupling",{},72);feedback=knob(ids::netFeedback,"Network feedback",additive(ModDest::NetFeedback),72);
    folder=knob(ids::wfFold,"Fold",additive(ModDest::Fold),72);brightness=knob(ids::resBrightness,"Brightness",additive(ModDest::Brightness),72);width=knob(ids::netWidth,"Width",additive(ModDest::NetWidth),72);room=knob(ids::reverbMix,"Reverb",additive(ModDest::ReverbMix),72);
    for(auto* c:std::initializer_list<juce::Component*>{&randomize,&mutate,&newSeed,&lockAll,&unlockAll,&lockSection,&unlockSection,&commit,&seed,&scope,&section,&help})addAndMakeVisible(c);
    seed.setInputRestrictions(10,"0123456789");seed.setTooltip("32-bit seed. Repeat from the same starting patch for identical mutations.");seed.setText(juce::String((juce::int64)p.getPatchTools().getSeed()),false);
    seed.onFocusLost=[this]{processor.getPatchTools().setSeed((uint32_t)seed.getText().getLargeIntValue());};seed.onReturnKey=seed.onFocusLost;
    scope.addItemList({"All unlocked","Exciters only","Network only","Modulation only","Effects only"},1);scope.setSelectedId(1);
    section.addItemList({"Breath","Exciters","Shaping","Resonator A","Network","Modulation","Effects","Master"},1);section.setSelectedId(1);
    randomize.onClick=[this]{processor.getPatchTools().randomize((PatchStateManager::Scope)(scope.getSelectedId()-1),1,false);};
    mutate.onClick=[this]{processor.getPatchTools().randomize((PatchStateManager::Scope)(scope.getSelectedId()-1),processor.getAPVTS().getRawParameterValue(ids::randomMutation)->load(),true);};
    newSeed.onClick=[this]{processor.getPatchTools().newSeed();seed.setText(juce::String((juce::int64)processor.getPatchTools().getSeed()),false);};
    lockAll.onClick=[this]{processor.getPatchTools().lockAll(true);};unlockAll.onClick=[this]{processor.getPatchTools().lockAll(false);};
    lockSection.onClick=[this]{processor.getPatchTools().lockSection((ParamSection)(section.getSelectedId()-1),true);};unlockSection.onClick=[this]{processor.getPatchTools().lockSection((ParamSection)(section.getSelectedId()-1),false);};
    commit.onClick=[this]{processor.getPatchTools().commitMorph();};
    commit.setTooltip("Commit Parameter Morph, or either Deep Morph endpoint. An interior Deep blend contains two physical structures and cannot be flattened into one patch.");
    help.setFont(font(12));help.setColour(juce::Label::textColourId,textSecondary);
    help.setText("Controls edit the selected endpoint. Right-click any knob to lock it. Parameter mode holds structure; Deep renders both structures.",juce::dontSendNotification);
    startTimerHz(15);timerCallback();
}
void PerformancePage::ToolsPanel::choosePreset(int slot) {
    juce::PopupMenu menu;auto& pm=processor.getPresetManager();pm.rescan();int i=0;for(const auto& e:pm.getEntries())menu.addItem(++i,e.name);
    juce::Component::SafePointer<ToolsPanel> safe(this);menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&load[(size_t)slot]),[safe,slot](int id){if(safe&&id>0)safe->processor.getPatchTools().loadSnapshot(slot,id-1);});
}
void PerformancePage::ToolsPanel::timerCallback() {
    auto& s=processor.getPatchTools();for(int i=0;i<2;++i){endpointName[(size_t)i].setText(juce::String(i==0?"A  /  ":"B  /  ")+s.snapshotName(i),juce::dontSendNotification);editEndpoint[(size_t)i].setToggleState(s.selectedEndpoint()==i,juce::dontSendNotification);}
    if(!seed.hasKeyboardFocus(false))seed.setText(juce::String((juce::int64)s.getSeed()),false);
    const float t=processor.getAPVTS().getRawParameterValue(ids::morphPosition)->load();commit.setEnabled(!s.deep()||t<0.001f||t>0.999f);repaint();
}
void PerformancePage::ToolsPanel::resized() {
    auto r=getContentArea().reduced(16,8);auto ends=r.removeFromTop(100);const int gap=40,w=(ends.getWidth()-gap)/2;
    for(int i=0;i<2;++i){auto box=ends.removeFromLeft(w);endpointName[(size_t)i].setBounds(box.removeFromTop(44));auto buttons=box.removeFromTop(30);capture[(size_t)i].setBounds(buttons.removeFromLeft(100));buttons.removeFromLeft(8);load[(size_t)i].setBounds(buttons.removeFromLeft(130));buttons.removeFromLeft(8);editEndpoint[(size_t)i].setBounds(buttons.removeFromLeft(100));ends.removeFromLeft(gap);}
    auto controls=r.removeFromTop(44);enabled->setBounds(controls.removeFromLeft(150));engine->setBounds(controls.removeFromLeft(180));commit.setBounds(controls.removeFromRight(100).withSizeKeepingCentre(100,28));morph->setBounds(controls.reduced(28,4));
    r.removeFromTop(112);help.setBounds(r.removeFromTop(26));r.removeFromTop(14);
    knobRow(r.removeFromTop(106),{repipe,coupling,feedback,folder,brightness,width,room,mutation},12);r.removeFromTop(24);
    auto random=r.removeFromTop(34);seed.setBounds(random.removeFromLeft(140));random.removeFromLeft(8);newSeed.setBounds(random.removeFromLeft(105));random.removeFromLeft(20);scope.setBounds(random.removeFromLeft(180));random.removeFromLeft(8);randomize.setBounds(random.removeFromLeft(140));random.removeFromLeft(8);mutate.setBounds(random.removeFromLeft(110));wild->setBounds(random.reduced(10,0));r.removeFromTop(14);
    auto locks=r.removeFromTop(32);lockAll.setBounds(locks.removeFromLeft(116));locks.removeFromLeft(8);unlockAll.setBounds(locks.removeFromLeft(116));locks.removeFromLeft(32);section.setBounds(locks.removeFromLeft(180));locks.removeFromLeft(8);lockSection.setBounds(locks.removeFromLeft(140));locks.removeFromLeft(8);unlockSection.setBounds(locks.removeFromLeft(150));
}
void PerformancePage::ToolsPanel::paint(juce::Graphics& g) {
    ParamPanel::paint(g);auto r=getContentArea().reduced(16,8);auto band=r.withTrimmedTop(152).withHeight(96).toFloat();
    g.setColour(inset);g.fillRoundedRectangle(band,12);
    const float t=processor.getAPVTS().getRawParameterValue(ids::morphPosition)->load();
    const float x=band.getX()+30+t*(band.getWidth()-60),y=band.getCentreY();
    g.setColour(copper.withAlpha(0.25f));g.drawLine(band.getX()+30,y,band.getRight()-30,y,2);
    auto& vis=processor.getVisualizerModel();const float energy=std::clamp(vis.masterEnergy.load(),0.0f,1.0f);
    for(int i=0;i<48;++i){float u=(float)i/47;float h=8+energy*34*(0.5f+0.5f*std::sin(i*.72f));g.setColour(copper.interpolatedWith(teal,u).withAlpha(0.2f+0.55f*std::exp(-12*std::abs(u-t))));g.fillRoundedRectangle(band.getX()+30+u*(band.getWidth()-60),y-h/2,3,h,1);}
    g.setColour(copperBright.interpolatedWith(teal,t));g.fillEllipse(x-9,y-9,18,18);g.drawEllipse(x-15,y-15,30,30,1);
    g.setColour(textSecondary);g.setFont(monoFont(11));g.drawText(juce::String(t*100,1)+"%  /  "+(processor.getPatchTools().deep()?"DUAL ENGINE":"PARAMETER"),band.toNearestInt().reduced(18).removeFromBottom(16),juce::Justification::centredRight);
}
}
