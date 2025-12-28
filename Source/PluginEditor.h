

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Matrix Display component for animated visualization
class MatrixDisplay : public juce::Component, public juce::Timer
{
public:
    MatrixDisplay(CritAudioProcessor& processor)
        : audioProcessor(processor), gridSize(8), cols(100), rows(30), time(0.0f)
    {
        // Initialize grid cells
        cells.resize(rows, std::vector<bool>(cols, false));
        
        // Randomize initial state
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                cells[row][col] = juce::Random::getSystemRandom().nextBool();
            }
        }
        
        startTimerHz(60); // 60fps animation
    }
    
    ~MatrixDisplay() override
    {
        stopTimer();
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        int width = bounds.getWidth();
        int height = bounds.getHeight();
        
        // Draw inset shadow effect (matching Figma) - minimal border
        g.setColour(juce::Colour(0xcc000000));
        g.fillRect(bounds);
        
        // Inner black background - use full width with minimal border
        int borderSize = 2;
        juce::Rectangle<int> innerBounds = bounds.reduced(borderSize);
        g.setColour(juce::Colour(0xff000000));
        g.fillRect(innerBounds);
        
        // Calculate actual grid dimensions - use full inner width
        int innerWidth = innerBounds.getWidth();
        int innerHeight = innerBounds.getHeight();
        // Use full width - calculate how many columns we can fit
        int actualCols = (innerWidth + gridSize - 1) / gridSize; // Round up
        int actualRows = (innerHeight + gridSize - 1) / gridSize; // Round up
        
        // Get parameter values
        float pressure = audioProcessor.getParameters().getRawParameterValue("pressure")->load();
        float fault = audioProcessor.getParameters().getRawParameterValue("fault")->load();
        float split = audioProcessor.getParameters().getRawParameterValue("split")->load();
        float momentum = audioProcessor.getParameters().getRawParameterValue("momentum")->load();
        float resolve = audioProcessor.getParameters().getRawParameterValue("resolve")->load();

        // Increase matrix influence for knobs (except resolve) by 2.5x
        constexpr float matrixBoost = 2.5f;
        float pressureM = juce::jlimit(0.0f, 1.0f, pressure * matrixBoost);
        float faultM = juce::jlimit(0.0f, 1.0f, fault * matrixBoost);
        float splitM = juce::jlimit(0.0f, 1.0f, split * matrixBoost);
        float momentumM = juce::jlimit(0.0f, 1.0f, momentum * matrixBoost);

        // Calculate derived values using boosted influences (resolve left unchanged)
        float stress = pressureM * (1.0f + faultM * 0.5f);
        float instability = faultM * pressureM;
        float divergence = splitM;
        float speed = 0.5f + momentumM * 2.0f;
        float stability = resolve;
        
        // Draw grid cells - use full width, dynamically create cells if needed
        int offsetX = borderSize;
        int offsetY = borderSize;
        // Ensure we have enough columns
        if (actualCols > cols)
        {
            int oldCols = cols;
            cols = actualCols;
            for (auto& row : cells)
            {
                int prev = (int)row.size();
                row.resize(cols);
                for (int c = prev; c < cols; ++c)
                    row[c] = juce::Random::getSystemRandom().nextBool();
            }
        }
        if (actualRows > rows)
        {
            int prevRows = rows;
            rows = actualRows;
            cells.resize(rows);
            for (int r = prevRows; r < rows; ++r)
            {
                cells[r].resize(cols);
                for (int c = 0; c < cols; ++c)
                    cells[r][c] = juce::Random::getSystemRandom().nextBool();
            }
        }
        
        for (int row = 0; row < actualRows && row < rows; ++row)
        {
            for (int col = 0; col < actualCols && col < cols; ++col)
            {
                if (cells[row][col])
                {
                    // Calculate alpha with flickering based on instability
                    float alpha = 1.0f;
                    if (instability > 0.1f)
                    {
                        alpha *= 0.7f + (float)juce::Random::getSystemRandom().nextFloat() * 0.3f * instability;
                    }
                    
                    // Resolve stabilizes
                    alpha *= (1.0f - stability * 0.3f);
                    
                    // Split creates horizontal offset
                    float divergenceOffset = divergence * std::sin(time * speed + row * 0.5f) * 2.0f;
                    
                    int x = offsetX + col * gridSize + (int)divergenceOffset;
                    int y = offsetY + row * gridSize;
                    
                    // Set color with glow effect when pressure is high
                    juce::Colour cellColor = juce::Colour(0xff29a12b).withAlpha(alpha);
                    if (stress > 0.5f)
                    {
                        // Draw glow
                        g.setColour(cellColor.withAlpha(alpha * 0.3f));
                        g.fillRect(x - 1, y - 1, gridSize + 2, gridSize + 2);
                    }
                    
                    g.setColour(cellColor);
                    g.fillRect(x, y, gridSize - 1, gridSize - 1);
                }
            }
        }
        
        // Add scan lines
        g.setColour(juce::Colour(0x1a000000));
        for (int i = offsetY; i < innerHeight + offsetY; i += 2)
        {
            g.fillRect(offsetX, i, innerWidth, 1);
        }
        
        // Add noise/grain effect
        if (instability > 0.0f)
        {
            for (int i = 0; i < innerWidth * innerHeight / 50; ++i)
            {
                if (juce::Random::getSystemRandom().nextFloat() < 0.02f)
                {
                    int noiseX = offsetX + juce::Random::getSystemRandom().nextInt(innerWidth);
                    int noiseY = offsetY + juce::Random::getSystemRandom().nextInt(innerHeight);
                    g.setColour(juce::Colour(0xff29a12b).withAlpha(0.3f));
                    g.fillRect(noiseX, noiseY, 1, 1);
                }
            }
        }
    }
    
    void timerCallback() override
    {
        time += 0.016f; // ~60fps
        
        // Get parameter values for animation
        float pressure = audioProcessor.getParameters().getRawParameterValue("pressure")->load();
        float fault = audioProcessor.getParameters().getRawParameterValue("fault")->load();
        float momentum = audioProcessor.getParameters().getRawParameterValue("momentum")->load();
        float resolve = audioProcessor.getParameters().getRawParameterValue("resolve")->load();

        // Apply same matrix boost in timer-based updates
        constexpr float matrixBoost = 2.5f;
        float pressureM = juce::jlimit(0.0f, 1.0f, pressure * matrixBoost);
        float faultM = juce::jlimit(0.0f, 1.0f, fault * matrixBoost);
        float momentumM = juce::jlimit(0.0f, 1.0f, momentum * matrixBoost);

        float instability = faultM * pressureM;
        float speed = 0.5f + momentumM * 2.0f;
        float stability = resolve;
        
        // Update cells based on stress
        if (juce::Random::getSystemRandom().nextFloat() < 0.1f + instability * 0.3f)
        {
            int randomRow = juce::Random::getSystemRandom().nextInt(rows);
            int randomCol = juce::Random::getSystemRandom().nextInt(cols);
            cells[randomRow][randomCol] = !cells[randomRow][randomCol];
        }
        
        repaint();
    }
    
private:
    CritAudioProcessor& audioProcessor;
    std::vector<std::vector<bool>> cells;
    int gridSize;
    mutable int cols;  // Mutable so we can resize dynamically
    mutable int rows;  // Mutable so we can resize dynamically
    float time;
};

class CritAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Slider::Listener, public juce::ComboBox::Listener
{
public:
    CritAudioProcessorEditor (CritAudioProcessor&);
    ~CritAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;

private:
    CritAudioProcessor& audioProcessor;

    // (Removed Gain Match button) Use output gain slider instead.

    // Input gain UI
    juce::Slider inputGainSlider;
    juce::Label inputGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;

    // (StressMeter removed) bottom row now contains only input and output gain sliders

    juce::Slider pressureSlider, faultSlider, splitSlider, momentumSlider, resolveSlider, mixSlider;
    juce::Label pressureLabel, faultLabel, splitLabel, momentumLabel, resolveLabel, mixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pressureAttachment, faultAttachment, splitAttachment, momentumAttachment, resolveAttachment, mixAttachment;
    std::unique_ptr<juce::Drawable> backgroundDrawable;
    std::unique_ptr<juce::Drawable> titleDrawable;
    std::unique_ptr<juce::Drawable> headerDrawable;
    juce::Image headerImage;
    bool headerImageLoaded = false;
    std::unique_ptr<juce::Drawable> knobDrawable;
    // LookAndFeel instance for knobs
    class CritKnobLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                              float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override
        {
            (void) slider;
            float radius = juce::jmin((float)width, (float)height) / 2.0f;
            float centreX = x + width * 0.5f;
            float centreY = y + height * 0.5f;
            float knobRadius = radius - 8.0f;

            // Draw base circle
            g.setColour(juce::Colours::black);
            g.fillEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
            g.setColour(juce::Colours::white);
            g.drawEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 2.0f);

            // Draw dynamic arc based on slider position
            float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
            if (angle > rotaryStartAngle)
            {
                float arcRadius = knobRadius - 8.0f;
                float arcThickness = juce::jlimit(4.0f, 20.0f, arcRadius * 0.12f);
                float outerR = arcRadius;
                float innerR = juce::jmax(2.0f, arcRadius - arcThickness);

                float startA = rotaryStartAngle;
                float endA = angle;

                juce::Path arcPath;
                // Force move to arc start to avoid accidental connecting lines
                arcPath.addCentredArc(centreX, centreY, outerR, outerR, 0.0f, startA, endA, true);
                float strokeW = juce::jlimit(4.0f, 20.0f, outerR * 0.12f);
                g.setColour(juce::Colour(0xff29a12b));
                g.strokePath(arcPath, juce::PathStrokeType(strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
    } knobLNF;
    // LookAndFeel for zero-centered horizontal gain bars
    class GainBarLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                      float sliderPos, float minSliderPos, float maxSliderPos, const juce::Slider& slider)
        {
            const float min = (float)slider.getMinimum();
            const float max = (float)slider.getMaximum();
            const float val = (float)slider.getValue();

            // track rect
            juce::Rectangle<float> track((float)x, (float)y + height * 0.4f, (float)width, height * 0.2f);
            g.setColour(juce::Colour(0xff2b2b2b));
            g.fillRect(track);

            // compute center position (value==0) within track
            float zeroPos = 0.0f;
            if (max > min)
                zeroPos = (0.0f - min) / (max - min);
            else
                zeroPos = 0.5f;
            float centerX = track.getX() + track.getWidth() * zeroPos;

            // compute current position
            float curPos = 0.0f;
            if (max > min)
                curPos = track.getX() + track.getWidth() * ((val - min) / (max - min));
            else
                curPos = centerX;

            // draw filled portion from center to current position
            g.setColour(juce::Colour(0xff29a12b));
            if (curPos >= centerX)
                g.fillRect(juce::Rectangle<float>(centerX, track.getY(), curPos - centerX, track.getHeight()));
            else
                g.fillRect(juce::Rectangle<float>(curPos, track.getY(), centerX - curPos, track.getHeight()));

            // draw thumb as small vertical marker
            float thumbX = juce::jlimit(track.getX(), track.getRight(), curPos);
            float thumbW = 6.0f;
            juce::Rectangle<float> thumb(thumbX - thumbW * 0.5f, track.getY() - 4.0f, thumbW, track.getHeight() + 8.0f);
            g.setColour(juce::Colour(0xff000000));
            g.fillRoundedRectangle(thumb, 2.0f);
        }
    } gainLNF;
    juce::Typeface::Ptr titleTypeface;
    juce::Font titleFont;
    bool titleTypefaceLoaded = false;
    int knobDiameter = 80;
    
    // New components
    MatrixDisplay matrixDisplay;
    juce::ComboBox presetComboBox;
    juce::TextButton saveButton {"Save"};
        juce::TextButton loadButton { "Load..." };
    // user preset files scanned from disk (keeps order matching combo ids)
    juce::Array<juce::File> userPresetFiles;
    void populatePresetsCombo();
    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    
    // Sublabels for knobs
    juce::StringArray knobSublabels {"stress it out", "break it", "pull it apart", "shape it", "rescue it", "blend"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CritAudioProcessorEditor)
};
