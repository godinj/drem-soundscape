#pragma once

#include <JuceHeader.h>
#include "SoundLayer.h"

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void resized() override;

private:
    void addFiles();
    void addLayer(const juce::File& file, juce::int64 loopStart, juce::int64 loopEnd,
                  int crossfadeSamples = 0, float curveX = 0.25f, float curveY = 0.75f);
    void removeLayer(SoundLayer* layer);
    void layoutLayers();
    void startPlayback();
    void stopPlayback();
    void savePreset();
    void loadPreset();

    // Audio infrastructure
    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    juce::TimeSliceThread readAheadThread { "audio-read-ahead" };

    // Mixer and player
    juce::MixerAudioSource mixer;
    juce::AudioSourcePlayer audioSourcePlayer;

    // Layers
    juce::OwnedArray<SoundLayer> layers;

    // GUI
    juce::TextButton addFileButton    { "Add File" };
    juce::TextButton savePresetButton { "Save Preset" };
    juce::TextButton loadPresetButton { "Load Preset" };
    juce::TextButton playButton       { "Play" };
    juce::TextButton stopButton       { "Stop" };

    juce::Viewport viewport;
    juce::Component layerContainer;

    std::unique_ptr<juce::FileChooser> fileChooser;

    static constexpr int layerHeight = 188;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
