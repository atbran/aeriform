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
      midiLearn (apvts)
{
}

AeriformProcessor::~AeriformProcessor() = default;

// ---------------------------------------------------------------------------
void AeriformProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate, juce::jmax (1, samplesPerBlock));
    setLatencySamples (0);
}

void AeriformProcessor::releaseResources()
{
}

void AeriformProcessor::reset()
{
    engine.reset();
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
    engine.process (outBus, extPtr, midi, position, &midiLearn, isNonRealtime());

    for (int ch = outBus.getNumChannels(); ch < totalOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    // Cheap CPU measurement: block render time / block real-time duration.
    const double elapsed = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - startTicks);
    const double blockSeconds = (double) numSamples / juce::jmax (1.0, currentSampleRate);
    const float load = (float) (elapsed / juce::jmax (1.0e-9, blockSeconds));
    cpuLoad.store (0.9f * cpuLoad.load (std::memory_order_relaxed) + 0.1f * load, std::memory_order_relaxed);
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

    if (auto params = apvts.copyState().createXml())
        xml->addChildElement (params.release());
    xml->addChildElement (midiLearn.toXml().release());
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
    setEditorScale ((float) xml.getDoubleAttribute ("editorScale", 1.0));
    setEditorPage (xml.getIntAttribute ("editorPage", 0));
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
