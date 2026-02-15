#include "MainComponent.h"

MainComponent::MainComponent()
{
    formatManager.registerBasicFormats();
    readAheadThread.startThread(juce::Thread::Priority::normal);

    auto result = deviceManager.initialiseWithDefaultDevices(0, 2);
    if (result.isNotEmpty())
        juce::Logger::writeToLog("Audio device error: " + result);

    deviceManager.addAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(&filteredOutput);

    // Toolbar buttons
    addFileButton.onClick    = [this] { addFiles(); };
    savePresetButton.onClick = [this] { savePreset(); };
    loadPresetButton.onClick = [this] { loadPreset(); };
    playButton.onClick       = [this] { startPlayback(); };
    stopButton.onClick       = [this] { stopPlayback(); };

    masterVolumeKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    masterVolumeKnob.setRange(0.0, 1.5, 0.01);
    masterVolumeKnob.setValue(1.0, juce::dontSendNotification);
    masterVolumeKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    masterVolumeKnob.setDoubleClickReturnValue(true, 1.0);
    masterVolumeKnob.onValueChange = [this] {
        audioSourcePlayer.setGain(static_cast<float>(masterVolumeKnob.getValue()));
    };

    hpfCutoffKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    hpfCutoffKnob.setRange(20.0, 2000.0, 1.0);
    hpfCutoffKnob.setValue(20.0, juce::dontSendNotification);
    hpfCutoffKnob.setSkewFactorFromMidPoint(200.0);
    hpfCutoffKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    hpfCutoffKnob.setDoubleClickReturnValue(true, 20.0);
    hpfCutoffKnob.onValueChange = [this] {
        filteredOutput.setCutoffFrequency(static_cast<float>(hpfCutoffKnob.getValue()));
    };

    hpfCutoffLabel.setJustificationType(juce::Justification::centred);
    masterVolumeLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(addFileButton);
    addAndMakeVisible(savePresetButton);
    addAndMakeVisible(loadPresetButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(hpfCutoffKnob);
    addAndMakeVisible(hpfCutoffLabel);
    addAndMakeVisible(masterVolumeKnob);
    addAndMakeVisible(masterVolumeLabel);

    viewport.setViewedComponent(&layerContainer, false);
    addAndMakeVisible(viewport);

    playButton.setEnabled(false);
    stopButton.setEnabled(false);
    savePresetButton.setEnabled(false);

    setWantsKeyboardFocus(true);
    setSize(800, 600);
}

MainComponent::~MainComponent()
{
    // Stop all transports and remove from mixer
    for (auto* layer : layers)
    {
        layer->stopPlayback();
        mixer.removeInputSource(&layer->getTransportSource());
    }

    layers.clear();

    audioSourcePlayer.setSource(nullptr);
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    mixer.removeAllInputs();
    readAheadThread.stopThread(500);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Toolbar row
    auto toolbar = area.removeFromTop(80);

    // Master volume knob — far right
    auto masterKnobArea = toolbar.removeFromRight(70);
    masterVolumeKnob.setBounds(masterKnobArea);
    auto masterLabelArea = toolbar.removeFromRight(50);
    masterVolumeLabel.setBounds(masterLabelArea.withHeight(20).withY(masterKnobArea.getCentreY() - 10));

    // HPF cutoff knob — left of master volume
    auto hpfKnobArea = toolbar.removeFromRight(70);
    hpfCutoffKnob.setBounds(hpfKnobArea);
    auto hpfLabelArea = toolbar.removeFromRight(50);
    hpfCutoffLabel.setBounds(hpfLabelArea.withHeight(20).withY(hpfKnobArea.getCentreY() - 10));

    // Buttons — left side
    addFileButton.setBounds(toolbar.removeFromLeft(100).withHeight(36));
    toolbar.removeFromLeft(8);
    savePresetButton.setBounds(toolbar.removeFromLeft(100).withHeight(36));
    toolbar.removeFromLeft(8);
    loadPresetButton.setBounds(toolbar.removeFromLeft(100).withHeight(36));
    toolbar.removeFromLeft(8);
    playButton.setBounds(toolbar.removeFromLeft(80).withHeight(36));
    toolbar.removeFromLeft(8);
    stopButton.setBounds(toolbar.removeFromLeft(80).withHeight(36));

    area.removeFromTop(10);

    // Viewport fills the rest
    viewport.setBounds(area);
    layoutLayers();
}

void MainComponent::addFiles()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select audio file(s)...",
        juce::File{},
        formatManager.getWildcardForAllFormats());

    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles
                      | juce::FileBrowserComponent::canSelectMultipleItems;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        auto results = chooser.getResults();
        for (const auto& file : results)
        {
            if (file.existsAsFile())
                addLayer(file, 0, -1);
        }
    });
}

void MainComponent::addLayer(const juce::File& file, juce::int64 loopStart, juce::int64 loopEnd,
                             int crossfadeSamples, float curveX, float curveY,
                             float volume)
{
    auto* layer = new SoundLayer(formatManager, readAheadThread);

    // Add to mixer first so the transport is prepared (matching the original
    // code path where audioSourcePlayer prepared the transport before setSource).
    mixer.addInputSource(&layer->getTransportSource(), false);

    if (!layer->loadFile(file, loopStart, loopEnd, crossfadeSamples, curveX, curveY))
    {
        mixer.removeInputSource(&layer->getTransportSource());
        delete layer;
        return;
    }

    layer->setVolume(volume);
    layer->onRemove = [this](SoundLayer* l) { removeLayer(l); };

    layerContainer.addAndMakeVisible(layer);
    layers.add(layer);

    layoutLayers();

    playButton.setEnabled(true);
    stopButton.setEnabled(true);
    savePresetButton.setEnabled(true);
}

void MainComponent::removeLayer(SoundLayer* layer)
{
    if (layer == nullptr)
        return;

    layer->stopPlayback();
    mixer.removeInputSource(&layer->getTransportSource());
    layerContainer.removeChildComponent(layer);
    layers.removeObject(layer, true);

    layoutLayers();

    if (layers.isEmpty())
    {
        playButton.setEnabled(false);
        stopButton.setEnabled(false);
        savePresetButton.setEnabled(false);
    }
}

void MainComponent::layoutLayers()
{
    int totalHeight = layers.size() * layerHeight;
    int width = viewport.getMaximumVisibleWidth();
    if (width <= 0)
        width = viewport.getWidth();

    layerContainer.setSize(width, juce::jmax(1, totalHeight));

    for (int i = 0; i < layers.size(); ++i)
        layers[i]->setBounds(0, i * layerHeight, width, layerHeight);
}

bool MainComponent::isPlaying() const
{
    for (auto* layer : layers)
        if (layer->getTransportSource().isPlaying())
            return true;

    return false;
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
    {
        if (layers.isEmpty())
            return true;

        if (isPlaying())
            stopPlayback();
        else
            startPlayback();

        return true;
    }

    return false;
}

void MainComponent::startPlayback()
{
    for (auto* layer : layers)
        layer->startPlayback();
}

void MainComponent::stopPlayback()
{
    for (auto* layer : layers)
        layer->stopPlayback();
}

void MainComponent::savePreset()
{
    if (layers.isEmpty())
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

        juce::Array<juce::var> layersArray;

        for (auto* layer : layers)
        {
            auto* layerObj = new juce::DynamicObject();
            layerObj->setProperty("filePath", layer->getFilePath().getFullPathName());

            if (auto* loop = layer->getLoopingSource())
            {
                layerObj->setProperty("loopStart", loop->getLoopStart());
                layerObj->setProperty("loopEnd", loop->getLoopEnd());
                layerObj->setProperty("crossfadeSamples", loop->getCrossfadeSamples());
            }

            layerObj->setProperty("crossfadeCurveX", static_cast<double>(layer->getCrossfadeCurveX()));
            layerObj->setProperty("crossfadeCurveY", static_cast<double>(layer->getCrossfadeCurveY()));
            layerObj->setProperty("volume", static_cast<double>(layer->getVolume()));

            layersArray.add(juce::var(layerObj));
        }

        auto* preset = new juce::DynamicObject();
        preset->setProperty("version", 1);
        preset->setProperty("masterVolume", masterVolumeKnob.getValue());
        preset->setProperty("hpfCutoff", hpfCutoffKnob.getValue());
        preset->setProperty("layers", juce::var(layersArray));

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

        if (obj->hasProperty("masterVolume"))
        {
            auto mv = static_cast<float>(static_cast<double>(obj->getProperty("masterVolume")));
            masterVolumeKnob.setValue(static_cast<double>(mv), juce::dontSendNotification);
            audioSourcePlayer.setGain(mv);
        }

        {
            auto cutoff = obj->hasProperty("hpfCutoff")
                ? static_cast<float>(static_cast<double>(obj->getProperty("hpfCutoff")))
                : 20.0f;
            hpfCutoffKnob.setValue(static_cast<double>(cutoff), juce::dontSendNotification);
            filteredOutput.setCutoffFrequency(cutoff);
        }

        auto layersVar = obj->getProperty("layers");
        auto* layersArray = layersVar.getArray();
        if (layersArray == nullptr || layersArray->isEmpty())
            return;

        // Clear existing layers
        for (auto* layer : layers)
        {
            layer->stopPlayback();
            mixer.removeInputSource(&layer->getTransportSource());
            layerContainer.removeChildComponent(layer);
        }
        layers.clear();

        // Load each layer from preset
        for (const auto& layerVar : *layersArray)
        {
            auto* layerObj = layerVar.getDynamicObject();
            if (layerObj == nullptr)
                continue;

            auto filePath = layerObj->getProperty("filePath").toString();
            auto loopStart = static_cast<juce::int64>(layerObj->getProperty("loopStart"));
            auto loopEnd = static_cast<juce::int64>(layerObj->getProperty("loopEnd"));
            auto crossfadeSamples = static_cast<int>(layerObj->getProperty("crossfadeSamples"));

            auto curveX = layerObj->hasProperty("crossfadeCurveX")
                ? static_cast<float>(static_cast<double>(layerObj->getProperty("crossfadeCurveX")))
                : 0.25f;
            auto curveY = layerObj->hasProperty("crossfadeCurveY")
                ? static_cast<float>(static_cast<double>(layerObj->getProperty("crossfadeCurveY")))
                : 0.75f;

            auto volume = layerObj->hasProperty("volume")
                ? static_cast<float>(static_cast<double>(layerObj->getProperty("volume")))
                : 1.0f;

            juce::File audioFile(filePath);
            if (!audioFile.existsAsFile())
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Missing File",
                    "Audio file not found:\n" + filePath);
                continue;
            }

            addLayer(audioFile, loopStart, loopEnd, crossfadeSamples, curveX, curveY, volume);
        }
    });
}
