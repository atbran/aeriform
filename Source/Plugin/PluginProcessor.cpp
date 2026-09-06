#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr const char* kStateTag = "AeriformState";
}

// Aeriform FX is a conventional insert effect: a real main stereo input and output,
// plus the optional aux "Sidechain" input (disabled by default so a host never forces
// a routing choice for it). The instrument build has only the aux sidechain input.
auto AeriformProcessor::makeBusesProperties() -> BusesProperties
{
   #if AERIFORM_FX
    return BusesProperties()
        .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false);
   #else
    return BusesProperties()
        .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), true);
   #endif
}

AeriformProcessor::AeriformProcessor()
    : AudioProcessor (makeBusesProperties()),
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

   #if AERIFORM_FX
    // Effect: the main input is expected but a host may still probe it disabled.
    // The optional second (Sidechain) bus may be disabled, mono or stereo.
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::stereo() && sc != juce::AudioChannelSet::mono())
            return false;
    }
   #endif

    return true;
}

void AeriformProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto startTicks = juce::Time::getHighResolutionTicks();

    const int numSamples = buffer.getNumSamples();
    const int totalOut = getTotalNumOutputChannels();

    // Views of the input buses (no allocation: getBusBuffer aliases the host buffer).
    // The engine copies every input internally before it writes any output, so
    // in-place hosts (main input == main output memory) are safe.
    auto busUsable = [this] (int index) -> bool
    {
        auto* b = getBus (true, index);
        return b != nullptr && b->isEnabled() && getChannelCountOfBus (true, index) > 0;
    };

   #if AERIFORM_FX
    auto mainInBus = getBusBuffer (buffer, true, 0);
    juce::AudioBuffer<float> scInBus = getBusCount (true) > 1 ? getBusBuffer (buffer, true, 1)
                                                             : juce::AudioBuffer<float>();
    const juce::AudioBuffer<float>* mainPtr = busUsable (0) ? &mainInBus : nullptr;
    const juce::AudioBuffer<float>* scPtr   = (getBusCount (true) > 1 && busUsable (1)) ? &scInBus : nullptr;
   #else
    auto scInBus = getBusBuffer (buffer, true, 0);
    const juce::AudioBuffer<float>* mainPtr = nullptr;
    const juce::AudioBuffer<float>* scPtr   = busUsable (0) ? &scInBus : nullptr;
   #endif

    juce::AudioPlayHead::PositionInfo position;
    if (auto* ph = getPlayHead())
        if (auto info = ph->getPosition())
            position = *info;

    // Render into the output bus. The engine writes (does not accumulate) into
    // the output buffer, so the input copies are consumed by the engine first.
    auto outBus = getBusBuffer (buffer, false, 0);
    engine.process (outBus, mainPtr, scPtr, midi, position, &midiLearn, isNonRealtime());

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
    juce::ignoreUnused (midi);
    engine.allNotesOff();

   #if AERIFORM_FX
    // A bypassed effect passes its main input straight through. With in-place hosts the
    // main input and output already share memory, so only copy when they differ.
    auto in  = getBusBuffer (buffer, true, 0);
    auto out = getBusBuffer (buffer, false, 0);
    const int n = buffer.getNumSamples();
    for (int ch = 0; ch < out.getNumChannels(); ++ch)
    {
        if (ch >= in.getNumChannels())
            out.clear (ch, 0, n);
        else if (out.getReadPointer (ch) != in.getReadPointer (ch))
            out.copyFrom (ch, 0, in, ch, 0, n);
    }
   #else
    // A bypassed synth outputs silence; the note release above stops anything hanging.
    buffer.clear();
   #endif
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
