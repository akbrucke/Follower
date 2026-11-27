#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

FollowerAudioProcessor::FollowerAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::mono(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout FollowerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // Collapsible Drawer Visibility States
    p.push_back (std::make_unique<juce::AudioParameterBool> ("odVisible",    "Show Overdrive Drawer", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("preFxVisible", "Show Pre-FX Drawer",    false));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("tapeVisible",  "Show Tape Echo Drawer", false));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("utilityVisible",  "Show Utility Drawer", false));
    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "tunerMute", 1 }, "Tuner Mute", false));

    // Left Drawer: Overdrive (+ Fatness & Variable Voltage)
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("odOn",      "Overdrive On", true));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("odGain",    "OD Gain",      0.0f, 10.0f, 2.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("odTone",    "OD Tone",      0.0f, 10.0f, 10.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("odLevel",   "OD Level",     0.0f, 10.0f, 6.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("odFatness", "OD Fatness",   0.0f, 10.0f, 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("odVoltage", "OD Voltage",   4.5f, 18.0f, 18.0f));

    // Bottom Drawer: Pre-Amp Time-Based Effects (Tremolo, Analog Delay, Hall Reverb)
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("tremOn",     "Tremolo On",     false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("tremRate",   "Trem Rate",      0.5f, 12.0f, 3.8f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("tremDepth",  "Trem Depth",     0.0f, 10.0f, 5.5f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("tremWave",   "Trem Wave",      0.0f, 10.0f, 2.5f));

    p.push_back (std::make_unique<juce::AudioParameterBool>  ("delayOn",    "Delay On",       false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("delayTime",  "Delay Time",     25.0f, 550.0f, 280.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("delayFb",    "Delay Feedback", 0.0f, 10.0f, 3.8f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("delayMix",   "Delay Mix",      0.0f, 10.0f, 3.5f));

    p.push_back (std::make_unique<juce::AudioParameterBool>  ("revOn",      "Reverb On",      false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("revDecay",   "Reverb Decay",   0.5f, 10.0f, 4.2f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("revDamp",    "Reverb Damping", 1000.0f, 9500.0f, 4200.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("revPreDel",  "Reverb PreDelay",0.0f, 120.0f, 22.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("revMix",     "Reverb Mix",     0.0f, 10.0f, 3.5f));

    // Center Amp Section: 0 = Trainer Mark III, 1 = Rocker 100
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("ampSelect", "Amp Model",
        juce::StringArray { "TRAINER MARK III", "ROCKER 100" }, 0));

    // Preserved Independent Controls for Trainer Mark III
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("trGain",   "Trainer Gain",   0.0f, 10.0f, 8.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("trTreble", "Trainer Treble", 0.0f, 10.0f, 7.5f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("trMid",    "Trainer Middle", 0.0f, 10.0f, 4.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("trBass",   "Trainer Bass",   0.0f, 10.0f, 4.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("trMaster", "Trainer Master", 0.0f, 10.0f, 8.0f));
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("trBoost",  "Trainer Treble Boost", true));

    // Preserved Independent Controls for Rocker 100
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("orGain",   "Rocker Gain",   0.0f, 10.0f, 6.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("orTreble", "Rocker Treble", 0.0f, 10.0f, 5.5f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("orMid",    "Rocker Middle", 0.0f, 10.0f, 6.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("orBass",   "Rocker Bass",   0.0f, 10.0f, 6.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("orVolume", "Rocker Volume", 0.0f, 10.0f, 5.5f));
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("orDirty",  "Rocker Dirty Channel", false));

    // Shared Output & Cabinet / Gate / Surgical Filters
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("outLevel", "Output Level", -24.0f, 12.0f, 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("irSelect", "Cabinet IR",
        juce::StringArray { "MARS T75 VINTAGE", "MEGA V30 PUNCHY", "ORNG V30 DARK", "FEND JEN GRITTY" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("cabBypass", "Cab Bypass (-12dB)", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("gateThresh", "Gate Threshold", -90.0f, -15.0f, -65.0f));
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("gateFast",   "Gate Fast Release (12ms)", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("hpfFreq", "HPF Freq", 20.0f, 350.0f, 72.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("lpfFreq", "LPF Freq", 3000.0f, 20000.0f, 20000.0f));

    // Right Drawer: Post-Cab Dusk Audio Tape Echo
    p.push_back (std::make_unique<juce::AudioParameterBool>   ("tapeOn",     "Tape Echo On", false));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("tapeMode",   "Tape Mode",
        juce::StringArray {
            "1: HEAD 1", "2: HEAD 2", "3: HEAD 3", "4: HEAD 2+3",
            "5: H1 + REV", "6: H2 + REV", "7: H3 + REV", "8: H2+3 + REV",
            "9: H1+2 + REV", "10: H1+3 + REV", "11: ALL + REV", "12: REVERB ONLY"
        }, 5));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  ("tapeRate",   "Tape Rate",     0.0f, 10.0f, 4.5f));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  ("tapeInt",    "Tape Intensity",0.0f, 10.0f, 3.8f));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  ("tapeEcho",   "Tape Echo Lvl", 0.0f, 10.0f, 6.5f));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  ("tapeRev",    "Tape Spring Lvl",0.0f, 10.0f, 3.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  ("tapeBass",   "Tape Bass",    -12.0f, 12.0f, 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  ("tapeTreble", "Tape Treble",  -12.0f, 12.0f, -1.5f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("tapeAge",    "Tape Age",
        juce::StringArray { "NEW", "USED", "OLD" }, 1));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  ("tapeWow",    "Wow & Flutter", 0.0f, 10.0f, 2.5f));
    p.push_back (std::make_unique<juce::AudioParameterFloat>  ("tapeMix",    "Tape Mix",      0.0f, 10.0f, 3.5f));

    return { p.begin(), p.end() };
}

void FollowerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    isPrepared = true;

    tuner.prepare (sampleRate);
    overdrive.prepare (sampleRate);
    tremolo.prepare (sampleRate);
    preDelay.prepare (sampleRate);
    hallReverb.prepare (sampleRate);
    trainerAmp.prepare (sampleRate);
    rockerAmp.prepare (sampleRate);
    tapeEcho.prepare (sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };
    cabinetConvolution.prepare (spec);
    noiseGate.prepare (spec);
    noiseGate.setAttack (1.5f);
    lastGateFast = apvts.getRawParameterValue ("gateFast")->load() > 0.5f;
    noiseGate.setRelease (lastGateFast ? 12.0f : 85.0f);
    noiseGate.setRatio (12.0f);

    hpfFilter.prepare (spec);
    lpfFilter.prepare (spec);

    // Restore persisted custom IR file if present; otherwise load selected built-in IR
    if (customIrFilePath.isNotEmpty())
    {
        const juce::File savedFile (customIrFilePath);
        if (savedFile.existsAsFile())
        {
            cabinetConvolution.loadImpulseResponse (savedFile,
                juce::dsp::Convolution::Stereo::yes,
                juce::dsp::Convolution::Trim::yes,
                0,
                juce::dsp::Convolution::Normalise::yes);
            usingCustomIr = true;
            loadedIrName = savedFile.getFileNameWithoutExtension().toUpperCase();
        }
        else
        {
            customIrFilePath.clear();
            usingCustomIr = false;
            lastBuiltInIrIndex = -1;
            selectBuiltInIr (static_cast<int> (apvts.getRawParameterValue ("irSelect")->load()));
        }
    }
    else
    {
        lastBuiltInIrIndex = -1;
        selectBuiltInIr (static_cast<int> (apvts.getRawParameterValue ("irSelect")->load()));
    }

    ampBlend = apvts.getRawParameterValue ("ampSelect")->load();
}

void FollowerAudioProcessor::releaseResources() {}

bool FollowerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    const auto mainIn = layouts.getMainInputChannelSet();
    return (mainIn == juce::AudioChannelSet::mono() || mainIn == juce::AudioChannelSet::stereo());
}

void FollowerAudioProcessor::selectBuiltInIr (int index)
{
    const int clamped = std::clamp (index, 0, 3);
    if (! usingCustomIr && clamped == lastBuiltInIrIndex)
        return;

    const void* data = nullptr;
    size_t size = 0;

    switch (index)
    {
        case 0:
        default:
            data = BinaryData::MARST75VINTAGE_wav;
            size = BinaryData::MARST75VINTAGE_wavSize;
            loadedIrName = "MARS T75 VINTAGE";
            break;
        case 1:
            data = BinaryData::MEGAV30PUNCHY_wav;
            size = BinaryData::MEGAV30PUNCHY_wavSize;
            loadedIrName = "MEGA V30 PUNCHY";
            break;
        case 2:
            data = BinaryData::ORNGV30DARK_wav;
            size = BinaryData::ORNGV30DARK_wavSize;
            loadedIrName = "ORNG V30 DARK";
            break;
        case 3:
            data = BinaryData::FENDJENGRITTY_wav;
            size = BinaryData::FENDJENGRITTY_wavSize;
            loadedIrName = "FEND JEN GRITTY";
            break;
    }

    if (data != nullptr && size > 0)
    {
        cabinetConvolution.loadImpulseResponse (data, size,
            juce::dsp::Convolution::Stereo::yes,
            juce::dsp::Convolution::Trim::yes,
            0,
            juce::dsp::Convolution::Normalise::yes);
        usingCustomIr = false;
        customIrFilePath.clear();
        lastBuiltInIrIndex = clamped;
    }
}

void FollowerAudioProcessor::loadUserIrFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return;

    customIrFilePath = file.getFullPathName();
    usingCustomIr = true;
    loadedIrName = file.getFileNameWithoutExtension().toUpperCase();

    if (isPrepared)
    {
        cabinetConvolution.loadImpulseResponse (file,
            juce::dsp::Convolution::Stereo::yes,
            juce::dsp::Convolution::Trim::yes,
            0,
            juce::dsp::Convolution::Normalise::yes);
    }
}

void FollowerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    float inPeak = 0.0f;
    const int numInCh = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    for (int ch = 0; ch < numInCh; ++ch)
        inPeak = juce::jmax (inPeak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));
    inputPeakLevel.store (inPeak, std::memory_order_relaxed);

    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples == 0 || numChannels == 0)
        return;

    auto* utilParam = apvts.getRawParameterValue ("utilityVisible");
    if (utilParam != nullptr && utilParam->load() > 0.5f && buffer.getNumChannels() > 0)
        tuner.pushSamples (buffer.getReadPointer (0), buffer.getNumSamples());

    // 1. Update DSP Parameters
    const bool odOn = apvts.getRawParameterValue ("odOn")->load() > 0.5f;
    overdrive.setParameters (
        apvts.getRawParameterValue ("odGain")->load(),
        apvts.getRawParameterValue ("odTone")->load(),
        apvts.getRawParameterValue ("odLevel")->load(),
        apvts.getRawParameterValue ("odFatness")->load(),
        apvts.getRawParameterValue ("odVoltage")->load());

    const bool tremOn = apvts.getRawParameterValue ("tremOn")->load() > 0.5f;
    tremolo.setParameters (
        apvts.getRawParameterValue ("tremRate")->load(),
        apvts.getRawParameterValue ("tremDepth")->load() * 0.1f,
        apvts.getRawParameterValue ("tremWave")->load() * 0.1f);

    const bool delayOn = apvts.getRawParameterValue ("delayOn")->load() > 0.5f;
    preDelay.setParameters (
        apvts.getRawParameterValue ("delayTime")->load(),
        apvts.getRawParameterValue ("delayFb")->load() * 0.105f,
        apvts.getRawParameterValue ("delayMix")->load() * 0.1f);

    const bool revOn = apvts.getRawParameterValue ("revOn")->load() > 0.5f;
    hallReverb.setParameters (
        apvts.getRawParameterValue ("revDecay")->load(),
        apvts.getRawParameterValue ("revDamp")->load(),
        apvts.getRawParameterValue ("revPreDel")->load(),
        apvts.getRawParameterValue ("revMix")->load() * 0.1f);

    const int ampChoice = static_cast<int> (apvts.getRawParameterValue ("ampSelect")->load());
    trainerAmp.setParameters (
        apvts.getRawParameterValue ("trGain")->load(),
        apvts.getRawParameterValue ("trTreble")->load(),
        apvts.getRawParameterValue ("trMid")->load(),
        apvts.getRawParameterValue ("trBass")->load(),
        apvts.getRawParameterValue ("trMaster")->load(),
        apvts.getRawParameterValue ("trBoost")->load() > 0.5f);

    rockerAmp.setParameters (
        apvts.getRawParameterValue ("orGain")->load(),
        apvts.getRawParameterValue ("orTreble")->load(),
        apvts.getRawParameterValue ("orMid")->load(),
        apvts.getRawParameterValue ("orBass")->load(),
        apvts.getRawParameterValue ("orVolume")->load(),
        apvts.getRawParameterValue ("orDirty")->load() > 0.5f);

    const bool tapeOn = apvts.getRawParameterValue ("tapeOn")->load() > 0.5f;
    tapeEcho.setParameters (
        static_cast<int> (apvts.getRawParameterValue ("tapeMode")->load()),
        apvts.getRawParameterValue ("tapeRate")->load() * 0.1f,
        apvts.getRawParameterValue ("tapeInt")->load() * 0.11f,
        apvts.getRawParameterValue ("tapeEcho")->load() * 0.14f,
        apvts.getRawParameterValue ("tapeRev")->load() * 0.12f,
        apvts.getRawParameterValue ("tapeBass")->load(),
        apvts.getRawParameterValue ("tapeTreble")->load(),
        static_cast<int> (apvts.getRawParameterValue ("tapeAge")->load()),
        apvts.getRawParameterValue ("tapeWow")->load() * 0.1f,
        apvts.getRawParameterValue ("tapeMix")->load() * 0.1f);

    // 2. Input Noise Gate (with Fast 12ms / Slow 85ms release toggle)
    const bool gateFast = apvts.getRawParameterValue ("gateFast")->load() > 0.5f;
    if (gateFast != lastGateFast)
    {
        lastGateFast = gateFast;
        noiseGate.setRelease (gateFast ? 12.0f : 85.0f);
    }

    const float gateThresh = apvts.getRawParameterValue ("gateThresh")->load();
    juce::dsp::AudioBlock<float> block (buffer);
    if (gateThresh > -89.5f)
    {
        noiseGate.setThreshold (gateThresh);
        noiseGate.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

    // Sum stereo input to mono guitar bus before pre-amp pedals & amp head
    if (numChannels > 1)
    {
        buffer.addFrom (0, 0, buffer, 1, 0, numSamples);
        buffer.applyGain (0, 0, numSamples, 0.5f);
    }

    auto* monoData = buffer.getWritePointer (0);
    float peakDrive = 0.0f;

    // 3. Pre-Amp Pedals -> Dual Switchable Amp Head (Crossfaded smoothly to eliminate clicks)
    const float targetBlend = (ampChoice == 1) ? 1.0f : 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        ampBlend += 0.002f * (targetBlend - ampBlend);
        float s = monoData[i];

        if (odOn)    s = overdrive.processSample (s);
        if (tremOn)  s = tremolo.processSample (s);
        if (delayOn) s = preDelay.processSample (s);
        if (revOn)   s = hallReverb.processSample (s);

        float ampOut = 0.0f;
        if (ampBlend < 0.001f)
            ampOut = trainerAmp.processSample (s);
        else if (ampBlend > 0.999f)
            ampOut = rockerAmp.processSample (s);
        else
            ampOut = (1.0f - ampBlend) * trainerAmp.processSample (s)
                   + ampBlend * rockerAmp.processSample (s);

        monoData[i] = ampOut;
        peakDrive = std::max (peakDrive, std::abs (ampOut));
    }

    jewelDriveActivity.store (std::clamp (peakDrive * 1.4f, 0.0f, 1.0f));

    // Duplicate mono amp output to both channels for Cabinet Convolution
    if (numChannels > 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    // 4. Cabinet IR Loader or Bypass (-12 dB automatic compensation)
    const bool cabBypass = apvts.getRawParameterValue ("cabBypass")->load() > 0.5f;
    if (! cabBypass)
    {
        cabinetConvolution.process (juce::dsp::ProcessContextReplacing<float> (block));
    }
    else
    {
        constexpr float minus12Db = 0.25118864f;
        buffer.applyGain (minus12Db);
    }

    // 5. Post-Cab Dusk Audio Tape Echo (True Stereo when plugin is on a stereo track)
    if (tapeOn)
    {
        if (numChannels > 1)
        {
            auto* ch0 = buffer.getWritePointer (0);
            auto* ch1 = buffer.getWritePointer (1);
            for (int i = 0; i < numSamples; ++i)
                tapeEcho.processStereoSample (ch0[i], ch1[i]);
        }
        else
        {
            auto* ch0 = buffer.getWritePointer (0);
            for (int i = 0; i < numSamples; ++i)
                ch0[i] = tapeEcho.processSample (ch0[i]);
        }
    }

    // 6. Surgical HPF / LPF & Master Output Level
    const float hpfHz = apvts.getRawParameterValue ("hpfFreq")->load();
    if (hpfHz > 20.5f)
    {
        *hpfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, hpfHz, 0.7071f);
        hpfFilter.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

    const float lpfHz = apvts.getRawParameterValue ("lpfFreq")->load();
    if (lpfHz < 19950.0f)
    {
        *lpfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (currentSampleRate, lpfHz, 0.7071f);
        lpfFilter.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

    const float outDb = apvts.getRawParameterValue ("outLevel")->load();
    const float outGain = juce::Decibels::decibelsToGain (outDb);
    buffer.applyGain (outGain);

    auto* muteParam = apvts.getRawParameterValue ("tunerMute");
    if (muteParam != nullptr && muteParam->load() > 0.5f)
        buffer.clear();

    float outPeak = 0.0f;
    const int numOutCh = juce::jmin (getTotalNumOutputChannels(), buffer.getNumChannels());
    for (int ch = 0; ch < numOutCh; ++ch)
        outPeak = juce::jmax (outPeak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));
    outputPeakLevel.store (outPeak, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* FollowerAudioProcessor::createEditor()
{
    return new FollowerAudioProcessorEditor (*this);
}

void FollowerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("customIrPath", usingCustomIr ? customIrFilePath : juce::String(), nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void FollowerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
    {
        auto restoredTree = juce::ValueTree::fromXml (*xmlState);
        const juce::String savedIrPath = restoredTree.getProperty ("customIrPath", juce::String()).toString();
        apvts.replaceState (restoredTree);

        if (savedIrPath.isNotEmpty() && juce::File (savedIrPath).existsAsFile())
        {
            loadUserIrFile (juce::File (savedIrPath));
        }
        else
        {
            usingCustomIr = false;
            customIrFilePath.clear();
            lastBuiltInIrIndex = -1;
            if (isPrepared)
                selectBuiltInIr (static_cast<int> (apvts.getRawParameterValue ("irSelect")->load()));
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FollowerAudioProcessor();
}
