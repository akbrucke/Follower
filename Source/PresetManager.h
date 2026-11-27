#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& apvtsRef,
                            std::function<juce::String()> getCustomIrPathFn,
                            std::function<void (const juce::String&)> loadCustomIrPathFn)
        : apvts (apvtsRef),
          getCustomIrPath (std::move (getCustomIrPathFn)),
          loadCustomIrPath (std::move (loadCustomIrPathFn))
    {
        auto dir = getPresetDirectory();
        if (! dir.exists())
            dir.createDirectory();

        refreshPresetList();
    }

    static juce::File getPresetDirectory()
    {
       #if JUCE_WINDOWS
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
            .getChildFile ("VST3 Presets")
            .getChildFile ("Hanging Puppet Audio")
            .getChildFile ("Follower");
       #elif JUCE_MAC
        return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
            .getChildFile ("Library")
            .getChildFile ("Audio")
            .getChildFile ("Presets")
            .getChildFile ("Hanging Puppet Audio")
            .getChildFile ("Follower");
       #else
        return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
            .getChildFile (".vst3")
            .getChildFile ("presets")
            .getChildFile ("Hanging Puppet Audio")
            .getChildFile ("Follower");
       #endif
    }

    void refreshPresetList()
    {
        presetFiles.clear();
        auto dir = getPresetDirectory();
        for (const auto& file : dir.findChildFiles (juce::File::findFiles, false, "*.xml"))
            presetFiles.add (file);

        presetFiles.sort();
    }

    int getNumPresets() const noexcept { return presetFiles.size(); }

    juce::String getPresetName (int index) const
    {
        if (juce::isPositiveAndBelow (index, presetFiles.size()))
            return presetFiles[index].getFileNameWithoutExtension();
        return "Init / Custom";
    }

    int getCurrentPresetIndex() const noexcept { return currentIndex; }
    juce::String getCurrentPresetName() const
    {
        return (currentIndex >= 0) ? getPresetName (currentIndex) : currentName;
    }

    void savePreset (const juce::String& name)
    {
        const auto cleanName = juce::File::createLegalFileName (name.trim());
        if (cleanName.isEmpty())
            return;

        auto state = apvts.copyState();
        if (getCustomIrPath)
        {
            const auto irPath = getCustomIrPath();
            if (irPath.isNotEmpty())
                state.setProperty ("customIrPath", irPath, nullptr);
            else
                state.removeProperty ("customIrPath", nullptr);
        }

        if (auto xml = state.createXml())
        {
            auto targetFile = getPresetDirectory().getChildFile (cleanName + ".xml");
            xml->writeTo (targetFile);
            refreshPresetList();

            for (int i = 0; i < presetFiles.size(); ++i)
            {
                if (presetFiles[i] == targetFile)
                {
                    currentIndex = i;
                    currentName  = cleanName;
                    break;
                }
            }
        }
    }

    void loadPreset (int index)
    {
        if (! juce::isPositiveAndBelow (index, presetFiles.size()))
            return;

        auto xml = juce::XmlDocument::parse (presetFiles[index]);
        if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
            return;

        auto loadedTree = juce::ValueTree::fromXml (*xml);
        if (! loadedTree.isValid())
            return;

        // Preserve current drawer/utility open states so the UI width never collapses mid-audition
        static const juce::StringArray uiOnlyParams { "odVisible", "preFxVisible", "tapeVisible", "utilityVisible", "tunerMute" };

        for (int i = 0; i < loadedTree.getNumChildren(); ++i)
        {
            auto child = loadedTree.getChild (i);
            const auto paramId = child.getProperty ("id").toString();
            if (paramId.isEmpty() || uiOnlyParams.contains (paramId))
                continue;

            if (auto* param = apvts.getParameter (paramId))
            {
                const float val = static_cast<float> (child.getProperty ("value", param->getDefaultValue()));
                param->setValueNotifyingHost (param->convertTo0to1 (val));
            }
        }

        if (loadCustomIrPath)
            loadCustomIrPath (loadedTree.getProperty ("customIrPath").toString());

        currentIndex = index;
        currentName  = presetFiles[index].getFileNameWithoutExtension();
    }

    void loadNextPreset()
    {
        if (presetFiles.isEmpty())
            return;
        loadPreset ((currentIndex + 1) % presetFiles.size());
    }

    void loadPreviousPreset()
    {
        if (presetFiles.isEmpty())
            return;
        const int prev = (currentIndex <= 0) ? (presetFiles.size() - 1) : (currentIndex - 1);
        loadPreset (prev);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::function<juce::String()> getCustomIrPath;
    std::function<void (const juce::String&)> loadCustomIrPath;

    juce::Array<juce::File> presetFiles;
    int currentIndex = -1;
    juce::String currentName { "Init / Custom" };
};