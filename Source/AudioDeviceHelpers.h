#pragma once

#include <JuceHeader.h>

namespace audioDeviceHelpers
{
inline bool isOutputReady (const juce::AudioDeviceManager& deviceManager)
{
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr || ! device->isOpen() || ! device->isPlaying())
        return false;

    return device->getActiveOutputChannels().countNumberOfSetBits() > 0;
}

inline juce::String getOutputDeviceName (const juce::AudioDeviceManager& deviceManager)
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
        return device->getName();

    return {};
}

inline int getActiveOutputChannelCount (const juce::AudioDeviceManager& deviceManager)
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
        return device->getActiveOutputChannels().countNumberOfSetBits();

    return 0;
}

inline int getAvailableOutputChannelCount (const juce::AudioDeviceManager& deviceManager)
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
        return device->getOutputChannelNames().size();

    return 0;
}

/** Prefer Audient Main L/R (channels 1-2). Headphones monitor Main, not Loop-back.
    Also enable the first two inputs so Mic + DI are both available. */
inline bool ensureActiveOutputChannels (juce::AudioDeviceManager& deviceManager)
{
    auto* device = deviceManager.getCurrentAudioDevice();
    auto setup = deviceManager.getAudioDeviceSetup();

    const int availableOuts = device != nullptr ? device->getOutputChannelNames().size()
                                                : juce::jmax (2, setup.outputChannels.getHighestBit() + 1);
    const int availableIns = device != nullptr ? device->getInputChannelNames().size()
                                               : juce::jmax (0, setup.inputChannels.getHighestBit() + 1);

    // Main stereo pair only — matches headphone monitoring on Audient iD4.
    setup.outputChannels.clear();
    setup.outputChannels.setRange (0, juce::jmin (2, juce::jmax (1, availableOuts)), true);
    setup.useDefaultOutputChannels = false;

    if (availableIns > 0)
    {
        setup.inputChannels.clear();
        // Enable Mic + Instrument (first two), which covers mono DI on ch2.
        setup.inputChannels.setRange (0, juce::jmin (2, availableIns), true);
        setup.useDefaultInputChannels = false;
    }

    if (setup.outputDeviceName.isEmpty() && device != nullptr)
        setup.outputDeviceName = device->getName();
    if (setup.inputDeviceName.isEmpty() && device != nullptr)
        setup.inputDeviceName = device->getName();

    // Force stereo buffer sizes so mono DI still lands in a predictable layout.
    setup.bufferSize = setup.bufferSize > 0 ? setup.bufferSize : 128;

    const auto error = deviceManager.setAudioDeviceSetup (setup, true);
    return error.isEmpty() && isOutputReady (deviceManager);
}

inline bool repairOutput (juce::StandalonePluginHolder& holder, juce::PropertySet* settings)
{
    if (settings != nullptr)
    {
        settings->removeValue ("audioSetup");
        settings->setValue ("shouldMuteInput", false);
    }

    holder.getMuteInputValue().setValue (false);
    holder.reloadAudioDeviceState (true, {}, nullptr);
    ensureActiveOutputChannels (holder.deviceManager);
    holder.startPlaying();

    return isOutputReady (holder.deviceManager);
}

inline void showOutputNotReadyMessage (const juce::String& deviceName = {})
{
    juce::String details = "Guitar Synth could not open a working audio output.\n\n"
                           "Open Options → Audio/MIDI Settings, choose an output device, "
                           "enable the output channels, then click Test Output again."
                           "\n\nIf you use an Audient iD4 (or similar), also turn the "
                           "hardware Monitor Mix control toward DAW / USB playback — "
                           "fully toward Input mutes all software audio in the headphones.";

    if (deviceName.isNotEmpty())
        details += "\n\nLast selected device: " + deviceName;

    juce::NativeMessageBox::showMessageBoxAsync (
        juce::MessageBoxIconType::WarningIcon,
        "No audio output",
        details);
}

} // namespace audioDeviceHelpers
