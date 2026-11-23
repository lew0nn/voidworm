#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"

#include <cstring>
#include <iostream>
#include <random>

// Debug aid (not shipped): drives the real processor, and paints the real editor, through every block size,
// sample rate and channel layout a host or CoreAudio can produce. CI builds it with AddressSanitizer, so any
// out-of-bounds write stops the run at the exact line. "oversize" also sends blocks larger than prepared,
// which some device combinations deliver even though the plugin contract says they shouldn't.
namespace
{
void run (bool oversize)
{
    std::mt19937 rng (7);
    std::uniform_real_distribution<float> unit (0.0f, 1.0f), noise (-1.0f, 1.0f);
    VoidwormAudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    auto& params = processor.getParameters();
    juce::MidiBuffer midi;
    const int blockSizes[] = { 14, 15, 31, 32, 33, 64, 100, 128, 256, 441, 480, 512, 1024, 2048, 4096 };
    const double rates[] = { 16000.0, 22050.0, 24000.0, 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
    int configs = 0;
    for (const auto channels : { 2, 1 })
        for (const auto rate : rates)
            for (const auto maxBlock : blockSizes)
            {
                processor.releaseResources();
                processor.setPlayConfigDetails (channels, channels, rate, maxBlock);
                processor.prepareToPlay (rate, maxBlock);
                const auto largest = oversize ? maxBlock * 2 : maxBlock;
                juce::AudioBuffer<float> storage (channels, largest);
                for (int block = 0; block < 40; ++block)
                {
                    if (block % 10 == 0)
                        for (auto* parameter : params)
                            parameter->setValueNotifyingHost (unit (rng));
                    const auto samples = std::uniform_int_distribution<int> (1, largest) (rng);
                    juce::AudioBuffer<float> view (storage.getArrayOfWritePointers(), channels, 0, samples);
                    for (int channel = 0; channel < channels; ++channel)
                        for (int sample = 0; sample < samples; ++sample)
                            view.setSample (channel, sample, noise (rng) * 0.5f);
                    processor.processBlock (view, midi);
                    if (block % 8 == 0)
                    {
                        juce::Timer::callPendingTimersSynchronously();
                        editor->createComponentSnapshot (editor->getLocalBounds());
                    }
                }
                ++configs;
            }
    std::cout << (oversize ? "oversize" : "contract") << " pass ok: " << configs << " configurations\n";
}
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    run (argc > 1 && std::strcmp (argv[1], "oversize") == 0);
    return 0;
}
