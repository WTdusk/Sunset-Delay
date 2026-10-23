#include "PluginEditor.h"
#include "BinaryData.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace guitar_delay
{
namespace
{

const juce::Colour kClay { 0xffe7e2db };
const juce::Colour kClayDeep { 0xffddd6cd };
const juce::Colour kInk { 0xff514c46 };
const juce::Colour kMuted { 0xff8b847b };
const juce::Colour kShadow { 0xffb3aa9f };
const juce::Colour kOrange { 0xffc56b38 };
const juce::Colour kAmber { 0xffb8883e };

juce::String percentText (double value)
{
    return juce::String (juce::roundToInt (value * 100.0)) + " %";
}

juce::Font softFont (float size, bool hero = false)
{
    return juce::Font (juce::FontOptions ("Inter", hero ? "SemiBold" : "Medium", size));
}

void dualShadow (juce::Graphics& g, juce::Rectangle<float> area, float radius, bool ellipse)
{
    const auto blob = [&] (juce::Colour colour, float ox, float oy)
    {
        for (int i = 5; i >= 1; --i)
        {
            const float t = static_cast<float> (i) / 5.0f;
            auto shape = area.translated (ox * t, oy * t).expanded (static_cast<float> (i) * 0.85f);
            g.setColour (colour.withAlpha (0.10f * t));
            if (ellipse)
                g.fillEllipse (shape);
            else
                g.fillRoundedRectangle (shape, radius + static_cast<float> (i) * 0.6f);
        }
    };

    blob (kShadow, 5.0f, 6.0f);
    blob (juce::Colours::white, -4.0f, -5.0f);
}

void fillClayBody (juce::Graphics& g, juce::Rectangle<float> area, float radius, bool ellipse)
{
    juce::ColourGradient body (juce::Colour (0xfff1ece5), area.getX(), area.getY(),
                               juce::Colour (0xffe1dad1), area.getRight(), area.getBottom(), false);
    g.setGradientFill (body);
    if (ellipse)
        g.fillEllipse (area);
    else
        g.fillRoundedRectangle (area, radius);
}

void fillRaised (juce::Graphics& g, juce::Rectangle<float> area, float radius)
{
    dualShadow (g, area, radius, false);
    fillClayBody (g, area, radius, false);
}

void fillPressed (juce::Graphics& g, juce::Rectangle<float> area, float radius)
{
    g.setColour (kClayDeep);
    g.fillRoundedRectangle (area, radius);
    juce::ColourGradient shade (kShadow.withAlpha (0.42f), area.getX(), area.getY(),
                                juce::Colours::transparentBlack, area.getCentreX(), area.getCentreY(), true);
    g.setGradientFill (shade);
    g.fillRoundedRectangle (area, radius);
    juce::ColourGradient light (juce::Colours::white.withAlpha (0.38f), area.getRight(), area.getBottom(),
                                juce::Colours::transparentWhite, area.getCentreX(), area.getCentreY(), true);
    g.setGradientFill (light);
    g.fillRoundedRectangle (area, radius);
}

void fillRaisedCircle (juce::Graphics& g, juce::Rectangle<float> area)
{
    dualShadow (g, area, area.getWidth() * 0.5f, true);
    fillClayBody (g, area, area.getWidth() * 0.5f, true);
}

void fillPressedCircle (juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour (kClayDeep);
    g.fillEllipse (area);
    juce::ColourGradient shade (kShadow.withAlpha (0.40f), area.getX(), area.getY(),
                                juce::Colours::transparentBlack, area.getCentreX(), area.getCentreY(), true);
    g.setGradientFill (shade);
    g.fillEllipse (area);
    juce::ColourGradient light (juce::Colours::white.withAlpha (0.32f), area.getRight(), area.getBottom(),
                                juce::Colours::transparentWhite, area.getCentreX(), area.getCentreY(), true);
    g.setGradientFill (light);
    g.fillEllipse (area);
}

juce::Image loadLandscape()
{
    return juce::ImageFileFormat::loadFrom (BinaryData::landscape_jpg, BinaryData::landscape_jpgSize);
}

juce::Image softenLandscape (const juce::Image& source)
{
    if (! source.isValid())
        return {};

    const int width = source.getWidth();
    const int height = source.getHeight();
    juce::Image image (juce::Image::ARGB, width, height, false);

    {
        juce::Image::BitmapData src (source, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData dst (image, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                dst.setPixelColour (x, y, src.getPixelColour (x, y));
    }

    const auto blurPass = [] (juce::Image& target, bool horizontal, int radius)
    {
        const int w = target.getWidth();
        const int h = target.getHeight();
        juce::Image out (juce::Image::ARGB, w, h, false);
        const int length = horizontal ? w : h;
        const int lines = horizontal ? h : w;
        const int window = radius * 2 + 1;

        {
            juce::Image::BitmapData in (target, juce::Image::BitmapData::readOnly);
            juce::Image::BitmapData dest (out, juce::Image::BitmapData::writeOnly);

            for (int line = 0; line < lines; ++line)
            {
                const auto at = [&] (int index)
                {
                    const int clamped = juce::jlimit (0, length - 1, index);
                    const int x = horizontal ? clamped : line;
                    const int y = horizontal ? line : clamped;
                    return in.getPixelColour (x, y);
                };

                int sumR = 0;
                int sumG = 0;
                int sumB = 0;
                for (int i = -radius; i <= radius; ++i)
                {
                    const auto colour = at (i);
                    sumR += colour.getRed();
                    sumG += colour.getGreen();
                    sumB += colour.getBlue();
                }

                for (int i = 0; i < length; ++i)
                {
                    const int x = horizontal ? i : line;
                    const int y = horizontal ? line : i;
                    dest.setPixelColour (x, y, juce::Colour::fromRGB (static_cast<juce::uint8> (sumR / window),
                                                                      static_cast<juce::uint8> (sumG / window),
                                                                      static_cast<juce::uint8> (sumB / window)));
                    const auto remove = at (i - radius);
                    const auto add = at (i + radius + 1);
                    sumR += add.getRed() - remove.getRed();
                    sumG += add.getGreen() - remove.getGreen();
                    sumB += add.getBlue() - remove.getBlue();
                }
            }
        }

        target = out;
    };

    blurPass (image, true, 16);
    blurPass (image, false, 16);
    blurPass (image, true, 10);
    blurPass (image, false, 10);
    return image;
}

class ClayCaptionButton final : public juce::Button
{
public:
    ClayCaptionButton (const juce::String& name, juce::Path shapeIn)
        : juce::Button (name), shape (std::move (shapeIn))
    {
    }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        auto area = getLocalBounds().toFloat();
        const float side = juce::jmin (area.getWidth(), area.getHeight()) - 10.0f;
        auto disc = juce::Rectangle<float> (side, side).withCentre (area.getCentre());

        if (down)
            fillPressedCircle (g, disc);
        else
            fillRaisedCircle (g, disc);

        const bool close = getName() == "close";
        g.setColour (close && (highlighted || down) ? kOrange : kInk);
        g.fillPath (shape, shape.getTransformToScaleToFit (disc.reduced (side * 0.30f), true));
    }

private:
    juce::Path shape;
};

void drawLandscape (juce::Graphics& g, const juce::Image& image, juce::Rectangle<float> window)
{
    const float scale = juce::jmax (window.getWidth() / static_cast<float> (image.getWidth()),
                                    window.getHeight() / static_cast<float> (image.getHeight()));
    const float dw = static_cast<float> (image.getWidth()) * scale;
    const float dh = static_cast<float> (image.getHeight()) * scale;
    g.setColour (juce::Colours::white);
    g.drawImage (image,
                 (window.getWidth() - dw) * 0.5f,
                 (window.getHeight() - dh) * 0.5f,
                 dw,
                 dh,
                 0,
                 0,
                 image.getWidth(),
                 image.getHeight());
}

void paintFrostedCard (juce::Graphics& g, juce::Rectangle<float> area, float radius,
                       const juce::Image& blurred, juce::Rectangle<float> window)
{
    dualShadow (g, area, radius, false);

    juce::Graphics::ScopedSaveState state (g);
    juce::Path clip;
    clip.addRoundedRectangle (area, radius);
    g.reduceClipRegion (clip);

    if (blurred.isValid())
        drawLandscape (g, blurred, window);
    else
    {
        g.setColour (kClay);
        g.fillRoundedRectangle (area, radius);
    }

    g.setColour (juce::Colour (0xff2a261f).withAlpha (0.08f));
    g.fillRoundedRectangle (area, radius);

    juce::ColourGradient body (juce::Colour (0xfff6f2ec).withAlpha (0.58f), area.getX(), area.getY(),
                               juce::Colour (0xffe6dfd6).withAlpha (0.66f), area.getRight(), area.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (area, radius);
}

} // namespace

LabeledKnob::LabeledKnob (const juce::String& name, const juce::String& tip)
{
    addAndMakeVisible (title);
    addAndMakeVisible (value);
    addAndMakeVisible (slider);

    title.setText (name, juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centred);
    title.setFont (softFont (11.0f));
    title.setColour (juce::Label::textColourId, kMuted);
    title.setInterceptsMouseClicks (false, false);

    value.setJustificationType (juce::Justification::centred);
    value.setFont (softFont (16.5f, true));
    value.setColour (juce::Label::textColourId, kInk);
    value.setInterceptsMouseClicks (false, false);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f,
                                true);
    slider.setMouseDragSensitivity (150);
    slider.setTooltip (tip);
    slider.setName (name);
}

void LabeledKnob::resized()
{
    auto area = getLocalBounds();
    title.setBounds (area.removeFromTop (15));
    value.setBounds (area.removeFromBottom (22));
    slider.setBounds (area.reduced (1, 0));
}

void ModeSwitch::setIndex (int next)
{
    const int clamped = juce::jlimit (0, 2, next);
    if (clamped == index)
        return;
    index = clamped;
    repaint();
}

void ModeSwitch::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (5.0f);
    const float radius = bounds.getHeight() * 0.5f;
    fillPressed (g, bounds, radius);

    const char* names[3] = { "Tape", "Analog", "Digital" };
    const float gap = 5.0f;
    const float segmentW = (bounds.getWidth() - gap * 4.0f) / 3.0f;

    for (int i = 0; i < 3; ++i)
    {
        auto segment = juce::Rectangle<float> (bounds.getX() + gap + static_cast<float> (i) * (segmentW + gap),
                                               bounds.getY() + gap,
                                               segmentW,
                                               bounds.getHeight() - gap * 2.0f);
        const bool on = i == index;
        const bool hot = i == hover && ! on;
        const float pill = segment.getHeight() * 0.5f;

        if (on)
            fillRaised (g, segment, pill);

        g.setColour (on ? kOrange : (hot ? kInk : kMuted));
        g.setFont (softFont (15.0f, on));
        g.drawFittedText (names[i], segment.toNearestInt(), juce::Justification::centred, 1);
    }
}

void ModeSwitch::mouseDown (const juce::MouseEvent& event)
{
    const int which = juce::jlimit (0, 2, event.x * 3 / juce::jmax (1, getWidth()));
    setIndex (which);
    if (onChange != nullptr)
        onChange (which);
}

void ModeSwitch::mouseMove (const juce::MouseEvent& event)
{
    const int which = juce::jlimit (0, 2, event.x * 3 / juce::jmax (1, getWidth()));
    if (which != hover)
    {
        hover = which;
        repaint();
    }
}

void ModeSwitch::mouseExit (const juce::MouseEvent&)
{
    hover = -1;
    repaint();
}

StompSwitch::StompSwitch()
    : juce::Button ("bypass")
{
    setClickingTogglesState (true);
    setTooltip ("Bypass with trails. Dry guitar comes back and the repeats ring out.");
}

void StompSwitch::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto area = getLocalBounds().toFloat();
    const bool engaged = ! getToggleState();

    const float ledSize = juce::jmin (10.0f, area.getWidth() * 0.14f);
    auto led = juce::Rectangle<float> (ledSize, ledSize).withCentre ({ area.getCentreX(), area.getY() + ledSize * 0.55f + 2.0f });

    if (engaged)
    {
        g.setColour (accent.withAlpha (0.22f));
        g.fillEllipse (led.expanded (5.0f));
        g.setColour (accent.withAlpha (0.40f));
        g.fillEllipse (led.expanded (2.4f));
        g.setColour (accent);
        g.fillEllipse (led);
        g.setColour (juce::Colour (0xfffff6ee).withAlpha (0.70f));
        g.fillEllipse (led.reduced (ledSize * 0.34f));
    }
    else
    {
        fillPressedCircle (g, led.expanded (1.2f));
    }

    auto caption = juce::Rectangle<float> (area.getX(), led.getBottom() + 2.0f, area.getWidth(), 14.0f);
    g.setColour (engaged ? accent : kMuted);
    g.setFont (softFont (10.5f, engaged));
    g.drawFittedText (engaged ? "ACTIVE" : "BYPASS", caption.toNearestInt(), juce::Justification::centred, 1);

    auto pedal = area.withTrimmedTop (caption.getBottom() - area.getY() + 2.0f).withTrimmedBottom (14.0f).reduced (10.0f, 2.0f);
    const float side = juce::jmin (pedal.getWidth(), pedal.getHeight());
    pedal = juce::Rectangle<float> (side, side).withCentre (pedal.getCentre());

    if (down)
        fillPressedCircle (g, pedal.reduced (1.0f));
    else
        fillRaisedCircle (g, highlighted ? pedal.reduced (0.5f) : pedal);

    g.setColour (kMuted);
    g.setFont (softFont (10.0f));
    g.drawFittedText ("TRAILS", juce::Rectangle<int> (0, getHeight() - 14, getWidth(), 12), juce::Justification::centred, 1);
}

TapPad::TapPad()
    : juce::Button ("tap")
{
    setClickingTogglesState (false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip ("Tap a steady pulse. Two or more taps set the time and turn sync off. Range is 50 ms to 1 second.");
}

void TapPad::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto pad = getLocalBounds().toFloat().reduced (4.0f);
    const float radius = pad.getHeight() * 0.5f;
    if (down)
        fillPressed (g, pad, radius);
    else
        fillRaised (g, highlighted ? pad.reduced (0.4f) : pad, radius);

    const float led = juce::jmin (8.0f, pad.getHeight() * 0.24f);
    auto lampBounds = juce::Rectangle<float> (led, led).withCentre ({ pad.getRight() - led * 0.5f - 10.0f, pad.getCentreY() });
    if (lamp)
    {
        g.setColour (accent.withAlpha (0.28f));
        g.fillEllipse (lampBounds.expanded (3.2f));
        g.setColour (accent);
        g.fillEllipse (lampBounds);
    }
    else
    {
        fillPressedCircle (g, lampBounds.expanded (0.6f));
    }

    g.setColour (lamp ? accent : kInk);
    g.setFont (softFont (juce::jmin (13.5f, pad.getHeight() * 0.40f), lamp));
    g.drawFittedText ("TAP", pad.withTrimmedRight (led + 14.0f).toNearestInt(), juce::Justification::centred, 1);
}

void PedalLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                          static_cast<float> (width), static_cast<float> (height))
                      .reduced (7.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const bool enabled = slider.isEnabled();
    const float arcR = radius - 1.5f;
    const auto arcColour = enabled ? accent : kMuted.withAlpha (0.45f);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (kShadow.withAlpha (enabled ? 0.55f : 0.28f));
    g.strokePath (track, juce::PathStrokeType (3.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (sliderPos > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, rotaryStartAngle, angle, true);
        g.setColour (arcColour);
        g.strokePath (value, juce::PathStrokeType (3.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float knobR = radius * 0.68f;
    fillRaisedCircle (g, juce::Rectangle<float> (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f));

    const float dx = std::cos (angle - juce::MathConstants<float>::halfPi);
    const float dy = std::sin (angle - juce::MathConstants<float>::halfPi);
    g.setColour (enabled ? kInk : kMuted.withAlpha (0.55f));
    g.drawLine (centre.x + dx * knobR * 0.18f, centre.y + dy * knobR * 0.18f,
                centre.x + dx * knobR * 0.72f, centre.y + dy * knobR * 0.72f, 2.3f);
}

juce::Font PedalLookAndFeel::getTextButtonFont (juce::TextButton& button, int buttonHeight)
{
    const float size = button.getComponentID() == "preset-name" ? 15.5f
                                                                : juce::jmin (14.0f, static_cast<float> (buttonHeight) * 0.36f);
    return softFont (size, button.getComponentID() == "preset-name");
}

void PedalLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                             bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (4.0f);
    if (bounds.getWidth() < 8.0f || bounds.getHeight() < 8.0f)
        return;

    const float radius = bounds.getHeight() * 0.5f;
    const bool sunk = down || button.getToggleState();
    if (sunk)
        fillPressed (g, bounds, radius);
    else
        fillRaised (g, highlighted ? bounds.reduced (0.4f) : bounds, radius);
}

void PedalLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int, int, int, int, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (4.0f);
    const float radius = bounds.getHeight() * 0.5f;
    if (isButtonDown)
        fillPressed (g, bounds, radius);
    else
        fillRaised (g, bounds, radius);

    const float arrow = juce::jmin (5.5f, bounds.getHeight() * 0.16f);
    juce::Path chevron;
    const float cx = bounds.getRight() - 14.0f;
    const float cy = bounds.getCentreY() + 0.5f;
    chevron.startNewSubPath (cx - arrow, cy - arrow * 0.35f);
    chevron.lineTo (cx, cy + arrow * 0.45f);
    chevron.lineTo (cx + arrow, cy - arrow * 0.35f);
    g.setColour (box.isEnabled() ? kMuted : kMuted.withAlpha (0.35f));
    g.strokePath (chevron, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Font PedalLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return softFont (juce::jmin (14.0f, static_cast<float> (box.getHeight()) * 0.34f));
}

void PedalLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (14, 1, juce::jmax (8, box.getWidth() - 32), box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void PedalLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (kClay);
    auto area = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (6.0f);
    fillRaised (g, area, 16.0f);
}

juce::Font PedalLookAndFeel::getPopupMenuFont()
{
    return softFont (15.0f);
}

void PedalLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                          const bool isSeparator, const bool isActive,
                                          const bool isHighlighted, const bool isTicked, bool,
                                          const juce::String& text, const juce::String&,
                                          const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
        return;

    auto row = area.reduced (10, 3);
    if (row.isEmpty())
        return;

    if (isHighlighted && isActive)
    {
        g.setColour (kOrange);
        g.fillRoundedRectangle (row.toFloat(), row.getHeight() * 0.45f);
        g.setColour (juce::Colour (0xfff6f1ea));
    }
    else
    {
        g.setColour (kInk.withMultipliedAlpha (isActive ? 1.0f : 0.4f));
    }

    g.setFont (softFont (14.5f));
    auto textArea = row;
    if (isTicked)
    {
        auto mark = textArea.removeFromLeft (16).toFloat();
        g.fillEllipse (mark.withSizeKeepingCentre (6.5f, 6.5f));
    }
    else
    {
        textArea.removeFromLeft (16);
    }

    g.drawFittedText (text, textArea, juce::Justification::centredLeft, 1);
}

void PedalLookAndFeel::applyChromeColours()
{
    setColourScheme (juce::LookAndFeel_V4::getLightColourScheme());
    setColour (juce::ResizableWindow::backgroundColourId, kClay);
    setColour (juce::DocumentWindow::textColourId, kInk);
    setColour (juce::Label::textColourId, kInk);
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::PopupMenu::backgroundColourId, kClay);
    setColour (juce::PopupMenu::textColourId, kInk);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kOrange);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xfff6f1ea));
    setColour (juce::TooltipWindow::backgroundColourId, kClay);
    setColour (juce::TooltipWindow::textColourId, kInk);
    setColour (juce::TooltipWindow::outlineColourId, kClayDeep);
    setColour (juce::ComboBox::backgroundColourId, kClay);
    setColour (juce::ComboBox::textColourId, kInk);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::arrowColourId, kMuted);
    setColour (juce::TextButton::buttonColourId, kClay);
    setColour (juce::TextButton::textColourOffId, kInk);
    setColour (juce::TextButton::textColourOnId, kOrange);
    setColour (juce::ListBox::backgroundColourId, kClay);
    setColour (juce::ListBox::textColourId, kInk);
    setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::backgroundColourId, kClayDeep);
    setColour (juce::TextEditor::textColourId, kInk);
    setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::focusedOutlineColourId, kOrange);
    setColour (juce::ToggleButton::textColourId, kInk);
    setColour (juce::ToggleButton::tickColourId, kOrange);
    setColour (juce::ToggleButton::tickDisabledColourId, kMuted);
    setColour (juce::AlertWindow::backgroundColourId, kClay);
    setColour (juce::AlertWindow::textColourId, kInk);
    setColour (juce::AlertWindow::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ScrollBar::thumbColourId, kShadow);
    setColour (juce::ScrollBar::backgroundColourId, kClayDeep);
}

void PedalLookAndFeel::drawDocumentWindowTitleBar (juce::DocumentWindow& window, juce::Graphics& g,
                                                   int width, int height, int, int, const juce::Image*, bool)
{
    g.setColour (kClay);
    g.fillRect (0, 0, width, height);

    juce::ColourGradient lip (juce::Colours::transparentBlack, 0.0f, static_cast<float> (height) - 10.0f,
                              kShadow.withAlpha (0.28f), 0.0f, static_cast<float> (height), false);
    g.setGradientFill (lip);
    g.fillRect (0, height - 10, width, 10);

    g.setColour (window.isActiveWindow() ? kInk : kMuted);
    g.setFont (softFont (15.5f, true));
    g.drawFittedText (window.getName(), 48, 0, width - 96, height, juce::Justification::centred, 1);
}

juce::Button* PedalLookAndFeel::createDocumentWindowButton (int buttonType)
{
    juce::Path shape;

    if (buttonType == juce::DocumentWindow::closeButton)
    {
        shape.addLineSegment ({ 0.0f, 0.0f, 1.0f, 1.0f }, 0.16f);
        shape.addLineSegment ({ 1.0f, 0.0f, 0.0f, 1.0f }, 0.16f);
        return new ClayCaptionButton ("close", shape);
    }

    if (buttonType == juce::DocumentWindow::minimiseButton)
    {
        shape.addLineSegment ({ 0.05f, 0.55f, 0.95f, 0.55f }, 0.16f);
        return new ClayCaptionButton ("minimise", shape);
    }

    if (buttonType == juce::DocumentWindow::maximiseButton)
    {
        shape.addRoundedRectangle (0.12f, 0.12f, 0.76f, 0.76f, 0.12f);
        return new ClayCaptionButton ("maximise", shape);
    }

    return nullptr;
}

void PedalLookAndFeel::positionDocumentWindowButtons (juce::DocumentWindow&, int titleBarX, int titleBarY,
                                                      int titleBarW, int titleBarH,
                                                      juce::Button* minimiseButton, juce::Button* maximiseButton,
                                                      juce::Button* closeButton, bool positionTitleBarButtonsOnLeft)
{
    const int button = juce::jlimit (28, 36, titleBarH - 8);
    const int gap = 6;
    int x = positionTitleBarButtonsOnLeft ? titleBarX + 8
                                          : titleBarX + titleBarW - button - 8;
    const int y = titleBarY + (titleBarH - button) / 2;
    const int step = positionTitleBarButtonsOnLeft ? button + gap : -(button + gap);

    if (closeButton != nullptr)
    {
        closeButton->setBounds (x, y, button, button);
        x += step;
    }

    if (positionTitleBarButtonsOnLeft)
        std::swap (minimiseButton, maximiseButton);

    if (maximiseButton != nullptr)
    {
        maximiseButton->setBounds (x, y, button, button);
        x += step;
    }

    if (minimiseButton != nullptr)
        minimiseButton->setBounds (x, y, button, button);
}

void PedalLookAndFeel::drawResizableFrame (juce::Graphics& g, int width, int height, const juce::BorderSize<int>& border)
{
    if (border.isEmpty())
        return;

    juce::Graphics::ScopedSaveState state (g);
    g.excludeClipRegion (border.subtractedFrom (juce::Rectangle<int> (0, 0, width, height)));
    g.setColour (kClay);
    g.fillAll();
}

void PedalLookAndFeel::drawCornerResizer (juce::Graphics& g, int width, int height, bool, bool)
{
    const float inset = 5.0f;
    g.setColour (kMuted.withAlpha (0.85f));
    for (int i = 0; i < 3; ++i)
    {
        const float offset = inset + static_cast<float> (i) * 4.0f;
        g.drawLine (static_cast<float> (width) - offset, static_cast<float> (height) - inset,
                    static_cast<float> (width) - inset, static_cast<float> (height) - offset, 1.4f);
    }
}

juce::Font PedalLookAndFeel::getLabelFont (juce::Label& label)
{
    return softFont (juce::jmin (15.0f, static_cast<float> (label.getHeight()) * 0.72f));
}

void PedalLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool down)
{
    const float box = juce::jmin (18.0f, static_cast<float> (button.getHeight()) - 4.0f);
    drawTickBox (g, button, 2.0f, (static_cast<float> (button.getHeight()) - box) * 0.5f, box, box,
                 button.getToggleState(), button.isEnabled(), highlighted, down);

    g.setColour (button.findColour (juce::ToggleButton::textColourId).withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.45f));
    g.setFont (softFont (juce::jmin (15.0f, static_cast<float> (button.getHeight()) * 0.62f)));
    g.drawFittedText (button.getButtonText(),
                      button.getLocalBounds().withTrimmedLeft (juce::roundToInt (box) + 10),
                      juce::Justification::centredLeft, 1);
}

void PedalLookAndFeel::drawTickBox (juce::Graphics& g, juce::Component&, float x, float y, float w, float h,
                                    bool ticked, bool isEnabled, bool, bool)
{
    auto box = juce::Rectangle<float> (x, y, w, h);
    const float radius = juce::jmin (6.0f, h * 0.35f);

    if (ticked)
    {
        g.setColour (isEnabled ? kOrange : kMuted);
        g.fillRoundedRectangle (box, radius);
        juce::Path tick;
        tick.startNewSubPath (x + w * 0.22f, y + h * 0.52f);
        tick.lineTo (x + w * 0.42f, y + h * 0.74f);
        tick.lineTo (x + w * 0.78f, y + h * 0.30f);
        g.setColour (juce::Colour (0xfff6f1ea));
        g.strokePath (tick, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else
    {
        fillPressed (g, box, radius);
    }
}

void PedalLookAndFeel::drawLevelMeter (juce::Graphics& g, int width, int height, float level)
{
    auto area = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (1.0f);
    const float radius = juce::jmin (6.0f, area.getHeight() * 0.5f);
    g.setColour (kClayDeep);
    g.fillRoundedRectangle (area, radius);
    const float clamped = juce::jlimit (0.0f, 1.0f, level);
    if (clamped > 0.001f)
    {
        g.setColour (kOrange.withAlpha (0.90f));
        g.fillRoundedRectangle (area.withWidth (juce::jmax (radius * 2.0f, area.getWidth() * clamped)), radius);
    }
}

void PedalLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor&)
{
    g.setColour (kClayDeep);
    g.fillRoundedRectangle (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height), 8.0f);
}

void PedalLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& editor)
{
    if (! editor.isEnabled())
        return;

    const bool focused = editor.hasKeyboardFocus (true) && ! editor.isReadOnly();
    g.setColour (focused ? kOrange.withAlpha (0.85f) : kShadow.withAlpha (0.45f));
    g.drawRoundedRectangle (0.5f, 0.5f, static_cast<float> (width) - 1.0f, static_cast<float> (height) - 1.0f, 8.0f, focused ? 1.5f : 1.0f);
}

void PedalLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int width, int height,
                                      bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool, bool)
{
    auto track = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                         static_cast<float> (width), static_cast<float> (height))
                     .reduced (isScrollbarVertical ? 3.0f : 2.0f, isScrollbarVertical ? 2.0f : 3.0f);
    const float radius = juce::jmin (track.getWidth(), track.getHeight()) * 0.5f;
    g.setColour (kClayDeep);
    g.fillRoundedRectangle (track, radius);

    juce::Rectangle<float> thumb = isScrollbarVertical
                                      ? juce::Rectangle<float> (track.getX(), static_cast<float> (thumbStartPosition), track.getWidth(), static_cast<float> (thumbSize))
                                      : juce::Rectangle<float> (static_cast<float> (thumbStartPosition), track.getY(), static_cast<float> (thumbSize), track.getHeight());
    g.setColour (kShadow);
    g.fillRoundedRectangle (thumb.reduced (1.0f), radius);
}

void PedalLookAndFeel::drawAlertBox (juce::Graphics& g, juce::AlertWindow&, const juce::Rectangle<int>& textArea, juce::TextLayout& textLayout)
{
    g.setColour (kClay);
    g.fillAll();
    g.setColour (kInk);
    textLayout.draw (g, textArea.toFloat());
}

GuitarDelayEditor::GuitarDelayEditor (GuitarDelayProcessor& processorIn)
    : juce::AudioProcessorEditor (processorIn),
      delayProcessor (processorIn)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);
    landscape = loadLandscape();
    landscapeBlur = softenLandscape (landscape);

    lookAndFeel.applyChromeColours();

    auto wire = [this] (LabeledKnob& knob, const char* paramId, std::unique_ptr<SliderAttachment>& attachment, std::function<juce::String (double)> formatter)
    {
        knob.formatter = std::move (formatter);
        addAndMakeVisible (knob);
        attachment = std::make_unique<SliderAttachment> (delayProcessor.apvts, paramId, knob.slider);
        if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (delayProcessor.apvts.getParameter (paramId)))
            knob.slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
    };

    const auto percent = [] (double value) { return percentText (value); };
    const auto millis = [] (double value) { return juce::String (juce::roundToInt (value)) + " ms"; };
    const auto kilo = [] (double value) { return juce::String (value / 1000.0, 2) + " kHz"; };

    wire (timeKnob, id::time, timeAttachment, millis);
    wire (feedbackKnob, id::feedback, feedbackAttachment, percent);
    wire (mixKnob, id::mix, mixAttachment, percent);
    wire (widthKnob, id::width, widthAttachment, percent);
    wire (wowKnob, id::tapeWow, wowAttachment, percent);
    wire (flutterKnob, id::tapeFlutter, flutterAttachment, percent);
    wire (ageKnob, id::tapeAge, ageAttachment, percent);
    wire (toneKnob, id::analogTone, toneAttachment, percent);
    wire (driveKnob, id::analogDrive, driveAttachment, percent);
    wire (hiCutKnob, id::digitalHiCut, hiCutAttachment, kilo);
    wire (diffuseKnob, id::digitalDiffuse, diffuseAttachment, percent);

    syncButton.setClickingTogglesState (true);
    syncButton.setTooltip ("Lock the delay to the host tempo.");
    syncButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible (syncButton);
    syncAttachment = std::make_unique<ButtonAttachment> (delayProcessor.apvts, id::sync, syncButton);

    tapPad.onClick = [this] { handleTap(); };
    addAndMakeVisible (tapPad);

    const auto styleNav = [] (juce::TextButton& button, const juce::String& tip)
    {
        button.setTooltip (tip);
        button.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        button.setColour (juce::TextButton::buttonColourId, kClay);
        button.setColour (juce::TextButton::textColourOffId, kInk);
        button.setColour (juce::TextButton::textColourOnId, kOrange);
    };
    styleNav (presetPrevious, "Previous factory preset.");
    styleNav (presetNext, "Next factory preset.");
    presetPrevious.onClick = [this] { stepPreset (-1); };
    presetNext.onClick = [this] { stepPreset (1); };
    addAndMakeVisible (presetPrevious);
    addAndMakeVisible (presetNext);

    presetName.setComponentID ("preset-name");
    presetName.setTooltip ("Factory presets. A star means a knob or a tap has edited this patch. Choose it again to recall the saved settings.");
    presetName.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    presetName.setColour (juce::TextButton::buttonColourId, kClay);
    presetName.setColour (juce::TextButton::textColourOffId, kInk);
    presetName.setColour (juce::TextButton::textColourOnId, kInk);
    presetName.onClick = [this] { showPresetMenu(); };
    addAndMakeVisible (presetName);
    refreshPresetLabel();

    divisionBox.addItemList (divisionNames(), 1);
    divisionBox.setTooltip ("Note division used while sync is on.");
    addAndMakeVisible (divisionBox);
    divisionAttachment = std::make_unique<ComboAttachment> (delayProcessor.apvts, id::division, divisionBox);

    bpmLabel.setJustificationType (juce::Justification::centred);
    bpmLabel.setFont (softFont (15.0f, true));
    bpmLabel.setColour (juce::Label::textColourId, kInk);
    bpmLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (bpmLabel);

    modeSwitch.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    modeSwitch.onChange = [this] (int mode)
    {
        if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (delayProcessor.apvts.getParameter (id::mode)))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (mode)));
        applyMode (mode);
        resized();
    };
    addAndMakeVisible (modeSwitch);

    stomp.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible (stomp);
    bypassAttachment = std::make_unique<ButtonAttachment> (delayProcessor.apvts, id::bypass, stomp);

    syncButton.setColour (juce::TextButton::buttonColourId, kClay);
    syncButton.setColour (juce::TextButton::textColourOffId, kInk);
    syncButton.setColour (juce::TextButton::textColourOnId, kOrange);
    divisionBox.setColour (juce::ComboBox::backgroundColourId, kClay);
    divisionBox.setColour (juce::ComboBox::textColourId, kInk);
    divisionBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    divisionBox.setColour (juce::ComboBox::arrowColourId, kMuted);

    applyMode (delayProcessor.getModeIndex());
    updateReadouts();

    setResizable (true, true);
    setResizeLimits (800, 552, 1280, 884);
    if (auto* boundsConstrainer = getConstrainer())
        boundsConstrainer->setFixedAspectRatio (840.0 / 580.0);
    setSize (840, 580);
    // Tooltips and the repaint timer touch the message thread. A scanner that
    // builds the editor off that thread must not start them here.
    ensureUiTimer();
}

void GuitarDelayEditor::ensureUiTimer()
{
    if (auto* manager = juce::MessageManager::getInstanceWithoutCreating())
    {
        if (! manager->isThisTheMessageThread())
            return;
    }
    else
    {
        return;
    }

    if (tooltipWindow == nullptr)
        tooltipWindow = std::make_unique<juce::TooltipWindow> (this, 700);

    if (! isTimerRunning())
        startTimerHz (30);
}

void GuitarDelayEditor::visibilityChanged()
{
    if (isVisible())
        ensureUiTimer();
}

void GuitarDelayEditor::parentHierarchyChanged()
{
    if (isShowing())
        ensureUiTimer();
}

GuitarDelayEditor::~GuitarDelayEditor()
{
    setLookAndFeel (nullptr);
}

juce::Colour GuitarDelayEditor::accentFor (int)
{
    return kOrange;
}

void GuitarDelayEditor::applyMode (int mode)
{
    shownMode = juce::jlimit (0, 2, mode);
    const auto accent = accentFor (shownMode);
    lookAndFeel.setAccent (accent);
    stomp.accent = accent;
    syncButton.setColour (juce::TextButton::buttonOnColourId, accent);
    tapPad.accent = accent;
    presetName.setColour (juce::TextButton::buttonOnColourId, accent);
    presetPrevious.setColour (juce::TextButton::buttonOnColourId, accent);
    presetNext.setColour (juce::TextButton::buttonOnColourId, accent);
    lookAndFeel.setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
    lookAndFeel.setColour (juce::TooltipWindow::outlineColourId, accent);

    wowKnob.setVisible (shownMode == 0);
    flutterKnob.setVisible (shownMode == 0);
    ageKnob.setVisible (shownMode == 0);
    toneKnob.setVisible (shownMode == 1);
    driveKnob.setVisible (shownMode == 1);
    hiCutKnob.setVisible (shownMode == 2);
    diffuseKnob.setVisible (shownMode == 2);

    LabeledKnob* knobs[] = { &timeKnob, &feedbackKnob, &mixKnob, &widthKnob, &wowKnob, &flutterKnob,
                              &ageKnob, &toneKnob, &driveKnob, &hiCutKnob, &diffuseKnob };
    for (auto* knob : knobs)
        knob->value.setColour (juce::Label::textColourId, kInk);

    modeSwitch.setIndex (shownMode);
    repaint();
}

void GuitarDelayEditor::updateReadouts()
{
    const bool synced = delayProcessor.isSyncEnabled();
    timeKnob.slider.setEnabled (! synced);
    divisionBox.setEnabled (synced);

    const auto paintKnob = [] (LabeledKnob& knob)
    {
        if (knob.formatter == nullptr)
            return;
        knob.value.setText (knob.formatter (knob.slider.getValue()), juce::dontSendNotification);
    };

    paintKnob (feedbackKnob);
    paintKnob (mixKnob);
    paintKnob (widthKnob);
    paintKnob (wowKnob);
    paintKnob (flutterKnob);
    paintKnob (ageKnob);
    paintKnob (toneKnob);
    paintKnob (driveKnob);
    paintKnob (hiCutKnob);
    paintKnob (diffuseKnob);

    if (synced)
    {
        const int division = divisionBox.getSelectedItemIndex();
        const float ms = clampDelayMs (delayMsForDivision (delayProcessor.getDisplayedBpm(), division));
        timeKnob.value.setText (juce::String (juce::roundToInt (ms)) + " ms", juce::dontSendNotification);
        bpmLabel.setText (juce::String (delayProcessor.getDisplayedBpm(), 1) + " BPM", juce::dontSendNotification);
    }
    else
    {
        paintKnob (timeKnob);
        bpmLabel.setText ("FREE", juce::dontSendNotification);
    }

    divisionBox.setAlpha (synced ? 1.0f : 0.45f);
}

void GuitarDelayEditor::layoutModeKnobs()
{
    std::array<LabeledKnob*, 3> slots { nullptr, nullptr, nullptr };
    int count = 0;
    const auto consider = [&] (LabeledKnob& knob)
    {
        if (knob.isVisible() && count < 3)
            slots[static_cast<size_t> (count++)] = &knob;
    };

    consider (wowKnob);
    consider (flutterKnob);
    consider (ageKnob);
    consider (toneKnob);
    consider (driveKnob);
    consider (hiCutKnob);
    consider (diffuseKnob);

    auto inner = modePanel.reduced (14, 8);
    inner.removeFromTop (36);
    if (count == 0 || inner.getWidth() <= 0)
        return;

    const int slotW = inner.getWidth() / count;
    for (int i = 0; i < count; ++i)
        if (slots[static_cast<size_t> (i)] != nullptr)
            slots[static_cast<size_t> (i)]->setBounds (inner.removeFromLeft (slotW).reduced (6, 0));
}

void GuitarDelayEditor::handleTap()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double gap = now - lastTapAtMs;
    tapFlashUntilMs = now + 180.0;
    tapPad.lamp = true;
    tapPad.repaint();

    if (lastTapAtMs <= 0.0 || gap > 2000.0)
    {
        lastTapAtMs = now;
        tapGaps.clear();
        tapArmed = true;
        return;
    }

    if (gap < 40.0)
        return;

    tapGaps.push_back (gap);
    if (tapGaps.size() > 4)
        tapGaps.erase (tapGaps.begin());

    double sum = 0.0;
    for (double value : tapGaps)
        sum += value;

    const float ms = std::clamp (static_cast<float> (sum / static_cast<double> (tapGaps.size())), 50.0f, 1000.0f);
    lastTapAtMs = now;

    if (auto* sync = delayProcessor.apvts.getParameter (id::sync))
    {
        sync->beginChangeGesture();
        sync->setValueNotifyingHost (0.0f);
        sync->endChangeGesture();
    }

    if (auto* time = dynamic_cast<juce::RangedAudioParameter*> (delayProcessor.apvts.getParameter (id::time)))
    {
        time->beginChangeGesture();
        time->setValueNotifyingHost (time->convertTo0to1 (ms));
        time->endChangeGesture();
    }

    tapArmed = false;
}

void GuitarDelayEditor::timerCallback()
{
    const int mode = delayProcessor.getModeIndex();
    if (mode != shownMode)
    {
        applyMode (mode);
        resized();
    }

    const bool dirty = ! delayProcessor.programMatchesCurrent();
    const int program = delayProcessor.getCurrentProgram();
    if (program != shownProgram || dirty != shownDirty)
        refreshPresetLabel();

    const double now = juce::Time::getMillisecondCounterHiRes();
    if (tapArmed && lastTapAtMs > 0.0 && now - lastTapAtMs > 2000.0)
        tapArmed = false;

    const bool lamp = tapArmed || now < tapFlashUntilMs;
    if (lamp != tapPad.lamp)
    {
        tapPad.lamp = lamp;
        tapPad.repaint();
    }

    updateReadouts();
    stomp.repaint();
}

void GuitarDelayEditor::refreshPresetLabel()
{
    shownProgram = delayProcessor.getCurrentProgram();
    shownDirty = ! delayProcessor.programMatchesCurrent();
    juce::String label = delayProcessor.getProgramName (shownProgram);
    if (shownDirty)
        label << " *";
    presetName.setColour (juce::TextButton::textColourOffId, shownDirty ? kAmber : kInk);
    presetName.setButtonText (label);
}

void GuitarDelayEditor::stepPreset (int delta)
{
    const int count = delayProcessor.getNumPrograms();
    if (count <= 0)
        return;
    int next = delayProcessor.getCurrentProgram() + delta;
    if (next < 0)
        next = count - 1;
    if (next >= count)
        next = 0;
    delayProcessor.setCurrentProgram (next);
    refreshPresetLabel();
}

void GuitarDelayEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    const int current = delayProcessor.getCurrentProgram();
    for (int i = 0; i < delayProcessor.getNumPrograms(); ++i)
        menu.addItem (i + 1, delayProcessor.getProgramName (i), true, i == current && ! shownDirty);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetName),
                        [this] (int result)
                        {
                            if (result > 0)
                            {
                                delayProcessor.setCurrentProgram (result - 1);
                                refreshPresetLabel();
                            }
                        });
}

void GuitarDelayEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    if (landscape.isValid())
        drawLandscape (g, landscape, bounds);
    else
    {
        g.setColour (kClay);
        g.fillAll();
    }

    g.setColour (juce::Colour (0xff2a261f).withAlpha (0.08f));
    g.fillAll();

    juce::ColourGradient vignette (juce::Colours::transparentBlack, bounds.getCentre(),
                                   juce::Colour (0xff241f1a).withAlpha (0.20f),
                                   bounds.getTopLeft(),
                                   true);
    g.setGradientFill (vignette);
    g.fillAll();

    if (! titleArea.isEmpty())
    {
        auto titleWash = titleArea.toFloat().expanded (6.0f, 4.0f);
        juce::ColourGradient sky (kClay.withAlpha (0.55f), titleWash.getX(), titleWash.getY(),
                                  juce::Colours::transparentWhite, titleWash.getX(), titleWash.getBottom(), false);
        g.setGradientFill (sky);
        g.fillRect (titleWash);
    }

    if (! echoPanel.isEmpty())
        paintFrostedCard (g, echoPanel.toFloat(), 22.0f, landscapeBlur, bounds);
    if (! modePanel.isEmpty())
        paintFrostedCard (g, modePanel.toFloat(), 22.0f, landscapeBlur, bounds);

    g.setColour (kMuted);
    g.setFont (softFont (12.0f));
    g.drawText ("Repeats", echoPanel.getX() + 18, echoPanel.getY() + 8, 140, 16, juce::Justification::centredLeft, false);

    auto blurbArea = modePanel.reduced (18, 8).removeFromTop (36);
    juce::String modeName = "Tape";
    juce::String blurb = "Wow, flutter, and age. The head drifts and the repeats darken.";
    if (shownMode == 1)
    {
        modeName = "Analog";
        blurb = "Tone and drive. Longer delays clock slower and lose highs.";
    }
    else if (shownMode == 2)
    {
        modeName = "Digital";
        blurb = "Hi-cut and diffuse. The repeat stays put until the time moves.";
    }

    g.setColour (kOrange);
    g.setFont (softFont (16.0f, true));
    g.drawText (modeName, blurbArea.getX(), blurbArea.getY(), blurbArea.getWidth(), 18, juce::Justification::centredLeft, false);
    blurbArea.removeFromTop (18);
    g.setColour (kMuted);
    g.setFont (softFont (12.5f));
    g.drawFittedText (blurb, blurbArea, juce::Justification::centredLeft, 1);

    g.setColour (kMuted);
    g.setFont (softFont (11.0f));
    g.drawText ("WHITE TEA", titleArea.reduced (4, 0).removeFromTop (14), juce::Justification::bottomLeft, false);

    g.setColour (kInk);
    g.setFont (softFont (22.0f, true));
    auto nameArea = titleArea.reduced (4, 0);
    nameArea.removeFromTop (14);
    g.drawFittedText ("Sunset Delay", nameArea, juce::Justification::centredLeft, 1);

    const float pillY = static_cast<float> (titleArea.getBottom()) - 5.0f;
    g.setColour (kOrange.withAlpha (0.90f));
    g.fillRoundedRectangle (static_cast<float> (titleArea.getX()) + 4.0f, pillY, 36.0f, 4.0f, 2.0f);
}

void GuitarDelayEditor::resized()
{
    const int margin = juce::jmax (18, getWidth() / 40);
    auto area = getLocalBounds().reduced (margin);
    const int height = area.getHeight();
    const int gap = juce::jmax (8, height / 68);

    auto footer = area.removeFromBottom (juce::jlimit (92, 120, juce::roundToInt (static_cast<float> (height) * 0.17f)));
    area.removeFromBottom (gap);

    titleArea = area.removeFromTop (juce::jlimit (40, 54, juce::roundToInt (static_cast<float> (height) * 0.078f)));

    auto presetRow = area.removeFromTop (juce::jlimit (40, 52, juce::roundToInt (static_cast<float> (height) * 0.076f)));
    const int nav = juce::jlimit (44, 56, presetRow.getHeight() + 6);
    presetPrevious.setBounds (presetRow.removeFromLeft (nav));
    presetNext.setBounds (presetRow.removeFromRight (nav));
    presetRow.removeFromLeft (4);
    presetRow.removeFromRight (4);
    presetName.setBounds (presetRow);

    area.removeFromTop (gap);
    auto transport = area.removeFromTop (juce::jlimit (44, 56, juce::roundToInt (static_cast<float> (height) * 0.084f)));
    tapPad.setBounds (transport.removeFromLeft (juce::jlimit (108, 140, transport.getWidth() / 5)));
    transport.removeFromLeft (6);
    divisionBox.setBounds (transport.removeFromRight (juce::jlimit (128, 168, transport.getWidth() / 4)));
    transport.removeFromRight (4);
    syncButton.setBounds (transport.removeFromRight (juce::jlimit (84, 108, transport.getWidth() / 5)));
    transport.removeFromRight (4);
    bpmLabel.setBounds (transport.reduced (2, 6));

    area.removeFromTop (gap);
    const int panels = area.getHeight() - gap;
    echoPanel = area.removeFromTop (juce::jmax (132, panels * 50 / 100));
    auto mainRow = echoPanel.reduced (12, 8);
    mainRow.removeFromTop (22);
    const int mainSlot = mainRow.getWidth() / 4;
    timeKnob.setBounds (mainRow.removeFromLeft (mainSlot).reduced (2, 0));
    feedbackKnob.setBounds (mainRow.removeFromLeft (mainSlot).reduced (2, 0));
    mixKnob.setBounds (mainRow.removeFromLeft (mainSlot).reduced (2, 0));
    widthKnob.setBounds (mainRow.reduced (2, 0));

    area.removeFromTop (gap);
    modePanel = area;
    layoutModeKnobs();

    auto stompArea = footer.removeFromRight (juce::jlimit (108, 136, juce::jmax (108, footer.getWidth() / 6)));
    stomp.setBounds (stompArea);
    const int switchH = juce::jlimit (52, 64, footer.getHeight());
    modeSwitch.setBounds (footer.withSizeKeepingCentre (juce::jmax (300, footer.getWidth() - 8), switchH));
}

} // namespace guitar_delay
