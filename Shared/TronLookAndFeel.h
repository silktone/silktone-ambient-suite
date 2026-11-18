// TronLookAndFeel.h — the shared Silktone ambient suite UI language.
// Classy, futuristic, Tron-style: near-black blue panel, thin hairline
// cyan lines, glowing cyan + orange neon accents, clean readable labels.
// Thin lines, organised sections, ergonomic spacing, easy on the eyes.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace tron
{

inline const juce::Colour kBg(0xff04070d);       // near-black blue
inline const juce::Colour kPanel(0xff0a1220);    // panel blue-black
inline const juce::Colour kPanelEdge(0xff1c2f42);
inline const juce::Colour kCyan(0xff00e5ff);     // neon cyan (primary)
inline const juce::Colour kOrange(0xffff7a1a);   // neon orange (secondary)
inline const juce::Colour kText(0xffd8f4ff);     // ice-blue text
inline const juce::Colour kDim(0xff6b8fa3);      // dim blue-grey labels
inline const juce::Colour kLine(0xff22455c);     // hairline

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator,
                           bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;
};

// Draw a section box: hairline border + small-caps title on the top edge.
void paintSection(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title,
                  juce::Colour accent = kCyan);

// Style helpers for knobs / labels / toggles (shared by both editors).
void styleKnob(juce::Slider& s, juce::Colour accent = kCyan);
void styleLabel(juce::Label& l, const juce::String& text, int fontSize = 11,
                juce::Colour colour = kDim);
void styleSectionLabel(juce::Label& l, const juce::String& text);

// A knob + label + APVTS attachment in one small unit, to keep the
// editors readable (the Ambient editor has ~30 controls).
struct Knob
{
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void init(juce::AudioProcessorValueTreeState& apvts, const char* paramId,
              const juce::String& labelText, juce::Component& parent,
              juce::Colour accent = kCyan);
    void setBounds(int x, int y, int w, int h); // label on top, knob below
};

struct Toggle
{
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    void init(juce::AudioProcessorValueTreeState& apvts, const char* paramId,
              const juce::String& text, juce::Component& parent,
              juce::Colour accent = kCyan);
};

} // namespace tron
