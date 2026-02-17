#pragma once

#include <JuceHeader.h>

class LoopingAudioSource;

class WaveformDisplay : public juce::Component,
                        public juce::ChangeListener,
                        private juce::Timer
{
public:
    explicit WaveformDisplay(juce::AudioFormatManager& formatManager);
    ~WaveformDisplay() override;

    void setTransportSource(juce::AudioTransportSource* source);
    void setFile(const juce::File& file);
    void clear();

    void setLoopingSource(LoopingAudioSource* source);
    void setSampleRate(double rate);

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override {}
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

    // ChangeListener override
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    void timerCallback() override;
    float sampleToX(juce::int64 sample) const;
    juce::int64 xToSample(float x) const;

    enum class DragTarget { None, Start, End };

    juce::AudioTransportSource* transport = nullptr;
    juce::AudioThumbnailCache thumbnailCache { 5 };
    juce::AudioThumbnail thumbnail;

    bool fileLoaded = false;
    LoopingAudioSource* loopingSource = nullptr;
    double sampleRate = 0.0;
    DragTarget dragging = DragTarget::None;

    static constexpr float handleHitRadius = 8.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};
