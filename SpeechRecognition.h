#ifndef SPEECHRECOGNITION_H
#define SPEECHRECOGNITION_H

#define NOMINMAX
#include <string>
#include <ixwebsocket/IXHttpClient.h>
#include <combaseapi.h>
#include <iostream>
#include <functional>
#include <cstdint>
#include <queue>
#include <mutex>
#include <thread>

#include "AudioCapture.h"
#include "sherpa-onnx/c-api/c-api.h"

enum LLMEngineType {
    StandardOpenAI = 0,
    LLaMA_CPP = 1
};

struct SamplingConfig {
    int maxTokens;
    float temperature;
    float topP;
};

struct RemoteLLMConfig {
    std::string apiToken;
    std::string apiBaseUrl;
    std::string modelName;
    bool isSakuraLLM;
    std::string promptTemplate;
    LLMEngineType engineType = StandardOpenAI;
    SamplingConfig samplingConfig;
};

class SpeechSubtitle {
public:
    SpeechSubtitle(const std::string &text, const std::string &lang)
        : text_(text), lang_(lang) {
    }

    std::string getText() const {
        return text_;
    }

    void setText(const std::string &text) {
        text_ = text;
    }

    std::string getLang() const {
        return lang_;
    }

    void setLang(const std::string &lang) {
        lang_ = lang;
    }

private:
    std::string text_; // 字幕文本
    std::string lang_; // 字幕语言
};

class SpeechRecognition {
public:
    // Constructor now takes the config file path
    SpeechRecognition(const std::string &configFilePath);

    ~SpeechRecognition();

    void init();

    void start();

    void stop();

    SpeechSubtitle getSubtitle();

    std::string getTranslate(const std::string &text);

private:
    void capture();

    void loadConfig();

    static bool initNetSystem();

    // Global config / status
    std::string configFilePath;
    bool running = false;
    std::mutex subtitlesMutex;

    // Audio capture pipeline
    AudioCapture audioCapture;
    std::thread captureThread;

    // ASR pipeline
    int modelSampleRate = 16000;
    float tailPaddings[4800] = {0.}; // 0.3 seconds at 16 kHz sample rate
    SherpaOnnxOfflineRecognizerConfig recognizer_config;
    const SherpaOnnxOfflineRecognizer *recognizer = nullptr;
    SherpaOnnxVadModelConfig vadConfig;
    const SherpaOnnxVoiceActivityDetector *vad = nullptr;
    std::function<void(short *, int32_t, int32_t)> asrCallback;

    // LLM based translate pipeline
    ix::HttpClient httpClient;
    RemoteLLMConfig remoteLLMConfig;

    // Render queue
    std::queue<SpeechSubtitle> subtitles;
};

#endif
