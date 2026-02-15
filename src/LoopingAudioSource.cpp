#include "LoopingAudioSource.h"
#include <limits>

LoopingAudioSource::LoopingAudioSource(juce::PositionableAudioSource* src, bool deleteWhenRemoved)
    : source(src, deleteWhenRemoved)
{
}

LoopingAudioSource::~LoopingAudioSource() = default;

void LoopingAudioSource::setLoopRange(juce::int64 startSample, juce::int64 endSample)
{
    jassert(startSample >= 0 && endSample > startSample);
    loopStart.store(startSample);
    loopEnd.store(endSample);
}

void LoopingAudioSource::setLooping(bool shouldLoop)
{
    looping.store(shouldLoop);
}

void LoopingAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    source->prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void LoopingAudioSource::releaseResources()
{
    source->releaseResources();
}

void LoopingAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto pos = nextPlayPos.load();

    if (!looping.load())
    {
        source->setNextReadPosition(pos);
        source->getNextAudioBlock(bufferToFill);
        nextPlayPos.store(source->getNextReadPosition());
        return;
    }

    const auto lStart = loopStart.load();
    const auto lEnd   = loopEnd.load();

    if (lEnd <= lStart || lEnd <= 0)
    {
        source->setNextReadPosition(pos);
        source->getNextAudioBlock(bufferToFill);
        nextPlayPos.store(source->getNextReadPosition());
        return;
    }

    // Wrap position into loop range using modular arithmetic so that
    // linear positions from BufferingAudioSource map correctly into the loop.
    if (pos >= lEnd || pos < lStart)
    {
        const auto loopLen = lEnd - lStart;
        pos = lStart + ((pos - lStart) % loopLen);
        if (pos < lStart)
            pos = lStart;
    }

    int samplesRemaining = bufferToFill.numSamples;
    int destOffset = bufferToFill.startSample;

    while (samplesRemaining > 0)
    {
        const auto samplesUntilEnd = static_cast<int>(lEnd - pos);
        const auto samplesToRead = juce::jmin(samplesRemaining, samplesUntilEnd);

        juce::AudioSourceChannelInfo chunk(bufferToFill.buffer, destOffset, samplesToRead);
        source->setNextReadPosition(pos);
        source->getNextAudioBlock(chunk);

        pos += samplesToRead;
        destOffset += samplesToRead;
        samplesRemaining -= samplesToRead;

        // Wrap around to loop start
        if (pos >= lEnd)
            pos = lStart;
    }

    nextPlayPos.store(pos);
}

void LoopingAudioSource::setNextReadPosition(juce::int64 newPosition)
{
    nextPlayPos.store(newPosition);
}

juce::int64 LoopingAudioSource::getNextReadPosition() const
{
    return nextPlayPos.load();
}

juce::int64 LoopingAudioSource::getTotalLength() const
{
    if (looping.load())
        return std::numeric_limits<juce::int64>::max();

    return source->getTotalLength();
}
