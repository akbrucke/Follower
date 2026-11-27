#if JucePlugin_Build_Standalone

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

class FollowerStandaloneWindow : public juce::StandaloneFilterWindow
{
public:
    FollowerStandaloneWindow (const juce::String& title,
                              juce::Colour backgroundColour,
                              juce::PropertySet* settingsToUse,
                              bool takeOwnershipOfSettings)
        : StandaloneFilterWindow (title, backgroundColour, settingsToUse, takeOwnershipOfSettings)
    {
        juce::MessageManager::callAsync ([this]
        {
            if (auto* holder = getPluginHolder())
                if (auto* ed = dynamic_cast<FollowerAudioProcessorEditor*> (holder->processor->getActiveEditor()))
                    ed->onAudioSettingsClicked = [this] { showMonoCapableAudioSettings(); };
        });
    }

    void showMonoCapableAudioSettings()
    {
        auto* holder = getPluginHolder();
        if (holder == nullptr)
            return;

        juce::DialogWindow::LaunchOptions o;

        auto* selector = new juce::AudioDeviceSelectorComponent (
            holder->deviceManager,
            0, 1,   // min/max input channels (1 = mono ASIO selection)
            0, 2,   // min/max output channels
            false,  // showMidiInputOptions
            false,  // showMidiOutputSelector
            false,  // showChannelsAsStereoPairs -> false for individual mono inputs
            false   // hideAdvancedOptionsWithButton
        );

        selector->setSize (500, 450);
        o.content.setOwned (selector);
        o.dialogTitle                  = "Audio/MIDI Settings";
        o.dialogBackgroundColour       = getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId);
        o.escapeKeyTriggersCloseButton = true;
        o.useNativeTitleBar            = true;
        o.resizable                    = false;

        o.launchAsync();
    }
};

class FollowerStandaloneApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override       { return JucePlugin_Name; }
    const juce::String getApplicationVersion() override    { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise (const juce::String&) override
    {
        appProperties.setStorageParameters (getPropertiesOptions());

        mainWindow = std::make_unique<FollowerStandaloneWindow> (
            getApplicationName(),
            juce::LookAndFeel::getDefaultLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId),
            appProperties.getUserSettings(),
            false);

        mainWindow->setUsingNativeTitleBar (true);
        mainWindow->setVisible (true);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

private:
    static juce::PropertiesFile::Options getPropertiesOptions()
    {
        juce::PropertiesFile::Options opts;
        opts.applicationName     = JucePlugin_Name;
        opts.filenameSuffix      = ".settings";
        opts.osxLibrarySubFolder = "Application Support";
        opts.folderName          = "Hanging Puppet Audio";
        return opts;
    }

    juce::ApplicationProperties appProperties;
    std::unique_ptr<FollowerStandaloneWindow> mainWindow;
};

START_JUCE_APPLICATION (FollowerStandaloneApp)

#endif