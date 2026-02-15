#pragma once

#include <JuceHeader.h>
#include "LoopingAudioSource.h"
#include "WaveformDisplay.h"

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void resized() override;

private:
    void loadFile();
    void startPlayback();
    void stopPlayback();
    void loadFileFromChooser(const juce::FileChooser& chooser);
    void savePreset();
    void loadPreset();
    void loadAudioFile(const juce::File& file, juce::int64 loopStart, juce::int64 loopEnd);

    // Audio infrastructure
    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    juce::TimeSliceThread readAheadThread { "audio-read-ahead" };

    // Source chain (owned bottom-up, destroyed top-down)
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::unique_ptr<LoopingAudioSource> loopingSource;
    juce::AudioTransportSource transportSource;
    juce::AudioSourcePlayer audioSourcePlayer;

    // GUI
    juce::TextButton loadButton       { "Load File" };
    juce::TextButton savePresetButton { "Save Preset" };
    juce::TextButton loadPresetButton { "Load Preset" };
    juce::TextButton playButton       { "Play" };
    juce::TextButton stopButton       { "Stop" };
    WaveformDisplay waveformDisplay;

    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File currentFilePath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
