#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class TBLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TBLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1a2138));
        setColour (juce::ComboBox::textColourId, juce::Colours::white);
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0x44ffffff));
        setColour (juce::ComboBox::arrowColourId, juce::Colour (0xff52e3c2));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff141a2e));
        setColour (juce::PopupMenu::textColourId, juce::Colours::white);
        setColour (juce::PopupMenu::headerTextColourId, juce::Colour (0xff52e3c2));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff52e3c2));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff0d1020));
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, juce::Colour (0xffd5dcf2));
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1e2640));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xffd5dcf2));
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xff0d1020));
        setColour (juce::ToggleButton::textColourId, juce::Colour (0xffd5dcf2));
        setColour (juce::ToggleButton::tickColourId, juce::Colour (0xff52e3c2));
        setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff555e7a));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float arcR = radius - 2.f;
        const auto fill = s.findColour (juce::Slider::rotarySliderFillColourId);

        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f, startAngle, endAngle, true);
        g.setColour (juce::Colour (0x30ffffff));
        g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path val;
        val.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f, startAngle, angle, true);
        g.setColour (fill);
        g.strokePath (val, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float kr = arcR - 7.f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a4468), centre.x, centre.y - kr,
                                                 juce::Colour (0xff171d33), centre.x, centre.y + kr, false));
        g.fillEllipse (centre.x - kr, centre.y - kr, kr * 2.f, kr * 2.f);

        juce::Path ptr;
        ptr.addRoundedRectangle (-1.5f, -kr + 2.f, 3.f, kr * 0.45f, 1.5f);
        g.setColour (juce::Colours::white);
        g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    }
};
