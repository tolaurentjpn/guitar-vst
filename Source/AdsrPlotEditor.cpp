#include "AdsrPlotEditor.h"

namespace
{
    constexpr float kMaxAttackMs = 5000.0f;
    constexpr float kMaxDecayMs = 4000.0f;
    constexpr float kMaxReleaseMs = 8000.0f;
    constexpr float kHandleRadius = 7.0f;
}

AdsrPlotEditor::AdsrPlotEditor (juce::AudioProcessorValueTreeState& state,
                                const juce::String& attackId,
                                const juce::String& decayId,
                                const juce::String& sustainId,
                                const juce::String& releaseId)
    : apvts (state),
      attackParamId (attackId),
      decayParamId (decayId),
      sustainParamId (sustainId),
      releaseParamId (releaseId)
{
    apvts.addParameterListener (attackParamId, this);
    apvts.addParameterListener (decayParamId, this);
    apvts.addParameterListener (sustainParamId, this);
    apvts.addParameterListener (releaseParamId, this);
    refreshFromParams();
}

AdsrPlotEditor::~AdsrPlotEditor()
{
    apvts.removeParameterListener (attackParamId, this);
    apvts.removeParameterListener (decayParamId, this);
    apvts.removeParameterListener (sustainParamId, this);
    apvts.removeParameterListener (releaseParamId, this);
}

void AdsrPlotEditor::parameterChanged (const juce::String&, float)
{
    refreshFromParams();
    repaint();
}

void AdsrPlotEditor::refreshFromParams()
{
    if (auto* p = apvts.getRawParameterValue (attackParamId))
        attackMs = p->load();
    if (auto* p = apvts.getRawParameterValue (decayParamId))
        decayMs = p->load();
    if (auto* p = apvts.getRawParameterValue (sustainParamId))
        sustainLevel = p->load();
    if (auto* p = apvts.getRawParameterValue (releaseParamId))
        releaseMs = p->load();
}

void AdsrPlotEditor::writeParams()
{
    if (auto* p = apvts.getParameter (attackParamId))
        p->setValueNotifyingHost (p->convertTo0to1 (attackMs));
    if (auto* p = apvts.getParameter (decayParamId))
        p->setValueNotifyingHost (p->convertTo0to1 (decayMs));
    if (auto* p = apvts.getParameter (sustainParamId))
        p->setValueNotifyingHost (p->convertTo0to1 (sustainLevel));
    if (auto* p = apvts.getParameter (releaseParamId))
        p->setValueNotifyingHost (p->convertTo0to1 (releaseMs));
}

juce::Rectangle<float> AdsrPlotEditor::plotBounds() const
{
    return getLocalBounds().toFloat().reduced (12.0f, 16.0f);
}

juce::Point<float> AdsrPlotEditor::attackPoint() const
{
    const auto b = plotBounds();
    const float ax = b.getX() + (attackMs / kMaxAttackMs) * b.getWidth() * 0.28f;
    return { ax, b.getY() };
}

juce::Point<float> AdsrPlotEditor::decayPoint() const
{
    const auto b = plotBounds();
    const auto a = attackPoint();
    const float dx = a.x + (decayMs / kMaxDecayMs) * b.getWidth() * 0.32f;
    const float dy = b.getBottom() - sustainLevel * b.getHeight();
    return { dx, dy };
}

juce::Point<float> AdsrPlotEditor::releasePoint() const
{
    const auto b = plotBounds();
    const auto d = decayPoint();
    const float sustainEndX = juce::jmin (d.x + b.getWidth() * 0.22f, b.getRight() - 8.0f);
    const float rx = sustainEndX + (releaseMs / kMaxReleaseMs) * (b.getRight() - sustainEndX);
    return { rx, b.getBottom() };
}

AdsrPlotEditor::Handle AdsrPlotEditor::hitTestHandle (juce::Point<float> pos) const
{
    const auto a = attackPoint();
    const auto d = decayPoint();
    const auto sustainEnd = juce::Point<float> (
        juce::jmin (d.x + plotBounds().getWidth() * 0.22f, plotBounds().getRight() - 8.0f), d.y);
    const auto r = releasePoint();

    if (a.getDistanceFrom (pos) < kHandleRadius * 2.0f)
        return Handle::attack;
    if (d.getDistanceFrom (pos) < kHandleRadius * 2.0f || sustainEnd.getDistanceFrom (pos) < kHandleRadius * 2.0f)
        return Handle::decaySustain;
    if (r.getDistanceFrom (pos) < kHandleRadius * 2.0f)
        return Handle::release;
    return Handle::none;
}

void AdsrPlotEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff10141a));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (juce::Colour (0xff3a3f47));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    const auto b = plotBounds();
    g.setColour (juce::Colour (0xff1c2028));
    g.fillRoundedRectangle (b, 4.0f);

    const auto p0 = juce::Point<float> (b.getX(), b.getBottom());
    const auto p1 = attackPoint();
    const auto p2 = decayPoint();
    const auto p3 = juce::Point<float> (
        juce::jmin (p2.x + b.getWidth() * 0.22f, b.getRight() - 8.0f), p2.y);
    const auto p4 = releasePoint();

    juce::Path curve;
    curve.startNewSubPath (p0);
    curve.lineTo (p1);
    curve.lineTo (p2);
    curve.lineTo (p3);
    curve.lineTo (p4);

    g.setColour (accent.withAlpha (0.25f));
    juce::Path fill = curve;
    fill.lineTo (p0.x + b.getWidth(), b.getBottom());
    fill.closeSubPath();
    g.fillPath (fill);

    g.setColour (accent);
    g.strokePath (curve, juce::PathStrokeType (2.0f));

    auto drawHandle = [&] (juce::Point<float> p)
    {
        g.setColour (juce::Colours::white);
        g.fillEllipse (p.x - kHandleRadius * 0.5f, p.y - kHandleRadius * 0.5f,
                       kHandleRadius, kHandleRadius);
        g.setColour (accent);
        g.drawEllipse (p.x - kHandleRadius * 0.5f, p.y - kHandleRadius * 0.5f,
                       kHandleRadius, kHandleRadius, 1.5f);
    };

    drawHandle (p1);
    drawHandle (p2);
    drawHandle (p4);

    g.setColour (juce::Colours::whitesmoke.withAlpha (0.55f));
    g.setFont (11.0f);
    g.drawText ("A", juce::Rectangle<float> (p1.x - 8.0f, b.getBottom() + 2.0f, 16.0f, 12.0f),
                juce::Justification::centred);
    g.drawText ("D/S", juce::Rectangle<float> (p2.x - 12.0f, b.getBottom() + 2.0f, 24.0f, 12.0f),
                juce::Justification::centred);
    g.drawText ("R", juce::Rectangle<float> (p4.x - 8.0f, b.getBottom() + 2.0f, 16.0f, 12.0f),
                juce::Justification::centred);
}

void AdsrPlotEditor::resized() {}

void AdsrPlotEditor::mouseDown (const juce::MouseEvent& e)
{
    activeHandle = hitTestHandle (e.position);
}

void AdsrPlotEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (activeHandle == Handle::none)
        return;

    const auto b = plotBounds();
    const float x = juce::jlimit (b.getX(), b.getRight(), e.position.x);
    const float y = juce::jlimit (b.getY(), b.getBottom(), e.position.y);

    switch (activeHandle)
    {
        case Handle::attack:
        {
            const float t = (x - b.getX()) / (b.getWidth() * 0.28f + 1.0e-6f);
            attackMs = juce::jlimit (1.0f, kMaxAttackMs, t * kMaxAttackMs);
            break;
        }
        case Handle::decaySustain:
        {
            const float ax = attackPoint().x;
            const float t = (x - ax) / (b.getWidth() * 0.32f + 1.0e-6f);
            decayMs = juce::jlimit (10.0f, kMaxDecayMs, t * kMaxDecayMs);
            sustainLevel = juce::jlimit (0.0f, 1.0f, 1.0f - (y - b.getY()) / b.getHeight());
            break;
        }
        case Handle::release:
        {
            const auto d = decayPoint();
            const float sustainEndX = juce::jmin (d.x + b.getWidth() * 0.22f, b.getRight() - 8.0f);
            const float span = juce::jmax (8.0f, b.getRight() - sustainEndX);
            const float t = (x - sustainEndX) / span;
            releaseMs = juce::jlimit (10.0f, kMaxReleaseMs, t * kMaxReleaseMs);
            break;
        }
        case Handle::none:
            break;
    }

    writeParams();
    repaint();
}

void AdsrPlotEditor::mouseUp (const juce::MouseEvent&)
{
    activeHandle = Handle::none;
}
