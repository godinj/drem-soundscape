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
    loadButton.onClick       = [this] { loadFile(); };
    savePresetButton.onClick = [this] { savePreset(); };
    loadPresetButton.onClick = [this] { loadPreset(); };
    playButton.onClick       = [this] { startPlayback(); };
    stopButton.onClick       = [this] { stopPlayback(); };

    addAndMakeVisible(loadButton);
    addAndMakeVisible(savePresetButton);
    addAndMakeVisible(loadPresetButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(waveformDisplay);

    playButton.setEnabled(false);
    stopButton.setEnabled(false);
    savePresetButton.setEnabled(false);

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
    savePresetButton.setBounds(toolbar.removeFromLeft(100));
    toolbar.removeFromLeft(8);
    loadPresetButton.setBounds(toolbar.removeFromLeft(100));
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

    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        loadFileFromChooser(chooser);
    });
}

void MainComponent::loadFileFromChooser(const juce::FileChooser& chooser)
{
    auto file = chooser.getResult();
    if (!file.existsAsFile())
        return;

    loadAudioFile(file, 0, -1);
}

void MainComponent::loadAudioFile(const juce::File& file, juce::int64 loopStart, juce::int64 loopEnd)
{
    // Stop current playback and tear down old chain
    transportSource.stop();
    transportSource.setSource(nullptr);
    loopingSource.reset();
    readerSource.reset();

    // Build new source chain
    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
        return;

    auto totalSamples = static_cast<juce::int64>(reader->lengthInSamples);
    if (loopEnd < 0 || loopEnd > totalSamples)
        loopEnd = totalSamples;
    if (loopStart < 0 || loopStart >= loopEnd)
        loopStart = 0;

    readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
    loopingSource = std::make_unique<LoopingAudioSource>(readerSource.get(), false);
    loopingSource->setLoopRange(loopStart, loopEnd);
    loopingSource->setLooping(true);

    transportSource.setSource(loopingSource.get(), 32768, &readAheadThread, reader->sampleRate);

    waveformDisplay.setSampleRate(reader->sampleRate);
    waveformDisplay.setLoopingSource(loopingSource.get());
    waveformDisplay.setFile(file);

    currentFilePath = file;
    playButton.setEnabled(true);
    stopButton.setEnabled(true);
    savePresetButton.setEnabled(true);
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

void MainComponent::savePreset()
{
    if (!currentFilePath.existsAsFile() || loopingSource == nullptr)
        return;

    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Preset...",
        juce::File{},
        "*.json");

    auto chooserFlags = juce::FileBrowserComponent::saveMode
                      | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file == juce::File{})
            return;

        auto* layer = new juce::DynamicObject();
        layer->setProperty("filePath", currentFilePath.getFullPathName());
        layer->setProperty("loopStart", loopingSource->getLoopStart());
        layer->setProperty("loopEnd", loopingSource->getLoopEnd());

        juce::Array<juce::var> layers;
        layers.add(juce::var(layer));

        auto* preset = new juce::DynamicObject();
        preset->setProperty("version", 1);
        preset->setProperty("layers", juce::var(layers));

        auto jsonString = juce::JSON::toString(juce::var(preset));
        file.replaceWithText(jsonString);
    });
}

void MainComponent::loadPreset()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Preset...",
        juce::File{},
        "*.json");

    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (!file.existsAsFile())
            return;

        auto jsonText = file.loadFileAsString();
        auto parsed = juce::JSON::parse(jsonText);

        if (!parsed.isObject())
            return;

        auto* obj = parsed.getDynamicObject();
        if (obj == nullptr)
            return;

        auto layersVar = obj->getProperty("layers");
        auto* layers = layersVar.getArray();
        if (layers == nullptr || layers->isEmpty())
            return;

        // Load the first layer (single-file support for now)
        auto layerVar = (*layers)[0];
        auto* layerObj = layerVar.getDynamicObject();
        if (layerObj == nullptr)
            return;

        auto filePath = layerObj->getProperty("filePath").toString();
        auto loopStart = static_cast<juce::int64>(layerObj->getProperty("loopStart"));
        auto loopEnd = static_cast<juce::int64>(layerObj->getProperty("loopEnd"));

        juce::File audioFile(filePath);
        if (!audioFile.existsAsFile())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Missing File",
                "Audio file not found:\n" + filePath);
            return;
        }

        loadAudioFile(audioFile, loopStart, loopEnd);
    });
}
