#pragma once

#include <JuceHeader.h>
#include "SoundLayer.h"
#include "FilteredAudioSource.h"

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    bool isPlaying() const;
    void addFiles();
    void addLayer(const juce::File& file, juce::int64 loopStart, juce::int64 loopEnd,
                  int crossfadeSamples = 0, float curveX = 0.25f, float curveY = 0.75f,
                  float volume = 1.0f);
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

    // Mixer, filter, and player
    juce::MixerAudioSource mixer;
    FilteredAudioSource filteredOutput { &mixer };
    juce::AudioSourcePlayer audioSourcePlayer;

    // Layers
    juce::OwnedArray<SoundLayer> layers;

    // GUI
    juce::TextButton addFileButton    { "Add File" };
    juce::TextButton savePresetButton { "Save Preset" };
    juce::TextButton loadPresetButton { "Load Preset" };
    juce::TextButton playButton       { "Play" };
    juce::TextButton stopButton       { "Stop" };

    juce::Slider hpfCutoffKnob;
    juce::Label hpfCutoffLabel { {}, "HPF" };

    juce::Slider masterVolumeKnob;
    juce::Label masterVolumeLabel { {}, "Master" };

    juce::Viewport viewport;
    juce::Component layerContainer;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::FileChooser> missingFileChooser;

    struct PendingLayer
    {
        juce::String originalPath;
        juce::int64 loopStart;
        juce::int64 loopEnd;
        int crossfadeSamples;
        float curveX;
        float curveY;
        float volume;
    };

    std::vector<PendingLayer> pendingMissingLayers;
    int pendingLayerIndex = 0;

    void processNextMissingLayer();

    static constexpr int layerHeight = 188;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
