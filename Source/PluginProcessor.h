#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

#include "Dsp/Overdrive.h"
#include "Dsp/PreAmpEffects.h"
#include "Dsp/TrainerMarkIII.h"
#include "Dsp/Rocker100.h"
#include "Dsp/DuskTapeEcho.h"

class FollowerAudioProcessor : public juce::AudioProcessor
{
public:
    FollowerAudioProcessor();
    ~FollowerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void selectBuiltInIr (int index);
    void loadUserIrFile (const juce::File& file);
    bool isUsingCustomIr() const noexcept { return usingCustomIr; }
    juce::String getLoadedIrName() const { return loadedIrName; }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> jewelDriveActivity { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    FollowerDsp::OverdrivePedal    overdrive;
    FollowerDsp::Tremolo           tremolo;
    FollowerDsp::AnalogDelay       preDelay;
    FollowerDsp::PreAmpHallReverb  hallReverb;
    FollowerDsp::TrainerMarkIIIAmp trainerAmp;
    FollowerDsp::Rocker100Amp      rockerAmp;
    FollowerDsp::DuskTapeEcho      tapeEcho;

    juce::dsp::Convolution cabinetConvolution;
    juce::dsp::NoiseGate<float> noiseGate;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> hpfFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lpfFilter;

    double currentSampleRate = 48000.0;
    int currentBlockSize = 512;
    bool isPrepared = false;
    bool usingCustomIr = false;
    juce::String customIrFilePath;
    juce::String loadedIrName { "ORANGE 412 57" };
    int lastBuiltInIrIndex = -1;
    bool lastGateFast = false;

    // Crossfade smoothing when toggling between Trainer Mark III and Rocker 100
    float ampBlend = 0.0f; // 0.0 = Trainer Mark III, 1.0 = Rocker 100

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FollowerAudioProcessor)
};
