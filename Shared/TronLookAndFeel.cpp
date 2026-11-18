// TronLookAndFeel.cpp — Tron-style UI for the Silktone ambient suite.
#include "TronLookAndFeel.h"
#include <cmath>

namespace tron
{

LookAndFeel::LookAndFeel()
{
    setColour(juce::Slider::rotarySliderFillColourId, kCyan);
    setColour(juce::Slider::rotarySliderOutlineColourId, kLine);
    setColour(juce::Slider::textBoxTextColourId, kText);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::thumbColourId, kCyan);
    setColour(juce::Slider::trackColourId, kCyan);
    setColour(juce::Slider::backgroundColourId, kPanelEdge);
    setColour(juce::Label::textColourId, kText);
    setColour(juce::ComboBox::backgroundColourId, kPanel);
    setColour(juce::ComboBox::textColourId, kText);
    setColour(juce::ComboBox::outlineColourId, kLine);
    setColour(juce::ComboBox::arrowColourId, kCyan);
    setColour(juce::TextButton::buttonColourId, kPanel);
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff0d2a38));
    setColour(juce::TextButton::textColourOffId, kText);
    setColour(juce::TextButton::textColourOnId, kCyan);
    setColour(juce::PopupMenu::backgroundColourId, kPanel);
    setColour(juce::PopupMenu::textColourId, kText);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff12384a));
    setColour(juce::PopupMenu::highlightedTextColourId, kCyan);
}

void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                   float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                   juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(7.0f);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);

    // Track (full sweep, hairline).
    juce::Path track;
    track.addArc(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f,
                 rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(kLine);
    g.strokePath(track, juce::PathStrokeType(1.5f));

    // Active arc with neon glow: wide faint pass, then a crisp pass.
    if (sliderPos > 0.001f)
    {
        juce::Path arc;
        arc.addArc(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f,
                   rotaryStartAngle, angle, true);
        g.setColour(accent.withAlpha(0.12f));
        g.strokePath(arc, juce::PathStrokeType(6.0f));
        g.setColour(accent.withAlpha(0.35f));
        g.strokePath(arc, juce::PathStrokeType(3.0f));
        g.setColour(accent);
        g.strokePath(arc, juce::PathStrokeType(1.8f));
        // Glowing dot at the pointer position.
        const float px = centre.x + std::sin(angle) * radius;
        const float py = centre.y - std::cos(angle) * radius;
        g.setColour(accent.withAlpha(0.30f));
        g.fillEllipse(px - 5.0f, py - 5.0f, 10.0f, 10.0f);
        g.setColour(accent);
        g.fillEllipse(px - 2.5f, py - 2.5f, 5.0f, 5.0f);
    }

    // Knob body: dark disc with a thin rim.
    g.setColour(kPanel);
    g.fillEllipse(centre.x - radius * 0.62f, centre.y - radius * 0.62f,
                  radius * 1.24f, radius * 1.24f);
    g.setColour(kLine);
    g.drawEllipse(centre.x - radius * 0.62f, centre.y - radius * 0.62f,
                  radius * 1.24f, radius * 1.24f, 1.0f);
    // Pointer line.
    const float px = centre.x + std::sin(angle) * radius * 0.55f;
    const float py = centre.y - std::cos(angle) * radius * 0.55f;
    g.setColour(accent);
    g.drawLine(centre.x, centre.y, px, py, 2.0f);
}

void LookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                   float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                   const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    const bool horizontal = (style == juce::Slider::LinearHorizontal);
    const auto track = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
    const auto accent = slider.findColour(juce::Slider::trackColourId);
    g.setColour(kPanelEdge);
    if (horizontal)
        g.fillRect(track.getX(), track.getCentreY() - 1.0f, track.getWidth(), 2.0f);
    else
        g.fillRect(track.getCentreX() - 1.0f, track.getY(), 2.0f, track.getHeight());
    g.setColour(accent.withAlpha(0.25f));
    if (horizontal)
        g.fillRect(track.getX(), track.getCentreY() - 2.0f, sliderPos - track.getX(), 4.0f);
    else
        g.fillRect(track.getCentreX() - 2.0f, sliderPos, 4.0f, track.getBottom() - sliderPos);
    g.setColour(accent);
    if (horizontal)
        g.fillRect(track.getX(), track.getCentreY() - 1.0f, sliderPos - track.getX(), 2.0f);
    else
        g.fillRect(track.getCentreX() - 1.0f, sliderPos, 2.0f, track.getBottom() - sliderPos);
    // Thumb: small glowing square.
    g.setColour(accent.withAlpha(0.25f));
    if (horizontal)
        g.fillRect(sliderPos - 7.0f, track.getCentreY() - 7.0f, 14.0f, 14.0f);
    else
        g.fillRect(track.getCentreX() - 7.0f, sliderPos - 7.0f, 14.0f, 14.0f);
    g.setColour(kText);
    if (horizontal)
        g.fillRect(sliderPos - 3.0f, track.getCentreY() - 6.0f, 6.0f, 12.0f);
    else
        g.fillRect(track.getCentreX() - 6.0f, sliderPos - 3.0f, 12.0f, 6.0f);
}

void LookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                   bool /*highlighted*/, bool /*down*/)
{
    const auto area = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = button.getToggleState();
    const auto accent = button.findColour(juce::ToggleButton::tickColourId);
    g.setColour(on ? juce::Colour(0xff0d2a38) : kPanel);
    g.fillRoundedRectangle(area, 3.0f);
    g.setColour(on ? accent : kLine);
    g.drawRoundedRectangle(area, 3.0f, on ? 1.5f : 1.0f);
    if (on)
    {
        g.setColour(accent.withAlpha(0.25f));
        g.drawRoundedRectangle(area.expanded(1.0f), 4.0f, 2.0f);
    }
    g.setColour(on ? kCyan : kDim);
    g.setFont(juce::Font(juce::FontOptions(11.0f, on ? juce::Font::bold : juce::Font::plain)));
    g.drawText(button.getButtonText(), area, juce::Justification::centred);
}

void LookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                               int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                               juce::ComboBox& box)
{
    const auto area = juce::Rectangle<float>(0, 0, (float)width, (float)height);
    g.setColour(kPanel);
    g.fillRect(area);
    g.setColour(kLine);
    g.drawRect(area, 1.0f);
    g.setColour(kCyan);
    juce::Path arrow;
    arrow.addTriangle((float)width - 16.0f, (float)height * 0.42f,
                      (float)width - 8.0f, (float)height * 0.42f,
                      (float)width - 12.0f, (float)height * 0.62f);
    g.fillPath(arrow);
    g.setColour(box.findColour(juce::ComboBox::textColourId));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText(box.getText(), 8, 0, width - 24, height, juce::Justification::centredLeft);
}

void LookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                       const juce::Colour& /*backgroundColour*/,
                                       bool /*highlighted*/, bool /*down*/)
{
    const auto area = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = button.getToggleState();
    const auto accent = button.findColour(juce::TextButton::textColourOnId);
    g.setColour(on ? juce::Colour(0xff0d2a38) : kPanel);
    g.fillRoundedRectangle(area, 3.0f);
    g.setColour(on ? accent : kLine);
    g.drawRoundedRectangle(area, 3.0f, 1.0f);
}

void LookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                 bool /*highlighted*/, bool /*down*/)
{
    const auto area = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = button.getToggleState();
    const auto accent = button.findColour(juce::TextButton::textColourOnId);
    g.setColour(on ? accent : kText);
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.drawText(button.getButtonText(), area, juce::Justification::centred);
}

void LookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.fillAll(kPanel);
    g.setColour(kLine);
    g.drawRect(0, 0, width, height, 1);
}

void LookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                    bool isSeparator, bool /*isActive*/, bool isHighlighted,
                                    bool /*isTicked*/, bool /*hasSubMenu*/,
                                    const juce::String& text, const juce::String& /*shortcut*/,
                                    const juce::Drawable* /*icon*/, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        g.setColour(kLine);
        g.fillRect(area.getX() + 8, area.getCentreY(), area.getWidth() - 16, 1);
        return;
    }
    if (isHighlighted)
    {
        g.setColour(juce::Colour(0xff12384a));
        g.fillRect(area);
    }
    g.setColour(textColour != nullptr ? *textColour : (isHighlighted ? kCyan : kText));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText(text, area.reduced(10, 0), juce::Justification::centredLeft);
}

// ---------------------------------------------------------------------------
void paintSection(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title,
                  juce::Colour accent)
{
    g.setColour(kPanel);
    g.fillRect(area);
    g.setColour(kLine);
    g.drawRect(area, 1);
    // Title tab: small caps on the top edge, with a short accent rule.
    g.setColour(kBg);
    g.fillRect(area.getX() + 10, area.getY() - 1, 10 + (int)(title.length() * 7.5f) + 10, 2);
    g.setColour(accent);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText(title, area.getX() + 14, area.getY() - 8, area.getWidth() - 28, 16,
               juce::Justification::centredLeft);
    g.setColour(accent.withAlpha(0.6f));
    g.fillRect(area.getX() + 14, area.getY() + 8, 24, 1);
}

void styleKnob(juce::Slider& s, juce::Colour accent)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 16);
    s.setColour(juce::Slider::rotarySliderFillColourId, accent);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, kLine);
    s.setColour(juce::Slider::textBoxTextColourId, kText);
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.setColour(juce::Slider::thumbColourId, accent);
}

void styleLabel(juce::Label& l, const juce::String& text, int fontSize, juce::Colour colour)
{
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(juce::FontOptions((float)fontSize, juce::Font::plain)));
    l.setColour(juce::Label::textColourId, colour);
    l.setJustificationType(juce::Justification::centred);
}

void styleSectionLabel(juce::Label& l, const juce::String& text)
{
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    l.setColour(juce::Label::textColourId, kCyan);
    l.setJustificationType(juce::Justification::centred);
}

void Knob::init(juce::AudioProcessorValueTreeState& apvts, const char* paramId,
                const juce::String& labelText, juce::Component& parent, juce::Colour accent)
{
    styleKnob(slider, accent);
    styleLabel(label, labelText, 11, kDim);
    parent.addAndMakeVisible(slider);
    parent.addAndMakeVisible(label);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, paramId, slider);
}

void Knob::setBounds(int x, int y, int w, int h)
{
    label.setBounds(x, y, w, 14);
    slider.setBounds(x, y + 14, w, h - 14);
}

void Toggle::init(juce::AudioProcessorValueTreeState& apvts, const char* paramId,
                  const juce::String& text, juce::Component& parent, juce::Colour accent)
{
    button.setButtonText(text);
    button.setColour(juce::ToggleButton::tickColourId, accent);
    button.setColour(juce::ToggleButton::textColourId, kText);
    parent.addAndMakeVisible(button);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, paramId, button);
}

} // namespace tron
