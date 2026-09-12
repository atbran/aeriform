#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "../Visualization/VisualizerModel.h"

namespace aeriform
{
/**
    Central animation: a resonant tube with airflow particles driven by breath
    pressure, an inner glow following resonator energy, the output waveform
    drawn as a ribbon along the bore, and per-voice pitch markers.
    All data comes from lock-free atomics; the static artwork is cached.
*/
class Visualizer : public juce::Component,
                   private juce::Timer
{
public:
    explicit Visualizer (VisualizerModel& model);
    ~Visualizer() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void setMode (int index) { mode = juce::jlimit (0, 2, index); repaint(); }
    int getMode() const noexcept { return mode; }

private:
    struct Particle { float x, y, speed, size, phase; };

    VisualizerModel& model;
    juce::Image background;
    std::vector<Particle> particles;
    std::vector<float> scope;
    juce::Random random;
    float smoothedPressure = 0.0f, smoothedEnergy = 0.0f, smoothedPeak = 0.0f;
    float displayPeak = 0.0f;
    int activeVoices = 0;
    int mode = 0;
    juce::dsp::FFT fft { 11 };
    juce::dsp::WindowingFunction<float> window { 2048, juce::dsp::WindowingFunction<float>::hann, false };
    std::array<float, 4096> fftData {};
    std::array<float, 1025> spectrum {};
    double lastTick = 0.0;
    float peakHold = 0.0f;
    juce::Rectangle<float> tube;

    void timerCallback() override;
    void rebuildBackground();
    void paintAnalysis (juce::Graphics&);
    static float pitchToX (float hz, juce::Rectangle<float> area) noexcept;
};
} // namespace aeriform
