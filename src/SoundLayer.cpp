#include "SoundLayer.h"

SoundLayer::SoundLayer(juce::AudioFormatManager& fm, juce::TimeSliceThread& thread)
    : formatManager(fm),
      readAheadThread(thread),
      waveformDisplay(fm)
{
    waveformDisplay.setTransportSource(&transportSource);

    removeButton.onClick = [this] {
        if (onRemove)
        {
            auto callback = onRemove;
            auto safeThis = juce::Component::SafePointer<SoundLayer>(this);
            juce::MessageManager::callAsync([callback, safeThis] {
                if (auto* self = safeThis.getComponent())
                    callback(self);
            });
        }
    };

    crossfadeSlider.setRange(0.0, 5000.0, 1.0);
    crossfadeSlider.setValue(0.0, juce::dontSendNotification);
    crossfadeSlider.setTextValueSuffix(" ms");
    crossfadeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    crossfadeSlider.onValueChange = [this] {
        if (loopingSource != nullptr && fileSampleRate > 0.0)
        {
            const int samples = static_cast<int>(crossfadeSlider.getValue() * fileSampleRate / 1000.0);
            loopingSource->setCrossfadeSamples(samples);
            waveformDisplay.repaint();
        }
    };

    crossfadeLabel.setJustificationType(juce::Justification::centredRight);
    crossfadeLabel.attachToComponent(&crossfadeSlider, true);

    volumeKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    volumeKnob.setRange(0.0, 1.5, 0.01);
    volumeKnob.setValue(1.0, juce::dontSendNotification);
    volumeKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 14);
    volumeKnob.onValueChange = [this] {
        transportSource.setGain(static_cast<float>(volumeKnob.getValue()));
    };

    volumeLabel.setJustificationType(juce::Justification::centred);

    curveEditor.onCurveChanged = [this](float cx, float cy) {
        if (loopingSource != nullptr)
            loopingSource->setCrossfadeCurve(cx, cy);
    };

    addAndMakeVisible(waveformDisplay);
    addAndMakeVisible(removeButton);
    addAndMakeVisible(volumeKnob);
    addAndMakeVisible(volumeLabel);
    addAndMakeVisible(crossfadeSlider);
    addAndMakeVisible(crossfadeLabel);
    addAndMakeVisible(curveEditor);
}

SoundLayer::~SoundLayer()
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    loopingSource.reset();
    readerSource.reset();
}

bool SoundLayer::loadFile(const juce::File& file, juce::int64 loopStart, juce::int64 loopEnd,
                          int crossfadeSamples, float curveX, float curveY)
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    loopingSource.reset();
    readerSource.reset();

    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
        return false;

    fileSampleRate = reader->sampleRate;

    auto totalSamples = static_cast<juce::int64>(reader->lengthInSamples);
    if (loopEnd < 0 || loopEnd > totalSamples)
        loopEnd = totalSamples;
    if (loopStart < 0 || loopStart >= loopEnd)
        loopStart = 0;

    readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
    loopingSource = std::make_unique<LoopingAudioSource>(readerSource.get(), false);
    loopingSource->setLoopRange(loopStart, loopEnd);
    loopingSource->setLooping(true);
    loopingSource->setCrossfadeSamples(crossfadeSamples);
    loopingSource->setCrossfadeCurve(curveX, curveY);

    curveEditor.setControlPoint(curveX, curveY);

    transportSource.setSource(loopingSource.get(), 32768, &readAheadThread, reader->sampleRate);

    waveformDisplay.setSampleRate(reader->sampleRate);
    waveformDisplay.setLoopingSource(loopingSource.get());
    waveformDisplay.setFile(file);

    // Set slider to match loaded crossfade value
    if (fileSampleRate > 0.0)
        crossfadeSlider.setValue(static_cast<double>(crossfadeSamples) / fileSampleRate * 1000.0, juce::dontSendNotification);
    else
        crossfadeSlider.setValue(0.0, juce::dontSendNotification);

    filePath = file;
    return true;
}

int SoundLayer::getCrossfadeSamples() const
{
    if (loopingSource != nullptr)
        return loopingSource->getCrossfadeSamples();
    return 0;
}

float SoundLayer::getCrossfadeCurveX() const
{
    if (loopingSource != nullptr)
        return loopingSource->getCurveX();
    return curveEditor.getControlPointX();
}

float SoundLayer::getCrossfadeCurveY() const
{
    if (loopingSource != nullptr)
        return loopingSource->getCurveY();
    return curveEditor.getControlPointY();
}

float SoundLayer::getVolume() const
{
    return transportSource.getGain();
}

void SoundLayer::setVolume(float v)
{
    volumeKnob.setValue(static_cast<double>(v), juce::dontSendNotification);
    transportSource.setGain(v);
}

void SoundLayer::startPlayback()
{
    if (readerSource != nullptr)
        transportSource.start();
}

void SoundLayer::stopPlayback()
{
    transportSource.stop();
}

void SoundLayer::resized()
{
    auto area = getLocalBounds();
    removeButton.setBounds(area.removeFromLeft(30));

    auto controlStrip = area.removeFromBottom(68);
    curveEditor.setBounds(controlStrip.removeFromLeft(64).reduced(2));

    auto volumeArea = controlStrip.removeFromLeft(50);
    volumeKnob.setBounds(volumeArea.removeFromTop(50));
    volumeLabel.setBounds(volumeArea);

    controlStrip.removeFromLeft(50); // space for XFade label
    crossfadeSlider.setBounds(controlStrip);

    waveformDisplay.setBounds(area);
}
