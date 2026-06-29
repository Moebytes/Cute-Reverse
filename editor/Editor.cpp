#include "Processor.h"
#include "Editor.h"
#include "Functions.hpp"
#include "Settings.hpp"
#include "BinaryData.h"

Editor::Editor(Processor& p) : AudioProcessorEditor(&p), processor(p),
    webview(webviewOptions()) {

    webview.goToURL(webview.getResourceProviderRoot());

    int width = static_cast<int>(Settings::getSettingKey("windowWidth", 400));
    int height = static_cast<int>(Settings::getSettingKey("windowHeight", 400));
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    int minWidth = 240;
    int minHeight = static_cast<int>(static_cast<float>(minWidth) / aspectRatio); 

    constrainer.setFixedAspectRatio(aspectRatio);
    constrainer.setMinimumSize(minWidth, minHeight);
    constrainer.setMaximumSize(10000, 10000);
    
    this->setConstrainer(&constrainer);
    this->setResizable(true, true);
    this->setSize(width, height);

    this->addAndMakeVisible(webview);

    EventEmitter::instance().addListener(this);
}

Editor::~Editor() {
    EventEmitter::instance().removeListener(this);
}

auto Editor::webviewOptions() -> WebBrowserComponent::Options {
    return WebBrowserComponent::Options{}
    .withBackend(WebBrowserComponent::Options::Backend::webview2)
    .withWinWebView2Options(WebBrowserComponent::Options::WinWebView2{}
    .withUserDataFolder(File::getSpecialLocation(File::tempDirectory)))
    .withResourceProvider([this](const auto& url) { return getResource(url); })
    .withNativeIntegrationEnabled()
    .withKeepPageLoadedWhenBrowserIsHidden()
    .withOptionsFrom(mixRelay)
    .withNativeFunction("getDefaultParameter", [this](auto args, auto completion){ 
        return this->processor.parameters.getDefaultParameter(args, completion); 
    })
    .withNativeFunction("startRecording", [this](auto args, auto completion){ 
        return this->processor.startRecording(args, completion);
    })
    .withNativeFunction("stopRecording", [this](auto args, auto completion){ 
        return this->processor.stopRecording(args, completion);
    })
    .withNativeFunction("getCaptureBars", [this](auto args, auto completion){ 
        return this->processor.getCaptureBars(args, completion);
    })
    .withNativeFunction("deleteCapture", [this](auto args, auto completion){ 
        return this->processor.deleteCapture(args, completion);
    });
}

auto Editor::resized() -> void {
    webview.setBounds(getLocalBounds());
    Settings::setSettingKey("windowWidth", getWidth());
    Settings::setSettingKey("windowHeight", getHeight());
}

auto Editor::getWebviewFileBytes(const String& resourceStr) -> std::vector<std::byte> {
    MemoryInputStream zipStream(BinaryData::webview_files_zip, BinaryData::webview_files_zipSize, false);
    ZipFile zip{zipStream};

    if (auto* entry = zip.getEntry(resourceStr)) {
        std::unique_ptr<InputStream> entryStream{zip.createStreamForEntry(*entry)};
        if (entryStream == nullptr) {
            jassertfalse;
            return {};
        }
        return Functions::streamToVector(*entryStream);
    }
    return {};
}

auto Editor::getResource(const String& url) -> std::optional<WebBrowserComponent::Resource> {
    static auto fileRoot = File::getCurrentWorkingDirectory().getChildFile("dist");
    auto resourceStr = url == "/" ? "index.html" : url.fromFirstOccurrenceOf("/", false, false);
    auto ext = resourceStr.fromLastOccurrenceOf(".", false, false);

    #if WEBVIEW_DEV_MODE
        auto stream = fileRoot.getChildFile(resourceStr).createInputStream();
        if (stream) {
            return WebBrowserComponent::Resource(Functions::streamToVector(*stream), Functions::getMimeForExtension(ext));
        }
    #else
        auto resource = Editor::getWebviewFileBytes(resourceStr);
        if (!resource.empty()) {
            return WebBrowserComponent::Resource(std::move(resource), Functions::getMimeForExtension(ext));
        }
    #endif
    return std::nullopt;
}


auto Editor::handleEvent(const String& name, const var& payload) -> void {
    if (name == "recordingState") {
        this->webview.emitEventIfBrowserIsVisible(Identifier{name}, payload);
    }

    if (name == "captureBars") {
        this->webview.emitEventIfBrowserIsVisible(Identifier{name}, payload);
    }
}