#pragma once

#include <JuceHeader.h>

class FilteredAudioSource : public juce::AudioSource
{
public:
    explicit FilteredAudioSource(juce::AudioSource* source);

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void setCutoffFrequency(float hz);

private:
    juce::AudioSource* source;
    juce::dsp::StateVariableTPTFilter<float> filter;
    std::atomic<float> cutoffHz { 20.0f };
    double currentSampleRate = 44100.0;
};
