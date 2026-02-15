#pragma once

#include <JuceHeader.h>

class CrossfadeCurveEditor : public juce::Component
{
public:
    CrossfadeCurveEditor();

    void setControlPoint(float cx, float cy);
    float getControlPointX() const { return cpX; }
    float getControlPointY() const { return cpY; }

    std::function<void(float, float)> onCurveChanged;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    float cpX = 0.25f, cpY = 0.75f;
    bool dragging = false;

    juce::Point<float> toScreen(float x, float y) const;
    juce::Point<float> fromScreen(float sx, float sy) const;
    float evalBezierY(float x) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossfadeCurveEditor)
};
