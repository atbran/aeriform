#pragma once
#include "../Params/ParameterLayout.h"
#include <bitset>
class AeriformProcessor;
namespace aeriform {
class PatchStateManager {
public:
    using Values=std::array<float,(size_t)kNumParams>;
    enum class Scope { All, Exciters, Network, Modulation, Effects };
    explicit PatchStateManager(AeriformProcessor&);
    juce::UndoManager undo;
    void begin(const juce::String& name);
    void end();
    void perform(const juce::String& name,const std::function<void()>& action);
    void setParameter(const juce::String& id,float value);
    void capture(int slot);
    bool loadSnapshot(int slot,int presetIndex);
    void selectEndpoint(int slot);
    int selectedEndpoint() const noexcept { return selected.load(); }
    juce::String snapshotName(int slot) const { return names[(size_t)std::clamp(slot,0,1)]; }
    void commitMorph();
    void randomize(Scope,float amount,bool mutation);
    uint32_t getSeed() const noexcept { return seed; }
    void setSeed(uint32_t value);
    void newSeed();
    void toggleFavorite(const juce::String&);
    bool isLocked(const juce::String& id) const;
    void setLocked(const juce::String& id,bool locked);
    void lockSection(ParamSection,bool);
    void lockAll(bool);
    std::unique_ptr<juce::XmlElement> toXml() const;
    void fromXml(const juce::XmlElement*);
    void prepare(double sr);
    void evaluate(int samples,Values& a,Values& b) noexcept;
    float position() const noexcept { return smoothedPosition; }
    bool enabled() const noexcept;
    bool deep() const noexcept;
    static float interpolate(P,float,float,float) noexcept;
private:
    AeriformProcessor& processor;
    std::array<std::atomic<float>*,(size_t)kNumParams> raw{};
    std::array<juce::RangedAudioParameter*,(size_t)kNumParams> parameters{};
    std::array<std::array<std::atomic<float>,(size_t)kNumParams>,2> snapshots{};
    std::array<std::atomic<unsigned>,2> revisions{};
    std::array<Values,2> audioCache{};
    std::array<juce::String,2> names{{"Snapshot A","Snapshot B"}};
    std::atomic<int> selected{0};
    std::bitset<(size_t)kNumParams> locks;
    uint32_t seed=20260905;
    double sampleRate=48000;
    float smoothedPosition=0;
    int transactionDepth=0;
    bool restoring=false;
    juce::String transactionName;
    std::unique_ptr<juce::XmlElement> before;
    Values current() const;
    void writeSnapshot(int,const Values&);
    Values readSnapshot(int) const;
    void apply(const Values&);
    int indexOf(const juce::String&) const;
    static bool administrative(P) noexcept;
    static bool protectedRandom(P) noexcept;
};
}
