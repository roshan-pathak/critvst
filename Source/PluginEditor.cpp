#include <JuceHeader.h>
#include <cmath>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include "GUIAssets/EmbeddedAssets.h"

class CritKnobLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override
    {
        static constexpr bool kDebugKnobOverlay = true; // set true to draw debug markers

        (void) slider;
        float radius = juce::jmin((float)width, (float)height) / 2.0f;
        float centreX = x + width * 0.5f;
        float centreY = y + height * 0.5f;
        float knobRadius = radius - 8.0f;

        // Draw knob artwork from embedded SVG (background)
        static std::unique_ptr<juce::Drawable> s_knobDrawable;
        if (s_knobDrawable)
        {
            juce::Rectangle<float> r (x + (width - knobRadius) * 0.5f, y + (height - knobRadius) * 0.5f, knobRadius, knobRadius);
            g.saveState();
            g.reduceClipRegion(juce::Rectangle<int>((int)std::floor(r.getX()), (int)std::floor(r.getY()), (int)std::ceil(r.getWidth()), (int)std::ceil(r.getHeight())));
            s_knobDrawable->drawWithin(g, r, juce::RectanglePlacement::centred, 1.0f);
            g.restoreState();
        }

        // Draw black circle with white outline (no inner circle)
        g.setColour(juce::Colours::black);
        g.fillEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
        g.setColour(juce::Colours::white);
        g.drawEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 2.0f);

        // Draw rotating indicator based on slider position
        float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Draw an annular sector (ring-shaped arc) instead of a filled pie
        if (angle > rotaryStartAngle)
        {
            float arcRadius = knobRadius - 8.0f;
            float arcThickness = juce::jlimit(4.0f, 20.0f, arcRadius * 0.12f);
            float outerR = arcRadius;
            float innerR = juce::jmax(2.0f, arcRadius - arcThickness);

            float startA = rotaryStartAngle;
            float endA = angle;

            // Draw a stroked arc with rounded end caps to avoid spikes at the endpoints.
            juce::Path arcPath;
            // Ensure the arc is started with a moveTo to avoid connecting lines
            arcPath.addCentredArc(centreX, centreY, outerR, outerR, 0.0f, startA, endA, true);
            float strokeW = juce::jlimit(4.0f, 20.0f, outerR * 0.12f);
            g.setColour(juce::Colour(0xff29a12b));
            g.strokePath(arcPath, juce::PathStrokeType(strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            if (kDebugKnobOverlay)
            {
                // Draw bounding box for knob area
                g.setColour(juce::Colours::red.withAlpha(0.6f));
                g.drawRect(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 1.0f);

                // Compute arc start/end points using angles on outer radius
                juce::Point<float> startPt (centreX + outerR * std::cos(startA), centreY + outerR * std::sin(startA));
                juce::Point<float> endPt   (centreX + outerR * std::cos(endA),   centreY + outerR * std::sin(endA));

                // Draw small markers at arc endpoints and centre
                g.setColour(juce::Colours::yellow);
                g.fillEllipse(startPt.x - 2.5f, startPt.y - 2.5f, 5.0f, 5.0f);
                g.fillEllipse(endPt.x - 2.5f,   endPt.y - 2.5f,   5.0f, 5.0f);

                g.setColour(juce::Colours::white);
                g.fillEllipse(centreX - 2.5f, centreY - 2.5f, 5.0f, 5.0f);

                // Draw a line from centre to the outer arc end
                g.setColour(juce::Colours::orange);
                g.drawLine(centreX, centreY, endPt.x, endPt.y, 1.0f);

                // Annotate with the angles for quick inspection
                g.setColour(juce::Colours::black);
                g.setFont(9.0f);
                juce::String s = "start:" + juce::String(startA, 2) + " end:" + juce::String(endA, 2);
                g.drawText(s, (int)(centreX - knobRadius), (int)(centreY - knobRadius) - 14, (int)knobRadius * 2, 12, juce::Justification::centred);
            }
        }
    }
};
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GUIAssets/EmbeddedAssets.h"

CritAudioProcessorEditor::CritAudioProcessorEditor(CritAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), matrixDisplay(p)
{
    // start with a reasonable default window size (scaled from the Figma layout)
    // Increased height to fit controls and avoid overlap
    setSize (980, 760);
    
    // Add matrix display
    addAndMakeVisible(&matrixDisplay);
    
    // Set up preset combo box (matching Figma: gray background, white text)
    presetComboBox.addListener(this);
    presetComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff4f4f4f));
    presetComboBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffffffff));
    presetComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xffffffff));
    presetComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffffffff));
    addAndMakeVisible(&presetComboBox);
    
    // Populate preset combo box
    // Populate combo with factory presets and any user .crit files
    populatePresetsCombo();

    // Save button only; loading is done from the combo box which includes user presets
    addAndMakeVisible(&saveButton);
    // Make save button visually prominent and add a tooltip for debugging
    saveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff29a12b));
    saveButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    saveButton.setTooltip("Save preset to user preset folder");
    saveButton.onClick = [this]() {
        juce::File dir = audioProcessor.getUserPresetDirectory();
        auto chooser = std::make_shared<juce::FileChooser>("Save preset as...", dir, "*.crit");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode,
                              [this, chooser](const juce::FileChooser& fc)
        {
            juce::File result = fc.getResult();
            if (! result.existsAsFile())
            {
                // If user entered a filename but it's not created yet, ensure extension
                juce::String fn = result.getFileName();
                if (! fn.endsWithIgnoreCase(".crit"))
                    result = result.withFileExtension("crit");
            }

            if (! result.getFileName().isNotEmpty()) return;

            // Avoid overwriting unless user explicitly chose existing file
            juce::File finalFile = result;
            if (finalFile.existsAsFile())
            {
                // Ask whether to overwrite (provide explicit button texts for non-modal builds)
                int resp = juce::AlertWindow::showYesNoCancelBox(juce::AlertWindow::QuestionIcon,
                                                                 "Overwrite?",
                                                                 "A preset with that name already exists. Overwrite?",
                                                                 "Yes",
                                                                 "No",
                                                                 "Cancel",
                                                                 nullptr,
                                                                 nullptr);
                if (resp != 1) // 1 == yes
                    return;
            }

            if (audioProcessor.savePresetToFile(finalFile))
            {
                populatePresetsCombo();
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Saved", "Preset saved: " + finalFile.getFileName());
            }
            else
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Save Failed", "Failed to save preset.");
            }
        });
    };

    // Load/Install button: allows user to select a file (anywhere) and move/copy it into the preset directory
    addAndMakeVisible(&loadButton);
    // Ensure load button is visually obvious and has a tooltip
    loadButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b2b2b));
    loadButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff29a12b));
    loadButton.setTooltip("Install a .crit preset file into the user preset folder");
    loadButton.onClick = [this]() {
        auto chooser = std::make_shared<juce::FileChooser>("Select a preset file to install", audioProcessor.getUserPresetDirectory(), "*.crit");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this, chooser](const juce::FileChooser& fc)
        {
            juce::File selected = fc.getResult();
            if (! selected.existsAsFile()) return;

            if (! selected.hasFileExtension("crit"))
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Invalid File",
                                                       "Only .crit preset files can be installed.");
                return;
            }

            juce::File destDir = audioProcessor.getUserPresetDirectory();
            juce::String dstName = selected.getFileName();
            juce::File dest = destDir.getChildFile(dstName);

            int idx = 1;
            while (dest.existsAsFile())
            {
                dest = destDir.getChildFile(selected.getFileNameWithoutExtension() + "_" + juce::String(idx) + ".crit");
                ++idx;
            }

            bool ok = false;
            if (selected.moveFileTo(dest))
                ok = true;
            else if (selected.copyFileTo(dest))
                ok = true;

            if (! ok)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Install Failed",
                                                       "Failed to move or copy the selected file into the preset folder.");
                return;
            }

            // Notify user of success and refresh the combo
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                   "Installed",
                                                   "Preset installed: " + dest.getFileName());

            populatePresetsCombo();
        });
    };

    // Set up sliders for interactive knobs
    juce::Slider* sliders[] = { &pressureSlider, &faultSlider, &splitSlider, &momentumSlider, &resolveSlider, &mixSlider };
    for (auto* s : sliders)
    {
        s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s->setLookAndFeel(&knobLNF);
        s->addListener(this);
        addAndMakeVisible(s);
    }

    // Input gain slider setup (horizontal, below knobs)
    inputGainSlider.setSliderStyle(juce::Slider::LinearBar);
    inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 20);
    inputGainSlider.setRange(-60.0, 24.0, 0.1);
    inputGainSlider.setValue(0.0);
    inputGainSlider.setTextValueSuffix(" dB");
    inputGainSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff000000));
    inputGainSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffffffff));
    inputGainSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2b2b2b));

    inputGainLabel.setText("INPUT GAIN", juce::dontSendNotification);
    inputGainLabel.setOpaque(true);
    inputGainLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff000000));
    inputGainLabel.setColour(juce::Label::textColourId, juce::Colour(0xff29a12b));
    inputGainLabel.setJustificationType(juce::Justification::centredLeft);
    inputGainLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    addAndMakeVisible(&inputGainSlider);
    inputGainSlider.setLookAndFeel(&gainLNF);
    addAndMakeVisible(&inputGainLabel);
    inputGainAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getParameters(), "input_gain", inputGainSlider));
    // Stress meter removed

    // Output gain slider (post-processing), visually identical to input gain
    outputGainSlider.setSliderStyle(juce::Slider::LinearBar);
    outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 20);
    outputGainSlider.setRange(-60.0, 24.0, 0.1);
    outputGainSlider.setValue(0.0);
    outputGainSlider.setTextValueSuffix(" dB");
    outputGainSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff000000));
    outputGainSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffffffff));
    outputGainSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2b2b2b));
    addAndMakeVisible(&outputGainSlider);
    outputGainLabel.setText("OUTPUT GAIN", juce::dontSendNotification);
    outputGainLabel.setOpaque(true);
    outputGainLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff000000));
    outputGainLabel.setColour(juce::Label::textColourId, juce::Colour(0xff29a12b));
    outputGainLabel.setJustificationType(juce::Justification::centred);
    outputGainLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    addAndMakeVisible(&outputGainLabel);
    outputGainAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getParameters(), "output_gain", outputGainSlider));
    outputGainSlider.setLookAndFeel(&gainLNF);

    pressureAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getParameters(), "pressure", pressureSlider));
    faultAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getParameters(), "fault", faultSlider));
    splitAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getParameters(), "split", splitSlider));
    momentumAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getParameters(), "momentum", momentumSlider));
    resolveAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getParameters(), "resolve", resolveSlider));
    mixAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.getParameters(), "mix", mixSlider));

    // Create drawables from embedded SVG strings so no external assets are required at runtime.
    {
        std::unique_ptr<juce::XmlElement> xmlBG (juce::XmlDocument::parse(juce::String(GUIAssets::layout_bg_svg)));
        if (xmlBG) backgroundDrawable = juce::Drawable::createFromSVG(*xmlBG);

        // Prefer an external logo SVG in the project root named crit_logo.svg (author-provided).
        juce::File cwd = juce::File::getCurrentWorkingDirectory();
        juce::File logoFile = cwd.getChildFile("crit_logo.svg");
        if (logoFile.existsAsFile())
        {
            std::unique_ptr<juce::XmlElement> xmlLogo(juce::XmlDocument::parse(logoFile.loadFileAsString()));
            if (xmlLogo)
                titleDrawable = juce::Drawable::createFromSVG(*xmlLogo);
        }

        // Prefer an embedded full logo SVG if present in the embedded assets.
        if (!titleDrawable)
        {
            std::unique_ptr<juce::XmlElement> xmlCritLogo (juce::XmlDocument::parse(juce::String(GUIAssets::crit_logo_svg)));
            if (xmlCritLogo)
            {
                // Remove any top-level full-bleed rect that would force an opaque background
                // Parse viewBox to detect numeric full-size rects
                juce::String vb = xmlCritLogo->getStringAttribute("viewBox");
                float vbW = 0.0f, vbH = 0.0f;
                if (vb.isNotEmpty())
                {
                    auto tokens = juce::StringArray::fromTokens(vb, " ", "");
                    if (tokens.size() >= 4)
                    {
                        vbW = tokens[2].getFloatValue();
                        vbH = tokens[3].getFloatValue();
                    }
                }

                for (auto* child = xmlCritLogo->getFirstChildElement(); child != nullptr; )
                {
                    auto* next = child->getNextElement();
                    if (child->hasTagName("rect"))
                    {
                        juce::String x = child->getStringAttribute("x", "0");
                        juce::String y = child->getStringAttribute("y", "0");
                        juce::String w = child->getStringAttribute("width", "");
                        juce::String h = child->getStringAttribute("height", "");

                        bool fullPercents = (w == "100%" && h == "100%");
                        bool fullNumeric = false;
                        if (vbW > 0 && vbH > 0 && w.isNotEmpty() && h.isNotEmpty())
                        {
                            // Compare numeric values approximately
                            float wf = w.getFloatValue();
                            float hf = h.getFloatValue();
                            if (juce::approximatelyEqual(wf, vbW) && juce::approximatelyEqual(hf, vbH))
                                fullNumeric = true;
                        }

                        if (fullPercents || fullNumeric)
                        {
                            // Remove the rect so the SVG can remain transparent
                            xmlCritLogo->removeChildElement(child, true);
                        }
                    }
                    child = next;
                }

                // Create drawable from cleaned SVG
                titleDrawable = juce::Drawable::createFromSVG(*xmlCritLogo);
            }
        }

        // Fallback to embedded title SVG (text version) if external or embedded full logo not found
        if (!titleDrawable)
        {
            std::unique_ptr<juce::XmlElement> xmlTitle (juce::XmlDocument::parse(juce::String(GUIAssets::layout_title_svg)));
            if (xmlTitle) titleDrawable = juce::Drawable::createFromSVG(*xmlTitle);
        }

        // Prefer the embedded vector-only header SVG (added to EmbeddedAssets.h)
        {
            std::unique_ptr<juce::XmlElement> xmlHdr(juce::XmlDocument::parse(juce::String(GUIAssets::layout_header_svg)));
            if (xmlHdr)
                headerDrawable = juce::Drawable::createFromSVG(*xmlHdr);
        }

        // Fallback: if for some reason the embedded header drawable isn't available, try to decode
        // the first embedded PNG in the external SVG file (assets_gui_initial/layout_headerbackground.svg)
        if (!headerDrawable)
        {
            juce::File cwd = juce::File::getCurrentWorkingDirectory();
            juce::File hdr = cwd.getChildFile("assets/gui/raw").getChildFile("layout_headerbackground.svg");
            if (hdr.existsAsFile())
            {
                juce::String svgText = hdr.loadFileAsString();
                const juce::String marker = "xlink:href=\"data:image/png;base64,";
                int pos = svgText.indexOf(marker);
                if (pos >= 0)
                {
                    int start = pos + marker.length();
                    int end = svgText.indexOfChar(start, '"');
                    if (end > start)
                    {
                        juce::String b64 = svgText.substring(start, end);
                        juce::MemoryOutputStream mo;
                        if (juce::Base64::convertFromBase64(mo, b64))
                        {
                            auto mb = mo.getMemoryBlock();
                            auto img = juce::ImageFileFormat::loadFrom(mb.getData(), (int) mb.getSize());
                            if (img.isValid())
                            {
                                headerImage = img;
                                headerImageLoaded = true;
                            }
                        }
                    }
                }
            }
        }

        // We intentionally avoid using a separate knob drawable; knobs are rendered by the LookAndFeel.
    }

    // Attempt to load a font from the project's GUI assets (prefer the Atkinson font folder added to assets)
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        juce::File cwd = juce::File::getCurrentWorkingDirectory();
        juce::File atkinsonDir = cwd.getChildFile("assets/gui/raw").getChildFile("Atkinson_Hyperlegible_Mono");
        if (atkinsonDir.exists() && atkinsonDir.isDirectory())
        {
            auto files = atkinsonDir.findChildFiles(juce::File::findFiles, false, "*.ttf");
            for (auto& f : files)
            {
                auto name = f.getFileName().toLowerCase();
                if (name.contains("italic")) continue; // prefer non-italic
                juce::MemoryBlock mb;
                if (f.loadFileAsData(mb))
                {
                    titleTypeface = juce::Typeface::createSystemTypefaceFor(mb.getData(), (size_t) mb.getSize());
                    if (titleTypeface)
                    {
                        titleTypefaceLoaded = true;
                        titleFont = juce::Font(titleTypeface);
                        titleFont.setHeight(40.0f);
                        break;
                    }
                }
            }
        }

        // Fall back to local Source/GUIAssets/fonts/ if present. Prefer a Zendots font if available.
        if (titleFont.getHeight() <= 0.0f)
        {
            juce::File fontsDir = cwd.getChildFile("assets/fonts");
            if (fontsDir.exists() && fontsDir.isDirectory())
            {
                auto files = fontsDir.findChildFiles(juce::File::findFiles, false, "*.ttf");
                // Prefer files with "zendot" (case-insensitive), otherwise take the first .ttf
                juce::File chosen;
                for (auto& f : files)
                {
                    auto n = f.getFileName().toLowerCase();
                    if (n.contains("zendot") || n.contains("zendots") || n.contains("zendo")) { chosen = f; break; }
                }
                if (! chosen.exists()) if (files.size() > 0) chosen = files.getReference(0);
                if (chosen.existsAsFile())
                {
                    juce::MemoryBlock mb;
                    if (chosen.loadFileAsData(mb))
                    {
                        titleTypeface = juce::Typeface::createSystemTypefaceFor(mb.getData(), (size_t) mb.getSize());
                        if (titleTypeface)
                        {
                            titleTypefaceLoaded = true;
                            titleFont = juce::Font(titleTypeface);
                            titleFont.setHeight(40.0f);
                        }
                    }
                }
            }
        }

#if __has_include("GUIAssets/EmbeddedFont.h")
#include "GUIAssets/EmbeddedFont.h"
        // If EmbeddedFont.h defines a binary array, use it; if it defines base64, decode it.
        if (titleFont.getHeight() <= 0.0f)
        {
#ifdef embedded_font
            titleTypeface = juce::Typeface::createSystemTypefaceFor(embedded_font, (size_t) embedded_font_size);
            if (titleTypeface) { titleTypefaceLoaded = true; titleFont.setHeight(40.0f); titleFont.setTypefaceName(titleTypeface->getName()); }
#elif defined(embedded_font_base64)
            juce::MemoryBlock mb;
            juce::Base64::convertFromBase64(mb, juce::String(embedded_font_base64));
            titleTypeface = juce::Typeface::createSystemTypefaceFor(mb.getData(), (size_t) mb.getSize());
            if (titleTypeface) { titleFont.setHeight(40.0f); titleFont.setTypefaceName(titleTypeface->getName()); }
#endif
        }
#endif

        if (titleFont.getHeight() <= 0.0f)
        {
            // Final fallback to named font (Kohinoor Gujarati) or default
            titleFont.setHeight(40.0f);
            titleFont.setTypefaceName("Kohinoor Gujarati");
            titleFont.setBold(true);
        }
#pragma clang diagnostic pop
    }

CritAudioProcessorEditor::~CritAudioProcessorEditor()
{
    // Clear custom look-and-feel pointers to avoid dangling references
    inputGainSlider.setLookAndFeel(nullptr);
    outputGainSlider.setLookAndFeel(nullptr);
    pressureSlider.setLookAndFeel(nullptr);
    faultSlider.setLookAndFeel(nullptr);
    splitSlider.setLookAndFeel(nullptr);
    momentumSlider.setLookAndFeel(nullptr);
    resolveSlider.setLookAndFeel(nullptr);
    mixSlider.setLookAndFeel(nullptr);
}

void CritAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    (void) slider;
    repaint();
}

void CritAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox != &presetComboBox) return;

    int selectedId = presetComboBox.getSelectedId();
    if (selectedId <= 0) return;

    const int numFactory = CritAudioProcessor::numFactoryPresets;

    if (selectedId <= numFactory)
    {
        int presetIndex = selectedId - 1;
        if (presetIndex >= 0 && presetIndex < numFactory)
        {
            audioProcessor.setCurrentProgram(presetIndex);
            const auto& preset = CritAudioProcessor::factoryPresets[presetIndex];
            pressureSlider.setValue(preset.pressure, juce::dontSendNotification);
            faultSlider.setValue(preset.fault, juce::dontSendNotification);
            splitSlider.setValue(preset.split, juce::dontSendNotification);
            momentumSlider.setValue(preset.momentum, juce::dontSendNotification);
            resolveSlider.setValue(preset.resolve, juce::dontSendNotification);
            mixSlider.setValue(preset.mix, juce::dontSendNotification);
            repaint();
        }
    }
    else
    {
        int userIndex = selectedId - numFactory - 1;
        if (userIndex >= 0 && userIndex < userPresetFiles.size())
        {
            auto f = userPresetFiles[userIndex];
            if (audioProcessor.loadPresetFromFile(f))
            {
                pressureSlider.setValue(audioProcessor.getParameters().getRawParameterValue("pressure")->load(), juce::dontSendNotification);
                faultSlider.setValue(audioProcessor.getParameters().getRawParameterValue("fault")->load(), juce::dontSendNotification);
                splitSlider.setValue(audioProcessor.getParameters().getRawParameterValue("split")->load(), juce::dontSendNotification);
                momentumSlider.setValue(audioProcessor.getParameters().getRawParameterValue("momentum")->load(), juce::dontSendNotification);
                resolveSlider.setValue(audioProcessor.getParameters().getRawParameterValue("resolve")->load(), juce::dontSendNotification);
                mixSlider.setValue(audioProcessor.getParameters().getRawParameterValue("mix")->load(), juce::dontSendNotification);
                repaint();
            }
        }
    }
}

void CritAudioProcessorEditor::populatePresetsCombo()
{
    presetComboBox.clear(juce::dontSendNotification);
    userPresetFiles.clearQuick();

    const int numFactory = CritAudioProcessor::numFactoryPresets;
    // Add factory presets first (IDs 1..numFactory)
    for (int i = 0; i < numFactory; ++i)
    {
        presetComboBox.addItem(CritAudioProcessor::factoryPresets[i].name, i + 1);
    }

    // Scan user preset directory for .crit files and append (IDs numFactory+1 ...)
    juce::File dir = audioProcessor.getUserPresetDirectory();
    juce::Array<juce::File> files = dir.findChildFiles(juce::File::findFiles, false, "*.crit");
    files.sort();
    for (int i = 0; i < files.size(); ++i)
    {
        auto& f = files.getReference(i);
        // Prefer the embedded name= field inside the preset file when present
        juce::String display;
        if (f.existsAsFile())
        {
            juce::String txt = f.loadFileAsString();
            auto lines = juce::StringArray::fromLines(txt);
            for (auto& l : lines)
            {
                if (l.startsWithIgnoreCase("name="))
                {
                    display = l.fromFirstOccurrenceOf("=", false, false).trim();
                    break;
                }
            }
        }
        if (display.isEmpty())
        {
            // Fallback to file name; strip leading numeric prefix like "01_" and convert underscores to spaces
            juce::String fn = f.getFileNameWithoutExtension();
            if (fn.length() > 3 && juce::CharacterFunctions::isDigit(fn[0]) && juce::CharacterFunctions::isDigit(fn[1]) && fn[2] == '_')
                fn = fn.substring(3);
            display = fn.replaceCharacter('_', ' ').trim();
        }

        // Skip if the display name duplicates a factory preset (case-insensitive)
        bool isDuplicate = false;
        for (int k = 0; k < numFactory; ++k)
        {
            if (display.compareIgnoreCase(CritAudioProcessor::factoryPresets[k].name) == 0)
            {
                isDuplicate = true;
                break;
            }
        }

        if (display.isNotEmpty() && !isDuplicate)
        {
            presetComboBox.addItem(display, numFactory + userPresetFiles.size() + 1);
            userPresetFiles.add(f);
        }
    }

    // Select first factory preset by default
    if (numFactory > 0)
        presetComboBox.setSelectedId(1, juce::dontSendNotification);
}

void CritAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background - match Figma design (light gray with dot pattern)
    g.fillAll(juce::Colour(0xffe8e8e8));
    
    // Draw dot pattern background
    g.setColour(juce::Colour(0x1a999999));
    int dotSize = 8;
    for (int x = 0; x < getWidth(); x += dotSize)
    {
        for (int y = 0; y < getHeight(); y += dotSize)
        {
            g.fillEllipse((float)x, (float)y, 1.0f, 1.0f);
        }
    }

    // Header/title (use proportional header derived from the SVG layout)
    int headerHeight = juce::jmax(56, (int)std::round(getHeight() * 0.11f));
    int headerPad = juce::jmax(12, (int)std::round(headerHeight * 0.18f));
    
    // Draw header background: prefer decoded embedded PNG, then headerDrawable, otherwise fallback to light gray
    if (headerImageLoaded && headerImage.isValid())
    {
        g.drawImageWithin(headerImage, 0, 0, getWidth(), headerHeight, juce::RectanglePlacement::stretchToFit);
    }
    else if (headerDrawable)
    {
        juce::Rectangle<float> hdrR (0.0f, 0.0f, (float)getWidth(), (float)headerHeight);
        headerDrawable->drawWithin(g, hdrR, juce::RectanglePlacement::stretchToFit, 1.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xfff2f2f2));
        g.fillRect(0, 0, getWidth(), headerHeight);
    }

    // Draw spaced dots over the header using CPU rendering to avoid SVG rasterization issues.
    // Tuned parameters: spacing 28 px, dot radius 1.6, alpha 0.12 (more visible)
    const int dotSpacing = 28;
    const float dotR = 1.6f;
    const float dotAlpha = 0.12f;
    g.setColour(juce::Colours::black.withAlpha(dotAlpha));
    for (int x = 0 + dotSpacing/2; x < getWidth(); x += dotSpacing)
    {
        for (int y = 0 + dotSpacing/2; y < headerHeight; y += dotSpacing)
        {
            g.fillEllipse((float)x - dotR, (float)y - dotR, dotR * 2.0f, dotR * 2.0f);
        }
    }
    
    // (Removed diagonal cross pattern from header to avoid visual clutter)
    
    // Title/logo placement - prefer SVG drawable (external `crit_logo.svg` or embedded), otherwise render text
    {
        float titleHeight = (float)(headerHeight - headerPad * 2);
        float titleWidth = juce::jmin(420.0f, getWidth() / 2.0f);

        if (titleDrawable)
        {
            juce::Rectangle<float> r ((float)headerPad, (float)headerPad, titleWidth, titleHeight);
            titleDrawable->drawWithin(g, r, juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yTop, 1.0f);
        }
        else if (titleTypefaceLoaded && titleTypeface)
        {
            g.setColour(juce::Colour(0xff29a12b));
            float targetFontHeight = titleHeight * 0.9f;
            juce::Font f = juce::Font(titleTypeface).withHeight(targetFontHeight);
            f.setBold(true);
            g.setFont(f);
            g.drawText("crit", headerPad, headerPad, (int)titleWidth, (int)titleHeight, juce::Justification::centredLeft, true);
        }
        else
        {
            juce::Rectangle<int> headerArea (headerPad, headerPad, (int)titleWidth, headerHeight - headerPad * 2);
            g.setColour(juce::Colour(0xff29a12b));
            float targetFontHeight = (float)(headerHeight - headerPad * 2) * 0.8f;
            juce::Font f;
            if (titleFont.getHeight() > 0.0f)
                f = titleFont.withHeight(targetFontHeight);
            else
                f = juce::Font(targetFontHeight);
            f.setBold(true);
            g.setFont(f);
            g.drawText("crit", headerArea, juce::Justification::centredLeft, true);
        }
    }

    // Draw debug overlay for gain-match values when enabled
    // (removed gain-match debug overlay)

    // Preset combo box styling is handled by the component itself
    // Save button (draw on top of header) - match Figma green style
    int saveW = juce::jmin(70, getWidth() / 12);
    int saveH = juce::jmin(36, headerHeight - headerPad * 2);
    int saveX = getWidth() - saveW - headerPad;
    int saveY = headerPad;
    // Reserve space for Save/Load buttons (actual buttons are created in ctor)
    

    // Draw knob area background (light gray matching Figma)
    int knobAreaY = matrixDisplay.getY() + matrixDisplay.getHeight() + 16;
    g.setColour(juce::Colour(0xffe8e8e8));
    g.fillRect(0, knobAreaY, getWidth(), getHeight() - knobAreaY);
    
    // Knob layout (use actual slider bounds when available to keep sizing consistent)
    int knobCount = 6;
    juce::StringArray knobNames {"PRESSURE", "FAULT", "SPLIT", "MOMENTUM", "RESOLVE", "MIX"};
    juce::Slider* sliders[] = { &pressureSlider, &faultSlider, &splitSlider, &momentumSlider, &resolveSlider, &mixSlider };

    // Knob sizing: make proportional to the overall window and the reference SVG
    int knobRadius = 0;
    if (knobDiameter > 0)
        knobRadius = knobDiameter;
    else if (sliders[0]->getWidth() > 0)
        knobRadius = sliders[0]->getWidth();
    else
    {
        int maxByWidth = (getWidth() - headerPad * 2) / (knobCount + 1);
        int maxByHeight = juce::jmax(100, (int)std::round((getHeight() - knobAreaY) * 0.4f));
        knobRadius = juce::jmin(juce::jmin(maxByWidth, maxByHeight), 100);
    }

    int knobSpacing = juce::jmax(8, (getWidth() - knobCount * knobRadius - headerPad * 2) / (knobCount + 1));
    int knobY = knobAreaY + 24;

    // Note: Knobs are drawn by the sliders themselves via LookAndFeel, no need to draw here

    // Labels and values for knobs (sublabels removed)
    for (int i = 0; i < knobCount; ++i)
    {
        int x = headerPad + knobSpacing * (i + 1) + knobRadius * i;
        int labelY = knobY + knobRadius + 16; // increased offset to avoid overlap
        
        // Knob label (black background box matching Figma)
        g.setColour(juce::Colour(0xff000000));
        juce::Font labelFont(11.0f);
        labelFont.setBold(true);
        if (titleFont.getHeight() > 0.0f && titleTypefaceLoaded)
            labelFont = titleFont.withHeight(11.0f).boldened();
        g.setFont(labelFont);
        juce::Rectangle<int> labelRect(x, labelY, knobRadius, 20);
        g.fillRect(labelRect);
        g.setColour(juce::Colour(0xff29a12b));
        g.drawText(knobNames[i], labelRect, juce::Justification::centred, true);

        // Knob value (black background box matching Figma)
        int valueY = labelY + 20;
        g.setColour(juce::Colour(0xff000000));
        juce::Font valueFont(8.0f);
        if (titleFont.getHeight() > 0.0f && titleTypefaceLoaded)
            valueFont = titleFont.withHeight(8.0f);
        g.setFont(valueFont);
        juce::String valueText = juce::String((int)(sliders[i]->getValue() * 100)) + "%";
        juce::Rectangle<int> valueRect(x, valueY, knobRadius, 16);
        g.fillRect(valueRect);
        g.setColour(juce::Colour(0xff29a12b));
        g.drawText(valueText, valueRect, juce::Justification::centred, true);
    }
}

void CritAudioProcessorEditor::resized()
{
    // Header layout
    int headerHeight = juce::jmax(56, (int)std::round(getHeight() * 0.11f));
    int headerPad = juce::jmax(12, (int)std::round(headerHeight * 0.18f));
    
    // Position preset combo box in header and pin Save/Load buttons to the right
    int presetH = juce::jmin(36, headerHeight - headerPad * 2);

    // Button sizing: scale modestly with window size but keep reasonable min/max
    int btnH = presetH;
    int btnW = juce::jmin(96, juce::jmax(56, getWidth() / 14));

    // Place buttons flush to the right edge
    int rightMargin = headerPad;
    int loadX = getWidth() - rightMargin - btnW;
    int saveX = loadX - 8 - btnW;

    // Compute available width for preset combo to the left of the Save button
    int maxPresetW = saveX - headerPad - 8;
    int presetW = juce::jmin(300, juce::jmax(120, maxPresetW));
    int presetX = juce::jmax(headerPad, saveX - presetW - 8);

    presetComboBox.setBounds(presetX, headerPad, presetW, presetH);
    saveButton.setBounds(saveX, headerPad, btnW, btnH);
    loadButton.setBounds(loadX, headerPad, btnW, btnH);
    
    // Position matrix display - full width with minimal padding for inset shadow effect
    int matrixY = headerHeight + 16;
    int matrixHeight = juce::jmax(200, (int)std::round(getHeight() * 0.35f));
    int matrixPad = 2; // Minimal padding for inset shadow
    matrixDisplay.setBounds(matrixPad, matrixY, getWidth() - matrixPad * 2, matrixHeight);
    
    // Layout knobs interactively
    int knobCount = 6;
    int knobAreaY = matrixY + matrixHeight + 32;
    int knobAreaHeight = getHeight() - knobAreaY - headerPad;
    int maxByWidth = (getWidth() - headerPad * 2) / (knobCount + 1);
    int maxByHeight = juce::jmin(knobAreaHeight - 60, 100); // Leave room for labels (sublabels removed)
    int knobRadius = juce::jmin(juce::jmin(maxByWidth, maxByHeight), 100);
    knobDiameter = knobRadius;
    int knobSpacing = juce::jmax(8, (getWidth() - knobCount * knobRadius - headerPad * 2) / (knobCount + 1));
    int knobY = knobAreaY + 24;
    juce::Slider* sliders[] = { &pressureSlider, &faultSlider, &splitSlider, &momentumSlider, &resolveSlider, &mixSlider };
    for (int i = 0; i < knobCount; ++i)
    {
        int x = headerPad + knobSpacing * (i + 1) + knobRadius * i;
        sliders[i]->setBounds(x, knobY, knobRadius, knobRadius);
    }

    // Make input label font/color match knob labels
    {
        juce::Font labelFont(11.0f);
        labelFont.setBold(true);
        if (titleFont.getHeight() > 0.0f && titleTypefaceLoaded)
            labelFont = titleFont.withHeight(11.0f).boldened();
        inputGainLabel.setFont(labelFont);
        inputGainLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff000000));
        inputGainLabel.setColour(juce::Label::textColourId, juce::Colour(0xff29a12b));
        inputGainLabel.setJustificationType(juce::Justification::centred);
    }

    // Position input and output gain sliders below the knobs (two-column layout)
    int controlStripY = knobY + knobRadius + 96; // comfortable spacing
    int controlStripH = 42;
    int controlPad = headerPad;
    int availableW = getWidth() - controlPad * 2;
    int gap = 16;
    int colW = (availableW - gap) / 2;
    int leftX = controlPad + 8;
    int rightX = leftX + colW + gap;

    inputGainSlider.setBounds(leftX, controlStripY, colW - 8, controlStripH);
    inputGainLabel.setBounds(leftX, controlStripY - 28, 160, 28);
    inputGainLabel.setFont(juce::Font(14.0f, juce::Font::bold));

    outputGainSlider.setBounds(rightX, controlStripY, colW - 8, controlStripH);
    outputGainLabel.setBounds(rightX, controlStripY - 28, 160, 28);

    // Visual styling for sliders
    inputGainSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff000000));
    inputGainSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff000000));
    inputGainSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff29a12b));
    inputGainSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff2b2b2b));

    outputGainSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff000000));
    outputGainSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff000000));
    outputGainSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff29a12b));
    outputGainSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff2b2b2b));

    // Ensure slider reflects processor state at startup
    outputGainSlider.setValue(audioProcessor.getParameters().getRawParameterValue("output_gain")->load(), juce::dontSendNotification);
}
#pragma clang diagnostic pop
