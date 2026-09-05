#include "MidiLearn.h"

namespace aeriform
{
MidiLearn::MidiLearn (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            params.push_back (rp);

    for (auto& a : ccToParam)
        a.store (kUnmapped);
}

int MidiLearn::indexOf (const juce::String& paramID) const
{
    for (size_t i = 0; i < params.size(); ++i)
        if (params[i]->paramID == paramID)
            return (int) i;
    return kUnmapped;
}

void MidiLearn::armLearn (const juce::String& paramID)
{
    const int idx = indexOf (paramID);
    if (idx == kUnmapped) return;
    learnCapturedCC.store (kUnmapped);
    learnParamIndex.store (idx);
    learnArmed.store (1);
}

void MidiLearn::cancelLearn()
{
    learnArmed.store (0);
    learnParamIndex.store (kUnmapped);
    learnCapturedCC.store (kUnmapped);
}

juce::String MidiLearn::getLearningParam() const
{
    const int idx = learnParamIndex.load();
    return (learnArmed.load() != 0 && idx >= 0 && idx < (int) params.size()) ? params[(size_t) idx]->paramID : juce::String();
}

bool MidiLearn::pollLearn()
{
    if (learnArmed.load() == 0) return false;
    const int cc = learnCapturedCC.load();
    if (cc == kUnmapped) return false;

    const int idx = learnParamIndex.load();
    // Remove any previous mapping of this parameter and any previous use of this CC.
    for (auto& a : ccToParam)
        if (a.load() == idx) a.store (kUnmapped);
    if (cc >= 0 && cc < kNumCCs)
        ccToParam[(size_t) cc].store (idx);
    cancelLearn();
    return true;
}

void MidiLearn::clearMapping (const juce::String& paramID)
{
    const int idx = indexOf (paramID);
    for (auto& a : ccToParam)
        if (a.load() == idx) a.store (kUnmapped);
}

void MidiLearn::clearAll()
{
    for (auto& a : ccToParam) a.store (kUnmapped);
}

int MidiLearn::getMappedCC (const juce::String& paramID) const
{
    const int idx = indexOf (paramID);
    if (idx == kUnmapped) return kUnmapped;
    for (int cc = 0; cc < kNumCCs; ++cc)
        if (ccToParam[(size_t) cc].load() == idx) return cc;
    return kUnmapped;
}

juce::String MidiLearn::getMappedParam (int cc) const
{
    if (cc < 0 || cc >= kNumCCs) return {};
    const int idx = ccToParam[(size_t) cc].load();
    return (idx >= 0 && idx < (int) params.size()) ? params[(size_t) idx]->paramID : juce::String();
}

bool MidiLearn::handleController (int cc, int value) noexcept
{
    if (cc < 0 || cc >= kNumCCs) return false;

    if (learnArmed.load (std::memory_order_relaxed) != 0)
    {
        learnCapturedCC.store (cc, std::memory_order_relaxed);
        return true;
    }

    const int idx = ccToParam[(size_t) cc].load (std::memory_order_relaxed);
    if (idx < 0 || idx >= (int) params.size()) return false;

    auto* p = params[(size_t) idx];
    const float norm = juce::jlimit (0.0f, 1.0f, (float) value / 127.0f);
    p->setValueNotifyingHost (norm);
    return true;
}

std::unique_ptr<juce::XmlElement> MidiLearn::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement> ("MidiLearn");
    for (int cc = 0; cc < kNumCCs; ++cc)
    {
        const int idx = ccToParam[(size_t) cc].load();
        if (idx >= 0 && idx < (int) params.size())
        {
            auto* e = xml->createNewChildElement ("Map");
            e->setAttribute ("cc", cc);
            e->setAttribute ("param", params[(size_t) idx]->paramID);
        }
    }
    return xml;
}

void MidiLearn::fromXml (const juce::XmlElement* xml)
{
    clearAll();
    if (xml == nullptr || ! xml->hasTagName ("MidiLearn")) return;
    for (auto* e : xml->getChildWithTagNameIterator ("Map"))
    {
        const int cc = e->getIntAttribute ("cc", kUnmapped);
        const int idx = indexOf (e->getStringAttribute ("param"));
        if (cc >= 0 && cc < kNumCCs && idx != kUnmapped)
            ccToParam[(size_t) cc].store (idx);
    }
}
} // namespace aeriform
