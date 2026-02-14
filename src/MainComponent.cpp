#include "MainComponent.h"

MainComponent::MainComponent()
    : waveformDisplay(formatManager, transportSource)
{
    formatManager.registerBasicFormats();
    readAheadThread.startThread(juce::Thread::Priority::normal);

    auto result = deviceManager.initialiseWithDefaultDevices(0, 2);
    if (result.isNotEmpty())
        juce::Logger::writeToLog("Audio device error: " + result);

    deviceManager.addAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(&transportSource);

    // Toolbar buttons
    loadButton.onClick = [this] { loadFile(); };
    playButton.onClick = [this] { startPlayback(); };
    stopButton.onClick = [this] { stopPlayback(); };

    addAndMakeVisible(loadButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(waveformDisplay);

    playButton.setEnabled(false);
    stopButton.setEnabled(false);

    setSize(800, 600);
}

MainComponent::~MainComponent()
{
    // Shutdown in reverse order of the audio chain
    transportSource.stop();
    audioSourcePlayer.setSource(nullptr);
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    transportSource.setSource(nullptr);
    loopingSource.reset();
    readerSource.reset();
    readAheadThread.stopThread(500);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Toolbar row
    auto toolbar = area.removeFromTop(36);
    loadButton.setBounds(toolbar.removeFromLeft(100));
    toolbar.removeFromLeft(8);
    playButton.setBounds(toolbar.removeFromLeft(80));
    toolbar.removeFromLeft(8);
    stopButton.setBounds(toolbar.removeFromLeft(80));

    area.removeFromTop(10);

    // Waveform fills the rest
    waveformDisplay.setBounds(area);
}

void MainComponent::loadFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select an audio file...",
        juce::File{},
        formatManager.getWildcardForAllFormats());

    auto flags = juce::FileBrowserComponent::openMode
               | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
    {
        loadFileFromChooser(chooser);
    });
}

void MainComponent::loadFileFromChooser(const juce::FileChooser& chooser)
{
    auto file = chooser.getResult();
    if (!file.existsAsFile())
        return;

    // Stop current playback and tear down old chain
    transportSource.stop();
    transportSource.setSource(nullptr);
    loopingSource.reset();
    readerSource.reset();

    // Build new source chain
    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
        return;

    readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
    loopingSource = std::make_unique<LoopingAudioSource>(readerSource.get(), false);
    loopingSource->setLoopRange(0, static_cast<juce::int64>(reader->lengthInSamples));
    loopingSource->setLooping(true);

    transportSource.setSource(loopingSource.get(), 32768, &readAheadThread, reader->sampleRate);

    waveformDisplay.setFile(file);

    playButton.setEnabled(true);
    stopButton.setEnabled(true);
}

void MainComponent::startPlayback()
{
    if (readerSource != nullptr)
        transportSource.start();
}

void MainComponent::stopPlayback()
{
    transportSource.stop();
}
