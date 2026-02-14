#pragma once

#include <JuceHeader.h>

class WaveformDisplay : public juce::Component,
                        public juce::ChangeListener,
                        private juce::Timer
{
public:
    WaveformDisplay(juce::AudioFormatManager& formatManager,
                    juce::AudioTransportSource& transportSource);
    ~WaveformDisplay() override;

    void setFile(const juce::File& file);
    void clear();

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override {}

    // ChangeListener override
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    void timerCallback() override;

    juce::AudioTransportSource& transport;
    juce::AudioThumbnailCache thumbnailCache { 5 };
    juce::AudioThumbnail thumbnail;

    bool fileLoaded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};
