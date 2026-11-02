#pragma once
#include <JuceHeader.h>
#include <array>

class StudioBolVocalManagerAudioProcessor : public juce::AudioProcessor
{
public:
    StudioBolVocalManagerAudioProcessor();
    ~StudioBolVocalManagerAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Studio Bol Vocal Manager"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    bool loadReferenceFile(const juce::File& file, juce::String& message);
    juce::String getReferenceName() const { return referenceName; }
    juce::String getAnalysisStatus() const { return analysisStatus; }
    void calculateMatch();
    void resetTargetAnalysis();
    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int bandCount = 6;
    const std::array<float, bandCount> bandEdges { 100.0f, 250.0f, 500.0f, 1200.0f, 3500.0f, 8000.0f };
    std::array<double, bandCount> referenceEnergy {}, targetEnergy {};
    std::array<std::atomic<double>, bandCount> targetEnergySum {};
    std::atomic<int> targetAnalysisFrames { 0 };
    juce::dsp::FFT fft { fftOrder };
    juce::AudioBuffer<float> analysisBuffer;
    std::array<float, fftSize * 2> fftData {};
    int analysisWrite = 0;
    double currentSampleRate = 44100.0;
    double referenceRms = 0.0, referencePeak = 0.0;
    std::atomic<double> targetRms { 0.0 };
    std::atomic<double> targetPeak { 0.0 };
    std::array<float, 2> deEssEnvelope {};
    std::array<float, 2> compressorEnvelope {};
    std::array<std::array<juce::dsp::IIR::Filter<float>, bandCount>, 2> eqFilters;
    std::array<juce::dsp::IIR::Filter<float>, 2> deEssFilters;
    std::atomic<bool> pendingDspUpdate { false };
    std::atomic<bool> pendingTargetReset { false };
    juce::String referenceName = "No reference loaded";
    juce::String analysisStatus = "Load a reference track or vocal stem";
    std::array<float, bandCount> matchedGains {};
    std::atomic<float> compressorThreshold { -18.0f };
    std::atomic<float> compressorRatio { 2.0f };
    void analyseReferenceBuffer(const juce::AudioBuffer<float>& audio, double sourceSampleRate);
    bool analyseReferenceReader(juce::AudioFormatReader& reader);
    void analyseTargetBlock(const juce::AudioBuffer<float>& audio);
    void updateDsp();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StudioBolVocalManagerAudioProcessor)
};
