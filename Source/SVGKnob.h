#pragma once
#include <JuceHeader.h>


class SVGKnob : public juce::Slider {
public:
    SVGKnob(const juce::File& svgFile) {
        setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        if (svgFile.existsAsFile()) {
            auto xml = juce::parseXML(svgFile);
            if (xml) {
                knobSvg = juce::Drawable::createFromSVG(*xml);
            }
        }
        svgLookAndFeel.setDrawable(knobSvg.get());
        setLookAndFeel(&svgLookAndFeel);
    }
    ~SVGKnob() override {
        setLookAndFeel(nullptr);
    }
    class SVGLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        SVGLookAndFeel() : knobSvg(nullptr) {}
        void setDrawable(juce::Drawable* svg) { knobSvg = svg; }
        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                              float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override {
            if (knobSvg) {
                knobSvg->drawWithin(g, juce::Rectangle<float>(x, y, width, height), juce::RectanglePlacement::stretchToFit, 1.0f);
                // Draw knob indicator (simple line)
                float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
                float radius = width * 0.4f;
                float cx = x + width * 0.5f;
                float cy = y + height * 0.5f;
                float ex = cx + radius * std::cos(angle);
                float ey = cy + radius * std::sin(angle);
                g.setColour(juce::Colours::aqua);
                g.drawLine(cx, cy, ex, ey, 3.0f);
            } else {
                juce::LookAndFeel_V4::drawRotarySlider(g, x, y, width, height, sliderPosProportional, rotaryStartAngle, rotaryEndAngle, slider);
            }
        }
    private:
        juce::Drawable* knobSvg;
    };
    std::unique_ptr<juce::Drawable> knobSvg;
    SVGLookAndFeel svgLookAndFeel;
