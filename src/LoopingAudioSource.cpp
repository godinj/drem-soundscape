#include "LoopingAudioSource.h"
#include <limits>
#include <cmath>

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

void LoopingAudioSource::setCrossfadeSamples(int samples)
{
    crossfadeSamples.store(juce::jmax(0, samples));
}

void LoopingAudioSource::setCrossfadeCurve(float cx, float cy)
{
    curveX.store(juce::jlimit(0.05f, 0.95f, cx));
    curveY.store(juce::jlimit(0.05f, 0.95f, cy));
}

float LoopingAudioSource::solveBezierT(float cx, float x)
{
    const float a = 1.0f - 2.0f * cx;
    if (std::abs(a) < 1e-6f)
        return (cx > 1e-6f) ? x / (2.0f * cx) : x;

    const float disc = 4.0f * cx * cx + 4.0f * a * x;
    const float t = (-2.0f * cx + std::sqrt(std::max(0.0f, disc))) / (2.0f * a);
    return juce::jlimit(0.0f, 1.0f, t);
}

float LoopingAudioSource::evalBezierY(float cy, float t)
{
    return 2.0f * (1.0f - t) * t * cy + t * t;
}

void LoopingAudioSource::rebuildLUT()
{
    const float cx = curveX.load();
    const float cy = curveY.load();

    for (int i = 0; i <= kLUTSize; ++i)
    {
        const float x = static_cast<float>(i) / static_cast<float>(kLUTSize);
        const float t = solveBezierT(cx, x);
        fadeLUT[i] = evalBezierY(cy, t);
    }

    cachedCurveX = cx;
    cachedCurveY = cy;
}

void LoopingAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    source->prepareToPlay(samplesPerBlockExpected, sampleRate);
    headCache.setSize(2, samplesPerBlockExpected);
    headCacheLength = 0;
    cachedLoopStart = -1;
    cachedXfade = -1;
}

void LoopingAudioSource::releaseResources()
{
    source->releaseResources();
    headCache.setSize(0, 0);
    headCacheLength = 0;
    cachedLoopStart = -1;
    cachedXfade = -1;
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

    const auto loopLen = lEnd - lStart;

    // Clamp crossfade to at most half the loop length
    const int xfade = juce::jmin(crossfadeSamples.load(), static_cast<int>(loopLen / 2));
    const auto xfadeStart = lEnd - static_cast<juce::int64>(xfade);

    // Rebuild fade LUT when curve parameters change
    const float cx = curveX.load();
    const float cy = curveY.load();
    if (std::abs(cx - cachedCurveX) > 1e-7f || std::abs(cy - cachedCurveY) > 1e-7f)
        rebuildLUT();

    // Pre-cache the head region [lStart, lStart+xfade) when parameters change.
    // This avoids seeking the source back and forth during crossfade.
    if (xfade > 0 && (cachedLoopStart != lStart || cachedXfade != xfade))
    {
        if (headCache.getNumSamples() < xfade)
            headCache.setSize(2, xfade, false, false, true);
        headCache.clear(0, xfade);

        juce::AudioSourceChannelInfo cacheChunk(&headCache, 0, xfade);
        source->setNextReadPosition(lStart);
        source->getNextAudioBlock(cacheChunk);

        headCacheLength = xfade;
        cachedLoopStart = lStart;
        cachedXfade = xfade;
    }

    // Wrap position into loop range.
    // When crossfading, each loop iteration after the first skips the head
    // region [lStart, lStart+xfade) since it was already blended in during
    // the previous crossfade.  The effective loop length is loopLen - xfade.
    if (pos < lStart || pos >= lEnd)
    {
        if (xfade > 0 && pos >= lEnd)
        {
            const auto effectiveLen = loopLen - static_cast<juce::int64>(xfade);
            auto offset = (pos - lEnd) % effectiveLen;
            pos = lStart + static_cast<juce::int64>(xfade) + offset;
        }
        else
        {
            pos = lStart + ((pos - lStart) % loopLen);
            if (pos < lStart)
                pos = lStart;
        }
    }

    int samplesRemaining = bufferToFill.numSamples;
    int destOffset = bufferToFill.startSample;
    const int numChannels = bufferToFill.buffer->getNumChannels();

    while (samplesRemaining > 0)
    {
        if (xfade == 0 || pos < xfadeStart)
        {
            // --- Normal zone: read directly from source ---
            const auto boundary = (xfade > 0) ? xfadeStart : lEnd;
            const auto samplesUntilBoundary = static_cast<int>(boundary - pos);
            const auto samplesToRead = juce::jmin(samplesRemaining, samplesUntilBoundary);

            juce::AudioSourceChannelInfo chunk(bufferToFill.buffer, destOffset, samplesToRead);
            source->setNextReadPosition(pos);
            source->getNextAudioBlock(chunk);

            pos += samplesToRead;
            destOffset += samplesToRead;
            samplesRemaining -= samplesToRead;

            // If no crossfade, wrap at lEnd
            if (xfade == 0 && pos >= lEnd)
                pos = lStart;
        }
        else
        {
            // --- Crossfade zone: blend tail with cached head ---
            const auto samplesUntilEnd = static_cast<int>(lEnd - pos);
            const auto samplesToRead = juce::jmin(samplesRemaining, samplesUntilEnd);

            // Read tail audio into output buffer
            juce::AudioSourceChannelInfo tailChunk(bufferToFill.buffer, destOffset, samplesToRead);
            source->setNextReadPosition(pos);
            source->getNextAudioBlock(tailChunk);

            // Blend with pre-cached head audio using LUT
            const auto posInXfade = static_cast<int>(pos - xfadeStart);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* dest = bufferToFill.buffer->getWritePointer(ch, destOffset);
                const int cacheCh = juce::jmin(ch, headCache.getNumChannels() - 1);
                const auto* head = headCache.getReadPointer(cacheCh, posInXfade);

                for (int i = 0; i < samplesToRead; ++i)
                {
                    const float progress = static_cast<float>(posInXfade + i) / static_cast<float>(xfade);
                    const float scaledIdx = progress * static_cast<float>(kLUTSize);
                    const int idx = juce::jmin(static_cast<int>(scaledIdx), kLUTSize - 1);
                    const float frac = scaledIdx - static_cast<float>(idx);
                    const float fadeIn = fadeLUT[idx] + frac * (fadeLUT[idx + 1] - fadeLUT[idx]);
                    const float fadeOut = 1.0f - fadeIn;
                    dest[i] = dest[i] * fadeOut + head[i] * fadeIn;
                }
            }

            pos += samplesToRead;
            destOffset += samplesToRead;
            samplesRemaining -= samplesToRead;

            // Wrap: jump past the crossfade head region (already blended in)
            if (pos >= lEnd)
                pos = lStart + static_cast<juce::int64>(xfade);
        }
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
