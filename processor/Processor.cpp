#include "Processor.h"
#include "Editor.h"
#include "Functions.hpp"

Processor::Processor() : AudioProcessor(
    BusesProperties()
        .withInput("Input", AudioChannelSet::stereo(), true)
        .withOutput("Output", AudioChannelSet::stereo(), true)
    ), parameters(tree), presetManager(*this) {
}

Processor::~Processor() {}

auto Processor::isBusesLayoutSupported(const BusesLayout& layouts) const -> bool {
    auto mono = AudioChannelSet::mono();
    auto stereo = AudioChannelSet::stereo();
    auto mainIn = layouts.getMainInputChannelSet();
    auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn == mono && mainOut == mono) return true;
    if (mainIn == mono && mainOut == stereo) return true;
    if (mainIn == stereo && mainOut == stereo) return true;
    return false;
}

auto Processor::getNumPrograms() -> int {
    return 0;
}

auto Processor::getCurrentProgram() -> int {
    return 0;
}

auto Processor::setCurrentProgram(int index) -> void {
}

auto Processor::getProgramName(int index) -> const String {
    return "";
}

auto Processor::changeProgramName([[maybe_unused]] int index, [[maybe_unused]] const String& newName) -> void {}

auto Processor::createEditor() -> AudioProcessorEditor* {
    return new Editor(*this);
}

auto Processor::getStateInformation(MemoryBlock& destData) -> void {
    auto jsonStr = this->presetManager.savePreset();
    destData.replaceAll(jsonStr.toUTF8(), jsonStr.getNumBytesAsUTF8());
}

auto Processor::setStateInformation(const void* data, int sizeInBytes) -> void {
    auto jsonStr = String::fromUTF8(static_cast<const char*>(data), sizeInBytes);
    this->presetManager.loadPreset(jsonStr);
}

auto JUCE_CALLTYPE createPluginFilter() -> AudioProcessor* {
    return new Processor();
}

auto Processor::prepareToPlay(double sampleRate, int samplesPerBlock) -> void {
    this->parameters.prepareToPlay(sampleRate, samplesPerBlock);
    this->parameters.reset();
}

auto Processor::releaseResources() -> void {}

auto Processor::getHostInfo() noexcept -> std::tuple<double, double, int64_t, TimeSignature, bool, bool, LoopPoints> {
    double bpm = 150.0;
    double ppq = 0.0;
    int64_t sampleTime = 0;
    TimeSignature timeSignature{4, 4};
    bool isPlaying = false;
    bool isLooping = false;
    LoopPoints loopPoints{0, 0};

    if (auto* playhead = this->getPlayHead()) {
        auto info = playhead->getPosition().orFallback(AudioPlayHead::PositionInfo{});
        bpm = info.getBpm().orFallback(150.0);
        ppq = info.getPpqPosition().orFallback(0.0);
        timeSignature = info.getTimeSignature().orFallback(TimeSignature{4, 4});
        isPlaying = info.getIsPlaying();
        isLooping = info.getIsLooping();
        sampleTime = info.getTimeInSamples().orFallback(0);
        loopPoints = info.getLoopPoints().orFallback(LoopPoints{0, 0});
    }

    return {bpm, ppq, sampleTime, timeSignature, isPlaying, isLooping, loopPoints};
}

auto Processor::startRecording([[maybe_unused]] const Array<var>& args,
    [[maybe_unused]] WebBrowserComponent::NativeFunctionCompletion completion) -> void {
    this->recording = true;

    auto [bpm, ppq, sampleTime, timeSignature, isPlaying, isLooping, loopPoints] = this->getHostInfo();
    if (isPlaying) this->pendingImmediateStart = true;
}

auto Processor::stopRecording([[maybe_unused]] const Array<var>& args,
    [[maybe_unused]] WebBrowserComponent::NativeFunctionCompletion completion) -> void {
    this->recording = false;
}

auto Processor::getCaptureBars([[maybe_unused]] const Array<var>& args,
    WebBrowserComponent::NativeFunctionCompletion completion) -> void {
    completion(this->captureBars);
}

auto Processor::deleteCapture([[maybe_unused]] const Array<var>& args,
    [[maybe_unused]] WebBrowserComponent::NativeFunctionCompletion completion) -> void {
    this->clearCapture();
}

auto Processor::clearCapture() -> void {
    this->recording = false;
    this->recordingStarted = false;
    this->writePosition = 0;

    this->captureBuffer.setSize(0, 0);
    this->captureLengthSamples = 0;
    this->captureStartSample = 0;
    this->captureBars = 0;
    this->hasRecording = false;

    EventEmitter::instance().emitEvent("recordingState", false);
    EventEmitter::instance().emitEvent("captureBars", 0);
}

auto Processor::processBlock(AudioBuffer<float>& buffer, [[maybe_unused]] MidiBuffer& midiMessages) -> void {
    ScopedNoDenormals noDenormals;

    auto mainInput = this->getBusBuffer(buffer, true, 0);
    auto mainOutput = this->getBusBuffer(buffer, false, 0);

    const float* inputL = mainInput.getReadPointer(0);
    const float* inputR = mainInput.getNumChannels() > 1 ? mainInput.getReadPointer(1) : inputL;

    float* outputL = mainOutput.getWritePointer(0);
    float* outputR = mainOutput.getNumChannels() > 1 ? mainOutput.getWritePointer(1) : outputL;

    auto [bpm, ppq, sampleTime, timeSignature, isPlaying, isLooping, loopPoints] = this->getHostInfo();
    this->parameters.setHostInfo(bpm, ppq, timeSignature);
    this->parameters.blockUpdate();

    bool playbackStopped = previousPlaying && !isPlaying;

    if (playbackStopped && this->recording && !this->recordingStarted) {
        this->recording = false;
        EventEmitter::instance().emitEvent("recordingState", false);
        this->writePosition = 0;
    }

    if (playbackStopped && this->recordingStarted) {
        this->clearCapture();
    }

    if (!this->recording && this->recordingStarted) {
        this->clearCapture();
    }

    bool insideLoop = false;
    bool crossedLoopStart = false;

    if (isLooping) {
        double loopStart = loopPoints.ppqStart;
        double loopEnd   = loopPoints.ppqEnd;

        insideLoop = (ppq >= loopStart && ppq < loopEnd);
        crossedLoopStart = (ppq >= loopStart && previousPPQ < loopStart);
    }

    bool shouldStart = (crossedLoopStart || this->pendingImmediateStart || 
        (!previousPlaying && isPlaying && insideLoop));

    if (this->recording && !this->recordingStarted && shouldStart) {
        double loopStart = loopPoints.ppqStart;
        double loopEnd = loopPoints.ppqEnd;

        auto seconds = (loopEnd - loopStart) * 60.0 / bpm;

        this->captureLengthSamples = static_cast<int>(std::round(seconds * getSampleRate()));

        this->captureBuffer.setSize(mainInput.getNumChannels(), this->captureLengthSamples);
        this->captureBuffer.clear();
        this->hasRecording = false;
        this->captureBars = 0;
        EventEmitter::instance().emitEvent("captureBars", 0);

        this->writePosition = 0;
        this->captureStartSample = sampleTime;
        this->recordingStarted = true;
        this->pendingImmediateStart = false;
    }

    for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
        this->parameters.update();

        float mix = this->parameters.mix;

        const float dryL = inputL[sample];
        const float dryR = inputR[sample];
        float wetL = dryL;
        float wetR = dryR;

        if (this->recording && this->recordingStarted) {
            this->captureBuffer.setSample(0, this->writePosition, dryL);

            if (this->captureBuffer.getNumChannels() > 1) {
                this->captureBuffer.setSample(1, this->writePosition, dryR);
            }

            ++this->writePosition;

            if (this->writePosition >= this->captureLengthSamples) {
                this->recording = false;
                EventEmitter::instance().emitEvent("recordingState", false);

                this->recordingStarted = false;
                this->writePosition = 0;
                this->captureStartSample += this->captureLengthSamples;
                this->hasRecording = true;

                double loopBeats = loopPoints.ppqEnd - loopPoints.ppqStart;
                double beatsPerBar = timeSignature.numerator;
                this->captureBars = static_cast<int>(std::round(loopBeats / beatsPerBar));
                EventEmitter::instance().emitEvent("captureBars", this->captureBars);
            }
        }

        if (this->hasRecording && isPlaying) {
            int64_t currentSample = sampleTime + sample;
            int64_t forwardPosition = currentSample - this->captureStartSample;

            forwardPosition %= this->captureLengthSamples;
            if (forwardPosition < 0) {
                forwardPosition += this->captureLengthSamples;
            }

            int reversePosition = this->captureLengthSamples - 1 - static_cast<int>(forwardPosition);

            float reverseL = this->captureBuffer.getSample(0, reversePosition);

            float reverseR = this->captureBuffer.getNumChannels() > 1
                ? this->captureBuffer.getSample(1, reversePosition)
                : reverseL;

            wetL = reverseL;
            wetR = reverseR;
        }

        outputL[sample] = (1.0f - mix) * dryL + mix * wetL;
        outputR[sample] = (1.0f - mix) * dryR + mix * wetR;
    }

    this->previousPlaying = isPlaying;
    this->previousSampleTime = sampleTime;
    this->previousPPQ = ppq;
 
    #if JUCE_DEBUG
        Functions::checkAudioSafety(buffer);
    #endif
}