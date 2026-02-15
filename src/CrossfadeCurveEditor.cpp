#include "CrossfadeCurveEditor.h"
#include <cmath>

CrossfadeCurveEditor::CrossfadeCurveEditor() {}

void CrossfadeCurveEditor::setControlPoint(float cx, float cy)
{
    cpX = juce::jlimit(0.05f, 0.95f, cx);
    cpY = juce::jlimit(0.05f, 0.95f, cy);
    repaint();
}

juce::Point<float> CrossfadeCurveEditor::toScreen(float x, float y) const
{
    auto b = getLocalBounds().toFloat().reduced(4.0f);
    return { b.getX() + x * b.getWidth(), b.getBottom() - y * b.getHeight() };
}

juce::Point<float> CrossfadeCurveEditor::fromScreen(float sx, float sy) const
{
    auto b = getLocalBounds().toFloat().reduced(4.0f);
    return { (sx - b.getX()) / b.getWidth(), (b.getBottom() - sy) / b.getHeight() };
}

float CrossfadeCurveEditor::evalBezierY(float x) const
{
    // Solve (1-2cx)t^2 + 2cx*t - x = 0 for t, then evaluate y(t)
    const float a = 1.0f - 2.0f * cpX;
    float t;

    if (std::abs(a) < 1e-6f)
        t = (cpX > 1e-6f) ? x / (2.0f * cpX) : x;
    else
    {
        const float disc = 4.0f * cpX * cpX + 4.0f * a * x;
        t = (-2.0f * cpX + std::sqrt(std::max(0.0f, disc))) / (2.0f * a);
    }

    t = juce::jlimit(0.0f, 1.0f, t);
    return 2.0f * (1.0f - t) * t * cpY + t * t;
}

void CrossfadeCurveEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff1a1a2e));
    g.fillRoundedRectangle(bounds, 3.0f);

    auto area = bounds.reduced(4.0f);

    // Grid lines at 0.25 intervals
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    for (int i = 1; i <= 3; ++i)
    {
        float frac = static_cast<float>(i) * 0.25f;
        auto px = toScreen(frac, 0.0f);
        auto py = toScreen(0.0f, frac);
        g.drawVerticalLine(static_cast<int>(px.x), area.getY(), area.getBottom());
        g.drawHorizontalLine(static_cast<int>(py.y), area.getX(), area.getRight());
    }

    // Control polygon (faint lines P0->P1 and P1->P2)
    auto p0 = toScreen(0.0f, 0.0f);
    auto p1 = toScreen(cpX, cpY);
    auto p2 = toScreen(1.0f, 1.0f);

    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawLine(p0.x, p0.y, p1.x, p1.y, 1.0f);
    g.drawLine(p1.x, p1.y, p2.x, p2.y, 1.0f);

    // Fade-in curve (bright)
    {
        juce::Path fadeInPath;
        constexpr int segments = 30;
        for (int i = 0; i <= segments; ++i)
        {
            float x = static_cast<float>(i) / static_cast<float>(segments);
            float y = evalBezierY(x);
            auto pt = toScreen(x, y);
            if (i == 0)
                fadeInPath.startNewSubPath(pt);
            else
                fadeInPath.lineTo(pt);
        }
        g.setColour(juce::Colour(0xff4fc3f7));
        g.strokePath(fadeInPath, juce::PathStrokeType(1.5f));
    }

    // Fade-out curve (dimmer)
    {
        juce::Path fadeOutPath;
        constexpr int segments = 30;
        for (int i = 0; i <= segments; ++i)
        {
            float x = static_cast<float>(i) / static_cast<float>(segments);
            float y = 1.0f - evalBezierY(x);
            auto pt = toScreen(x, y);
            if (i == 0)
                fadeOutPath.startNewSubPath(pt);
            else
                fadeOutPath.lineTo(pt);
        }
        g.setColour(juce::Colour(0xff4fc3f7).withAlpha(0.35f));
        g.strokePath(fadeOutPath, juce::PathStrokeType(1.0f));
    }

    // Control point handle
    g.setColour(juce::Colour(0xff4fc3f7));
    g.fillEllipse(p1.x - 5.0f, p1.y - 5.0f, 10.0f, 10.0f);
    g.setColour(juce::Colours::white);
    g.drawEllipse(p1.x - 5.0f, p1.y - 5.0f, 10.0f, 10.0f, 1.0f);
}

void CrossfadeCurveEditor::mouseDown(const juce::MouseEvent& e)
{
    auto cp = toScreen(cpX, cpY);
    if (e.position.getDistanceFrom(cp) < 8.0f)
        dragging = true;
}

void CrossfadeCurveEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (!dragging)
        return;

    auto pt = fromScreen(e.position.x, e.position.y);
    cpX = juce::jlimit(0.05f, 0.95f, pt.x);
    cpY = juce::jlimit(0.05f, 0.95f, pt.y);
    repaint();

    if (onCurveChanged)
        onCurveChanged(cpX, cpY);
}

void CrossfadeCurveEditor::mouseUp(const juce::MouseEvent&)
{
    dragging = false;
}
