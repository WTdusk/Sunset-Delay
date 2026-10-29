#pragma once

#include "PluginProcessor.h"

#include <functional>
#include <memory>
#include <vector>

namespace guitar_delay
{

class LabeledKnob final : public juce::Component
{
public:
    LabeledKnob (const juce::String& name, const juce::String& tip);

    void resized() override;

    juce::Slider slider;
    juce::Label title;
    juce::Label value;
    std::function<juce::String (double)> formatter;
    double shownValue = 0.0;
    bool valueShown = false;
};

class ModeSwitch final : public juce::Component
{
public:
    std::function<void (int)> onChange;

    void setIndex (int next);
    int getIndex() const noexcept { return index; }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseMove (const juce::MouseEvent& event) override;
    void mouseExit (const juce::MouseEvent& event) override;

private:
    int index = 0;
    int hover = -1;
};

class StompSwitch final : public juce::Button
{
public:
    StompSwitch();

    juce::Colour accent { juce::Colour (0xffc56b38) };

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
};

class TapPad final : public juce::Button
{
public:
    TapPad();

    juce::Colour accent { juce::Colour (0xffc56b38) };
    bool lamp = false;

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
};

class PedalLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void setAccent (juce::Colour colour) { accent = colour; }
    juce::Colour getAccent() const noexcept { return accent; }

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;

    void applyChromeColours();

    void drawDocumentWindowTitleBar (juce::DocumentWindow&, juce::Graphics&, int w, int h,
                                     int titleSpaceX, int titleSpaceW,
                                     const juce::Image* icon, bool drawTitleTextOnLeft) override;
    juce::Button* createDocumentWindowButton (int buttonType) override;
    void positionDocumentWindowButtons (juce::DocumentWindow&, int titleBarX, int titleBarY,
                                        int titleBarW, int titleBarH,
                                        juce::Button* minimiseButton, juce::Button* maximiseButton,
                                        juce::Button* closeButton, bool positionTitleBarButtonsOnLeft) override;
    void drawResizableFrame (juce::Graphics&, int w, int h, const juce::BorderSize<int>&) override;
    void drawCornerResizer (juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;

    juce::Font getLabelFont (juce::Label&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawTickBox (juce::Graphics&, juce::Component&, float x, float y, float w, float h,
                      bool ticked, bool isEnabled, bool highlighted, bool down) override;
    void drawLevelMeter (juce::Graphics&, int width, int height, float level) override;
    void fillTextEditorBackground (juce::Graphics&, int width, int height, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int width, int height, juce::TextEditor&) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height,
                        bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                        bool isMouseOver, bool isMouseDown) override;
    void drawAlertBox (juce::Graphics&, juce::AlertWindow&, const juce::Rectangle<int>& textArea,
                       juce::TextLayout&) override;

private:
    juce::Colour accent { juce::Colour (0xffc56b38) };
};

class GuitarDelayEditor final : public juce::AudioProcessorEditor,
                                private juce::Timer
{
public:
    explicit GuitarDelayEditor (GuitarDelayProcessor&);
    ~GuitarDelayEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

private:
    void timerCallback() override;
    void ensureUiTimer();
    void applyMode (int mode);
    void updateReadouts();
    void layoutModeKnobs();
    void handleTap();
    void refreshPresetLabel();
    void showPresetMenu();
    void stepPreset (int delta);
    static juce::Colour accentFor (int mode);

    GuitarDelayProcessor& delayProcessor;
    PedalLookAndFeel lookAndFeel;

    LabeledKnob timeKnob { "TIME", "Delay time. Short slap at the left, about a second at the right." };
    LabeledKnob feedbackKnob { "FEEDBACK", "How many repeats. Full up is a long wash that still dies away." };
    LabeledKnob mixKnob { "MIX", "Wet level against the dry pick. Noon sits the echo behind the note." };
    LabeledKnob widthKnob { "WIDTH", "Stereo spread. Tape drifts apart, analog mismatches the clock, digital ping-pongs." };

    LabeledKnob wowKnob { "WOW", "Slow capstan drift. More is a seasick Echoplex, less is a steadier head." };
    LabeledKnob flutterKnob { "FLUTTER", "Fast scrape and motor flutter on the repeats." };
    LabeledKnob ageKnob { "AGE", "Oxide wear: darker repeats, more tape squash and grit." };
    LabeledKnob toneKnob { "TONE", "Opens the BBD filters. Short delays stay brighter than long ones." };
    LabeledKnob driveKnob { "DRIVE", "Pushes the bucket-brigade clipper. Feedback makes this grittier." };
    LabeledKnob hiCutKnob { "HI-CUT", "Lowpass inside the digital loop. Open it for bright rhythmic repeats." };
    LabeledKnob diffuseKnob { "DIFFUSE", "A little smear in the loop. Zero is a hard, clean repeat." };

    juce::TextButton syncButton { "SYNC" };
    TapPad tapPad;
    juce::TextButton presetPrevious { juce::String::fromUTF8 ("‹") };
    juce::TextButton presetNext { juce::String::fromUTF8 ("›") };
    juce::TextButton presetName { "Pedal" };
    juce::ComboBox divisionBox;
    juce::Label bpmLabel;
    ModeSwitch modeSwitch;
    StompSwitch stomp;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> timeAttachment;
    std::unique_ptr<SliderAttachment> feedbackAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> widthAttachment;
    std::unique_ptr<SliderAttachment> wowAttachment;
    std::unique_ptr<SliderAttachment> flutterAttachment;
    std::unique_ptr<SliderAttachment> ageAttachment;
    std::unique_ptr<SliderAttachment> toneAttachment;
    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<SliderAttachment> hiCutAttachment;
    std::unique_ptr<SliderAttachment> diffuseAttachment;
    std::unique_ptr<ButtonAttachment> syncAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<ComboAttachment> divisionAttachment;

    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    juce::Image landscape;
    juce::Image landscapeBlur;
    juce::Rectangle<int> titleArea;
    juce::Rectangle<int> echoPanel;
    juce::Rectangle<int> modePanel;
    int shownMode = -1;
    int shownProgram = -1;
    bool shownDirty = false;
    bool syncedTimeShown = false;
    int syncedTimeRounded = -1;
    int shownBpmTenths = -1;
    bool shownFreeTempo = false;
    bool tapArmed = false;
    double lastTapAtMs = 0.0;
    double tapFlashUntilMs = 0.0;
    std::vector<double> tapGaps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarDelayEditor)
};

} // namespace guitar_delay
