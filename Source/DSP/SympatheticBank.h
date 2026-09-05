#pragma once
#include "DspUtils.h"
#include <array>
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
namespace aeriform::dsp {
struct SympatheticParams {
    bool enabled=false,freeze=false,clear=false,capture=false;
    float send=.4f,returnLevel=.4f,damper=0,decayMs=6000,damping=.3f,brightness=.5f,detune=0,spread=.7f,thresholdDb=-72;
    int tuning=1,root=48,count=12;
    std::array<int,12> intervals{0,4,7,12,16,19,24,28,31,36,40,43};
};
class SympatheticBank {
public:
    SympatheticBank() noexcept;
    using Chord=std::array<int,12>; // -1 means unused; absolute MIDI notes otherwise.
    void prepare(float sampleRate) noexcept;
    void reset() noexcept;
    void update(SympatheticParams,int samples=32) noexcept;
    void handleMidi(const juce::MidiMessage&) noexcept;
    void next(float mono,int voiceCount,float& left,float& right) noexcept;
    void requestCapturedChord(const Chord&) noexcept;
    Chord capturedChord() const noexcept;
    Chord currentHeldChord() const noexcept {Chord c;for(int i=0;i<12;++i)c[(size_t)i]=publishedHeld[(size_t)i].load(std::memory_order_relaxed);return c;}
    float modeEnergy(int i) const noexcept {return energy[(size_t)std::clamp(i,0,11)];}
    float frequency(int i) const noexcept {return frequencyHz[(size_t)std::clamp(i,0,11)];}
    bool active() const noexcept {return p.enabled||wet>1e-6f;}
private:
    struct Mode {double re=0,im=0,cosine=1,sine=0,radius=.99;float weight=0,panL=.707f,panR=.707f,gain=1;};
    std::array<Mode,12> modes;
    std::array<float,12> energy{},frequencyHz{};
    std::array<unsigned char,2048> held{};
    std::array<bool,16> sustain{};
    Chord captured{},lastHeld{};
    std::array<std::atomic<int>,12> published{},requested{},publishedHeld{};
    std::atomic<unsigned> publication{0},requestRevision{0},appliedRevision{0};
    unsigned consumedRequest=0;
    SympatheticParams p;
    float sr=48000,wet=0,freezeAmount=0,step=.001f,threshold=0,frequencySmooth=.1f;
    bool previousClear=false,previousCapture=false;
    void clearEnergy() noexcept;
    void tune(bool immediate=false) noexcept;
    Chord heldChord() const noexcept;
    void publish() noexcept;
};
}
