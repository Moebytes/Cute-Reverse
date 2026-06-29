#include "Parameters.h"
#include "Functions.hpp"

template<typename T>
static auto castParameter(const AudioProcessorValueTreeState& tree, 
    const ParameterID* id, T*& dest) -> void {
    dest = dynamic_cast<T*>(tree.getParameter(id->getParamID()));
    jassert(dest != nullptr);
}

template <typename T>
static auto resetParameter(const AudioProcessorValueTreeState& tree, 
    const AudioParameterFloat* param, T*& dest) -> void {
    auto* paramObj = tree.getParameter(param->getParameterID());
    if (paramObj) *dest = paramObj->getDefaultValue();
}

ParameterIDs Parameters::paramIDs = ParameterIDs::loadFromJSON();

Parameters::Parameters(AudioProcessorValueTreeState& tree) : tree(tree) {
    using FloatPair = std::pair<AudioParameterFloat*&, const ParameterID*>;

    auto floatParameters = std::vector<FloatPair>{
        {mixParam, &paramIDs.mix}
    };

    for (const auto& [param, paramID] : floatParameters) {
        castParameter(tree, paramID, param);
    }
}

auto Parameters::createParameterLayout() -> AudioProcessorValueTreeState::ParameterLayout {
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<AudioParameterFloat>(
        paramIDs.mix, "Mix", NormalisableRange<float>{0.0f, 1.0f, 0.01f}, 1.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction(Functions::displayPercent)
        .withValueFromStringFunction(Functions::parsePercent)
    ));

    return layout;
}

auto Parameters::getDefaultParameter(const Array<var>& args,
    WebBrowserComponent::NativeFunctionCompletion completion) -> void {

    auto paramID = args[0].toString();
    auto* param = this->tree.getParameter(paramID);
    float defaultValue = param->convertFrom0to1(param->getDefaultValue());

    completion(defaultValue);
}

auto Parameters::prepareToPlay(double sampleRate, int blockSize) noexcept -> void {
    this->sampleRate = sampleRate;
    this->blockSize = blockSize;

    double duration = 0.001;

    auto smoothers = std::vector{
        &mixSmoother
    };

    for (const auto& smoother : smoothers) {
        smoother->reset(this->sampleRate, duration);
    }
}

auto Parameters::reset() noexcept -> void {
    auto paramFloats = std::vector{
        std::pair{mixParam, &mix}
    };

    for (auto& [param, value] : paramFloats) {
        resetParameter(tree, param, value);
    }
    
    auto smoothers = std::vector{
        std::pair{mixParam, &mixSmoother}
    };

    for (const auto& [param, smoother] : smoothers) {
        smoother->setCurrentAndTargetValue(param->get());
    }
}

auto Parameters::setHostInfo(double bpm, double ppq, const AudioPlayHead::TimeSignature& timeSignature) noexcept -> void {
    this->bpm = bpm;
    this->ppq = ppq;
    this->timeSignature = timeSignature;

    if (ppq > 0.0) {
        this->ppq = ppq;
        this->internalPPQ = ppq;
    } else {
        double ppqPerSample = (this->bpm / 60.0) / this->sampleRate;
        this->internalPPQ += ppqPerSample * this->blockSize; 
        this->ppq = this->internalPPQ;
    }
}

auto Parameters::blockUpdate() noexcept -> void {
    auto smoothers = std::vector{
        std::pair{mixParam, &mixSmoother}
    };

    for (const auto& [param, smoother] : smoothers) {
        smoother->setTargetValue(param->get());
    }
}

auto Parameters::update() noexcept -> void {
    this->mix = mixSmoother.getNextValue();
}