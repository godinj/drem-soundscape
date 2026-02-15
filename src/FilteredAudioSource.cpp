#include "FilteredAudioSource.h"

FilteredAudioSource::FilteredAudioSource(juce::AudioSource* sourceToFilter)
    : source(sourceToFilter)
{
}

void FilteredAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    source->prepareToPlay(samplesPerBlockExpected, sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlockExpected);
    spec.numChannels = 2;

    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    filter.setCutoffFrequency(cutoffHz.load());
}

void FilteredAudioSource::releaseResources()
{
    source->releaseResources();
    filter.reset();
}

void FilteredAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    source->getNextAudioBlock(bufferToFill);

    filter.setCutoffFrequency(cutoffHz.load());

    juce::dsp::AudioBlock<float> block(*bufferToFill.buffer,
                                       static_cast<size_t>(bufferToFill.startSample));
    auto context = juce::dsp::ProcessContextReplacing<float>(block);
    filter.process(context);
}

void FilteredAudioSource::setCutoffFrequency(float hz)
{
    cutoffHz.store(hz);
}
