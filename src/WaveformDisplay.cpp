#include "WaveformDisplay.h"

WaveformDisplay::WaveformDisplay(juce::AudioFormatManager& formatManager,
                                 juce::AudioTransportSource& transportSource)
    : transport(transportSource),
      thumbnail(512, formatManager, thumbnailCache)
{
    thumbnail.addChangeListener(this);
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
    stopTimer();
    repaint();
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

    // Draw playhead
    if (transport.isPlaying() || transport.getCurrentPosition() > 0.0)
    {
        const double totalLength = thumbnail.getTotalLength();
        if (totalLength > 0.0)
        {
            const double pos = transport.getCurrentPosition();
            // Wrap position into file length (since LoopingAudioSource may report large positions)
            const double wrappedPos = std::fmod(pos, totalLength);
            const float x = static_cast<float>(wrappedPos / totalLength) * bounds.getWidth();

            g.setColour(juce::Colours::white);
            g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 2.0f);
        }
    }
}

void WaveformDisplay::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    repaint();
}

void WaveformDisplay::timerCallback()
{
    repaint();
}
