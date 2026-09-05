#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "FactoryPresets.h"

namespace aeriform
{
/**
    Preset management: factory bank (compiled in), user presets (portable XML
    files), load / save / save-as / previous / next / init, dirty tracking and
    a version field for forward migration.

    All methods are intended for the message thread (or host callbacks). Parameter
    values are applied through the public parameter API so the host sees them.
*/
class PresetManager : private juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr const char* kFileExtension = ".aerpreset";
    static constexpr int kPresetFormatVersion = 1;

    struct Entry
    {
        juce::String name;
        juce::String category;
        bool isFactory = true;
        int factoryIndex = -1;
        juce::File file;
    };

    explicit PresetManager (juce::AudioProcessorValueTreeState& state);
    ~PresetManager() override;

    // ---- browsing -------------------------------------------------------
    const std::vector<Entry>& getEntries() const noexcept { return entries; }
    int getCurrentIndex() const noexcept { return currentIndex; }
    juce::String getCurrentName() const { return currentName; }
    juce::String getCurrentCategory() const { return currentCategory; }
    bool isDirty() const noexcept { return dirty; }
    void rescan();

    // ---- loading --------------------------------------------------------
    bool loadPreset (int index);
    bool loadNext();
    bool loadPrevious();
    void loadInit();
    bool loadFromFile (const juce::File& file);

    // ---- saving ---------------------------------------------------------
    /** Overwrites the current user preset, or falls back to saveAs when the current preset is a factory one. */
    bool saveCurrent();
    bool saveAs (const juce::String& name, const juce::String& category);
    bool saveToFile (const juce::File& file, const juce::String& name, const juce::String& category);
    bool deleteUserPreset (int index);

    // ---- serialisation shared with the processor state ------------------
    std::unique_ptr<juce::XmlElement> createPresetXml (const juce::String& name, const juce::String& category) const;
    /** Applies a preset XML element. Returns false if the XML is not a recognised preset. */
    bool applyPresetXml (const juce::XmlElement& xml);
    void applyFactoryPreset (const FactoryPreset& preset);

    void setCurrentName (const juce::String& name, const juce::String& category, bool markClean);

    static juce::File getUserPresetDirectory();

    /** Called on the message thread whenever the current preset (or dirty state) changes. */
    std::function<void()> onPresetChanged;

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Entry> entries;
    int currentIndex = 0;
    juce::String currentName { "Init" };
    juce::String currentCategory { "Init" };
    bool dirty = false;
    bool applying = false;

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void resetAllToDefaults();
    void setParamValue (const juce::String& id, float dspValue);
    void notify();
    static juce::String makeSafeFileName (const juce::String& name);
};
} // namespace aeriform
