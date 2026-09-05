#pragma once

#include "Plugin/PluginProcessor.h"
#include "DSP/DspUtils.h"
#include <cmath>
#include <vector>
#include <functional>

namespace aeriform::test
{
struct RenderStats
{
    float peak = 0.0f;
    double rms = 0.0;
    bool finite = true;
    long samples = 0;
};

/** Drives an AeriformProcessor like a host would. */
struct TestHost
{
    AeriformProcessor processor;
    double sampleRate = 48000.0;
    int blockSize = 256;
    juce::AudioBuffer<float> buffer;
    juce::MidiBuffer midi;

    explicit TestHost (double sr = 48000.0, int block = 256) { prepare (sr, block); }

    bool useInput = false;
    std::function<float (long sampleIndex)> inputSource;   // optional sidechain generator
    long inputSampleCounter = 0;

    void prepare (double sr, int block, bool withInput = false)
    {
        sampleRate = sr;
        blockSize = block;
        useInput = withInput;
        if (withInput)
        {
            processor.enableAllBuses();
            processor.setPlayConfigDetails (2, 2, sr, block);
        }
        else
        {
            processor.setPlayConfigDetails (0, 2, sr, block);
        }
        processor.prepareToPlay (sr, block);
        buffer.setSize (2, block);
    }

    void set (const juce::String& id, float dspValue)
    {
        auto* p = processor.getAPVTS().getParameter (id);
        jassert (p != nullptr);
        p->setValueNotifyingHost (p->convertTo0to1 (dspValue));
    }

    float get (const juce::String& id)
    {
        auto* p = processor.getAPVTS().getParameter (id);
        return p != nullptr ? p->convertFrom0to1 (p->getValue()) : 0.0f;
    }

    void noteOn (int note, int velocity = 100, int channel = 1, int sample = 0)
    {
        midi.addEvent (juce::MidiMessage::noteOn (channel, note, (juce::uint8) velocity), sample);
    }
    void noteOff (int note, int channel = 1, int sample = 0) { midi.addEvent (juce::MidiMessage::noteOff (channel, note), sample); }
    void cc (int controller, int value, int channel = 1, int sample = 0)
    {
        midi.addEvent (juce::MidiMessage::controllerEvent (channel, controller, value), sample);
    }
    void pitchBend (int value14, int channel = 1) { midi.addEvent (juce::MidiMessage::pitchWheel (channel, value14), 0); }
    void aftertouch (int value, int channel = 1) { midi.addEvent (juce::MidiMessage::channelPressureChange (channel, value), 0); }

    /** Renders one block; MIDI queued since the last block is delivered with it. */
    RenderStats renderBlock (std::vector<float>* monoOut = nullptr)
    {
        buffer.clear();
        if (useInput && inputSource)
            for (int i = 0; i < blockSize; ++i)
            {
                const float v = inputSource (inputSampleCounter++);
                buffer.setSample (0, i, v);
                buffer.setSample (1, i, v);
            }
        processor.processBlock (buffer, midi);
        midi.clear();
        RenderStats s;
        double sumSq = 0.0;
        for (int i = 0; i < blockSize; ++i)
        {
            const float l = buffer.getSample (0, i), r = buffer.getSample (1, i);
            if (! std::isfinite (l) || ! std::isfinite (r)) s.finite = false;
            s.peak = std::max (s.peak, std::max (std::fabs (l), std::fabs (r)));
            sumSq += (double) l * l + (double) r * r;
            if (monoOut != nullptr) monoOut->push_back (0.5f * (l + r));
        }
        s.samples = 2L * blockSize;
        s.rms = std::sqrt (sumSq / (double) s.samples);
        return s;
    }

    RenderStats render (double seconds, std::vector<float>* monoOut = nullptr)
    {
        RenderStats total;
        double sumSq = 0.0;
        const int blocks = std::max (1, (int) std::ceil (seconds * sampleRate / blockSize));
        for (int b = 0; b < blocks; ++b)
        {
            auto s = renderBlock (monoOut);
            total.finite = total.finite && s.finite;
            total.peak = std::max (total.peak, s.peak);
            sumSq += s.rms * s.rms * (double) s.samples;
            total.samples += s.samples;
        }
        total.rms = std::sqrt (sumSq / (double) std::max (1L, total.samples));
        return total;
    }

    int activeVoices() { return processor.getEngine().getActiveVoiceCount(); }
};

/** Estimates the fundamental frequency of a harmonic signal by autocorrelation
    around an expected value (searches +/- 6 % of the expected period). */
inline double estimateFrequency (const std::vector<float>& xIn, double sampleRate, double expectedHz)
{
    // Low-pass above the fundamental first so the autocorrelation peak is smooth (unbiased parabolic fit).
    std::vector<float> x (xIn.size());
    {
        const double a = 1.0 - std::exp (-2.0 * 3.14159265358979 * (1.6 * expectedHz) / sampleRate);
        double y1 = 0.0, y2 = 0.0;
        for (size_t i = 0; i < xIn.size(); ++i)
        {
            y1 += a * (xIn[i] - y1);
            y2 += a * (y1 - y2);
            x[i] = (float) y2;
        }
    }
    const double expectedPeriod = sampleRate / expectedHz;
    const int minLag = std::max (2, (int) std::floor (expectedPeriod * 0.94));
    const int maxLag = (int) std::ceil (expectedPeriod * 1.06);
    const int n = (int) x.size();
    if (maxLag * 3 >= n) return 0.0;

    auto acf = [&] (int lag)
    {
        double s = 0.0;
        for (int i = 0; i + lag < n; ++i) s += (double) x[(size_t) i] * x[(size_t) (i + lag)];
        return s;
    };

    int bestLag = minLag;
    double best = -1.0e300;
    std::vector<double> r ((size_t) (maxLag - minLag + 3), 0.0);
    for (int lag = minLag - 1; lag <= maxLag + 1; ++lag)
    {
        const double v = acf (lag);
        r[(size_t) (lag - minLag + 1)] = v;
        if (lag >= minLag && lag <= maxLag && v > best) { best = v; bestLag = lag; }
    }
    // parabolic interpolation around the peak
    const double ym = r[(size_t) (bestLag - minLag)], y0 = r[(size_t) (bestLag - minLag + 1)], yp = r[(size_t) (bestLag - minLag + 2)];
    const double denom = ym - 2.0 * y0 + yp;
    double delta = std::fabs (denom) > 1.0e-12 ? 0.5 * (ym - yp) / denom : 0.0;
    if (! std::isfinite (delta) || std::fabs (delta) > 1.0) delta = 0.0;
    return sampleRate / ((double) bestLag + delta);
}

inline double centsBetween (double a, double b) { return 1200.0 * std::log2 (a / b); }
} // namespace aeriform::test
