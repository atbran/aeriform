#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr const char* kStateTag = "AeriformState";
}

AeriformProcessor::AeriformProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AeriformParams", aeriform::createParameterLayout()),
      engine (apvts, visualizer),
      presetManager (apvts),
      midiLearn (apvts),
      patchTools (*this),
      morphEngine(apvts,morphVisualizer)
{
    presetManager.toolsToXml=[this]{return patchTools.toXml();};
    presetManager.toolsFromXml=[this](const juce::XmlElement* xml){patchTools.fromXml(xml);};
}

AeriformProcessor::~AeriformProcessor() = default;

// ---------------------------------------------------------------------------
void AeriformProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate, juce::jmax (1, samplesPerBlock));
    preparedBlock=juce::jmax(1,samplesPerBlock);
    morphEngine.prepare(sampleRate,preparedBlock);morphOutput.prepare(sampleRate);patchTools.prepare(sampleRate);
    morphBuffer.setSize(2,preparedBlock);morphInput.setSize(2,preparedBlock);primeMidi.ensureSize(65536);
    heldNotes.fill(0);wasDeep=false;
    setLatencySamples (0);
}

void AeriformProcessor::releaseResources()
{
}

void AeriformProcessor::reset()
{
    engine.reset();morphEngine.reset();morphOutput.reset();heldNotes.fill(0);wasDeep=false;
}

bool AeriformProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;

    const auto in = layouts.getMainInputChannelSet();
    if (! in.isDisabled() && in != juce::AudioChannelSet::stereo() && in != juce::AudioChannelSet::mono())
        return false;

    return true;
}

void AeriformProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto startTicks = juce::Time::getHighResolutionTicks();

    const int numSamples = buffer.getNumSamples();
    const int totalIn = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();

    // View of the external input bus (no allocation: getBusBuffer aliases the host buffer).
    // The engine copies it internally before rendering, because hosts may process in place.
    auto inputBus = getBusBuffer (buffer, true, 0);
    const juce::AudioBuffer<float>* extPtr = nullptr;
    if (totalIn > 0 && getBus (true, 0) != nullptr && getBus (true, 0)->isEnabled() && inputBus.getNumChannels() > 0)
        extPtr = &inputBus;

    juce::AudioPlayHead::PositionInfo position;
    if (auto* ph = getPlayHead())
        if (auto info = ph->getPosition())
            position = *info;

    // Render into the output bus. The engine writes (does not accumulate) into
    // the output buffer, so the input copy is consumed by the engine first.
    auto outBus = getBusBuffer (buffer, false, 0);
    processMorph(outBus,extPtr,midi,position);
    for(const auto event:midi) {if(event.numBytes>3)continue;auto m=event.getMessage();
        if(m.isNoteOnOrOff())heldNotes[(size_t)((m.getChannel()-1)*128+m.getNoteNumber())]=m.isNoteOn()?m.getVelocity():0;
        else if(m.isAllNotesOff()||m.isAllSoundOff())heldNotes.fill(0);
    }

    for (int ch = outBus.getNumChannels(); ch < totalOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    // Cheap CPU measurement: block render time / block real-time duration.
    const double elapsed = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - startTicks);
    const double blockSeconds = (double) numSamples / juce::jmax (1.0, currentSampleRate);
    const float load = (float) (elapsed / juce::jmax (1.0e-9, blockSeconds));
    cpuLoad.store (0.9f * cpuLoad.load (std::memory_order_relaxed) + 0.1f * load, std::memory_order_relaxed);
}

void AeriformProcessor::processMorph(juce::AudioBuffer<float>& out,const juce::AudioBuffer<float>* input,juce::MidiBuffer& midi,const juce::AudioPlayHead::PositionInfo& position) {
    if(!patchTools.enabled()) {
        engine.setEffectiveValues(nullptr);engine.process(out,input,midi,position,&midiLearn,isNonRealtime());wasDeep=false;return;
    }
    const bool deep=patchTools.deep();
    if(deep&&!wasDeep) {
        morphEngine.reset();primeMidi.clear();
        for(int i=0;i<2048;++i)if(heldNotes[(size_t)i])primeMidi.addEvent(juce::MidiMessage::noteOn(i/128+1,i%128,(juce::uint8)heldNotes[(size_t)i]),0);
        // Existing held notes enter the newly activated engine before this block's events.
        if(!primeMidi.isEmpty()) {
            patchTools.evaluate(0,effectiveA,effectiveB);morphEngine.setEffectiveValues(&effectiveB,true);
            juce::AudioBuffer<float> silent(morphBuffer.getArrayOfWritePointers(),2,1);silent.clear();
            morphEngine.process(silent,nullptr,primeMidi,position,nullptr,isNonRealtime());
        }
    }
    for(int start=0;start<out.getNumSamples();start+=preparedBlock) {
        const int n=std::min(preparedBlock,out.getNumSamples()-start);
        const int inputChannels=input?std::min(2,input->getNumChannels()):0;
        for(int c=0;c<2;++c)if(inputChannels>0)morphInput.copyFrom(c,0,*input,std::min(c,inputChannels-1),start,n);else morphInput.clear(c,0,n);
        const float previous=patchTools.position();patchTools.evaluate(n,effectiveA,effectiveB);
        juce::AudioBuffer<float> a(out.getArrayOfWritePointers(),out.getNumChannels(),start,n);
        juce::AudioBuffer<float> b(morphBuffer.getArrayOfWritePointers(),out.getNumChannels(),n);
        juce::AudioBuffer<float> ext(morphInput.getArrayOfWritePointers(),inputChannels,n);
        engine.setEffectiveValues(&effectiveA,deep);
        engine.processRange(a,inputChannels?&ext:nullptr,midi,position,&midiLearn,isNonRealtime(),start);
        if(deep) {
            morphEngine.setEffectiveValues(&effectiveB,true);
            morphEngine.processRange(b,inputChannels?&ext:nullptr,midi,position,nullptr,isNonRealtime(),start);
            for(int i=0;i<n;++i) {
                float t=previous+(patchTools.position()-previous)*(float)(i+1)/(float)n;
                const float ga=std::cos(t*juce::MathConstants<float>::halfPi),gb=std::sin(t*juce::MathConstants<float>::halfPi);
                for(int c=0;c<a.getNumChannels();++c)a.setSample(c,i,ga*a.getSample(c,i)+gb*b.getSample(c,i));
            }
            if(a.getNumChannels()==1)morphBuffer.copyFrom(1,0,a,0,0,n);
            morphOutput.setParams(effectiveA[(size_t)aeriform::P::outGain],effectiveA[(size_t)aeriform::P::outHighpass],effectiveA[(size_t)aeriform::P::limiterOn]>0.5f);
            morphOutput.process(a.getWritePointer(0),a.getNumChannels()>1?a.getWritePointer(1):morphBuffer.getWritePointer(1),n);
            const auto& m=morphOutput.getMeter();visualizer.limiterGain.store(morphOutput.getLimiterGain());
            visualizer.preLimiterPeak.store(m.prePeak);visualizer.preLimiterRms.store(m.preRms);visualizer.postLimiterRms.store(m.postRms);
            visualizer.limiterFraction.store(m.limitedFraction);visualizer.ceilingFraction.store(m.ceilingFraction);
        }
    }
    engine.setEffectiveValues(nullptr);morphEngine.setEffectiveValues(nullptr);wasDeep=deep;
}

void AeriformProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    // A bypassed synth outputs silence; release every held note so nothing hangs when un-bypassed.
    juce::ignoreUnused (midi);
    engine.allNotesOff();
    buffer.clear();
}

// ---------------------------------------------------------------------------
std::unique_ptr<juce::XmlElement> AeriformProcessor::createStateXml()
{
    auto xml = std::make_unique<juce::XmlElement> (kStateTag);
    xml->setAttribute ("version", aeriform::kStateVersion);
    xml->setAttribute ("pluginVersion", AERIFORM_VERSION_STRING);
    xml->setAttribute ("presetName", presetManager.getCurrentName());
    xml->setAttribute ("presetCategory", presetManager.getCurrentCategory());
    xml->setAttribute ("presetDirty", presetManager.isDirty());
    xml->setAttribute ("editorScale", (double) editorScale.load());
    xml->setAttribute ("editorPage", editorPage.load());
    for(int i=0;i<6;++i)xml->setAttribute("editorSection"+juce::String(i),getEditorSection(i));

    if (auto params = apvts.copyState().createXml())
        xml->addChildElement (params.release());
    xml->addChildElement (midiLearn.toXml().release());
    xml->addChildElement (patchTools.toXml().release());
    return xml;
}

void AeriformProcessor::applyStateXml (const juce::XmlElement& xml)
{
    if (! xml.hasTagName (kStateTag))
        return;

    const int version = xml.getIntAttribute ("version", 1);
    juce::ignoreUnused (version);   // future migrations branch on this

    if (auto* params = xml.getChildByName (apvts.state.getType()))
    {
        // replaceState keeps parameters that are missing from the saved tree at their current
        // value; reset to defaults first so older states restore deterministically.
        for (auto* p : getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                rp->setValueNotifyingHost (rp->getDefaultValue());
        apvts.replaceState (juce::ValueTree::fromXml (*params));
    }

    midiLearn.fromXml (xml.getChildByName ("MidiLearn"));
    patchTools.fromXml(xml.getChildByName("PatchTools"));
    setEditorScale ((float) xml.getDoubleAttribute ("editorScale", 1.0));
    setEditorPage (xml.getIntAttribute ("editorPage", 0));
    for(int i=0;i<6;++i)setEditorSection(i,xml.getIntAttribute("editorSection"+juce::String(i),0));
    presetManager.setCurrentName (xml.getStringAttribute ("presetName", "Init"),
                                  xml.getStringAttribute ("presetCategory", "Init"),
                                  ! xml.getBoolAttribute ("presetDirty", false));
}

void AeriformProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    copyXmlToBinary (*createStateXml(), destData);
}

void AeriformProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0) return;
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        applyStateXml (*xml);
}

// ---------------------------------------------------------------------------
juce::AudioProcessorEditor* AeriformProcessor::createEditor()
{
    return new AeriformEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AeriformProcessor();
}
