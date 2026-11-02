#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class StudioBolVocalManagerAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit StudioBolVocalManagerAudioProcessorEditor(StudioBolVocalManagerAudioProcessor&);
    ~StudioBolVocalManagerAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
private:
    StudioBolVocalManagerAudioProcessor& processor;
    juce::Label title, referenceLabel, statusLabel;
    juce::TextButton loadButton { "LOAD REFERENCE AUDIO" }, matchButton { "ANALYSE & MATCH" }, resetButton { "RESET ANALYSIS" }, bypassButton { "MATCH ON / OFF" };
    juce::Slider amount, compression, deesser, saturation, output;
    juce::Label amountLabel, compressionLabel, deesserLabel, saturationLabel, outputLabel;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> amountAttachment, compressionAttachment, deesserAttachment, saturationAttachment, outputAttachment;
    juce::FileChooser chooser { "Choose reference audio", juce::File{}, "*.wav;*.mp3;*.aiff;*.aif" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StudioBolVocalManagerAudioProcessorEditor)
};
