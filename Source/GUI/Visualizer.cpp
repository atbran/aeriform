#include "Visualizer.h"
#include "Theme.h"

namespace aeriform
{
using namespace theme;

Visualizer::Visualizer (VisualizerModel& m) : model (m)
{
    setOpaque (false);
    setInterceptsMouseClicks (false, false);
    scope.resize (256, 0.0f);
    particles.resize (56);
    for (auto& p : particles)
        p = { random.nextFloat(), random.nextFloat(), 0.6f + random.nextFloat() * 0.8f, 1.0f + random.nextFloat() * 2.0f, random.nextFloat() * 6.283f };
    startTimerHz (30);
}

Visualizer::~Visualizer() { stopTimer(); }

float Visualizer::pitchToX (float hz, juce::Rectangle<float> area) noexcept
{
    const float lo = std::log2 (27.5f), hi = std::log2 (8000.0f);
    const float t = juce::jlimit (0.0f, 1.0f, (std::log2 (juce::jmax (hz, 1.0f)) - lo) / (hi - lo));
    return area.getX() + t * area.getWidth();
}

void Visualizer::resized()
{
    const auto b = getLocalBounds().toFloat().reduced (10.0f, 12.0f);
    tube = b.withTrimmedBottom (18.0f);
    rebuildBackground();
}

void Visualizer::rebuildBackground()
{
    if (getWidth() <= 0 || getHeight() <= 0) return;
    background = juce::Image (juce::Image::ARGB, getWidth(), getHeight(), true);
    juce::Graphics g (background);

    // inset well
    auto well = getLocalBounds().toFloat();
    g.setColour (inset);
    g.fillRoundedRectangle (well, cornerRadius);
    g.setColour (panelBorder);
    g.drawRoundedRectangle (well.reduced (0.5f), cornerRadius, 1.0f);

    // faint pitch grid (octaves)
    g.setColour (grid.withAlpha (0.7f));
    for (float hz = 27.5f; hz <= 8000.0f; hz *= 2.0f)
    {
        const float x = pitchToX (hz, tube);
        g.drawVerticalLine ((int) x, tube.getY() + 4.0f, tube.getBottom() - 4.0f);
    }

    // tube body: glass-like vertical gradient
    auto bore = tube.reduced (26.0f, tube.getHeight() * 0.2f);
    juce::ColourGradient glass (juce::Colour (0xff2b3036), bore.getX(), bore.getY(),
                                juce::Colour (0xff1b1e23), bore.getX(), bore.getBottom(), false);
    glass.addColour (0.35, juce::Colour (0xff363b42));
    glass.addColour (0.5, juce::Colour (0xff2a2e34));
    g.setGradientFill (glass);
    g.fillRoundedRectangle (bore, bore.getHeight() * 0.5f);
    g.setColour (knobRim);
    g.drawRoundedRectangle (bore, bore.getHeight() * 0.5f, 1.0f);
    // specular line
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (bore.reduced (12.0f, 0.0f).withHeight (bore.getHeight() * 0.22f).translated (0.0f, bore.getHeight() * 0.12f), 4.0f);

    // copper end caps (mouthpiece left, bell right)
    auto cap = [&] (juce::Rectangle<float> r, bool bell)
    {
        juce::ColourGradient metal (copperBright.darker (0.1f), r.getX(), r.getY(), copperDim, r.getX(), r.getBottom(), false);
        metal.addColour (0.45, copper.brighter (0.15f));
        g.setGradientFill (metal);
        if (bell)
        {
            juce::Path p;
            p.startNewSubPath (r.getX(), r.getY() + r.getHeight() * 0.3f);
            p.quadraticTo (r.getRight() - 4.0f, r.getY() + r.getHeight() * 0.2f, r.getRight(), r.getY());
            p.lineTo (r.getRight(), r.getBottom());
            p.quadraticTo (r.getRight() - 4.0f, r.getBottom() - r.getHeight() * 0.2f, r.getX(), r.getBottom() - r.getHeight() * 0.3f);
            p.closeSubPath();
            g.fillPath (p);
            g.setColour (copperDim.darker (0.3f));
            g.strokePath (p, juce::PathStrokeType (1.0f));
        }
        else
        {
            g.fillRoundedRectangle (r, 3.0f);
            g.setColour (copperDim.darker (0.3f));
            g.drawRoundedRectangle (r, 3.0f, 1.0f);
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            for (int i = 1; i < 4; ++i)
                g.drawVerticalLine ((int) (r.getX() + r.getWidth() * (float) i / 4.0f), r.getY() + 2.0f, r.getBottom() - 2.0f);
        }
    };
    cap (juce::Rectangle<float> (tube.getX(), bore.getY() - 4.0f, 30.0f, bore.getHeight() + 8.0f), false);
    cap (juce::Rectangle<float> (tube.getRight() - 32.0f, bore.getY() - 10.0f, 32.0f, bore.getHeight() + 20.0f), true);
}

void Visualizer::timerCallback()
{
    const float pressure = model.masterPressure.load (std::memory_order_relaxed);
    const float energy = model.masterEnergy.load (std::memory_order_relaxed);
    const float peak = model.masterPeak.load (std::memory_order_relaxed);
    smoothedPressure += 0.25f * (pressure - smoothedPressure);
    smoothedEnergy += 0.2f * (juce::jmin (1.5f, energy * 2.0f) - smoothedEnergy);
    smoothedPeak = peak > smoothedPeak ? peak : smoothedPeak * 0.92f;
    activeVoices = model.activeVoices.load (std::memory_order_relaxed);
    model.readScope (scope.data(), (int) scope.size());

    // advance airflow particles: speed follows breath pressure, idle drift when silent
    const float flow = 0.15f + 2.2f * smoothedPressure + 0.6f * smoothedEnergy;
    for (auto& p : particles)
    {
        p.x += 0.004f * flow * p.speed;
        p.phase += 0.15f;
        if (p.x > 1.0f) { p.x -= 1.0f; p.y = random.nextFloat(); p.speed = 0.6f + random.nextFloat() * 0.8f; }
    }
    repaint();
}

void Visualizer::paint (juce::Graphics& g)
{
    if (background.isValid())
        g.drawImageAt (background, 0, 0);

    auto bore = tube.reduced (26.0f, tube.getHeight() * 0.2f);
    const auto inner = bore.reduced (34.0f, 4.0f);

    // resonance glow inside the bore
    if (smoothedEnergy > 0.01f)
    {
        juce::ColourGradient glow (teal.withAlpha (juce::jmin (0.55f, smoothedEnergy * 0.5f)), inner.getCentreX(), inner.getCentreY(),
                                   teal.withAlpha (0.0f), inner.getCentreX(), inner.getY() - inner.getHeight(), true);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (inner, inner.getHeight() * 0.5f);
    }

    // airflow particles
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (inner.toNearestInt());
        const float visibility = juce::jlimit (0.0f, 1.0f, 0.12f + smoothedPressure * 1.6f + smoothedEnergy * 0.4f);
        for (const auto& p : particles)
        {
            const float x = inner.getX() + p.x * inner.getWidth();
            const float y = inner.getY() + 4.0f + p.y * (inner.getHeight() - 8.0f) + std::sin (p.phase) * 1.5f;
            const float len = 3.0f + 14.0f * smoothedPressure * p.speed;
            g.setColour (tealBright.withAlpha (visibility * (0.35f + 0.65f * p.speed) * 0.85f));
            g.fillRoundedRectangle (x - len, y - p.size * 0.5f, len, p.size, p.size * 0.5f);
        }
    }

    // waveform ribbon along the bore
    {
        juce::Path ribbon;
        const float mid = inner.getCentreY();
        const float amp = inner.getHeight() * 0.42f;
        const int n = (int) scope.size();
        for (int i = 0; i < n; ++i)
        {
            const float x = inner.getX() + (float) i / (float) (n - 1) * inner.getWidth();
            const float y = mid - juce::jlimit (-1.0f, 1.0f, scope[(size_t) i] * 1.6f) * amp;
            if (i == 0) ribbon.startNewSubPath (x, y); else ribbon.lineTo (x, y);
        }
        g.setColour (copperBright.withAlpha (0.25f + 0.6f * juce::jmin (1.0f, smoothedPeak * 1.5f)));
        g.strokePath (ribbon, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved));
    }

    // per-voice pitch markers below the tube
    const auto markerArea = juce::Rectangle<float> (tube.getX() + 30.0f, tube.getBottom() + 2.0f, tube.getWidth() - 60.0f, 14.0f);
    for (int i = 0; i < VisualizerModel::kMaxVoices; ++i)
    {
        const auto& v = model.voices[(size_t) i];
        if (v.active.load (std::memory_order_relaxed) == 0) continue;
        const float hz = v.pitchHz.load (std::memory_order_relaxed);
        const float e = juce::jlimit (0.15f, 1.0f, v.energy.load (std::memory_order_relaxed) * 3.0f);
        const float x = pitchToX (hz, markerArea);
        g.setColour (teal.withAlpha (0.35f + 0.65f * e));
        g.fillRoundedRectangle (x - 1.5f, markerArea.getBottom() - 2.0f - 10.0f * e, 3.0f, 10.0f * e, 1.5f);
    }

    // readouts
    g.setFont (monoFont (10.5f));
    g.setColour (textSecondary);
    const float db = smoothedPeak > 1.0e-4f ? 20.0f * std::log10 (smoothedPeak) : -99.0f;
    g.drawText ("VOICES " + juce::String (activeVoices), getLocalBounds().reduced (12, 6), juce::Justification::bottomLeft);
    g.drawText (juce::String (db, 1) + " dB", getLocalBounds().reduced (12, 6), juce::Justification::bottomRight);
    g.setColour (textDim);
    g.drawText ("AIRFLOW  /  RESONANCE", getLocalBounds().reduced (12, 6), juce::Justification::topRight);
}
} // namespace aeriform
