#include "PluginEditor.h"
#include <cmath>

namespace FollowerPalette
{
    const juce::Colour bgWhiteTop     = juce::Colour::fromString ("FFFAFAFC"); // Crisp porcelain white top
    const juce::Colour bgWhiteBottom  = juce::Colour::fromString ("FFEAEEF4"); // Soft cool off-white bottom
    const juce::Colour panelSurface   = juce::Colour::fromString ("FFFFFFFF"); // Pure white card surface
    const juce::Colour panelRecessed  = juce::Colour::fromString ("FFF2F4F8"); // Subtle cool gray inset
    const juce::Colour inkBlack       = juce::Colour::fromString ("FF121417"); // Stark matte black accent
    const juce::Colour slateMuted     = juce::Colour::fromString ("FF525864"); // Secondary label slate
    const juce::Colour borderHairline = juce::Colour::fromString ("FFCFD5E0"); // Subtle divider line
    const juce::Colour ledBlue        = juce::Colour::fromString ("FF0088FF"); // Signature electric LED blue
    const juce::Colour ledBlueGlow    = juce::Colour::fromString ("FF38B6FF"); // Bright cyan-blue bloom
}

juce::Font FollowerLookAndFeel::getGaramondFont (float height, bool isBold)
{
    static const juce::Typeface::Ptr garamondTypeface = juce::Typeface::createSystemTypefaceFor (
        BinaryData::EBGaramond_ttf, BinaryData::EBGaramond_ttfSize);

    auto options = juce::FontOptions (garamondTypeface).withHeight (height);
    if (isBold)
        options = options.withStyle ("Bold");

    return juce::Font (options);
}

juce::Font FollowerLookAndFeel::getInterFont (float height, bool isBold)
{
    static const juce::Typeface::Ptr interTypeface = juce::Typeface::createSystemTypefaceFor (
        BinaryData::Inter_ttf, BinaryData::Inter_ttfSize);

    auto options = juce::FontOptions (interTypeface).withHeight (height);
    return juce::Font (options).withExtraKerningFactor (isBold ? 0.03f : 0.0f);
}

FollowerLookAndFeel::FollowerLookAndFeel()
{
    setColour (juce::Label::textColourId, FollowerPalette::inkBlack);
    setColour (juce::TextEditor::backgroundColourId, FollowerPalette::panelSurface);
    setColour (juce::TextEditor::textColourId, FollowerPalette::inkBlack);
    setColour (juce::TextEditor::outlineColourId, FollowerPalette::ledBlue);
    setColour (juce::TextButton::textColourOffId, FollowerPalette::inkBlack);
    setColour (juce::TextButton::textColourOnId, FollowerPalette::panelSurface);
    setColour (juce::PopupMenu::backgroundColourId, FollowerPalette::panelSurface);
    setColour (juce::PopupMenu::textColourId, FollowerPalette::inkBlack);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, FollowerPalette::ledBlue.withAlpha (0.15f));
    setColour (juce::PopupMenu::highlightedTextColourId, FollowerPalette::inkBlack);
    setColour (juce::ComboBox::backgroundColourId, FollowerPalette::panelRecessed);
    setColour (juce::ComboBox::textColourId, FollowerPalette::inkBlack);
    setColour (juce::ComboBox::outlineColourId, FollowerPalette::inkBlack.withAlpha (0.35f));
    setColour (juce::ComboBox::arrowColourId, FollowerPalette::inkBlack);
}

void FollowerLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float rotaryStartAngle,
                                            float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                                static_cast<float> (y),
                                                static_cast<float> (width),
                                                static_cast<float> (height)).reduced (3.0f);

    const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const float radius   = diameter * 0.5f;
    const auto centre    = bounds.getCentre();
    const float angle    = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto accent    = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // 0. Subtle outer value arc + calibration tick dots around the guitar head knob
    const float arcRadius = radius + 2.0f;
    {
        juce::Path bgArc;
        bgArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (FollowerPalette::borderHairline);
        g.strokePath (bgArc, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (slider.isEnabled())
        {
            juce::Path valArc;
            valArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
            g.setColour (accent);
            g.strokePath (valArc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // 11 classic amp dial ticks (0 through 10)
        for (int t = 0; t <= 10; ++t)
        {
            const float ta = rotaryStartAngle + (static_cast<float> (t) / 10.0f) * (rotaryEndAngle - rotaryStartAngle);
            const float sinT = std::sin (ta);
            const float cosT = -std::cos (ta);
            const float r0 = arcRadius + 1.8f;
            const float r1 = arcRadius + ((t == 0 || t == 5 || t == 10) ? 4.2f : 3.0f);
            g.setColour (FollowerPalette::inkBlack.withAlpha ((t == 0 || t == 5 || t == 10) ? 0.55f : 0.28f));
            g.drawLine (centre.x + r0 * sinT, centre.y + r0 * cosT,
                        centre.x + r1 * sinT, centre.y + r1 * cosT, 1.0f);
        }
    }

    const float knobRadius = radius - 2.5f;

    // 1. Directional drop shadow onto the white faceplate (fast radial gradient, zero offscreen blur)
    {
        const float shadowR = knobRadius * 1.16f;
        const float shadowY = centre.y + knobRadius * 0.14f;
        juce::ColourGradient shadowGrad (FollowerPalette::inkBlack.withAlpha (0.26f), centre.x, shadowY,
                                         FollowerPalette::inkBlack.withAlpha (0.0f),  centre.x + shadowR, shadowY,
                                         true);
        shadowGrad.addColour (0.72, FollowerPalette::inkBlack.withAlpha (0.14f));
        g.setGradientFill (shadowGrad);
        g.fillEllipse (centre.x - shadowR, shadowY - shadowR, shadowR * 2.0f, shadowR * 2.0f);
    }

    // 2. Classic Fluted / Knurled Guitar Amplifier Skirt (satin silver-gray metallic ring)
    {
        juce::ColourGradient skirtGrad (juce::Colour (0xfff8f9fb), centre.x - knobRadius * 0.6f, centre.y - knobRadius * 0.7f,
                                        juce::Colour (0xffaeb5c0), centre.x + knobRadius * 0.7f, centre.y + knobRadius * 0.75f,
                                        false);
        g.setGradientFill (skirtGrad);
        g.fillEllipse (centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

        // Rotating radial flutes around the skirt
        const int numFlutes = 20;
        const float outerFluteR = knobRadius * 0.985f;
        const float innerFluteR = knobRadius * 0.78f;

        for (int i = 0; i < numFlutes; ++i)
        {
            const float fluteAngle = angle + (static_cast<float> (i) / static_cast<float> (numFlutes)) * juce::MathConstants<float>::twoPi;
            const float cosA = std::cos (fluteAngle);
            const float sinA = std::sin (fluteAngle);

            g.setColour (FollowerPalette::inkBlack.withAlpha (0.20f));
            g.drawLine (centre.x + innerFluteR * cosA,
                        centre.y + innerFluteR * sinA,
                        centre.x + outerFluteR * cosA,
                        centre.y + outerFluteR * sinA,
                        1.15f);
        }

        g.setColour (FollowerPalette::inkBlack.withAlpha (0.55f));
        g.drawEllipse (centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 1.0f);
    }

    // 3. Recessed Matte-Black Guitar Head Top Cap
    const float capRadius = knobRadius * 0.73f;
    {
        juce::ColourGradient capGrad (juce::Colour (0xff262a30), centre.x - capRadius * 0.5f, centre.y - capRadius * 0.6f,
                                      FollowerPalette::inkBlack, centre.x + capRadius * 0.6f, centre.y + capRadius * 0.7f,
                                      false);
        g.setGradientFill (capGrad);
        g.fillEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);

        // Crisp specular bevel ring
        g.setColour (juce::Colours::white.withAlpha (0.24f));
        g.drawEllipse (centre.x - capRadius + 0.6f, centre.y - capRadius + 0.6f,
                       (capRadius - 0.6f) * 2.0f, (capRadius - 0.6f) * 2.0f, 0.85f);

        g.setColour (FollowerPalette::inkBlack);
        g.drawEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, 1.0f);
    }

    // 4. High-Contrast White Guitar Amp Pointer Line extending onto the skirt
    {
        const float lineStart = capRadius * 0.16f;
        const float lineEnd   = knobRadius * 0.94f;
        const float sinA = std::sin (angle);
        const float cosA = -std::cos (angle);

        juce::Path pointer;
        pointer.startNewSubPath (centre.x + lineStart * sinA, centre.y + lineStart * cosA);
        pointer.lineTo          (centre.x + lineEnd   * sinA, centre.y + lineEnd   * cosA);

        g.setColour (FollowerPalette::panelSurface);
        g.strokePath (pointer, juce::PathStrokeType (2.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void FollowerLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                            bool shouldDrawButtonAsHighlighted, bool)
{
    auto b = button.getLocalBounds().toFloat().reduced (1.5f);
    const bool hasProxy  = button.getProperties().contains ("circuitActive");
    const bool circuitOn = static_cast<bool> (button.getProperties().getWithDefault ("circuitActive", false));
    const bool isDrawerToggle = hasProxy;
    const bool visualOn  = isDrawerToggle ? button.getToggleState() : button.getToggleState();
    const bool ledLit    = isDrawerToggle ? circuitOn : button.getToggleState();

    // Subtle Meadow-style drop shadow
    g.setColour (FollowerPalette::inkBlack.withAlpha (0.10f));
    g.fillRoundedRectangle (b.translated (0.0f, 1.0f), 5.0f);

    const juce::Colour fillCol = visualOn ? FollowerPalette::inkBlack : FollowerPalette::panelRecessed;
    g.setColour (shouldDrawButtonAsHighlighted ? fillCol.brighter (0.07f) : fillCol);
    g.fillRoundedRectangle (b, 5.0f);

    g.setColour (ledLit ? FollowerPalette::ledBlue : FollowerPalette::inkBlack.withAlpha (0.45f));
    g.drawRoundedRectangle (b, 5.0f, ledLit ? 1.35f : 1.1f);

    // Integrated Meadow-style LED indicator dot on the left side of the pill
    const float ledR = 3.0f;
    const auto ledCentre = juce::Point<float> (b.getX() + 9.5f, b.getCentreY());
    if (ledLit)
    {
        g.setColour (FollowerPalette::ledBlueGlow.withAlpha (0.38f));
        g.fillEllipse (ledCentre.x - ledR * 1.9f, ledCentre.y - ledR * 1.9f, ledR * 3.8f, ledR * 3.8f);
        g.setColour (FollowerPalette::ledBlueGlow);
    }
    else
    {
        g.setColour (visualOn ? FollowerPalette::panelSurface.withAlpha (0.28f)
                              : FollowerPalette::inkBlack.withAlpha (0.22f));
    }
    g.fillEllipse (ledCentre.x - ledR, ledCentre.y - ledR, ledR * 2.0f, ledR * 2.0f);

    g.setColour (visualOn ? FollowerPalette::panelSurface : FollowerPalette::inkBlack);
    g.setFont (getInterFont (9.8f, true));
    g.drawText (button.getButtonText(),
                b.withTrimmedLeft (13.0f).reduced (2.0f, 0.0f),
                juce::Justification::centred,
                false);
}

void FollowerLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                const juce::Colour&,
                                                bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto b = button.getLocalBounds().toFloat().reduced (1.5f);

    g.setColour (FollowerPalette::inkBlack.withAlpha (0.10f));
    g.fillRoundedRectangle (b.translated (0.0f, 1.0f), 5.0f);

    g.setColour (shouldDrawButtonAsDown ? FollowerPalette::inkBlack
                                        : (shouldDrawButtonAsHighlighted ? FollowerPalette::panelSurface
                                                                         : FollowerPalette::panelRecessed));
    g.fillRoundedRectangle (b, 5.0f);

    g.setColour (FollowerPalette::inkBlack.withAlpha (0.55f));
    g.drawRoundedRectangle (b, 5.0f, 1.15f);
}

juce::Font FollowerLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return getInterFont (9.8f, true);
}

void FollowerLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                          bool, bool shouldDrawButtonAsDown)
{
    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.setColour (shouldDrawButtonAsDown ? FollowerPalette::panelSurface
                                        : FollowerPalette::inkBlack);
    g.drawText (button.getButtonText(),
                button.getLocalBounds().reduced (6, 0),
                juce::Justification::centred,
                false);
}

juce::Font FollowerLookAndFeel::getLabelFont (juce::Label& label)
{
    return label.getFont().getTypefaceName() == getInterFont (10.0f).getTypefaceName()
               ? label.getFont()
               : getInterFont (label.getFont().getHeight(), true);
}

void FollowerLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                        int, int, int, int, juce::ComboBox& box)
                                        
{
    auto b = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (1.0f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (b, 4.5f);

    g.setColour (box.hasKeyboardFocus (true) ? FollowerPalette::ledBlue : box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (b, 4.5f, 1.1f);

    const float ax = static_cast<float> (width) - 13.0f;
    const float ay = static_cast<float> (height) * 0.5f - 1.5f;
    juce::Path p;
    p.startNewSubPath (ax - 3.5f, ay);
    p.lineTo          (ax,        ay + 3.5f);
    p.lineTo          (ax + 3.5f, ay);
    g.setColour (FollowerPalette::inkBlack);
    g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void FollowerLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 0, box.getWidth() - 22, box.getHeight());
    label.setFont (getInterFont (11.0f, true));
    label.setColour (juce::Label::textColourId, FollowerPalette::inkBlack);
    label.setJustificationType (juce::Justification::centredLeft);
}

juce::Font FollowerLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return getInterFont (11.0f, true);
}

juce::Font FollowerLookAndFeel::getPopupMenuFont()
{
    return getInterFont (12.0f, false);
}

//==============================================================================
FollowerKnob::FollowerKnob (juce::AudioProcessorValueTreeState& apvts,
                            const juce::String& paramID,
                            const juce::String& labelText,
                            const juce::String& unitText,
                            juce::Colour accent)
    : unit (unitText)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setMouseDragSensitivity (170);
    slider.onValueChange = [this] { updateValueText(); };
    setBufferedToImage (true);
    addAndMakeVisible (slider);

    nameLabel.setText (labelText, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setFont (FollowerLookAndFeel::getInterFont (9.5f, true));
    nameLabel.setColour (juce::Label::textColourId, FollowerPalette::slateMuted);
    addAndMakeVisible (nameLabel);

    valueLabel.setJustificationType (juce::Justification::centred);
    valueLabel.setFont (FollowerLookAndFeel::getInterFont (10.0f, true));
    valueLabel.setColour (juce::Label::textColourId, FollowerPalette::inkBlack);
    valueLabel.setEditable (false, true, false);
    valueLabel.onTextChange = [this]
    {
        slider.setValue (valueLabel.getText().getDoubleValue(), juce::sendNotificationSync);
        updateValueText();
    };
    addAndMakeVisible (valueLabel);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramID, slider);
    updateValueText();
}

void FollowerKnob::updateValueText()
{
    const double val = slider.getValue();

    if (unit == "Gate" && val <= -89.5) { valueLabel.setText ("OFF", juce::dontSendNotification); return; }
    if (unit == "HPF"  && val <= 20.5)  { valueLabel.setText ("OFF", juce::dontSendNotification); return; }
    if (unit == "LPF"  && val >= 19950) { valueLabel.setText ("OFF", juce::dontSendNotification); return; }

    if (unit == "HPF" || unit == "LPF" || unit == "Hz")
    {
        if (val >= 1000.0)
            valueLabel.setText (juce::String (val / 1000.0, 2) + " kHz", juce::dontSendNotification);
        else
            valueLabel.setText (juce::String (static_cast<int> (std::round (val))) + " Hz", juce::dontSendNotification);
        return;
    }

    if (unit == "ms")
    {
        valueLabel.setText (juce::String (static_cast<int> (std::round (val))) + " ms", juce::dontSendNotification);
        return;
    }

    if (unit == "Gate")
    {
        valueLabel.setText (juce::String (val, 1) + " dB", juce::dontSendNotification);
        return;
    }

    juce::String txt (val, 1);
    if (unit == "dB" && val > 0.05)
        txt = "+" + txt;

    valueLabel.setText (unit.isEmpty() ? txt : (txt + " " + unit), juce::dontSendNotification);
}

void FollowerKnob::resized()
{
    auto b = getLocalBounds();
    nameLabel.setBounds (b.removeFromTop (13));
    valueLabel.setBounds (b.removeFromBottom (14));
    slider.setBounds (b.reduced (1));
}

//==============================================================================
FollowerAudioProcessorEditor::FollowerAudioProcessorEditor (FollowerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      odGainKnob     (p.apvts, "odGain",     "GAIN"),
      odToneKnob     (p.apvts, "odTone",     "TONE"),
      odLevelKnob    (p.apvts, "odLevel",    "LEVEL"),
      odFatnessKnob  (p.apvts, "odFatness",  "FATNESS"),
      odVoltageKnob  (p.apvts, "odVoltage",  "VOLTAGE", "V"),
      trGainKnob     (p.apvts, "trGain",     "GAIN"),
      trTrebleKnob   (p.apvts, "trTreble",   "TREBLE"),
      trMidKnob      (p.apvts, "trMid",      "MIDDLE"),
      trBassKnob     (p.apvts, "trBass",     "BASS"),
      trMasterKnob   (p.apvts, "trMaster",   "MASTER"),
      orGainKnob     (p.apvts, "orGain",     "GAIN"),
      orTrebleKnob   (p.apvts, "orTreble",   "TREBLE"),
      orMidKnob      (p.apvts, "orMid",      "MIDDLE"),
      orBassKnob     (p.apvts, "orBass",     "BASS"),
      orVolumeKnob   (p.apvts, "orVolume",   "VOLUME"),
      outLevelKnob   (p.apvts, "outLevel",   "OUTPUT",  "dB",   FollowerPalette::inkBlack),
      gateKnob       (p.apvts, "gateThresh", "GATE",    "Gate"),
      hpfKnob        (p.apvts, "hpfFreq",    "HPF",     "HPF"),
      lpfKnob        (p.apvts, "lpfFreq",    "LPF",     "LPF"),
      tremRateKnob   (p.apvts, "tremRate",   "RATE",    "Hz"),
      tremDepthKnob  (p.apvts, "tremDepth",  "DEPTH"),
      tremWaveKnob   (p.apvts, "tremWave",   "WAVE"),
      delayTimeKnob  (p.apvts, "delayTime",  "TIME",    "ms"),
      delayFbKnob    (p.apvts, "delayFb",    "REPEAT"),
      delayMixKnob   (p.apvts, "delayMix",   "MIX"),
      revDecayKnob   (p.apvts, "revDecay",   "DECAY",   "s"),
      revDampKnob    (p.apvts, "revDamp",    "DAMP",    "Hz"),
      revPreDelKnob  (p.apvts, "revPreDel",  "PRE-DEL", "ms"),
      revMixKnob     (p.apvts, "revMix",     "MIX"),
      tapeRateKnob   (p.apvts, "tapeRate",   "RATE"),
      tapeIntKnob    (p.apvts, "tapeInt",    "INTENSITY"),
      tapeEchoKnob   (p.apvts, "tapeEcho",   "ECHO LVL"),
      tapeRevKnob    (p.apvts, "tapeRev",    "SPRING"),
      tapeBassKnob   (p.apvts, "tapeBass",   "BASS",    "dB"),
      tapeTrebleKnob (p.apvts, "tapeTreble", "TREBLE",  "dB"),
      tapeWowKnob    (p.apvts, "tapeWow",    "WOW/FLT"),
      tapeMixKnob    (p.apvts, "tapeMix",    "MIX")
{
    setLookAndFeel (&followerLnf);
    buildSvgLogo();

    audioSettingsBtn.setVisible (juce::JUCEApplicationBase::isStandaloneApp());
    audioSettingsBtn.onClick = [this]
    {
        if (onAudioSettingsClicked)
            onAudioSettingsClicked();
    };

    // Header Drawer Toggles
    showOdAtt    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "odVisible", showOdBtn);
    showPreFxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "preFxVisible", showPreFxBtn);
    showTapeAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "tapeVisible", showTapeBtn);
    showUtilityAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "utilityVisible", showUtilityBtn);
    tunerMuteAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "tunerMute", tunerMuteBtn);
    addAndMakeVisible (showOdBtn);
    addAndMakeVisible (showPreFxBtn);
    addAndMakeVisible (showTapeBtn);
   addAndMakeVisible (showUtilityBtn);
    showUtilityBtn.setButtonText ("");
    showUtilityBtn.setTooltip ("Toggle Chromatic Tuner & I/O Meters");

    lastOdActive   = p.apvts.getRawParameterValue ("odOn")->load() > 0.5f;
    lastPreActive  = (p.apvts.getRawParameterValue ("tremOn")->load() > 0.5f)
                  || (p.apvts.getRawParameterValue ("delayOn")->load() > 0.5f)
                  || (p.apvts.getRawParameterValue ("revOn")->load() > 0.5f);
    lastTapeActive = p.apvts.getRawParameterValue ("tapeOn")->load() > 0.5f;

    showOdBtn.getProperties().set ("circuitActive", lastOdActive);
    showPreFxBtn.getProperties().set ("circuitActive", lastPreActive);
    showTapeBtn.getProperties().set ("circuitActive", lastTapeActive);

    // Audio button for standalone app
    addChildComponent (audioSettingsBtn);
    audioSettingsBtn.setVisible (juce::JUCEApplicationBase::isStandaloneApp());
    audioSettingsBtn.onClick = [this]
    {
        if (onAudioSettingsClicked)
            onAudioSettingsClicked();
    };

    // Tuner mute button
    addChildComponent (tunerMuteBtn);

    // Preset Browser
    for (auto* comp : { static_cast<juce::Component*> (&presetPrevBtn),
                        static_cast<juce::Component*> (&presetBox),
                        static_cast<juce::Component*> (&presetNextBtn),
                        static_cast<juce::Component*> (&presetSaveBtn) })
    {
        addChildComponent (comp);
    }

    presetBox.setJustificationType (juce::Justification::centredLeft);
    refreshPresetBox();

    presetPrevBtn.onClick = [this]
    {
        audioProcessor.presetManager->loadPreviousPreset();
        refreshPresetBox();
    };

    presetNextBtn.onClick = [this]
    {
        audioProcessor.presetManager->loadNextPreset();
        refreshPresetBox();
    };

    presetBox.onChange = [this]
    {
        const int selId = presetBox.getSelectedId();
        if (selId > 0)
            audioProcessor.presetManager->loadPreset (selId - 1);
    };

    presetSaveBtn.onClick = [this]
    {
        auto* alert = new juce::AlertWindow ("Save Preset",
                                            "Enter a name for this Follower preset:",
                                            juce::MessageBoxIconType::NoIcon);
        alert->addTextEditor ("presetName", audioProcessor.presetManager->getCurrentPresetName(), "Preset Name:");
        alert->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        alert->enterModalState (true, juce::ModalCallbackFunction::create (
            [this, alert] (int result)
            {
                if (result == 1)
                {
                    audioProcessor.presetManager->savePreset (alert->getTextEditorContents ("presetName"));
                    refreshPresetBox();
                }
                delete alert;
            }));
    };

    // Left Overdrive Drawer
    odOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "odOn", odOnBtn);
    addAndMakeVisible (odOnBtn);
    for (auto* k : { &odGainKnob, &odToneKnob, &odLevelKnob, &odFatnessKnob, &odVoltageKnob })
        addAndMakeVisible (k);

    // Center Dual Amp Switcher
    ampToggleBtn.onClick = [this]
    {
        if (auto* param = audioProcessor.apvts.getParameter ("ampSelect"))
        {
            const int cur  = static_cast<int> (audioProcessor.apvts.getRawParameterValue ("ampSelect")->load());
            const int next = (cur == 0) ? 1 : 0;
            param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (next)));
            syncVisibilityAndDimensions (false);
        }
    };
    addAndMakeVisible (ampToggleBtn);

    trBoostAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "trBoost", trBoostBtn);
    addAndMakeVisible (trBoostBtn);
    for (auto* k : { &trGainKnob, &trTrebleKnob, &trMidKnob, &trBassKnob, &trMasterKnob })
        addAndMakeVisible (k);

    orDirtyAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "orDirty", orDirtyBtn);
    addAndMakeVisible (orDirtyBtn);
    for (auto* k : { &orGainKnob, &orTrebleKnob, &orMidKnob, &orBassKnob, &orVolumeKnob })
        addAndMakeVisible (k);

    addAndMakeVisible (outLevelKnob);

    // Cabinet IR & Surgical Filters
    irSelector.addItemList ({ "MARS T75 VINTAGE", "MEGA V30 PUNCHY", "ORNG V30 DARK", "FEND JEN GRITTY" }, 1);
    irAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "irSelect", irSelector);
    irSelector.onChange = [this]
    {
        audioProcessor.selectBuiltInIr (irSelector.getSelectedItemIndex());
        repaint();
    };
    addAndMakeVisible (irSelector);

    loadIrBtn.onClick = [this] { openIrFileChooser(); };
    addAndMakeVisible (loadIrBtn);

    cabBypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "cabBypass", cabBypassBtn);
    addAndMakeVisible (cabBypassBtn);

    gateFastAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "gateFast", gateFastBtn);
    addAndMakeVisible (gateFastBtn);

    for (auto* k : { &gateKnob, &hpfKnob, &lpfKnob })
        addAndMakeVisible (k);

    // Bottom Pre-Amp FX Drawer
    tremOnAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "tremOn", tremOnBtn);
    delayOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "delayOn", delayOnBtn);
    revOnAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "revOn", revOnBtn);
    addAndMakeVisible (tremOnBtn);
    addAndMakeVisible (delayOnBtn);
    addAndMakeVisible (revOnBtn);

    for (auto* k : { &tremRateKnob, &tremDepthKnob, &tremWaveKnob,
                     &delayTimeKnob, &delayFbKnob, &delayMixKnob,
                     &revDecayKnob, &revDampKnob, &revPreDelKnob, &revMixKnob })
        addAndMakeVisible (k);

    // Right Tape Echo Drawer
    tapeOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "tapeOn", tapeOnBtn);
    addAndMakeVisible (tapeOnBtn);

    tapeModeBox.addItemList ({
        "1: HEAD 1", "2: HEAD 2", "3: HEAD 3", "4: HEAD 2+3",
        "5: H1 + REV", "6: H2 + REV", "7: H3 + REV", "8: H2+3 + REV",
        "9: H1+2 + REV", "10: H1+3 + REV", "11: ALL + REV", "12: REVERB ONLY"
    }, 1);
    tapeModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "tapeMode", tapeModeBox);
    addAndMakeVisible (tapeModeBox);

    tapeAgeBox.addItemList ({ "AGE: NEW", "AGE: USED", "AGE: OLD" }, 1);
    tapeAgeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "tapeAge", tapeAgeBox);
    addAndMakeVisible (tapeAgeBox);

    for (auto* k : { &tapeRateKnob, &tapeIntKnob, &tapeEchoKnob, &tapeRevKnob,
                     &tapeBassKnob, &tapeTrebleKnob, &tapeWowKnob, &tapeMixKnob })
        addAndMakeVisible (k);

    syncVisibilityAndDimensions (true);
    startTimerHz (30); // Reduced from 60Hz to 30Hz for better performance
}

FollowerAudioProcessorEditor::~FollowerAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void FollowerAudioProcessorEditor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    auto& pm = *audioProcessor.presetManager;
    const int count = pm.getNumPresets();

    for (int i = 0; i < count; ++i)
        presetBox.addItem (pm.getPresetName (i), i + 1);

    const int idx = pm.getCurrentPresetIndex();
    if (idx >= 0)
        presetBox.setSelectedId (idx + 1, juce::dontSendNotification);
    else
        presetBox.setText (pm.getCurrentPresetName(), juce::dontSendNotification);
}

void FollowerAudioProcessorEditor::buildSvgLogo()
{
    followerLogoSvg = juce::Drawable::createFromImageData (
        BinaryData::FollowerLogo_svg, BinaryData::FollowerLogo_svgSize);
    jewelIndicatorSvg = juce::Drawable::createFromImageData (
        BinaryData::JewelIndicator_svg, BinaryData::JewelIndicator_svgSize);
}

void FollowerAudioProcessorEditor::openIrFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Select Speaker Cabinet Impulse Response (.wav)",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.wav;*.aif;*.aiff");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            const auto result = fc.getResult();
            if (result.existsAsFile())
            {
                audioProcessor.loadUserIrFile (result);
                repaint();
            }
        });
}

void FollowerAudioProcessorEditor::syncVisibilityAndDimensions (bool immediate)
{
    const float targetOd    = (audioProcessor.apvts.getRawParameterValue ("odVisible")->load() > 0.5f) ? 1.0f : 0.0f;
    const float targetPreFx = (audioProcessor.apvts.getRawParameterValue ("preFxVisible")->load() > 0.5f) ? 1.0f : 0.0f;
    const float targetTape  = (audioProcessor.apvts.getRawParameterValue ("tapeVisible")->load() > 0.5f) ? 1.0f : 0.0f;
    auto*       utilParam   = audioProcessor.apvts.getRawParameterValue ("utilityVisible");
    const float targetUtility = (utilParam != nullptr && utilParam->load() > 0.5f) ? 1.0f : 0.0f;
    const int   ampChoice   = static_cast<int> (audioProcessor.apvts.getRawParameterValue ("ampSelect")->load());
    const bool  gateFast    = audioProcessor.apvts.getRawParameterValue ("gateFast")->load() > 0.5f;

    if (immediate)
    {
        odGlide      = targetOd;
        preFxGlide   = targetPreFx;
        tapeGlide    = targetTape;
        utilityGlide = targetUtility;
    }

    lastAmpChoice     = ampChoice;
    lastGateFastState = gateFast;

    ampToggleBtn.setButtonText (ampChoice == 0 ? "AMP: TRAINER MARK III" : "AMP: ROCKER 100");
    gateFastBtn.setButtonText  (gateFast ? "REL: FAST 12ms" : "REL: SLOW 85ms");

    // Left Overdrive visibility (show once drawer begins opening)
    const bool showOdControls = (odGlide > 0.15f);
    odOnBtn.setVisible (showOdControls);
    for (auto* k : { &odGainKnob, &odToneKnob, &odLevelKnob, &odFatnessKnob, &odVoltageKnob })
        k->setVisible (showOdControls);

    // Amp channel visibility (preserves separate Trainer Mark III vs Rocker 100 controls)
    const bool isTrainer = (ampChoice == 0);
    trBoostBtn.setVisible (isTrainer);
    for (auto* k : { &trGainKnob, &trTrebleKnob, &trMidKnob, &trBassKnob, &trMasterKnob })
        k->setVisible (isTrainer);

    orDirtyBtn.setVisible (! isTrainer);
    for (auto* k : { &orGainKnob, &orTrebleKnob, &orMidKnob, &orBassKnob, &orVolumeKnob })
        k->setVisible (! isTrainer);

    // Bottom Pre-FX visibility
    const bool showPreControls = (preFxGlide > 0.15f);
    tremOnBtn.setVisible (showPreControls);
    delayOnBtn.setVisible (showPreControls);
    revOnBtn.setVisible (showPreControls);
    for (auto* k : { &tremRateKnob, &tremDepthKnob, &tremWaveKnob,
                     &delayTimeKnob, &delayFbKnob, &delayMixKnob,
                     &revDecayKnob, &revDampKnob, &revPreDelKnob, &revMixKnob })
        k->setVisible (showPreControls);

    // Right Tape Echo visibility
    const bool showTapeControls = (tapeGlide > 0.15f);
    tapeOnBtn.setVisible (showTapeControls);
    tapeModeBox.setVisible (showTapeControls);
    tapeAgeBox.setVisible (showTapeControls);
    for (auto* k : { &tapeRateKnob, &tapeIntKnob, &tapeEchoKnob, &tapeRevKnob,
                     &tapeBassKnob, &tapeTrebleKnob, &tapeWowKnob, &tapeMixKnob })
        k->setVisible (showTapeControls);

    const int width  = 648 + juce::roundToInt (odGlide * 226.0f) + juce::roundToInt (tapeGlide * 266.0f);
    const int height = 402 + juce::roundToInt (preFxGlide * 142.0f) + juce::roundToInt (utilityGlide * 48.0f);

    if (getWidth() != width || getHeight() != height)
        setSize (width, height);
    else
        resized();

    repaint();
}

void FollowerAudioProcessorEditor::timerCallback()
{
    const float targetOd    = (audioProcessor.apvts.getRawParameterValue ("odVisible")->load() > 0.5f) ? 1.0f : 0.0f;
    const float targetPreFx = (audioProcessor.apvts.getRawParameterValue ("preFxVisible")->load() > 0.5f) ? 1.0f : 0.0f;
    const float targetTape  = (audioProcessor.apvts.getRawParameterValue ("tapeVisible")->load() > 0.5f) ? 1.0f : 0.0f;
    const int   ampChoice   = static_cast<int> (audioProcessor.apvts.getRawParameterValue ("ampSelect")->load());
    const bool  gateFast    = audioProcessor.apvts.getRawParameterValue ("gateFast")->load() > 0.5f;
    auto* utilParam = audioProcessor.apvts.getRawParameterValue ("utilityVisible");
    const float targetUtility = (utilParam != nullptr && utilParam->load() > 0.5f) ? 1.0f : 0.0f;

    // Smooth exponential ease for drawer gliding (~180ms settle time at 30Hz)
    constexpr float glideSpeed = 0.42f; // Increased from 0.24f 60Hz speed
    bool animating = false;

    auto stepGlide = [&] (float& current, float target)
    {
        const float diff = target - current;
        if (std::abs (diff) > 0.004f)
        {
            current += diff * glideSpeed;
            animating = true;
        }
        else if (current != target)
        {
            current = target;
            animating = true;
        }
    };

    stepGlide (odGlide,    targetOd);
    stepGlide (preFxGlide, targetPreFx);
    stepGlide (tapeGlide,  targetTape);
    stepGlide (utilityGlide,  targetUtility);

    if (animating || ampChoice != lastAmpChoice || gateFast != lastGateFastState)
        syncVisibilityAndDimensions (false);

    // Sync header pill active LED border/dot with circuit bypass states
    const bool odActive   = audioProcessor.apvts.getRawParameterValue ("odOn")->load() > 0.5f;
    const bool preActive  = (audioProcessor.apvts.getRawParameterValue ("tremOn")->load() > 0.5f)
                         || (audioProcessor.apvts.getRawParameterValue ("delayOn")->load() > 0.5f)
                         || (audioProcessor.apvts.getRawParameterValue ("revOn")->load() > 0.5f);
    const bool tapeActive = audioProcessor.apvts.getRawParameterValue ("tapeOn")->load() > 0.5f;

    if (odActive != lastOdActive)
    {
        lastOdActive = odActive;
        showOdBtn.getProperties().set ("circuitActive", odActive);
        showOdBtn.repaint();
    }

    if (preActive != lastPreActive)
    {
        lastPreActive = preActive;
        showPreFxBtn.getProperties().set ("circuitActive", preActive);
        showPreFxBtn.repaint();
    }

    if (tapeActive != lastTapeActive)
    {
        lastTapeActive = tapeActive;
        showTapeBtn.getProperties().set ("circuitActive", tapeActive);
        showTapeBtn.repaint();
    }

    if (utilityGlide > 0.15f)
    {
        auto updateMeter = [] (float raw, float& smooth, float& hold, int& holdFrames)
        {
            smooth = (raw > smooth) ? raw : (smooth * 0.80f);
            if (raw >= hold)
            {
                hold = raw;
                holdFrames = 24;
            }
            else if (holdFrames > 0)
            {
                --holdFrames;
            }
            else
            {
                hold *= 0.88f;
            }
        };

        updateMeter (audioProcessor.inputPeakLevel.load (std::memory_order_relaxed),
                    smoothInPeak, holdInPeak, inHoldFrames);
        updateMeter (audioProcessor.outputPeakLevel.load (std::memory_order_relaxed),
                    smoothOutPeak, holdOutPeak, outHoldFrames);

        auto reading = audioProcessor.tuner.computePitch();
        if (reading.valid)
        {
            currentTunerReading = reading;
            smoothCents += (reading.cents - smoothCents) * 0.35f;
            tunerHoldFrames = 12;
        }
        else if (tunerHoldFrames > 0)
        {
            --tunerHoldFrames;
        }
        else
        {
            currentTunerReading.valid = false;
        }

        repaint (utilityPanelBounds);
    }
}

void FollowerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (14);

    // Header Bar (Top Left Vector SVG Logo, Top Right Drawer Pills + Blue Jewel)
    auto header = bounds.removeFromTop (46);
    header.removeFromRight (36); // Reserve space for top-right blue LED
    showTapeBtn.setBounds  (header.removeFromRight (94).reduced (2, 9));
    showPreFxBtn.setBounds (header.removeFromRight (80).reduced (2, 9));
    showOdBtn.setBounds    (header.removeFromRight (98).reduced (2, 9));
    showUtilityBtn.setBounds (header.removeFromRight (46).reduced (2, 9));
    bounds.removeFromTop (8);
    if (utilityGlide > 0.02f)
    {
        const int utilH = juce::roundToInt (40.0f * utilityGlide);
        utilityPanelBounds = bounds.removeFromTop (utilH);
        bounds.removeFromTop (juce::roundToInt (8.0f * utilityGlide));

        auto centerRow = utilityPanelBounds.reduced (196, 8);
        tunerMuteBtn.setVisible (utilityGlide > 0.35f);
        tunerMuteBtn.setBounds (centerRow.removeFromRight (52));

        if (juce::JUCEApplicationBase::isStandaloneApp())
        {
            centerRow.removeFromRight (6);
            audioSettingsBtn.setVisible (utilityGlide > 0.35f);
            audioSettingsBtn.setBounds (centerRow.removeFromRight (58));
        }
    }
    else
    {
        utilityPanelBounds = {};
        tunerMuteBtn.setVisible (false);
        audioSettingsBtn.setVisible (false);
    }

    // Show Preset Strip only at full width
    const bool showPresetStrip = (odGlide > 0.92f && tapeGlide > 0.92f);
    presetPrevBtn.setVisible (showPresetStrip);
    presetBox.setVisible     (showPresetStrip);
    presetNextBtn.setVisible (showPresetStrip);
    presetSaveBtn.setVisible (showPresetStrip);

    if (showPresetStrip)
    {
        // Total strip width = 22 + 2 + 168 + 2 + 22 + 6 + 44 = 266 px, ending 12 px left of showUtilityBtn
        int stripRight = showUtilityBtn.getX() - 12;
        const int y    = 25;
        const int h    = 24;

        presetSaveBtn.setBounds (stripRight - 44, y, 44, h);
        stripRight -= (44 + 6);

        presetNextBtn.setBounds (stripRight - 22, y, 22, h);
        stripRight -= (22 + 2);

        presetBox.setBounds     (stripRight - 168, y, 168, h);
        stripRight -= (168 + 2);

        presetPrevBtn.setBounds (stripRight - 22, y, 22, h);
    }

    // Bottom Collapsible Drawer: Pre-Amp Time-Based Effects (glides vertically)
    if (preFxGlide > 0.02f)
    {
        const int drawerH = juce::roundToInt (134.0f * preFxGlide);
        preFxPanelBounds = bounds.removeFromBottom (drawerH);
        bounds.removeFromBottom (juce::roundToInt (8.0f * preFxGlide));

        if (preFxGlide > 0.15f)
        {
            auto pInner = preFxPanelBounds.reduced (10, 6);
            pInner.removeFromTop (16);

            const int thirdW = pInner.getWidth() / 3;
            auto tremArea  = pInner.removeFromLeft (thirdW).reduced (4, 0);
            auto delayArea = pInner.removeFromLeft (thirdW).reduced (4, 0);
            auto revArea   = pInner.reduced (4, 0);

            auto tTop = tremArea.removeFromTop (21);
            tremOnBtn.setBounds (tTop.removeFromRight (74));
            const int tw = tremArea.getWidth() / 3;
            tremRateKnob.setBounds  (tremArea.removeFromLeft (tw));
            tremDepthKnob.setBounds (tremArea.removeFromLeft (tw));
            tremWaveKnob.setBounds  (tremArea);

            auto dTop = delayArea.removeFromTop (21);
            delayOnBtn.setBounds (dTop.removeFromRight (74));
            const int dw = delayArea.getWidth() / 3;
            delayTimeKnob.setBounds (delayArea.removeFromLeft (dw));
            delayFbKnob.setBounds   (delayArea.removeFromLeft (dw));
            delayMixKnob.setBounds  (delayArea);

            auto rTop = revArea.removeFromTop (21);
            revOnBtn.setBounds (rTop.removeFromRight (76));
            const int rw = revArea.getWidth() / 4;
            revDecayKnob.setBounds  (revArea.removeFromLeft (rw));
            revDampKnob.setBounds   (revArea.removeFromLeft (rw));
            revPreDelKnob.setBounds (revArea.removeFromLeft (rw));
            revMixKnob.setBounds    (revArea);
        }
    }
    else
    {
        preFxPanelBounds = {};
    }

    // Left Collapsible Drawer: Overdrive (glides horizontally)
    if (odGlide > 0.02f)
    {
        const int odW = juce::roundToInt (217.0f * odGlide);
        odPanelBounds = bounds.removeFromLeft (odW);
        bounds.removeFromLeft (juce::roundToInt (9.0f * odGlide));

        if (odGlide > 0.15f)
        {
            // Lay out against full target width (217px) so knobs slide cleanly without squishing
            auto oInner = juce::Rectangle<int> (odPanelBounds.getRight() - 217,
                                                odPanelBounds.getY(),
                                                217,
                                                odPanelBounds.getHeight()).reduced (10, 8);
            auto oTop = oInner.removeFromTop (22);
            odOnBtn.setBounds (oTop.removeFromRight (84));
            oInner.removeFromTop (8);

            auto row1 = oInner.removeFromTop (114);
            const int w3 = row1.getWidth() / 3;
            odGainKnob.setBounds  (row1.removeFromLeft (w3).reduced (2));
            odToneKnob.setBounds  (row1.removeFromLeft (w3).reduced (2));
            odLevelKnob.setBounds (row1.reduced (2));

            oInner.removeFromTop (8);
            auto row2 = oInner.removeFromTop (114);
            const int w2 = row2.getWidth() / 2;
            odFatnessKnob.setBounds (row2.removeFromLeft (w2).reduced (6, 2));
            odVoltageKnob.setBounds (row2.reduced (6, 2));
        }
    }
    else
    {
        odPanelBounds = {};
    }

    // Right Collapsible Drawer: Post-Cab Dusk Audio Tape Echo (glides horizontally)
    if (tapeGlide > 0.02f)
    {
        const int tapeW = juce::roundToInt (257.0f * tapeGlide);
        tapePanelBounds = bounds.removeFromRight (tapeW);
        bounds.removeFromRight (juce::roundToInt (9.0f * tapeGlide));

        if (tapeGlide > 0.15f)
        {
            auto tInner = juce::Rectangle<int> (tapePanelBounds.getX(),
                                                tapePanelBounds.getY(),
                                                257,
                                                tapePanelBounds.getHeight()).reduced (10, 8);
            auto tTop = tInner.removeFromTop (22);
            tapeOnBtn.setBounds (tTop.removeFromRight (80));
            tInner.removeFromTop (6);

            auto comboRow = tInner.removeFromTop (24);
            tapeModeBox.setBounds (comboRow.removeFromLeft (static_cast<int> (comboRow.getWidth() * 0.56f)));
            comboRow.removeFromLeft (6);
            tapeAgeBox.setBounds (comboRow);
            tInner.removeFromTop (6);

            auto row1 = tInner.removeFromTop (104);
            const int w4 = row1.getWidth() / 4;
            tapeRateKnob.setBounds (row1.removeFromLeft (w4).reduced (1));
            tapeIntKnob.setBounds  (row1.removeFromLeft (w4).reduced (1));
            tapeEchoKnob.setBounds (row1.removeFromLeft (w4).reduced (1));
            tapeRevKnob.setBounds  (row1.reduced (1));

            tInner.removeFromTop (4);
            auto row2 = tInner.removeFromTop (104);
            tapeBassKnob.setBounds   (row2.removeFromLeft (w4).reduced (1));
            tapeTrebleKnob.setBounds (row2.removeFromLeft (w4).reduced (1));
            tapeWowKnob.setBounds    (row2.removeFromLeft (w4).reduced (1));
            tapeMixKnob.setBounds    (row2.reduced (1));
        }
    }
    else
    {
        tapePanelBounds = {};
    }

    // Center Stack: Top = Dual Amp Panel, Bottom = Cabinet IR & Surgical Filters
    cabPanelBounds = bounds.removeFromBottom (136);
    bounds.removeFromBottom (8);
    ampPanelBounds = bounds;

    // Amp Panel Layout
    auto aInner = ampPanelBounds.reduced (12, 8);
    auto aTop   = aInner.removeFromTop (24);
    ampToggleBtn.setBounds (aTop.removeFromLeft (195));
    trBoostBtn.setBounds   (aTop.removeFromRight (114));
    orDirtyBtn.setBounds   (trBoostBtn.getBounds());
    aInner.removeFromTop (8);

    const int colW = aInner.getWidth() / 6;
    trGainKnob.setBounds   (aInner.removeFromLeft (colW).reduced (2));
    trTrebleKnob.setBounds (aInner.removeFromLeft (colW).reduced (2));
    trMidKnob.setBounds    (aInner.removeFromLeft (colW).reduced (2));
    trBassKnob.setBounds   (aInner.removeFromLeft (colW).reduced (2));
    trMasterKnob.setBounds (aInner.removeFromLeft (colW).reduced (2));
    outLevelKnob.setBounds (aInner.reduced (2));

    orGainKnob.setBounds   (trGainKnob.getBounds());
    orTrebleKnob.setBounds (trTrebleKnob.getBounds());
    orMidKnob.setBounds    (trMidKnob.getBounds());
    orBassKnob.setBounds   (trBassKnob.getBounds());
    orVolumeKnob.setBounds (trMasterKnob.getBounds());

    // Cabinet IR & Gate (w/ Fast/Slow release toggle) / HPF / LPF Layout
    auto cInner = cabPanelBounds.reduced (12, 8);
    auto cLeft  = cInner.removeFromLeft (static_cast<int> (cInner.getWidth() * 0.44f));
    cInner.removeFromLeft (10);

    cLeft.removeFromTop (20);
    irSelector.setBounds (cLeft.removeFromTop (26));
    cLeft.removeFromTop (8);
    auto cBtns = cLeft.removeFromTop (24);
    loadIrBtn.setBounds    (cBtns.removeFromLeft (cBtns.getWidth() / 2 - 3));
    cBtns.removeFromLeft (6);
    cabBypassBtn.setBounds (cBtns);

    // Top row of right half has Gate Fast/Slow toggle aligned right
    auto filterTopRow = cInner.removeFromTop (20);
    gateFastBtn.setBounds (filterTopRow.removeFromRight (106));

    const int fColW = cInner.getWidth() / 3;
    gateKnob.setBounds (cInner.removeFromLeft (fColW).reduced (2));
    hpfKnob.setBounds  (cInner.removeFromLeft (fColW).reduced (2));
    lpfKnob.setBounds  (cInner.reduced (2));
}

void FollowerAudioProcessorEditor::drawSvgHeaderLogo (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    if (followerLogoSvg != nullptr)
    {
        followerLogoSvg->drawWithin (g, bounds, juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yMid, 1.0f);
    }
    else
    {
        g.setColour (FollowerPalette::inkBlack);
        auto font = FollowerLookAndFeel::getGaramondFont (28.0f, false);
        font.setExtraKerningFactor (0.14f);
        g.setFont (font);
        g.drawText ("FOLLOWER", bounds, juce::Justification::centredLeft, false);
    }
}

void FollowerAudioProcessorEditor::drawBlueJewelIndicator (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const auto square = bounds.withSizeKeepingCentre (28.0f, 28.0f);
    const auto centre = square.getCentre();

    // Soft, wide-radius diffused optical bloom around the jewel bezel
    const float glowRadius = 24.0f + smoothJewelGlow * 10.0f;
    const float peakAlpha  = 0.16f + smoothJewelGlow * 0.34f;

    juce::ColourGradient diffusedBloom (FollowerPalette::ledBlueGlow.withAlpha (peakAlpha),
                                        centre.x, centre.y,
                                        FollowerPalette::ledBlueGlow.withAlpha (0.0f),
                                        centre.x + glowRadius, centre.y,
                                        true);
    diffusedBloom.addColour (0.35, FollowerPalette::ledBlueGlow.withAlpha (peakAlpha * 0.55f));
    diffusedBloom.addColour (0.68, FollowerPalette::ledBlueGlow.withAlpha (peakAlpha * 0.18f));

    g.setGradientFill (diffusedBloom);
    g.fillEllipse (juce::Rectangle<float> (glowRadius * 2.0f, glowRadius * 2.0f).withCentre (centre));

    if (jewelIndicatorSvg != nullptr)
    {
        jewelIndicatorSvg->drawWithin (g, square, juce::RectanglePlacement::centred, 1.0f);

        // Soft feathered specular bloom across the faceted lens dome on transients
        if (smoothJewelGlow > 0.02f)
        {
            const float lensRadius = 7.2f;
            juce::ColourGradient lensBloom (juce::Colours::white.withAlpha (smoothJewelGlow * 0.28f),
                                            centre.x, centre.y,
                                            juce::Colours::white.withAlpha (0.0f),
                                            centre.x + lensRadius, centre.y,
                                            true);
            g.setGradientFill (lensBloom);
            g.fillEllipse (juce::Rectangle<float> (lensRadius * 2.0f, lensRadius * 2.0f).withCentre (centre));
        }
    }
}

void FollowerAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // Gentle white-to-off-white radial/vertical chassis gradient (Meadow faceplate finish)
    juce::ColourGradient bgGrad (FollowerPalette::bgWhiteTop,    bounds.getCentreX(), 0.0f,
                                 FollowerPalette::bgWhiteBottom, bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (bgGrad);
    g.fillAll();

    // Crisp Meadow-style inner border
    g.setColour (FollowerPalette::inkBlack.withAlpha (0.32f));
    g.drawRoundedRectangle (bounds.reduced (5.0f), 7.0f, 1.1f);

    // Header Bar
    auto header = getLocalBounds().reduced (14).removeFromTop (46).toFloat();
    drawSvgHeaderLogo (g, header.removeFromLeft (192.0f));

    // Only draw subtitle when the full text fits cleanly before the leftmost pill button
    {
        const juce::String subtitleText = "DUAL-AMP PRE & POST TIME-BASED RIG";
        const auto subtitleFont = FollowerLookAndFeel::getInterFont (10.0f, true);
        const float rightLimit = presetPrevBtn.isVisible()
                             ? static_cast<float> (presetPrevBtn.getX()) - 12.0f
                             : static_cast<float> (showUtilityBtn.getX()) - 12.0f;
        const auto subtitleArea = header.withTrimmedLeft (28.0f).withRight (rightLimit);
        const float textWidth   = juce::GlyphArrangement::getStringWidth (subtitleFont, subtitleText);

        if (subtitleArea.getWidth() >= textWidth + 2.0f)
        {
            g.setColour (FollowerPalette::slateMuted);
            g.setFont (subtitleFont);
            g.drawText (subtitleText, subtitleArea, juce::Justification::centredLeft, false);
        }
    }

    drawBlueJewelIndicator (g, header.removeFromRight (30.0f));

    // Hairline divider under header
    g.setColour (FollowerPalette::inkBlack.withAlpha (0.14f));
    g.drawLine (16.0f, 62.0f, static_cast<float> (getWidth()) - 16.0f, 62.0f, 1.0f);

    auto drawCard = [&g] (juce::Rectangle<int> rect, const juce::String& title, float alpha = 1.0f)
    {
        if (rect.isEmpty() || alpha <= 0.02f) return;
        const auto rf = rect.toFloat();

        g.setColour (FollowerPalette::inkBlack.withAlpha (0.04f * alpha));
        g.fillRoundedRectangle (rf.translated (0.0f, 1.5f), 8.0f);

        g.setColour (FollowerPalette::panelSurface.withAlpha (alpha));
        g.fillRoundedRectangle (rf, 8.0f);

        g.setColour (FollowerPalette::inkBlack.withAlpha (0.24f * alpha));
        g.drawRoundedRectangle (rf, 8.0f, 1.0f);

        if (title.isNotEmpty() && alpha > 0.25f)
        {
            auto tArea = rect.reduced (11, 8).removeFromTop (18);
            g.setColour (FollowerPalette::inkBlack.withAlpha (0.88f * alpha));
            g.setFont (FollowerLookAndFeel::getInterFont (10.2f, true));
            g.drawText (title, tArea, juce::Justification::centredLeft);
        }
    };

    drawCard (odPanelBounds,    "OVERDRIVE", odGlide);
    drawCard (ampPanelBounds,   {}, 1.0f);
    drawCard (cabPanelBounds,   {}, 1.0f);
    drawCard (preFxPanelBounds, "PRE-AMP TIME-BASED EFFECTS", preFxGlide);
    drawCard (tapePanelBounds,  "DUSK AUDIO TAPE ECHO", tapeGlide);

    // Utility panel sub-header
    if (utilityGlide > 0.35f)
    {
        auto uInner = utilityPanelBounds.reduced (12, 8).toFloat();
        const float meterW = 175.0f;

        auto drawHorizontalMeter = [&] (juce::Rectangle<float> area, const juce::String& label,
                                        float linearPeak, float linearHold)
        {
            const float db     = juce::Decibels::gainToDecibels (linearPeak, -60.0f);
            const float holdDb = juce::Decibels::gainToDecibels (linearHold, -60.0f);
            const float norm   = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f);
            const float holdN  = juce::jlimit (0.0f, 1.0f, (holdDb + 60.0f) / 66.0f);
            const bool  clip   = (db > -0.1f);

            g.setColour (FollowerPalette::slateMuted.withAlpha (utilityGlide));
            g.setFont (FollowerLookAndFeel::getInterFont (9.5f, true));
            g.drawText (label, area.removeFromLeft (34.0f), juce::Justification::centredLeft);

            const juce::String dbStr = (db <= -59.5f) ? "-inf" : juce::String (db, 1) + " dB";
            g.setColour ((clip ? juce::Colour (0xFFE53935) : FollowerPalette::inkBlack).withAlpha (utilityGlide));
            g.drawText (dbStr, area.removeFromRight (48.0f), juce::Justification::centredRight);

            auto track = area.reduced (4.0f, 6.0f);
            g.setColour (FollowerPalette::panelRecessed.withAlpha (utilityGlide));
            g.fillRoundedRectangle (track, 3.0f);

            if (norm > 0.005f)
            {
                auto fill = track.withWidth (track.getWidth() * norm);
                g.setColour ((clip ? juce::Colour (0xFFE53935) : FollowerPalette::ledBlue).withAlpha (utilityGlide));
                g.fillRoundedRectangle (fill, 3.0f);
            }

            if (holdN > 0.01f)
            {
                const float hx = track.getX() + track.getWidth() * holdN;
                g.setColour (FollowerPalette::inkBlack.withAlpha (0.75f * utilityGlide));
                g.drawLine (hx, track.getY(), hx, track.getBottom(), 1.5f);
            }

            g.setColour (FollowerPalette::borderHairline.withAlpha (utilityGlide));
            g.drawRoundedRectangle (track, 3.0f, 1.0f);
        };

        drawHorizontalMeter (uInner.removeFromLeft (meterW),  "IN",  smoothInPeak,  holdInPeak);
        drawHorizontalMeter (uInner.removeFromRight (meterW), "OUT", smoothOutPeak, holdOutPeak);

        // Chromatic Tuner in the remaining center space
        auto tunerArea = uInner;
        tunerArea.removeFromLeft (8.0f);
        tunerArea.removeFromRight (juce::JUCEApplicationBase::isStandaloneApp() ? 124.0f : 62.0f);

        auto noteBox = tunerArea.removeFromLeft (44.0f);
        const bool inTune = currentTunerReading.valid && std::abs (smoothCents) <= 3.0f;

        g.setColour ((inTune ? FollowerPalette::ledBlue : FollowerPalette::inkBlack).withAlpha (utilityGlide));
        g.setFont (FollowerLookAndFeel::getInterFont (14.0f, true));
        g.drawText (currentTunerReading.valid ? currentTunerReading.noteName : "--",
                    noteBox, juce::Justification::centred);

        auto centsTrack = tunerArea.reduced (4.0f, 6.0f);
        g.setColour (FollowerPalette::panelRecessed.withAlpha (utilityGlide));
        g.fillRoundedRectangle (centsTrack, 3.0f);

        const float centerX = centsTrack.getCentreX();
        g.setColour (FollowerPalette::inkBlack.withAlpha (0.35f * utilityGlide));
        g.drawLine (centerX, centsTrack.getY() + 1.0f, centerX, centsTrack.getBottom() - 1.0f, 1.5f);

        if (currentTunerReading.valid)
        {
            const float normOffset = juce::jlimit (-1.0f, 1.0f, smoothCents / 50.0f);
            const float indicatorX = centerX + normOffset * (centsTrack.getWidth() * 0.46f);
            auto pillRect = juce::Rectangle<float> (10.0f, centsTrack.getHeight() - 2.0f)
                                .withCentre ({ indicatorX, centsTrack.getCentreY() });

            g.setColour ((inTune ? FollowerPalette::ledBlue : FollowerPalette::inkBlack).withAlpha (utilityGlide));
            g.fillRoundedRectangle (pillRect, 2.5f);
        }

        g.setColour (FollowerPalette::borderHairline.withAlpha (utilityGlide));
        g.drawRoundedRectangle (centsTrack, 3.0f, 1.0f);
    }

    // Cabinet & Filter sub-headers
    if (! cabPanelBounds.isEmpty())
    {
        auto cTitle = cabPanelBounds.reduced (12, 8).removeFromTop (18);
        g.setColour (FollowerPalette::inkBlack.withAlpha (0.88f));
        g.setFont (FollowerLookAndFeel::getInterFont (10.2f, true));
        g.drawText ("CABINET IR", cTitle.removeFromLeft (static_cast<int> (cTitle.getWidth() * 0.46f)),
                    juce::Justification::centredLeft);
        g.drawText ("GATE & FILTERS", cTitle, juce::Justification::centredLeft);

        if (audioProcessor.isUsingCustomIr())
        {
            auto customBadge = cabPanelBounds.reduced (12, 7).removeFromBottom (15);
            g.setColour (FollowerPalette::ledBlue);
            g.setFont (FollowerLookAndFeel::getInterFont (9.5f, true));
            g.drawText ("CUSTOM IR: " + audioProcessor.getLoadedIrName(), customBadge, juce::Justification::centredLeft);
        }
    }

    // Bottom Pre-FX sub-titles & vertical section dividers
    if (! preFxPanelBounds.isEmpty() && preFxGlide > 0.25f)
    {
        auto pInner = preFxPanelBounds.reduced (10, 6);
        pInner.removeFromTop (16);
        const int thirdW = pInner.getWidth() / 3;
        const char* names[3] = { "TREMOLO", "ANALOG DELAY", "HALL REVERB" };
        g.setFont (FollowerLookAndFeel::getInterFont (9.5f, true));

        for (int i = 0; i < 3; ++i)
        {
            auto section = pInner.removeFromLeft (thirdW);

            if (i < 2)
            {
                const float dividerX = static_cast<float> (section.getRight());
                g.setColour (FollowerPalette::borderHairline.withAlpha (0.85f * preFxGlide));
                g.drawLine (dividerX,
                            static_cast<float> (section.getY() + 18),
                            dividerX,
                            static_cast<float> (section.getBottom() - 10),
                            1.0f);
            }

            auto sub = section.reduced (6, 0).removeFromTop (21);
            g.setColour (FollowerPalette::slateMuted.withAlpha (preFxGlide));
            g.drawText (names[i], sub, juce::Justification::centredLeft);
        }
    }

}

void FollowerAudioProcessorEditor::paintOverChildren (juce::Graphics& g)
{
    if (! showUtilityBtn.isVisible())
        return;

    const auto b      = showUtilityBtn.getBounds().toFloat();
    const float cx    = b.getCentreX() + 6.5f; // Clear separation to the right of the LED dot
    const float cy    = b.getCentreY();
    const bool active = showUtilityBtn.getToggleState();

    g.setColour (active ? FollowerPalette::ledBlue : FollowerPalette::inkBlack);

    juce::Path fork;
    const float prongHalfW = 2.6f;
    const float topY       = cy - 6.0f;
    const float yokeY      = cy + 0.8f;
    const float stemBotY   = cy + 6.2f;

    // Two straight parallel tines joined by a flat U-yoke
    fork.startNewSubPath (cx - prongHalfW, topY);
    fork.lineTo          (cx - prongHalfW, yokeY);
    fork.lineTo          (cx + prongHalfW, yokeY);
    fork.lineTo          (cx + prongHalfW, topY);

    // Straight vertical handle
    fork.startNewSubPath (cx, yokeY);
    fork.lineTo          (cx, stemBotY);

    g.strokePath (fork, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    g.fillEllipse (cx - 1.3f, stemBotY - 1.0f, 2.6f, 2.6f);
}