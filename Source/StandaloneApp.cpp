#if JucePlugin_Build_Standalone

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#include "PluginProcessor.h"

class FollowerStandaloneWindow : public juce::StandaloneFilterWindow
{
public:
    FollowerStandaloneWindow (const juce::String& title,
                              juce::Colour backgroundColour,
                              juce::PropertySet* settingsToUse,
                              bool takeOwnershipOfSettings)
        : StandaloneFilterWindow (title, backgroundColour, settingsToUse, takeOwnershipOfSettings)
    {
        // Locate the Options button in the standalone header bar and rewire it
        // to open our mono-capable AudioDeviceSelectorComponent
        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (auto* btn = dynamic_cast<juce::Button*> (getChildComponent (i)))
            {
                if (btn->getButtonText().containsIgnoreCase ("Options"))
                {
                    btn->onClick = [this] { showMonoCapableAudioSettings(); };
                    break;
                }
            }
        }
    }

    void showMonoCapableAudioSettings()
    {
        auto* holder = getPluginHolder();
        if (holder == nullptr)
            return;

        juce::DialogWindow::LaunchOptions o;

        int maxNumInputs = 0, maxNumOutputs = 0;
        if (auto* processor = holder->processor.get())
        {
            maxNumInputs  = processor->getTotalNumInputChannels();
            maxNumOutputs = processor->getTotalNumOutputChannels();
        }

        // Passing false for showChannelsAsStereoPairs exposes individual mono input
        // checkboxes (e.g., Input 1, Input 2) in the ASIO device settings
        auto* selector = new juce::AudioDeviceSelectorComponent (
            holder->deviceManager,
            0, juce::jmax (1, maxNumInputs),
            0, juce::jmax (2, maxNumOutputs),
            false, // showMidiInputOptions
            false, // showMidiOutputSelector
            false, // showChannelsAsStereoPairs -> FALSE for individual mono inputs!
            false  // hideAdvancedOptionsWithButton
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