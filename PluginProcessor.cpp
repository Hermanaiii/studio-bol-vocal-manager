#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr std::array<float, 6> centres { 140.0f, 350.0f, 750.0f, 1800.0f, 4500.0f, 9500.0f };
    float clampGain(float x) { return juce::jlimit(-6.0f, 6.0f, x); }
}

StudioBolVocalManagerAudioProcessor::StudioBolVocalManagerAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    analysisBuffer.setSize(1, fftSize);
    for (auto& value : targetEnergySum) value.store(0.0, std::memory_order_relaxed);
}

juce::AudioProcessorValueTreeState::ParameterLayout StudioBolVocalManagerAudioProcessor::createParameterLayout()
{
    using P = juce::AudioProcessorValueTreeState;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>("amount", "Match Amount", 0.0f, 1.0f, 0.8f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("compression", "Compression", 0.0f, 1.0f, 0.65f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("deesser", "De-Esser", 0.0f, 1.0f, 0.45f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("saturation", "Saturation", 0.0f, 1.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("output", "Output", -12.0f, 12.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterBool>("matchEnabled", "Match Enabled", true));
    return { p.begin(), p.end() };
}

void StudioBolVocalManagerAudioProcessor::prepareToPlay(double sr, int)
{
    currentSampleRate = sr;
    for (auto& channelFilters : eqFilters)
        for (auto& f : channelFilters) f.reset();
    for (auto& f : deEssFilters) f.reset();
    compressorEnvelope.fill(0.0f);
    deEssEnvelope.fill(0.0f);
    updateDsp();
}
void StudioBolVocalManagerAudioProcessor::releaseResources() {}
bool StudioBolVocalManagerAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet();
    const auto out = l.getMainOutputChannelSet();
    return (in == out) && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void StudioBolVocalManagerAudioProcessor::analyseReferenceBuffer(const juce::AudioBuffer<float>& audio, double sourceSampleRate)
{
    referenceEnergy.fill(0.0); referenceRms = 0.0; referencePeak = 0.0;
    const int channels = audio.getNumChannels();
    const int total = audio.getNumSamples();
    double squareSum = 0.0; int count = 0;
    std::array<double, bandCount> sums {};
    int frames = 0;
    std::array<float, fftSize * 2> temp {};
    // Reference loading runs on the UI thread while processBlock may analyse
    // the live input. Use a private FFT instance so the two analyses never
    // mutate the same JUCE FFT workspace concurrently.
    juce::dsp::FFT referenceFft { fftOrder };
    for (int start = 0; start + fftSize <= total; start += fftSize / 2)
    {
        for (int i = 0; i < fftSize; ++i)
        {
            float s = 0.0f;
            for (int c = 0; c < channels; ++c) s += audio.getSample(c, start + i);
            s /= (float) channels;
            temp[(size_t)i] = s * (0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * i / (fftSize - 1)));
            squareSum += (double)s * s; ++count;
            referencePeak = juce::jmax(referencePeak, (double)std::abs(s));
        }
        std::fill(temp.begin() + fftSize, temp.end(), 0.0f);
        referenceFft.performFrequencyOnlyForwardTransform(temp.data());
        for (int k = 1; k < fftSize / 2; ++k)
        {
            const float hz = (float)k * (float)sourceSampleRate / (float)fftSize;
            int b = hz < 200.0f ? 0 : hz < 500.0f ? 1 : hz < 1000.0f ? 2 : hz < 2500.0f ? 3 : hz < 6000.0f ? 4 : 5;
            sums[(size_t)b] += (double)temp[(size_t)k] * temp[(size_t)k];
        }
        ++frames;
    }
    if (count > 0) referenceRms = std::sqrt(squareSum / count);
    if (frames > 0) for (int b = 0; b < bandCount; ++b) referenceEnergy[(size_t)b] = sums[(size_t)b] / frames;
}

bool StudioBolVocalManagerAudioProcessor::analyseReferenceReader(juce::AudioFormatReader& reader)
{
    referenceEnergy.fill(0.0);
    referenceRms = 0.0;
    referencePeak = 0.0;

    const int channels = (int) reader.numChannels;
    const juce::int64 totalSamples = reader.lengthInSamples;
    if (channels < 1 || channels > 2 || totalSamples < fftSize)
        return false;

    juce::AudioBuffer<float> window(channels, fftSize);
    std::array<float, fftSize * 2> fftWork {};
    juce::dsp::FFT localFft { fftOrder };
    std::array<double, bandCount> bandSums {};
    double squareSum = 0.0;
    juce::int64 sampleCount = 0;
    int frameCount = 0;

    constexpr int hopSize = fftSize / 2;
    for (juce::int64 start = 0; start + fftSize <= totalSamples; start += hopSize)
    {
        window.clear();
        if (!reader.read(&window, 0, fftSize, start, true, true))
            return false;

        for (int i = 0; i < fftSize; ++i)
        {
            float mono = 0.0f;
            for (int c = 0; c < channels; ++c)
                mono += window.getSample(c, i);
            mono /= (float) channels;
            squareSum += (double) mono * mono;
            ++sampleCount;
            referencePeak = juce::jmax(referencePeak, (double) std::abs(mono));
            fftWork[(size_t) i] = mono * (0.5f - 0.5f * std::cos(
                juce::MathConstants<float>::twoPi * (float) i / (float) (fftSize - 1)));
        }

        std::fill(fftWork.begin() + fftSize, fftWork.end(), 0.0f);
        localFft.performFrequencyOnlyForwardTransform(fftWork.data());
        for (int k = 1; k < fftSize / 2; ++k)
        {
            const float hz = (float) k * (float) reader.sampleRate / (float) fftSize;
            const int band = hz < 200.0f ? 0 : hz < 500.0f ? 1 : hz < 1000.0f ? 2
                              : hz < 2500.0f ? 3 : hz < 6000.0f ? 4 : 5;
            bandSums[(size_t) band] += (double) fftWork[(size_t) k] * fftWork[(size_t) k];
        }
        ++frameCount;
    }

    if (sampleCount == 0 || frameCount == 0)
        return false;
    referenceRms = std::sqrt(squareSum / (double) sampleCount);
    for (int b = 0; b < bandCount; ++b)
        referenceEnergy[(size_t) b] = bandSums[(size_t) b] / (double) frameCount;
    return referenceRms > 0.0;
}

bool StudioBolVocalManagerAudioProcessor::loadReferenceFile(const juce::File& file, juce::String& message)
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
    if (!reader) { message = "Could not read this audio file. Use WAV, AIFF or MP3."; return false; }
    if (reader->lengthInSamples <= 0 || reader->lengthInSamples > (juce::int64)reader->sampleRate * 60 * 20)
    { message = "Reference must be between 1 sample and 20 minutes."; return false; }
    if (reader->numChannels < 1 || reader->numChannels > 2)
    { message = "Only mono and stereo references are supported."; return false; }
    // Stream the reference in short FFT windows instead of allocating the entire
    // song. A 20-minute stereo file can otherwise require well over 1 GB of RAM.
    if (!analyseReferenceReader(*reader))
    {
        message = "Reference is too short to analyse.";
        return false;
    }
    referenceName = file.getFileName();
    analysisStatus = "Reference analysed. Play your vocal, then press Match.";
    message = analysisStatus;
    return true;
}

void StudioBolVocalManagerAudioProcessor::analyseTargetBlock(const juce::AudioBuffer<float>& audio)
{
    // Rolling block analysis: collect input RMS and a simple frequency profile using FFT windows.
    const int n = audio.getNumSamples();
    const int inputChannels = audio.getNumChannels();
    if (inputChannels <= 0 || n <= 0) return;
    double sq = 0.0;
    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int c = 0; c < inputChannels; ++c) s += audio.getSample(c, i);
        s /= (float)inputChannels; sq += (double)s * s;
    }
    const double blockRms = std::sqrt(sq / juce::jmax(1, n));
    const double previousRms = targetRms.load(std::memory_order_relaxed);
    targetRms.store(previousRms * 0.97 + blockRms * 0.03, std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
    {
        float sample = 0.0f;
        for (int c = 0; c < inputChannels; ++c) sample += audio.getSample(c, i);
        sample /= (float) inputChannels;
        double peak = targetPeak.load(std::memory_order_relaxed);
        const double candidate = std::abs((double) sample);
        while (candidate > peak && !targetPeak.compare_exchange_weak(peak, candidate, std::memory_order_relaxed)) {}
    }

    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int c = 0; c < inputChannels; ++c) s += audio.getSample(c, i);
        analysisBuffer.setSample(0, analysisWrite++, s / (float)inputChannels);
        if (analysisWrite >= fftSize)
        {
            for (int j = 0; j < fftSize; ++j)
                fftData[(size_t)j] = analysisBuffer.getSample(0, j) * (0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * j / (fftSize - 1)));
            std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
            fft.performFrequencyOnlyForwardTransform(fftData.data());
            std::array<double, bandCount> e {};
            for (int k = 1; k < fftSize / 2; ++k)
            {
                const float hz = (float)k * (float)currentSampleRate / (float)fftSize;
                int b = hz < 200.0f ? 0 : hz < 500.0f ? 1 : hz < 1000.0f ? 2 : hz < 2500.0f ? 3 : hz < 6000.0f ? 4 : 5;
                e[(size_t)b] += (double)fftData[(size_t)k] * fftData[(size_t)k];
            }
            for (int b = 0; b < bandCount; ++b)
            {
                auto& accumulator = targetEnergySum[(size_t)b];
                double previous = accumulator.load(std::memory_order_relaxed);
                while (!accumulator.compare_exchange_weak(previous, previous + e[(size_t)b], std::memory_order_relaxed)) {}
            }
            targetAnalysisFrames.fetch_add(1, std::memory_order_relaxed);
            analysisWrite = 0;
        }
    }
}

void StudioBolVocalManagerAudioProcessor::calculateMatch()
{
    const int frameCount = targetAnalysisFrames.load(std::memory_order_acquire);
    if (referenceRms <= 0.0 || frameCount == 0) { analysisStatus = "Load a reference and play at least a few seconds of your vocal first."; return; }
    double refTotal = 0.0, targetTotal = 0.0;
    for (int b = 0; b < bandCount; ++b)
    {
        refTotal += referenceEnergy[(size_t)b];
        targetTotal += targetEnergySum[(size_t)b].load(std::memory_order_relaxed) / (double) frameCount;
    }
    for (int b = 0; b < bandCount; ++b)
    {
        // Compare normalized spectral balance, not raw loudness. This avoids
        // turning a simple recording-level difference into an EQ correction.
        const double ref = referenceEnergy[(size_t)b] / juce::jmax(1.0e-12, refTotal) + 1.0e-9;
        const double tgt = (targetEnergySum[(size_t)b].load(std::memory_order_relaxed) / (double) frameCount)
                           / juce::jmax(1.0e-12, targetTotal) + 1.0e-9;
        matchedGains[(size_t)b] = clampGain((float) (10.0 * std::log10(ref / tgt)) * 0.30f);
    }
    const float refCrest = (float)(20.0 * std::log10((referencePeak + 1.0e-8) / (referenceRms + 1.0e-8)));
    const float tgtCrest = (float)(20.0 * std::log10((targetPeak.load() + 1.0e-8) / (targetRms.load() + 1.0e-8)));
    // A restrained starting point; the reference is a sound target, not a
    // recoverable copy of the original mixer's compressor settings.
    compressorThreshold.store(juce::jlimit(-30.0f, -8.0f, -18.0f + (tgtCrest - refCrest) * 0.35f));
    compressorRatio.store(juce::jlimit(1.3f, 4.0f, 2.0f + (tgtCrest - refCrest) * 0.08f));
    pendingDspUpdate.store(true, std::memory_order_release);
    analysisStatus = "Match calculated. Adjust the controls by ear; use Reset Analysis before recording a new comparison.";
}

void StudioBolVocalManagerAudioProcessor::resetTargetAnalysis()
{
    pendingTargetReset.store(true, std::memory_order_release);
    analysisStatus = "Reset requested. Play your vocal for several seconds.";
}

void StudioBolVocalManagerAudioProcessor::updateDsp()
{
    for (int channel = 0; channel < 2; ++channel)
    {
        for (int b = 0; b < bandCount; ++b)
        {
            auto coeff = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, centres[(size_t)b], 0.85f,
                                juce::Decibels::decibelsToGain(matchedGains[(size_t)b]));
            eqFilters[(size_t)channel][(size_t)b].coefficients = coeff;
        }
        deEssFilters[(size_t)channel].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, 6500.0f);
    }
}

void StudioBolVocalManagerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = getTotalNumInputChannels();
    for (int c = channels; c < getTotalNumOutputChannels(); ++c) buffer.clear(c, 0, buffer.getNumSamples());
    if (pendingTargetReset.exchange(false, std::memory_order_acq_rel))
    {
        for (auto& value : targetEnergySum) value.store(0.0, std::memory_order_relaxed);
        targetAnalysisFrames.store(0, std::memory_order_relaxed);
        targetRms.store(0.0, std::memory_order_relaxed);
        targetPeak.store(0.0, std::memory_order_relaxed);
        analysisWrite = 0;
    }
    analyseTargetBlock(buffer);
    if (pendingDspUpdate.exchange(false, std::memory_order_acq_rel)) updateDsp();
    const bool enabled = parameters.getRawParameterValue("matchEnabled")->load() > 0.5f;
    const float amount = parameters.getRawParameterValue("amount")->load();
    const float comp = parameters.getRawParameterValue("compression")->load();
    const float deess = parameters.getRawParameterValue("deesser")->load();
    const float saturation = parameters.getRawParameterValue("saturation")->load();
    const float outGain = juce::Decibels::decibelsToGain(parameters.getRawParameterValue("output")->load());
    const float threshold = juce::Decibels::decibelsToGain(compressorThreshold.load(std::memory_order_relaxed));
    const float ratio = compressorRatio.load(std::memory_order_relaxed);
    const float attack = (float)std::exp(-1.0 / (0.010 * currentSampleRate));
    const float release = (float)std::exp(-1.0 / (0.100 * currentSampleRate));
    const float deEssAttack = (float)std::exp(-1.0 / (0.002 * currentSampleRate));
    const float deEssRelease = (float)std::exp(-1.0 / (0.070 * currentSampleRate));
    for (int c = 0; c < juce::jmin(channels, 2); ++c)
    {
        auto* x = buffer.getWritePointer(c);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float s = x[i];
            if (enabled)
            {
                const float dry = s;
                float equalized = s;
                for (auto& f : eqFilters[(size_t) juce::jmin(c, 1)]) equalized = f.processSample(equalized);
                s = dry + (equalized - dry) * amount;
                const int channel = juce::jmin(c, 1);
                const float high = deEssFilters[(size_t)channel].processSample(s);
                const float sibilance = std::abs(high);
                const float broadband = std::abs(s);
                auto& compEnv = compressorEnvelope[(size_t)channel];
                auto& essEnv = deEssEnvelope[(size_t)channel];
                compEnv = broadband > compEnv ? attack * compEnv + (1.0f - attack) * broadband
                                              : release * compEnv + (1.0f - release) * broadband;
                essEnv = sibilance > essEnv ? deEssAttack * essEnv + (1.0f - deEssAttack) * sibilance
                                            : deEssRelease * essEnv + (1.0f - deEssRelease) * sibilance;
                const float over = compEnv > threshold ? compEnv / juce::jmax(0.001f, threshold) : 1.0f;
                const float reduction = std::pow(over, -(ratio - 1.0f) / ratio * comp);
                s *= reduction;
                // Attenuate only the high-frequency component when sibilance rises.
                const float essThreshold = 0.018f;
                const float essOver = juce::jmax(0.0f, essEnv - essThreshold);
                const float essAmount = juce::jlimit(0.0f, 0.75f, essOver * 8.0f * deess);
                s -= high * essAmount;
            }
            // Gentle, level-compensated soft saturation. At zero this is exactly transparent.
            if (saturation > 0.0001f)
            {
                const float drive = 1.0f + saturation * 3.0f;
                // Unity small-signal gain: the curve adds peak density without the large
                // level boost caused by normalising tanh(drive) to full scale.
                const float shaped = std::tanh(s * drive) / drive;
                s = s + (shaped - s) * saturation;
            }
            x[i] = s * outGain;
        }
    }
}

juce::AudioProcessorEditor* StudioBolVocalManagerAudioProcessor::createEditor() { return new StudioBolVocalManagerAudioProcessorEditor(*this); }
void StudioBolVocalManagerAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = parameters.copyState();
    // Store calculated matching values as well as user-facing parameters.
    // Without this, reopening a Cubase session silently loses the matched EQ.
    for (int b = 0; b < bandCount; ++b)
        state.setProperty("matchBand" + juce::String(b), matchedGains[(size_t)b], nullptr);
    state.setProperty("matchThreshold", compressorThreshold.load(), nullptr);
    state.setProperty("matchRatio", compressorRatio.load(), nullptr);
    state.setProperty("referenceLabel", referenceName, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}
void StudioBolVocalManagerAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (!xml || !xml->hasTagName(parameters.state.getType())) return;
    auto state = juce::ValueTree::fromXml(*xml);
    for (int b = 0; b < bandCount; ++b)
        matchedGains[(size_t)b] = clampGain((float)state.getProperty("matchBand" + juce::String(b), 0.0f));
    compressorThreshold.store(juce::jlimit(-30.0f, -8.0f, (float)state.getProperty("matchThreshold", -18.0f)));
    compressorRatio.store(juce::jlimit(1.0f, 8.0f, (float)state.getProperty("matchRatio", 2.0f)));
    referenceName = state.getProperty("referenceLabel", "No reference loaded").toString();
    // The original audio is not embedded. Reimport it before calculating a new match.
    analysisStatus = "Saved match restored. Reimport reference to calculate a new match.";
    parameters.replaceState(state);
    pendingDspUpdate.store(true, std::memory_order_release);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new StudioBolVocalManagerAudioProcessor(); }
