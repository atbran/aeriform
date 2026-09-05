#include "Displays.h"
#include "../DSP/Wavefolder.h"
#include "../DSP/ResonatorNetwork.h"

namespace aeriform
{
using namespace theme;

// ---------------------------------------------------------------------------
ScopeDisplay::ScopeDisplay (const ScopeBuffer& b, const std::atomic<float>* lvl, juce::Colour c, juce::String text)
    : buffer (b), levelAtomic (lvl), colour (c), label (std::move (text))
{
    setInterceptsMouseClicks (false, false);
    data.resize (192, 0.0f);
    startTimerHz (30);
}

ScopeDisplay::~ScopeDisplay() { stopTimer(); }

void ScopeDisplay::setActiveLook (bool a)
{
    if (active != a) { active = a; repaint(); }
}

void ScopeDisplay::timerCallback()
{
    buffer.read (data.data(), (int) data.size());
    if (levelAtomic != nullptr)
    {
        const float l = levelAtomic->load (std::memory_order_relaxed);
        level = l > level ? l : level * 0.9f;
    }
    // slow auto-gain so quiet exciters still show a waveform
    float peak = 1.0e-4f;
    for (float v : data) peak = juce::jmax (peak, std::fabs (v));
    const float target = juce::jlimit (0.5f, 12.0f, 0.8f / peak);
    gainNorm += 0.15f * (target - gainNorm);
    repaint();
}

void ScopeDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (inset);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (panelBorder);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);

    auto meter = r.removeFromRight (6.0f).reduced (1.0f, 3.0f);
    auto plot = r.reduced (4.0f, 3.0f);
    g.setColour (grid);
    g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());

    juce::Path p;
    const int n = (int) data.size();
    const float mid = plot.getCentreY();
    for (int i = 0; i < n; ++i)
    {
        const float x = plot.getX() + (float) i / (float) (n - 1) * plot.getWidth();
        const float y = mid - juce::jlimit (-1.0f, 1.0f, data[(size_t) i] * gainNorm) * plot.getHeight() * 0.48f;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour ((active ? colour : textDim).withAlpha (active ? 0.9f : 0.5f));
    g.strokePath (p, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved));

    // level bar
    g.setColour (knobTrack);
    g.fillRoundedRectangle (meter, 2.0f);
    const float h = juce::jlimit (0.0f, 1.0f, level * 1.4f) * meter.getHeight();
    g.setColour (colour.withAlpha (0.85f));
    g.fillRoundedRectangle (meter.withTop (meter.getBottom() - h), 2.0f);

    g.setColour (textDim);
    g.setFont (font (9.0f, true));
    g.drawText (label, getLocalBounds().reduced (6, 2), juce::Justification::topLeft);
}

// ---------------------------------------------------------------------------
FoldCurveDisplay::FoldCurveDisplay (AeriformProcessor& p) : processor (p)
{
    setInterceptsMouseClicks (false, false);
    auto& s = processor.getAPVTS();
    pOn = s.getRawParameterValue (ids::wfOn);     pMode = s.getRawParameterValue (ids::wfMode);
    pFold = s.getRawParameterValue (ids::wfFold); pDrive = s.getRawParameterValue (ids::wfDrive);
    pSym = s.getRawParameterValue (ids::wfSymmetry); pBias = s.getRawParameterValue (ids::wfBias);
    pStages = s.getRawParameterValue (ids::wfStages); pShape = s.getRawParameterValue (ids::wfShape);
    pMix = s.getRawParameterValue (ids::wfMix);   pComp = s.getRawParameterValue (ids::wfComp);
    scope.resize (192, 0.0f);
    startTimerHz (30);
}

FoldCurveDisplay::~FoldCurveDisplay() { stopTimer(); }

void FoldCurveDisplay::timerCallback()
{
    processor.getVisualizerModel().foldScope.read (scope.data(), (int) scope.size());
    repaint();
}

void FoldCurveDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (inset);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (panelBorder);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
    auto plot = r.reduced (10.0f, 8.0f);

    const bool on = pOn != nullptr && pOn->load() > 0.5f;
    dsp::Wavefolder::Params p;
    p.on = true;
    p.mode = (FoldMode) juce::jlimit (0, (int) FoldMode::Count - 1, (int) pMode->load());
    p.fold = pFold->load(); p.drive = pDrive->load(); p.symmetry = pSym->load(); p.bias = pBias->load();
    p.stages = (int) pStages->load(); p.shape = pShape->load(); p.mix = pMix->load(); p.comp = pComp->load();

    // grid: unit square of the transfer function, input range +/- 1.25
    const float range = 1.25f;
    auto toX = [&] (float x) { return plot.getX() + (x + range) / (2.0f * range) * plot.getWidth(); };
    auto toY = [&] (float y) { return plot.getCentreY() - juce::jlimit (-1.6f, 1.6f, y) / 1.6f * plot.getHeight() * 0.5f; };
    g.setColour (grid);
    g.drawVerticalLine ((int) toX (0.0f), plot.getY(), plot.getBottom());
    g.drawHorizontalLine ((int) toY (0.0f), plot.getX(), plot.getRight());
    g.setColour (grid.withAlpha (0.6f));
    for (float v : { -1.0f, 1.0f })
    {
        g.drawVerticalLine ((int) toX (v), plot.getY(), plot.getBottom());
        g.drawHorizontalLine ((int) toY (v), plot.getX(), plot.getRight());
    }

    // live post-fold waveform (time axis) behind the curve
    {
        juce::Path w;
        const int n = (int) scope.size();
        for (int i = 0; i < n; ++i)
        {
            const float x = plot.getX() + (float) i / (float) (n - 1) * plot.getWidth();
            const float y = toY (scope[(size_t) i] * 1.2f);
            if (i == 0) w.startNewSubPath (x, y); else w.lineTo (x, y);
        }
        g.setColour (folder.withAlpha (on ? 0.28f : 0.12f));
        g.strokePath (w, juce::PathStrokeType (1.0f));
    }

    // transfer curve
    juce::Path c;
    const int steps = 160;
    for (int i = 0; i <= steps; ++i)
    {
        const float x = -range + 2.0f * range * (float) i / (float) steps;
        const float y = dsp::Wavefolder::foldSample (p, x);
        if (i == 0) c.startNewSubPath (toX (x), toY (y)); else c.lineTo (toX (x), toY (y));
    }
    g.setColour (on ? folder : textDim);
    g.strokePath (c, juce::PathStrokeType (on ? 1.8f : 1.2f, juce::PathStrokeType::curved));

    g.setColour (textDim);
    g.setFont (font (9.0f, true));
    g.drawText (on ? "TRANSFER  in > out" : "FOLDER OFF", getLocalBounds().reduced (6, 2), juce::Justification::topLeft);
}

// ---------------------------------------------------------------------------
EnergyBar::EnergyBar (VisualizerModel& m, int s, juce::Colour c) : model (m), slot (s), colour (c)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

EnergyBar::~EnergyBar() { stopTimer(); }

void EnergyBar::timerCallback()
{
    const float e = juce::jlimit (0.0f, 1.0f, model.resonatorEnergy[(size_t) slot].load (std::memory_order_relaxed) * 1.6f);
    value = e > value ? value + 0.5f * (e - value) : value * 0.9f;
    running = model.resonatorRunning[(size_t) slot].load (std::memory_order_relaxed) != 0;
    repaint();
}

void EnergyBar::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto bar = r.withTrimmedLeft (52.0f).reduced (0.0f, r.getHeight() * 0.3f);
    g.setColour (textDim);
    g.setFont (font (9.0f, true));
    g.drawText (running ? "ENERGY" : "IDLE", r.withWidth (50.0f), juce::Justification::centredLeft);
    g.setColour (knobTrack);
    g.fillRoundedRectangle (bar, 2.0f);
    g.setColour (colour.withAlpha (running ? 0.9f : 0.3f));
    g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * value), 2.0f);
}

// ---------------------------------------------------------------------------
NetworkDiagram::NetworkDiagram (AeriformProcessor& p, bool c) : processor (p), compact (c)
{
    routes = { { { ids::netAB, "A > B", 0, 1 }, { ids::netBA, "B > A", 1, 0 }, { ids::netBC, "B > C", 1, 2 },
                { ids::netCB, "C > B", 2, 1 }, { ids::netCA, "C > A", 2, 0 }, { ids::netAC, "A > C", 0, 2 } } };
    setTooltip ("Resonator network. Drag a route arrow (or scroll) to change its amount; double-click a node to enable / disable it.");
    startTimerHz (30);
}

NetworkDiagram::~NetworkDiagram() { stopTimer(); }

std::atomic<float>* NetworkDiagram::raw (const char* id) const { return processor.getAPVTS().getRawParameterValue (id); }
float NetworkDiagram::value (const char* id) const
{
    auto* a = raw (id);
    return a != nullptr ? a->load (std::memory_order_relaxed) : 0.0f;
}

const char* NetworkDiagram::enableId (int node) { return node == 0 ? ids::resOn : (node == 1 ? ids::rbOn : ids::rcOn); }

void NetworkDiagram::resized() { rebuildGeometry(); }

void NetworkDiagram::rebuildGeometry()
{
    auto r = getLocalBounds().toFloat().reduced (compact ? 30.0f : 46.0f, compact ? 18.0f : 26.0f);
    nodeRadius = juce::jlimit (12.0f, 30.0f, juce::jmin (r.getWidth(), r.getHeight()) * (compact ? 0.15f : 0.13f));
    nodePos[0] = { r.getX() + nodeRadius, r.getCentreY() };
    nodePos[1] = { r.getRight() - nodeRadius, r.getY() + nodeRadius };
    nodePos[2] = { r.getRight() - nodeRadius, r.getBottom() - nodeRadius };

    for (auto& route : routes)
    {
        const auto a = nodePos[(size_t) route.from], b = nodePos[(size_t) route.to];
        auto dir = b - a;
        const float len = juce::jmax (1.0f, std::sqrt (dir.x * dir.x + dir.y * dir.y));
        dir /= len;
        const juce::Point<float> normal (-dir.y, dir.x);           // right-hand side of the travel direction
        const float bow = nodeRadius * 1.1f;
        const auto start = a + dir * (nodeRadius + 3.0f) + normal * (bow * 0.35f);
        const auto end = b - dir * (nodeRadius + 5.0f) + normal * (bow * 0.35f);
        const auto ctrl = (a + b) * 0.5f + normal * bow;
        route.path.clear();
        route.path.startNewSubPath (start);
        route.path.quadraticTo (ctrl, end);
        route.mid = route.path.getPointAlongPath (route.path.getLength() * 0.5f);
    }
}

int NetworkDiagram::routeAt (juce::Point<float> pt) const
{
    int best = -1; float bestD = 1.0e9f;
    for (int i = 0; i < 6; ++i)
    {
        juce::Point<float> nearest;
        const float d = routes[(size_t) i].path.getNearestPoint (pt, nearest);
        if (d < bestD) { bestD = d; best = i; }
    }
    return bestD < 9.0f ? best : -1;
}

int NetworkDiagram::nodeAt (juce::Point<float> pt) const
{
    for (int i = 0; i < 3; ++i)
        if (pt.getDistanceFrom (nodePos[(size_t) i]) <= nodeRadius) return i;
    return -1;
}

void NetworkDiagram::timerCallback()
{
    auto& vis = processor.getVisualizerModel();
    for (int i = 0; i < 3; ++i)
    {
        const float e = juce::jlimit (0.0f, 1.0f, vis.resonatorEnergy[(size_t) i].load (std::memory_order_relaxed) * 1.6f);
        energy[i] = e > energy[i] ? energy[i] + 0.5f * (e - energy[i]) : energy[i] * 0.9f;
        running[i] = vis.resonatorRunning[(size_t) i].load (std::memory_order_relaxed) != 0;
    }
    const float ne = juce::jlimit (0.0f, 1.0f, vis.networkEnergy.load (std::memory_order_relaxed) * 3.0f);
    netEnergy += 0.3f * (ne - netEnergy);
    governor = vis.governorGain.load (std::memory_order_relaxed);

    // route values (parameter + Repipe minimum) scaled by the network feedback
    const float repipe = value (ids::netRepipe);
    const auto rp = dsp::ResonatorNetwork::repipeRoutes (repipe);
    const float fbScale = juce::jmax (value (ids::netFeedback), repipe > 0.001f ? rp.fbScale : 0.0f);
    const float mins[6] = { 0.0f, rp.ba, 0.0f, rp.cb, rp.ca, rp.ac };
    for (int i = 0; i < 6; ++i)
    {
        routes[(size_t) i].value = value (routes[(size_t) i].id);
        routes[(size_t) i].effective = juce::jmax (routes[(size_t) i].value, repipe > 0.001f ? mins[i] : 0.0f) * fbScale;
    }
    repaint();
}

void NetworkDiagram::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (inset);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (panelBorder);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);

    const auto mode = (NetMode) juce::jlimit (0, (int) NetMode::Count - 1, (int) value (ids::netMode));
    const float repipe = value (ids::netRepipe);
    const auto rp = dsp::ResonatorNetwork::repipeRoutes (repipe);
    const bool repiped = repipe > 0.001f;
    const auto inject = (InjectPoint) juce::jlimit (0, (int) InjectPoint::Count - 1, (int) value (ids::netInject));
    const auto tap = (OutputTap) juce::jlimit (0, (int) OutputTap::Count - 1, (int) value (ids::netTap));
    const bool negative = value (ids::netPolarity) > 0.5f;
    const juce::Colour nodeColours[3] = { nodeA, nodeB, nodeC };

    auto arrowHead = [&] (juce::Point<float> tip, juce::Point<float> dir, float size)
    {
        const juce::Point<float> n (-dir.y, dir.x);
        juce::Path h;
        h.addTriangle (tip, tip - dir * size + n * size * 0.55f, tip - dir * size - n * size * 0.55f);
        g.fillPath (h);
    };
    auto straight = [&] (juce::Point<float> a, juce::Point<float> b, float amount, juce::Colour col, float width)
    {
        auto dir = b - a; const float len = juce::jmax (1.0f, std::sqrt (dir.x * dir.x + dir.y * dir.y)); dir /= len;
        const auto s = a + dir * (nodeRadius + 2.0f), e = b - dir * (nodeRadius + 4.0f);
        g.setColour (col.withAlpha (0.15f + 0.7f * amount));
        g.drawLine (juce::Line<float> (s, e), width);
        arrowHead (e + dir * 3.0f, dir, 6.0f + 3.0f * amount);
    };

    // ---- excitation input --------------------------------------------------------------
    const float inX = r.getX() + 8.0f;
    bool injected[3] = { false, false, false };
    if (mode == NetMode::Parallel) injected[0] = injected[1] = injected[2] = true;
    else
    {
        switch (inject)
        {
            case InjectPoint::B: injected[1] = true; break;
            case InjectPoint::C: injected[2] = true; break;
            case InjectPoint::All: injected[0] = injected[1] = injected[2] = true; break;
            case InjectPoint::A: default: injected[0] = true; break;
        }
    }
    for (int i = 0; i < 3; ++i)
    {
        if (! injected[i] || ! running[i]) continue;
        const float inLevel = value (i == 0 ? ids::resInput : (i == 1 ? ids::rbInput : ids::rcInput));
        const juce::Point<float> from (inX, nodePos[(size_t) i].y);
        auto dir = nodePos[(size_t) i] - from; const float len = juce::jmax (1.0f, std::sqrt (dir.x * dir.x + dir.y * dir.y)); dir /= len;
        const auto e = nodePos[(size_t) i] - dir * (nodeRadius + 4.0f);
        g.setColour (copper.withAlpha (0.25f + 0.6f * inLevel));
        g.drawLine (juce::Line<float> (from, e), 1.6f);
        arrowHead (e + dir * 3.0f, dir, 6.0f);
    }
    g.setColour (textDim);
    g.setFont (font (8.5f, true));
    g.drawText ("IN", juce::Rectangle<float> (r.getX() + 4.0f, nodePos[0].y - 20.0f, 24.0f, 12.0f), juce::Justification::centredLeft);

    // ---- serial / hybrid sends -------------------------------------------------------------------
    if (running[1] && (mode == NetMode::Serial || mode == NetMode::Hybrid || (mode == NetMode::Single && repiped)))
        straight (nodePos[0], nodePos[1], juce::jmax (value (ids::netSendAB), repiped ? rp.sendAB : 0.0f), brass, 2.6f);
    if (running[2])
    {
        const float sendBC = juce::jmax (value (ids::netSendBC), repiped ? rp.sendBC : 0.0f);
        if (mode == NetMode::Hybrid) straight (nodePos[0], nodePos[2], sendBC, brass, 2.6f);
        else if (mode == NetMode::Serial || (mode == NetMode::Single && repiped)) straight (nodePos[1], nodePos[2], sendBC, brass, 2.6f);
    }

    // ---- cross-feedback routes -------------------------------------------------------------------
    for (int i = 0; i < 6; ++i)
    {
        const auto& route = routes[(size_t) i];
        if (! running[route.from] || ! running[route.to]) continue;
        const float amt = juce::jlimit (0.0f, 1.0f, route.effective);
        const bool hot = i == hoverRoute || i == dragRoute;
        const auto col = (negative ? amber : teal).withAlpha (hot ? 0.95f : 0.12f + 0.75f * amt);
        g.setColour (col);
        g.strokePath (route.path, juce::PathStrokeType (1.0f + 3.0f * amt + (hot ? 1.0f : 0.0f), juce::PathStrokeType::curved));
        const auto end = route.path.getPointAlongPath (route.path.getLength());
        const auto before = route.path.getPointAlongPath (route.path.getLength() - 4.0f);
        auto dir = end - before; const float len = juce::jmax (0.1f, std::sqrt (dir.x * dir.x + dir.y * dir.y)); dir /= len;
        arrowHead (end + dir * 3.0f, dir, 5.0f + 3.0f * amt);
        if (hot || (! compact && amt > 0.02f))
        {
            g.setColour (hot ? textPrimary : textSecondary);
            g.setFont (monoFont (8.5f));
            g.drawText (juce::String (juce::roundToInt (route.value * 100.0f)) + "%", juce::Rectangle<float> (route.mid.x - 16.0f, route.mid.y - 7.0f, 32.0f, 14.0f), juce::Justification::centred);
        }
    }

    // ---- output taps -------------------------------------------------------------------------------------
    const float outX = r.getRight() - 8.0f;
    bool tapped[3] = { true, true, true };
    switch (tap)
    {
        case OutputTap::A: tapped[1] = tapped[2] = false; break;
        case OutputTap::B: tapped[0] = tapped[2] = false; break;
        case OutputTap::C: tapped[0] = tapped[1] = false; break;
        case OutputTap::Last: { const int last = running[2] ? 2 : (running[1] ? 1 : 0); for (int i = 0; i < 3; ++i) tapped[i] = i == last; break; }
        case OutputTap::Mix: default: break;
    }
    for (int i = 0; i < 3; ++i)
    {
        if (! tapped[i] || ! running[i]) continue;
        const float outLevel = value (i == 0 ? ids::resOutput : (i == 1 ? ids::rbOutput : ids::rcOutput));
        const juce::Point<float> to (outX, nodePos[(size_t) i].y);
        auto dir = to - nodePos[(size_t) i]; const float len = juce::jmax (1.0f, std::sqrt (dir.x * dir.x + dir.y * dir.y)); dir /= len;
        const auto s = nodePos[(size_t) i] + dir * (nodeRadius + 2.0f);
        g.setColour (copperBright.withAlpha (0.2f + 0.6f * outLevel));
        g.drawLine (juce::Line<float> (s, to - dir * 4.0f), 1.6f);
        arrowHead (to, dir, 6.0f);
    }
    g.setColour (textDim);
    g.setFont (font (8.5f, true));
    g.drawText ("OUT", juce::Rectangle<float> (r.getRight() - 30.0f, nodePos[1].y - 20.0f, 26.0f, 12.0f), juce::Justification::centredRight);

    if(value(ids::netBypass)>.5f||(!running[0]&&!running[1]&&!running[2])){g.setColour(teal);g.drawLine(10,r.getCentreY(),r.getRight()-10,r.getCentreY(),2);g.setFont(font(10,true));g.drawText("DIRECT EXCITER PATH",r.reduced(10).removeFromBottom(20),juce::Justification::centred);}
    // ---- nodes ---------------------------------------------------------------------------------------------
    static const char* names[3] = { "A", "B", "C" };
    for (int i = 0; i < 3; ++i)
    {
        const auto c = nodePos[(size_t) i];
        const auto col = nodeColours[i];
        const bool on = running[i];
        auto circle = juce::Rectangle<float> (nodeRadius * 2.0f, nodeRadius * 2.0f).withCentre (c);
        if (on && energy[i] > 0.01f)
        {
            juce::ColourGradient glow (col.withAlpha (0.55f * energy[i]), c.x, c.y, col.withAlpha (0.0f), c.x + nodeRadius * 2.2f, c.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (circle.expanded (nodeRadius * 1.2f));
        }
        g.setColour (on ? knobBody : inset);
        g.fillEllipse (circle);
        g.setColour (on ? col.withAlpha (0.9f) : knobRim);
        g.drawEllipse (circle, i == hoverNode ? 2.5f : 1.5f);
        if (on)
        {
            g.setColour (col.withAlpha (0.35f + 0.65f * energy[i]));
            g.fillEllipse (circle.reduced (nodeRadius * (1.0f - 0.85f * energy[i])));
        }
        g.setColour (on ? textPrimary : textDim);
        g.setFont (titleFont (compact ? 12.0f : 14.0f));
        g.drawText (names[i], circle, juce::Justification::centred);
        if (! compact)
        {
            const auto typeId = i == 0 ? ids::resMode : (i == 1 ? ids::rbType : ids::rcType);
            const int type = juce::jlimit (0, (int) ResMode::Count - 1, (int) value (typeId));
            g.setColour (on ? textSecondary : textDim);
            g.setFont (font (9.0f));
            const auto labelArea = juce::Rectangle<float> (c.x - 44.0f, i == 0 ? c.y + nodeRadius + 2.0f : (i == 1 ? c.y - nodeRadius - 15.0f : c.y + nodeRadius + 2.0f), 88.0f, 13.0f);
            g.drawText (on ? choices::resModes()[type] : "off", labelArea, juce::Justification::centred);
        }
    }

    // ---- readouts ------------------------------------------------------------------------------------------
    g.setColour (textDim);
    g.setFont (font (8.5f, true));
    juce::String head = choices::netModes()[(int) mode].toUpperCase();
    if (repiped) head += "  +  REPIPE " + juce::String (juce::roundToInt (repipe * 100.0f)) + "%";
    g.drawText (head, getLocalBounds().reduced (8, 4), juce::Justification::topLeft);
    if (! compact)
    {
        juce::String foot = "FEEDBACK ENERGY " + juce::String (juce::roundToInt (netEnergy * 100.0f)) + "%";
        if (governor < 0.98f) { g.setColour (amber); foot += "   GOVERNOR " + juce::String (juce::roundToInt (governor * 100.0f)) + "%"; }
        g.drawText (foot, getLocalBounds().reduced (8, 4), juce::Justification::bottomLeft);
    }
    if (negative)
    {
        g.setColour (amber.withAlpha (0.8f));
        g.drawText ("NEG", getLocalBounds().reduced (8, 4), juce::Justification::topRight);
    }
}

// ---- interaction ------------------------------------------------------------------------------------
void NetworkDiagram::mouseMove (const juce::MouseEvent& e)
{
    const int rt = routeAt (e.position);
    const int nd = rt < 0 ? nodeAt (e.position) : -1;
    if (rt != hoverRoute || nd != hoverNode)
    {
        hoverRoute = rt; hoverNode = nd;
        if (rt >= 0)
            setTooltip (juce::String ("Route ") + routes[(size_t) rt].name + ": " + juce::String (juce::roundToInt (routes[(size_t) rt].value * 100.0f)) + " %. Drag or scroll to change.");
        else if (nd >= 0)
            setTooltip (juce::String ("Resonator ") + (nd == 0 ? "A" : (nd == 1 ? "B" : "C")) + ": double-click to " + (running[nd] ? "disable" : "enable") + ".");
        else
            setTooltip ("Resonator network. Drag a route arrow (or scroll) to change its amount; double-click a node to enable / disable it.");
        setMouseCursor (rt >= 0 ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void NetworkDiagram::mouseExit (const juce::MouseEvent&)
{
    hoverRoute = hoverNode = -1;
    repaint();
}

void NetworkDiagram::setRouteValue (int route, float v)
{
    if (route < 0) return;
    if (auto* p = processor.getAPVTS().getParameter (routes[(size_t) route].id))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (0.0f, 1.0f, v)));
}

void NetworkDiagram::mouseDown (const juce::MouseEvent& e)
{
    dragRoute = routeAt (e.position);
    if (dragRoute >= 0)
    {
        dragParam = processor.getAPVTS().getParameter (routes[(size_t) dragRoute].id);
        if (dragParam != nullptr) dragParam->beginChangeGesture();
        dragStartY = e.y;
        dragStartValue = routes[(size_t) dragRoute].value;
    }
}

void NetworkDiagram::mouseDrag (const juce::MouseEvent& e)
{
    if (dragRoute < 0) return;
    const float sens = e.mods.isShiftDown() ? 1200.0f : 160.0f;
    setRouteValue (dragRoute, dragStartValue + (float) (dragStartY - e.y) / sens);
}

void NetworkDiagram::mouseUp (const juce::MouseEvent&)
{
    if (dragParam != nullptr) { dragParam->endChangeGesture(); dragParam = nullptr; }
    dragRoute = -1;
    repaint();
}

void NetworkDiagram::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int nd = nodeAt (e.position);
    if (nd < 0) return;
    if (auto* p = processor.getAPVTS().getParameter (enableId (nd)))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f);
        p->endChangeGesture();
    }
}

void NetworkDiagram::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const int rt = routeAt (e.position);
    if (rt < 0) return;
    if (auto* p = processor.getAPVTS().getParameter (routes[(size_t) rt].id))
    {
        p->beginChangeGesture();
        setRouteValue (rt, routes[(size_t) rt].value + wheel.deltaY * 0.5f);
        p->endChangeGesture();
    }
}
} // namespace aeriform
