#include "PresetManager.h"
#include "../Params/ParameterLayout.h"

namespace aeriform
{
PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            apvts.addParameterListener (rp->paramID, this);
    rescan();
}

PresetManager::~PresetManager()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            apvts.removeParameterListener (rp->paramID, this);
}

juce::File PresetManager::getUserPresetDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Aeriform").getChildFile ("Presets");
}

void PresetManager::rescan()
{
    const juce::String previousName = currentName;
    entries.clear();

    const auto& factory = factoryPresets();
    for (int i = 0; i < (int) factory.size(); ++i)
    {
        Entry e;
        e.name = factory[(size_t) i].name;
        e.category = factory[(size_t) i].category;
        e.isFactory = true;
        e.factoryIndex = i;
        entries.push_back (e);
    }

    auto dir = getUserPresetDirectory();
    if (dir.isDirectory())
    {
        juce::Array<juce::File> files;
        dir.findChildFiles (files, juce::File::findFiles, true, "*" + juce::String (kFileExtension));
        files.sort();
        for (const auto& f : files)
        {
            Entry e;
            e.name = f.getFileNameWithoutExtension();
            e.category = "User";
            e.isFactory = false;
            e.file = f;
            if (auto xml = juce::XmlDocument::parse (f))
            {
                if (xml->hasTagName ("AeriformPreset"))
                {
                    if (xml->hasAttribute ("name")) e.name = xml->getStringAttribute ("name");
                    if (xml->hasAttribute ("category")) e.category = xml->getStringAttribute ("category");
                }
            }
            entries.push_back (e);
        }
    }

    currentIndex = -1;
    for (int i = 0; i < (int) entries.size(); ++i)
        if (entries[(size_t) i].name == previousName) { currentIndex = i; break; }
    if (currentIndex < 0) currentIndex = 0;
}

// ---------------------------------------------------------------------------
void PresetManager::parameterChanged (const juce::String&, float)
{
    if (applying) return;
    if (! dirty.exchange (true, std::memory_order_relaxed))
        pendingNotify.store (true, std::memory_order_release);
}

void PresetManager::notify()
{
    pendingNotify.store (true, std::memory_order_release);
}

void PresetManager::setParamValue (const juce::String& id, float dspValue)
{
    if (auto* p = apvts.getParameter (id))
    {
        const float norm = p->convertTo0to1 (juce::jlimit (p->getNormalisableRange().start, p->getNormalisableRange().end, dspValue));
        p->setValueNotifyingHost (norm);
    }
}

void PresetManager::resetAllToDefaults()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
}

void PresetManager::applyFactoryPreset (const FactoryPreset& preset)
{
    applying = true;
    resetAllToDefaults();
    for (const auto& [id, value] : preset.values)
        setParamValue (id, value);
    applying = false;
}

// ---------------------------------------------------------------------------
std::unique_ptr<juce::XmlElement> PresetManager::createPresetXml (const juce::String& name, const juce::String& category) const
{
    auto xml = std::make_unique<juce::XmlElement> ("AeriformPreset");
    xml->setAttribute ("version", kPresetFormatVersion);
    xml->setAttribute ("name", name);
    xml->setAttribute ("category", category);
    xml->setAttribute ("plugin", "AERIFORM");
    xml->setAttribute ("pluginVersion", AERIFORM_VERSION_STRING);

    auto* params = xml->createNewChildElement ("Parameters");
    for (auto* p : apvts.processor.getParameters())
    {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            auto* e = params->createNewChildElement ("Param");
            e->setAttribute ("id", rp->paramID);
            e->setAttribute ("value", (double) rp->convertFrom0to1 (rp->getValue()));
        }
    }
    return xml;
}

bool PresetManager::applyPresetXml (const juce::XmlElement& xml)
{
    if (! xml.hasTagName ("AeriformPreset")) return false;
    const int version = xml.getIntAttribute ("version", 1);
    juce::ignoreUnused (version); // future migrations switch on this value

    applying = true;
    resetAllToDefaults();
    if (auto* params = xml.getChildByName ("Parameters"))
    {
        for (auto* e : params->getChildWithTagNameIterator ("Param"))
        {
            const auto id = e->getStringAttribute ("id");
            if (! e->hasAttribute ("value") || apvts.getParameter (id) == nullptr) continue;   // unknown IDs are ignored
            setParamValue (id, (float) e->getDoubleAttribute ("value"));
        }
    }
    applying = false;
    return true;
}

void PresetManager::setCurrentName (const juce::String& name, const juce::String& category, bool markClean)
{
    currentName = name;
    currentCategory = category;
    if (markClean) dirty = false;
    currentIndex = -1;
    for (int i = 0; i < (int) entries.size(); ++i)
        if (entries[(size_t) i].name == name) { currentIndex = i; break; }
    if (currentIndex < 0) currentIndex = 0;
    notify();
}

// ---------------------------------------------------------------------------
bool PresetManager::loadPreset (int index)
{
    if (index < 0 || index >= (int) entries.size()) return false;
    const auto& e = entries[(size_t) index];
    bool ok = false;
    if (e.isFactory)
    {
        applyFactoryPreset (factoryPresets()[(size_t) e.factoryIndex]);
        ok = true;
    }
    else if (auto xml = juce::XmlDocument::parse (e.file))
    {
        ok = applyPresetXml (*xml);
    }
    if (! ok) return false;
    currentIndex = index;
    currentName = e.name;
    currentCategory = e.category;
    dirty = false;
    notify();
    return true;
}

bool PresetManager::loadNext()
{
    if (entries.empty()) return false;
    return loadPreset ((currentIndex + 1) % (int) entries.size());
}

bool PresetManager::loadPrevious()
{
    if (entries.empty()) return false;
    return loadPreset ((currentIndex - 1 + (int) entries.size()) % (int) entries.size());
}

void PresetManager::loadInit()
{
    applying = true;
    resetAllToDefaults();
    applying = false;
    setCurrentName ("Init", "Init", true);
}

bool PresetManager::loadFromFile (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! applyPresetXml (*xml)) return false;
    rescan();
    setCurrentName (xml->getStringAttribute ("name", file.getFileNameWithoutExtension()),
                    xml->getStringAttribute ("category", "User"), true);
    return true;
}

// ---------------------------------------------------------------------------
juce::String PresetManager::makeSafeFileName (const juce::String& name)
{
    auto s = juce::File::createLegalFileName (name).trim();
    return s.isEmpty() ? juce::String ("Untitled") : s;
}

bool PresetManager::saveToFile (const juce::File& file, const juce::String& name, const juce::String& category)
{
    auto xml = createPresetXml (name, category);
    file.getParentDirectory().createDirectory();
    return xml->writeTo (file, {});
}

bool PresetManager::saveAs (const juce::String& name, const juce::String& category)
{
    const auto file = getUserPresetDirectory().getChildFile (makeSafeFileName (name) + kFileExtension);
    if (! saveToFile (file, name, category.isEmpty() ? "User" : category)) return false;
    rescan();
    setCurrentName (name, category.isEmpty() ? "User" : category, true);
    return true;
}

bool PresetManager::saveCurrent()
{
    if (currentIndex >= 0 && currentIndex < (int) entries.size() && ! entries[(size_t) currentIndex].isFactory)
    {
        const auto& e = entries[(size_t) currentIndex];
        if (! saveToFile (e.file, e.name, e.category)) return false;
        dirty = false;
        notify();
        return true;
    }
    return saveAs (currentName, "User");
}

bool PresetManager::deleteUserPreset (int index)
{
    if (index < 0 || index >= (int) entries.size() || entries[(size_t) index].isFactory) return false;
    if (! entries[(size_t) index].file.deleteFile()) return false;
    rescan();
    notify();
    return true;
}
} // namespace aeriform
