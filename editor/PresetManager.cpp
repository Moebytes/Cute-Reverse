#include "PresetManager.h"
#include "ParameterIDs.hpp"
#include "Processor.h"

PresetManager::PresetManager(Processor& processor) : processor(processor) {}

auto PresetManager::savePreset(const String& name, const String& author) -> String {
    auto obj = std::make_unique<DynamicObject>();

    obj->setProperty("plugin", JucePlugin_Name);
    obj->setProperty("version", JucePlugin_VersionString);
    obj->setProperty("name", name);
    obj->setProperty("author", author);
    obj->setProperty("modified", Time::getCurrentTime().toISO8601(true));
    obj->setProperty("presetFormat", 1);

    auto parameters = std::make_unique<DynamicObject>();

    for (const auto& id : ParameterIDs::getStringKeys()) {
        auto* param = this->processor.tree.getParameter(id);

        if (param) {
            parameters->setProperty(id, param->getCurrentValueAsText());
        }
    }

    obj->setProperty("parameters", var(parameters.release()));

    if (this->processor.hasRecording) {
        auto capture = std::make_unique<DynamicObject>();

        capture->setProperty("channels", processor.captureBuffer.getNumChannels());
        capture->setProperty("length", processor.captureLengthSamples);
        capture->setProperty("startSample", processor.captureStartSample);
        capture->setProperty("captureBars", processor.captureBars);

        MemoryOutputStream binary;

        for (int i = 0; i < processor.captureBuffer.getNumChannels(); i++) {
            const auto* data = processor.captureBuffer.getReadPointer(i);
            size_t dataSize = sizeof(float) * static_cast<size_t>(processor.captureLengthSamples);

            binary.write(data, dataSize);
        }

        MemoryOutputStream base64;
        Base64::convertToBase64(base64, binary.getData(), binary.getDataSize());

        capture->setProperty("samples", base64.toString());
        obj->setProperty("capture", var(capture.release()));
    }

    auto json = var{obj.release()};
    return JSON::toString(json);
}

auto PresetManager::loadPreset(const String& jsonStr) -> String {
    auto parsed = JSON::fromString(jsonStr);
    auto* obj = parsed.getDynamicObject();
    if (obj == nullptr) return "";

    String presetName = "";

    if (obj->hasProperty("name")) {
        presetName = obj->getProperty("name").toString();
    }

    auto parameters = var{obj->getProperty("parameters")};
    auto* paramObj = parameters.getDynamicObject();
    if (paramObj == nullptr) return "";

    for (const auto& property : paramObj->getProperties()) {
        auto id = property.name.toString();
        auto* param = this->processor.tree.getParameter(id);

        float value = param->getValueForText(property.value.toString());
        param->setValueNotifyingHost(value);
    }

    if (obj->hasProperty("capture")) {
        auto* capture = obj->getProperty("capture").getDynamicObject();
        if (capture == nullptr) return "";

        int channels = static_cast<int>(capture->getProperty("channels"));
        int length = static_cast<int>(capture->getProperty("length"));
        int64_t startSample = static_cast<int64_t>(capture->getProperty("startSample"));
        int bars = static_cast<int>(capture->getProperty("captureBars"));

        processor.captureBuffer.setSize(channels, length);

        MemoryOutputStream binary;
        Base64::convertFromBase64(binary, capture->getProperty("samples").toString());

        const size_t channelSize = sizeof(float) * static_cast<size_t>(length);
        jassert(binary.getDataSize() == channelSize * static_cast<size_t>(channels));

        auto* sampleData = static_cast<const char*>(binary.getData());

        for (int i = 0; i < channels; i++) {
            std::memcpy(processor.captureBuffer.getWritePointer(i),
                        sampleData + static_cast<size_t>(i) * channelSize,
                        channelSize);
        }

        processor.captureLengthSamples = length;
        processor.captureStartSample = startSample;
        processor.captureBars = bars;
        processor.hasRecording = true;
    }

    return presetName;
}