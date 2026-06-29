#pragma once
#include <JuceHeader.h>
#include "ParameterIDs.hpp"

class Parameters {
public:
    Parameters(AudioProcessorValueTreeState& tree);
    ~Parameters() = default;

    static auto createParameterLayout() -> AudioProcessorValueTreeState::ParameterLayout;

    auto prepareToPlay(double sampleRate, int blockSize) noexcept -> void;
    auto reset() noexcept -> void;
    auto init() noexcept -> void;
    auto blockUpdate() noexcept -> void;
    auto update() noexcept -> void;
    auto setHostInfo(double bpm, double ppq, const AudioPlayHead::TimeSignature& timeSignature) noexcept -> void;

    auto getDefaultParameter(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

    static ParameterIDs paramIDs;


    float mix = 1.0f;
    AudioParameterFloat* mixParam;

private:
    AudioProcessorValueTreeState& tree;

    LinearSmoothedValue<float> mixSmoother;

    double sampleRate = 44100.0;
    int blockSize = 512;
    double bpm = 150.0;
    double ppq = 0.0;
    double internalPPQ = 0.0;
    AudioPlayHead::TimeSignature timeSignature{4, 4};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Parameters)
};