#pragma once
#include <JuceHeader.h>
#include "Parameters.h"
#include "PresetManager.h"

using TimeSignature = AudioPlayHead::TimeSignature;
using LoopPoints = AudioPlayHead::LoopPoints;

class Processor : public AudioProcessor {
public:
  Processor();
  ~Processor() override;

  auto prepareToPlay(double sampleRate, int samplesPerBlock) -> void override;
  auto releaseResources() -> void override;
  auto processBlock(AudioBuffer<float>&, MidiBuffer&) -> void override;
  auto getHostInfo() noexcept -> std::tuple<double, double, int64, TimeSignature, bool, bool, LoopPoints>;

  auto isBusesLayoutSupported (const BusesLayout& layouts) const -> bool override;
  auto createEditor() -> AudioProcessorEditor* override;

  inline auto hasEditor() const -> bool override { return true; }
  inline auto getName() const -> const String override { return JucePlugin_Name; }
  inline auto acceptsMidi() const -> bool override { return false; }
  inline auto producesMidi() const -> bool override { return false; }
  inline auto isMidiEffect() const -> bool override { return false; }
  inline auto getTailLengthSeconds() const -> double override { return 0.0; }

  auto getNumPrograms() -> int override;
  auto getCurrentProgram() -> int override;
  auto setCurrentProgram(int index) -> void override;
  auto getProgramName(int index) -> const String override;
  auto changeProgramName(int index, const String& newName) -> void override;

  auto getStateInformation(MemoryBlock& destData) -> void override;
  auto setStateInformation(const void* data, int sizeInBytes) -> void override;

  auto startRecording(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

  auto stopRecording(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

  auto getCaptureBars(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

  auto deleteCapture(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

  auto clearCapture() -> void;

  AudioProcessorValueTreeState tree {
    *this, nullptr, "Parameters", Parameters::createParameterLayout()
  };

  Parameters parameters;
  PresetManager presetManager;

  AudioBuffer<float> captureBuffer;
  bool hasRecording = false;
  int captureLengthSamples = 0;
  int64 captureStartSample = 0;
  int captureBars = 0;
  
private:
  bool recording = false;
  bool recordingStarted = false;
  bool pendingImmediateStart = false;
  int writePosition = 0;

  int64 previousSampleTime = 0;
  double previousPPQ = 0;
  bool previousPlaying = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
};