#include <JuceHeader.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#include "AudioDeviceHelpers.h"

namespace
{
void ensureLiveInputUnmuted (juce::PropertySet* settings)
{
    if (settings != nullptr)
        settings->setValue ("shouldMuteInput", false);
}

void ensureAudioDeviceRunning (juce::StandaloneFilterWindow& window, juce::PropertySet* settings)
{
    if (window.pluginHolder == nullptr)
        return;

    if (audioDeviceHelpers::isOutputReady (window.getDeviceManager()))
        return;

    if (! audioDeviceHelpers::repairOutput (*window.pluginHolder, settings))
        audioDeviceHelpers::showOutputNotReadyMessage();
}

class GuitarSynthStandaloneApp final : public juce::JUCEApplication,
                                       private juce::Timer
{
public:
    GuitarSynthStandaloneApp()
    {
        juce::PropertiesFile::Options options;
        options.applicationName     = JucePlugin_Name;
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        appProperties.setStorageParameters (options);
    }

    const juce::String getApplicationName() override    { return JucePlugin_Name; }
    const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override          { return true; }

    void initialise (const juce::String&) override
    {
        ensureLiveInputUnmuted (appProperties.getUserSettings());

        auto launch = [this]
        {
            createMainWindow();
        };

       #if JUCE_MAC || JUCE_IOS
        if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
            && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
        {
            juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                               [launch] (bool) { launch(); });
            return;
        }
       #endif

        launch();
    }

    void shutdown() override
    {
        stopTimer();

        if (mainWindow != nullptr && mainWindow->pluginHolder != nullptr)
            mainWindow->pluginHolder->savePluginState();

        mainWindow = nullptr;
        appProperties.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (mainWindow != nullptr && mainWindow->pluginHolder != nullptr)
            mainWindow->pluginHolder->savePluginState();

        quit();
    }

private:
    void createMainWindow()
    {
        // Constrain standalone I/O to mono-in / stereo-out (guitar → Main L/R).
        juce::Array<juce::StandalonePluginHolder::PluginInOuts> ioConfig;
        ioConfig.add ({ 1, 2 });

        auto holder = std::make_unique<juce::StandalonePluginHolder> (
            appProperties.getUserSettings(),
            false,
            juce::String{},
            nullptr,
            ioConfig);
        holder->getMuteInputValue().setValue (false);

        mainWindow = std::make_unique<juce::StandaloneFilterWindow> (
            getApplicationName(),
            juce::LookAndFeel::getDefaultLookAndFeel()
                .findColour (juce::ResizableWindow::backgroundColourId),
            std::move (holder));

        if (mainWindow == nullptr)
            return;

        ensureLiveInputUnmuted (appProperties.getUserSettings());

        // Prefer Main L/R only (Audient headphones monitor Main, not Loop-back).
        audioDeviceHelpers::ensureActiveOutputChannels (mainWindow->getDeviceManager());
        mainWindow->pluginHolder->startPlaying();

        if (! audioDeviceHelpers::isOutputReady (mainWindow->getDeviceManager()))
        {
            if (mainWindow->getDeviceManager().getCurrentAudioDevice() == nullptr)
                startTimer (100);
            else
                ensureAudioDeviceRunning (*mainWindow, appProperties.getUserSettings());
        }

        mainWindow->setVisible (true);
    }

    void timerCallback() override
    {
        if (mainWindow == nullptr)
        {
            stopTimer();
            return;
        }

        if (audioDeviceHelpers::isOutputReady (mainWindow->getDeviceManager()))
        {
            stopTimer();
            return;
        }

        if (++repairAttempts > 5)
        {
            stopTimer();
            ensureAudioDeviceRunning (*mainWindow, appProperties.getUserSettings());
        }
    }

    juce::ApplicationProperties appProperties;
    std::unique_ptr<juce::StandaloneFilterWindow> mainWindow;
    int repairAttempts = 0;
};
} // namespace

START_JUCE_APPLICATION (GuitarSynthStandaloneApp)
