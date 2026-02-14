#pragma once

#include <JuceHeader.h>
#include <atomic>

class LoopingAudioSource : public juce::PositionableAudioSource
{
public:
    explicit LoopingAudioSource(juce::PositionableAudioSource* source, bool deleteWhenRemoved);
    ~LoopingAudioSource() override;

    void setLoopRange(juce::int64 startSample, juce::int64 endSample);
    juce::int64 getLoopStart() const { return loopStart.load(); }
    juce::int64 getLoopEnd() const   { return loopEnd.load(); }

    void setLooping(bool shouldLoop);

    // PositionableAudioSource overrides
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void setNextReadPosition(juce::int64 newPosition) override;
    juce::int64 getNextReadPosition() const override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override { return looping.load(); }

private:
    juce::OptionalScopedPointer<juce::PositionableAudioSource> source;

    std::atomic<juce::int64> loopStart { 0 };
    std::atomic<juce::int64> loopEnd   { 0 };
    std::atomic<bool> looping          { true };

    std::atomic<juce::int64> nextPlayPos { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoopingAudioSource)
};
