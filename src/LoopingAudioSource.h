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

    void setLooping(bool shouldLoop) override;

    void setCrossfadeSamples(int samples);
    int getCrossfadeSamples() const { return crossfadeSamples.load(); }

    void setCrossfadeCurve(float cx, float cy);
    float getCurveX() const { return curveX.load(); }
    float getCurveY() const { return curveY.load(); }

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

    std::atomic<int> crossfadeSamples { 0 };
    juce::AudioBuffer<float> headCache;
    int headCacheLength = 0;
    juce::int64 cachedLoopStart = -1;
    int cachedXfade = -1;

    std::atomic<float> curveX { 0.25f };
    std::atomic<float> curveY { 0.75f };

    static constexpr int kLUTSize = 256;
    float fadeLUT[kLUTSize + 1];
    float cachedCurveX = -1.0f;
    float cachedCurveY = -1.0f;

    void rebuildLUT();
    static float solveBezierT(float cx, float x);
    static float evalBezierY(float cy, float t);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoopingAudioSource)
};
