#pragma once
#pragma clang diagnostic ignored "-Wshadow-field"
#include <JuceHeader.h>
#include "Processor.h"
#include "EventEmitter.hpp"

class Editor : public AudioProcessorEditor, public EventEmitter::Listener {
public:
    Editor(Processor& p);
    ~Editor() override;
    
    auto resized() -> void override;

    auto getResource(const String& url) -> std::optional<WebBrowserComponent::Resource>;
    auto webviewOptions() -> WebBrowserComponent::Options;
    auto getWebviewFileBytes(const String& resourceStr) -> std::vector<std::byte>;

    auto handleEvent(const String& name, const var& payload) -> void override;
        
private:
    Processor& processor;
    ComponentBoundsConstrainer constrainer;

    WebSliderRelay mixRelay {Parameters::paramIDs.mix.getParamID()};
    WebSliderParameterAttachment mixAttachment {*this->processor.parameters.mixParam, mixRelay, nullptr};

    WebBrowserComponent webview;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};