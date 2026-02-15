#include "WaveformDisplay.h"
#include "LoopingAudioSource.h"

WaveformDisplay::WaveformDisplay(juce::AudioFormatManager& formatManager)
    : thumbnail(512, formatManager, thumbnailCache)
{
    thumbnail.addChangeListener(this);
}

void WaveformDisplay::setTransportSource(juce::AudioTransportSource* source)
{
    transport = source;
}

WaveformDisplay::~WaveformDisplay()
{
    thumbnail.removeChangeListener(this);
    stopTimer();
}

void WaveformDisplay::setFile(const juce::File& file)
{
    thumbnail.setSource(new juce::FileInputSource(file));
    fileLoaded = true;
    startTimerHz(30);
    repaint();
}

void WaveformDisplay::clear()
{
    thumbnail.clear();
    fileLoaded = false;
    loopingSource = nullptr;
    sampleRate = 0.0;
    stopTimer();
    repaint();
}

void WaveformDisplay::setLoopingSource(LoopingAudioSource* source)
{
    loopingSource = source;
    repaint();
}

void WaveformDisplay::setSampleRate(double rate)
{
    sampleRate = rate;
}

float WaveformDisplay::sampleToX(juce::int64 sample) const
{
    const double totalSeconds = thumbnail.getTotalLength();
    if (totalSeconds <= 0.0 || sampleRate <= 0.0)
        return 0.0f;

    const double totalSamples = totalSeconds * sampleRate;
    return static_cast<float>((static_cast<double>(sample) / totalSamples) * getWidth());
}

juce::int64 WaveformDisplay::xToSample(float x) const
{
    const double totalSeconds = thumbnail.getTotalLength();
    if (totalSeconds <= 0.0 || sampleRate <= 0.0)
        return 0;

    const double totalSamples = totalSeconds * sampleRate;
    const double ratio = static_cast<double>(x) / getWidth();
    return static_cast<juce::int64>(ratio * totalSamples);
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.fillAll(juce::Colour(0xff1e1e2e));

    if (!fileLoaded || thumbnail.getTotalLength() <= 0.0)
    {
        g.setColour(juce::Colours::grey);
        g.setFont(15.0f);
        g.drawFittedText("No file loaded", getLocalBounds(), juce::Justification::centred, 1);
        return;
    }

    // Draw waveform
    g.setColour(juce::Colour(0xff94e2d5));
    thumbnail.drawChannels(g, getLocalBounds(), 0.0, thumbnail.getTotalLength(), 1.0f);

    // Draw loop region overlay
    if (loopingSource != nullptr && sampleRate > 0.0)
    {
        const float startX = sampleToX(loopingSource->getLoopStart());
        const float endX   = sampleToX(loopingSource->getLoopEnd());

        // Dim non-loop regions
        g.setColour(juce::Colour(0x80000000));
        g.fillRect(bounds.getX(), bounds.getY(), startX, bounds.getHeight());
        g.fillRect(endX, bounds.getY(), bounds.getRight() - endX, bounds.getHeight());

        // Loop start line (green) and handle
        g.setColour(juce::Colour(0xffa6e3a1));
        g.fillRect(startX - 1.0f, bounds.getY(), 2.0f, bounds.getHeight());
        g.fillRect(startX - 4.0f, bounds.getY(), 8.0f, 10.0f);

        // Loop end line (red) and handle
        g.setColour(juce::Colour(0xfff38ba8));
        g.fillRect(endX - 1.0f, bounds.getY(), 2.0f, bounds.getHeight());
        g.fillRect(endX - 4.0f, bounds.getY(), 8.0f, 10.0f);

        // Crossfade zone overlays
        const int xfadeSamples = loopingSource->getCrossfadeSamples();
        if (xfadeSamples > 0)
        {
            const float xfadeHeadEndX = sampleToX(loopingSource->getLoopStart() + xfadeSamples);
            const float xfadeTailStartX = sampleToX(loopingSource->getLoopEnd() - xfadeSamples);

            // Head zone overlay (blue tint at loop start)
            g.setColour(juce::Colour(0x3089b4fa));
            g.fillRect(startX, bounds.getY(), xfadeHeadEndX - startX, bounds.getHeight());

            // Tail zone overlay (blue tint at loop end)
            g.fillRect(xfadeTailStartX, bounds.getY(), endX - xfadeTailStartX, bounds.getHeight());

            // Inner boundary lines
            g.setColour(juce::Colour(0x6089b4fa));
            g.drawLine(xfadeHeadEndX, bounds.getY(), xfadeHeadEndX, bounds.getBottom(), 1.0f);
            g.drawLine(xfadeTailStartX, bounds.getY(), xfadeTailStartX, bounds.getBottom(), 1.0f);
        }
    }

    // Draw playhead — wrap the transport's linear position into the loop region
    if (transport != nullptr && loopingSource != nullptr && sampleRate > 0.0
        && (transport->isPlaying() || transport->getCurrentPosition() > 0.0))
    {
        const auto posSamples = static_cast<juce::int64>(transport->getCurrentPosition() * sampleRate);
        const auto lStart = loopingSource->getLoopStart();
        const auto lEnd   = loopingSource->getLoopEnd();
        const auto loopLen = lEnd - lStart;

        if (loopLen > 0)
        {
            juce::int64 wrapped;
            const int xfadeSamps = loopingSource->getCrossfadeSamples();

            if (xfadeSamps > 0 && posSamples >= lEnd)
            {
                const auto effectiveLen = loopLen - static_cast<juce::int64>(xfadeSamps);
                auto offset = (posSamples - lEnd) % effectiveLen;
                wrapped = lStart + static_cast<juce::int64>(xfadeSamps) + offset;
            }
            else
            {
                wrapped = lStart + ((posSamples - lStart) % loopLen);
                if (wrapped < lStart)
                    wrapped = lStart;
            }

            const auto xfadeStart = lEnd - static_cast<juce::int64>(xfadeSamps);

            if (xfadeSamps > 0 && wrapped >= xfadeStart)
            {
                const float progress = static_cast<float>(wrapped - xfadeStart)
                                     / static_cast<float>(xfadeSamps);
                const float tailAlpha = juce::jmax(0.15f, 1.0f - progress);
                const float headAlpha = juce::jmax(0.15f, progress);

                // Tail playhead (fading out)
                const float tailX = sampleToX(wrapped);
                g.setColour(juce::Colours::white.withAlpha(tailAlpha));
                g.drawLine(tailX, bounds.getY(), tailX, bounds.getBottom(), 2.0f);

                // Head playhead (fading in)
                const auto headPos = lStart + (wrapped - xfadeStart);
                const float headX = sampleToX(headPos);
                g.setColour(juce::Colours::white.withAlpha(headAlpha));
                g.drawLine(headX, bounds.getY(), headX, bounds.getBottom(), 2.0f);
            }
            else
            {
                const float x = sampleToX(wrapped);
                g.setColour(juce::Colours::white);
                g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 2.0f);
            }
        }
    }
}

void WaveformDisplay::mouseDown(const juce::MouseEvent& event)
{
    if (loopingSource == nullptr || sampleRate <= 0.0)
        return;

    const float mx = static_cast<float>(event.x);
    const float startX = sampleToX(loopingSource->getLoopStart());
    const float endX   = sampleToX(loopingSource->getLoopEnd());

    if (std::abs(mx - startX) <= handleHitRadius)
        dragging = DragTarget::Start;
    else if (std::abs(mx - endX) <= handleHitRadius)
        dragging = DragTarget::End;
    else
        dragging = DragTarget::None;

    if (dragging != DragTarget::None)
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& event)
{
    if (dragging == DragTarget::None || loopingSource == nullptr || sampleRate <= 0.0)
        return;

    const double totalSeconds = thumbnail.getTotalLength();
    const auto totalSamples = static_cast<juce::int64>(totalSeconds * sampleRate);

    const float clampedX = juce::jlimit(0.0f, static_cast<float>(getWidth()), static_cast<float>(event.x));
    juce::int64 sample = xToSample(clampedX);

    juce::int64 loopStart = loopingSource->getLoopStart();
    juce::int64 loopEnd   = loopingSource->getLoopEnd();

    const juce::int64 minLoopSamples = juce::jmax(
        static_cast<juce::int64>(256),
        static_cast<juce::int64>(loopingSource->getCrossfadeSamples() * 2));

    if (dragging == DragTarget::Start)
    {
        sample = juce::jlimit(static_cast<juce::int64>(0), loopEnd - minLoopSamples, sample);
        loopingSource->setLoopRange(sample, loopEnd);
    }
    else
    {
        sample = juce::jlimit(loopStart + minLoopSamples, totalSamples, sample);
        loopingSource->setLoopRange(loopStart, sample);
    }

    repaint();
}

void WaveformDisplay::mouseUp(const juce::MouseEvent&)
{
    dragging = DragTarget::None;
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void WaveformDisplay::mouseMove(const juce::MouseEvent& event)
{
    if (loopingSource == nullptr || sampleRate <= 0.0)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    const float mx = static_cast<float>(event.x);
    const float startX = sampleToX(loopingSource->getLoopStart());
    const float endX   = sampleToX(loopingSource->getLoopEnd());

    if (std::abs(mx - startX) <= handleHitRadius || std::abs(mx - endX) <= handleHitRadius)
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

void WaveformDisplay::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    repaint();
}

void WaveformDisplay::timerCallback()
{
    repaint();
}
