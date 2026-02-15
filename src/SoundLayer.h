#pragma once

#include <JuceHeader.h>
#include "LoopingAudioSource.h"
#include "WaveformDisplay.h"
#include "CrossfadeCurveEditor.h"

class SoundLayer : public juce::Component
{
public:
    SoundLayer(juce::AudioFormatManager& formatManager, juce::TimeSliceThread& readAheadThread);
    ~SoundLayer() override;

    bool loadFile(const juce::File& file, juce::int64 loopStart, juce::int64 loopEnd,
                  int crossfadeSamples = 0, float curveX = 0.25f, float curveY = 0.75f);
    int getCrossfadeSamples() const;
    float getCrossfadeCurveX() const;
    float getCrossfadeCurveY() const;

    void startPlayback();
    void stopPlayback();

    juce::AudioTransportSource& getTransportSource() { return transportSource; }
    const juce::AudioTransportSource& getTransportSource() const { return transportSource; }
    LoopingAudioSource* getLoopingSource() { return loopingSource.get(); }
    const juce::File& getFilePath() const { return filePath; }
    bool isFileLoaded() const { return readerSource != nullptr; }

    std::function<void(SoundLayer*)> onRemove;

    void resized() override;

private:
    juce::AudioFormatManager& formatManager;
    juce::TimeSliceThread& readAheadThread;

    // Audio chain
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::unique_ptr<LoopingAudioSource> loopingSource;
    juce::AudioTransportSource transportSource;

    // GUI
    WaveformDisplay waveformDisplay;
    juce::TextButton removeButton { "X" };
    juce::Slider crossfadeSlider;
    juce::Label crossfadeLabel { {}, "XFade" };
    CrossfadeCurveEditor curveEditor;

    // State
    juce::File filePath;
    double fileSampleRate = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundLayer)
};
