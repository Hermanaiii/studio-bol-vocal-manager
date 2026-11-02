#include "PluginEditor.h"

StudioBolVocalManagerAudioProcessorEditor::StudioBolVocalManagerAudioProcessorEditor(StudioBolVocalManagerAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(620, 430);
    startTimerHz(2);
    title.setText("STUDIO BOL VOCAL MANAGER", juce::dontSendNotification);
    title.setFont(juce::Font(25.0f, juce::Font::bold)); title.setJustificationType(juce::Justification::centredLeft); addAndMakeVisible(title);
    referenceLabel.setText(processor.getReferenceName(), juce::dontSendNotification); referenceLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen); addAndMakeVisible(referenceLabel);
    statusLabel.setText(processor.getAnalysisStatus(), juce::dontSendNotification); statusLabel.setJustificationType(juce::Justification::centredLeft); statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey); addAndMakeVisible(statusLabel);
    addAndMakeVisible(loadButton); addAndMakeVisible(matchButton); addAndMakeVisible(resetButton); addAndMakeVisible(bypassButton);
    loadButton.onClick = [this]
    {
        // The chooser completes asynchronously. The editor can be closed while
        // the native file dialog is open, so never capture an unguarded `this`
        // in its completion callback. SafePointer becomes null on destruction.
        juce::Component::SafePointer<StudioBolVocalManagerAudioProcessorEditor> safeThis(this);
        chooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safeThis](const juce::FileChooser& fc) mutable
            {
                if (safeThis == nullptr) return;
                const auto file = fc.getResult();
                if (!file.existsAsFile()) return;
                juce::String message;
                safeThis->processor.loadReferenceFile(file, message);
                safeThis->referenceLabel.setText(safeThis->processor.getReferenceName(), juce::dontSendNotification);
                safeThis->statusLabel.setText(message, juce::dontSendNotification);
            });
    };
    matchButton.onClick = [this] { processor.calculateMatch(); statusLabel.setText(processor.getAnalysisStatus(), juce::dontSendNotification); };
    resetButton.onClick = [this] { processor.resetTargetAnalysis(); statusLabel.setText(processor.getAnalysisStatus(), juce::dontSendNotification); };
    bypassButton.onClick = [this] { auto* v = processor.parameters.getRawParameterValue("matchEnabled"); processor.parameters.getParameter("matchEnabled")->setValueNotifyingHost(v->load() > 0.5f ? 0.0f : 1.0f); };

    auto setup = [this](juce::Slider& s, juce::Label& l, const juce::String& name)
    { s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 22); addAndMakeVisible(s); l.setText(name, juce::dontSendNotification); l.setJustificationType(juce::Justification::centred); addAndMakeVisible(l); };
    setup(amount, amountLabel, "MATCH AMOUNT"); setup(compression, compressionLabel, "COMPRESSION"); setup(deesser, deesserLabel, "DE-ESSER"); setup(saturation, saturationLabel, "SATURATION"); setup(output, outputLabel, "OUTPUT");
    amountAttachment = std::make_unique<Attachment>(processor.parameters, "amount", amount);
    compressionAttachment = std::make_unique<Attachment>(processor.parameters, "compression", compression);
    deesserAttachment = std::make_unique<Attachment>(processor.parameters, "deesser", deesser);
    saturationAttachment = std::make_unique<Attachment>(processor.parameters, "saturation", saturation);
    outputAttachment = std::make_unique<Attachment>(processor.parameters, "output", output);
}
void StudioBolVocalManagerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff171b22));
    g.setColour(juce::Colour(0xff252d38)); g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(12.0f), 12.0f);
    g.setColour(juce::Colour(0xff42c7a5)); g.fillRoundedRectangle(28.0f, 83.0f, 564.0f, 3.0f, 1.5f);
    g.setColour(juce::Colours::white); g.setFont(13.0f); g.drawText("Load reference  •  Play your vocal  •  Analyse & Match  •  Adjust", 30, 185, 560, 24, juce::Justification::centredLeft);
}
void StudioBolVocalManagerAudioProcessorEditor::resized()
{
    title.setBounds(28, 20, 560, 42); referenceLabel.setBounds(30, 65, 560, 24);
    loadButton.setBounds(30, 95, 165, 34); matchButton.setBounds(205, 95, 165, 34); resetButton.setBounds(380, 95, 115, 34); bypassButton.setBounds(505, 95, 85, 34);
    statusLabel.setBounds(30, 150, 560, 48);
    const int y = 220, w = 105, gap = 10; juce::Slider* sliders[] = { &amount, &compression, &deesser, &saturation, &output }; juce::Label* labels[] = { &amountLabel, &compressionLabel, &deesserLabel, &saturationLabel, &outputLabel };
    for (int i = 0; i < 5; ++i) { const int x = 30 + i * (w + gap); labels[i]->setBounds(x, y, w, 25); sliders[i]->setBounds(x, y + 25, w, 145); }
}

void StudioBolVocalManagerAudioProcessorEditor::timerCallback()
{
    statusLabel.setText(processor.getAnalysisStatus(), juce::dontSendNotification);
    referenceLabel.setText(processor.getReferenceName(), juce::dontSendNotification);
}
