#pragma once
#include <JuceHeader.h>

class Processor;

class PresetManager {
public:
    PresetManager(Processor& processor);
    ~PresetManager() = default;

    auto savePreset(const String& name = "", const String& author = "") -> String;
    auto loadPreset(const String& jsonStr) -> String;
    
private:
    Processor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};