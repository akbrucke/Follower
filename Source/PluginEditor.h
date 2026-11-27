#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>

class FollowerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FollowerLookAndFeel();

    static juce::Font getGaramondFont (float height, bool isBold = false);
    static juce::Font getInterFont (float height, bool isBold = false);

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    juce::Font getTextButtonFont (juce::TextButton& button, int buttonHeight) override;
    juce::Font getLabelFont (juce::Label& label) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;
    
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;
    juce::Font getComboBoxFont (juce::ComboBox& box) override;
    juce::Font getPopupMenuFont() override;
};

class FollowerKnob : public juce::Component
{
public:
    FollowerKnob (juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramID,
                  const juce::String& labelText,
                  const juce::String& unitText = {},
                  juce::Colour accent = juce::Colour::fromString ("FF0088FF"));

    void resized() override;
    void paint (juce::Graphics&) override {}

private:
    juce::Slider slider;
    juce::Label nameLabel;
    juce::Label valueLabel;
    juce::String unit;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void updateValueText();
};

class FollowerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit FollowerAudioProcessorEditor (FollowerAudioProcessor&);
    ~FollowerAudioProcessorEditor() override;


    std::function<void()> onAudioSettingsClicked;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FollowerAudioProcessor& audioProcessor;
    FollowerLookAndFeel followerLnf;

    // Header Drawer Toggle Pills
    juce::ToggleButton showOdBtn    { "OVERDRIVE" };
    juce::ToggleButton showPreFxBtn { "PRE FX" };
    juce::ToggleButton showTapeBtn  { "TAPE ECHO" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> showOdAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> showPreFxAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> showTapeAtt;
    // Header Utility Drawer Toggle Pill (Metering + Future Tuner)
    juce::ToggleButton showUtilityBtn { "UTILITY" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> showUtilityAtt;
    
    // Audio button for standalone app
    juce::TextButton audioSettingsBtn { "AUDIO" };

    // Left Collapsible Drawer: Overdrive (+ Fatness & Voltage)
    juce::ToggleButton odOnBtn { "PEDAL ON" };
    FollowerKnob odGainKnob, odToneKnob, odLevelKnob, odFatnessKnob, odVoltageKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> odOnAtt;

    // Center Amp Section: Dual Amp Selector + Independent Preserved Controls
    juce::TextButton ampToggleBtn { "AMP: TRAINER MARK III" };
    juce::ToggleButton trBoostBtn { "TREBLE BOOST" };
    FollowerKnob trGainKnob, trTrebleKnob, trMidKnob, trBassKnob, trMasterKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> trBoostAtt;

    juce::ToggleButton orDirtyBtn { "DIRTY CH" };
    FollowerKnob orGainKnob, orTrebleKnob, orMidKnob, orBassKnob, orVolumeKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> orDirtyAtt;

    FollowerKnob outLevelKnob;

    // Center Cabinet IR & Gate / HPF / LPF
    juce::ComboBox irSelector;
    juce::TextButton loadIrBtn    { "LOAD IR" };
    juce::ToggleButton cabBypassBtn { "BYPASS (-12dB)" };
    juce::ToggleButton gateFastBtn  { "GATE: SLOW" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> irAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> cabBypassAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> gateFastAtt;
    FollowerKnob gateKnob, hpfKnob, lpfKnob;

    // Bottom Collapsible Drawer: Pre-Amp Tremolo, Analog Delay, Hall Reverb
    juce::ToggleButton tremOnBtn  { "TREM ON" };
    FollowerKnob tremRateKnob, tremDepthKnob, tremWaveKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tremOnAtt;

    juce::ToggleButton delayOnBtn { "DELAY ON" };
    FollowerKnob delayTimeKnob, delayFbKnob, delayMixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayOnAtt;

    juce::ToggleButton revOnBtn   { "HALL ON" };
    FollowerKnob revDecayKnob, revDampKnob, revPreDelKnob, revMixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> revOnAtt;

    // Right Collapsible Drawer: Post-Cab Dusk Audio Tape Echo
    juce::ToggleButton tapeOnBtn { "ECHO ON" };
    juce::ComboBox tapeModeBox;
    juce::ComboBox tapeAgeBox;
    FollowerKnob tapeRateKnob, tapeIntKnob, tapeEchoKnob, tapeRevKnob;
    FollowerKnob tapeBassKnob, tapeTrebleKnob, tapeWowKnob, tapeMixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tapeOnAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> tapeModeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> tapeAgeAtt;

    std::unique_ptr<juce::Drawable> followerLogoSvg;
    std::unique_ptr<juce::Drawable> jewelIndicatorSvg;
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Smooth Drawer Gliding Factors (0.0 = collapsed, 1.0 = fully open)
    float utilityGlide      = 0.0f;
    float smoothInPeak      = 0.0f;
    float smoothOutPeak     = 0.0f;
    float holdInPeak        = 0.0f;
    float holdOutPeak       = 0.0f;
    int   inHoldFrames      = 0;
    int   outHoldFrames     = 0;
    float odGlide    = 1.0f;
    float preFxGlide = 1.0f;
    float tapeGlide  = 1.0f;
    
    int   lastAmpChoice = 0;
    bool  lastGateFastState = false;
    bool  lastOdActive      = false;
    bool  lastPreActive     = false;
    bool  lastTapeActive    = false;
    float smoothJewelGlow = 0.0f;

    juce::Rectangle<int> utilityPanelBounds;
    juce::Rectangle<int> odPanelBounds;
    juce::Rectangle<int> ampPanelBounds;
    juce::Rectangle<int> cabPanelBounds;
    juce::Rectangle<int> preFxPanelBounds;
    juce::Rectangle<int> tapePanelBounds;

    void timerCallback() override;
    void syncVisibilityAndDimensions (bool immediate);
    void openIrFileChooser();
    void buildSvgLogo();
    void drawSvgHeaderLogo (juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawBlueJewelIndicator (juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FollowerAudioProcessorEditor)
};
