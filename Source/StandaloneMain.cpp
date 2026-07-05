#include <JuceHeader.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace
{
class GuitarSynthStandaloneApp final : public juce::JUCEApplication
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
        auto holder = std::make_unique<juce::StandalonePluginHolder> (appProperties.getUserSettings(), false);

        // Live guitar input is required; JUCE defaults to muted input to prevent speaker feedback.
        holder->getMuteInputValue().setValue (false);

        mainWindow = std::make_unique<juce::StandaloneFilterWindow> (
            getApplicationName(),
            juce::LookAndFeel::getDefaultLookAndFeel()
                .findColour (juce::ResizableWindow::backgroundColourId),
            std::move (holder));

        if (mainWindow != nullptr)
            mainWindow->setVisible (true);
    }

    void shutdown() override
    {
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
    juce::ApplicationProperties appProperties;
    std::unique_ptr<juce::StandaloneFilterWindow> mainWindow;
};
} // namespace

START_JUCE_APPLICATION (GuitarSynthStandaloneApp)
