#pragma once
#include <JuceHeader.h>

#if JUCE_WINDOWS
    #include <windows.h>
    #include <shlobj.h>
    #pragma comment(lib, "Shell32.lib")
#endif

class Functions {
public:
    static auto streamToVector(InputStream& stream) -> std::vector<std::byte> {
        std::vector<std::byte> result(static_cast<size_t>(stream.getTotalLength()));
        stream.setPosition(0);
        [[maybe_unused]] auto bytesRead = stream.read (result.data(), result.size());
        jassert (bytesRead == static_cast<ssize_t>(result.size()));
        return result;
    }

    static auto getMimeForExtension(const String& extension) -> const char* {
        static std::unordered_map<String, const char*> mimeMap = {
            {"html", "text/html"       },
            {"css",  "text/css"        },
            {"js",   "text/javascript" },
            {"txt",  "text/plain"      },
            {"jpg",  "image/jpeg"      },
            {"png",  "image/png"       },
            {"jpeg", "image/jpeg"      },
            {"svg",  "image/svg+xml"   },
            {"json", "application/json"},
            {"map",  "application/json"},
            {"ttf",  "font/ttf"        },
            {"otf",  "font/otf"        },
            {"woff2","font/woff2"      }
        };
    
        auto it = mimeMap.find(extension.toLowerCase());
        if (it != mimeMap.end()) {
            return it->second;
        }
    
        jassertfalse;
        return "";
    }

    static auto checkAudioSafety(AudioBuffer<float>& buffer) -> void {
        for (int channel = 0; channel < buffer.getNumChannels(); channel++) {
            float* channelData = buffer.getWritePointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
                float value = channelData[sample];
                if (std::isnan(value)) {
                    Logger::outputDebugString("NaN detected");
                    return buffer.clear();
                } else if (std::isinf(value)) {
                    Logger::outputDebugString("Inf detected");
                    return buffer.clear();
                } else if (value < -2.0f || value > 2.0f) {
                    Logger::outputDebugString("Sample out of range");
                    return buffer.clear();
                }
            }
        }
    }

    static auto displayPercent(float value, int) -> String {
        return String::formatted("%.0f%%", value * 100.0f);
    }

    static auto parsePercent(const String& text) -> float {
        auto clean = text.trim();
        if (clean.endsWithChar('%')) {
            clean = clean.dropLastCharacters(1).trim();
        }
        return clean.getFloatValue() / 100.0f;
    }
};