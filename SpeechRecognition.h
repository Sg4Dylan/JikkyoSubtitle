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
#include "c-api.h"

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

    // Loads configuration from the JSON file
    void loadConfig();

    static bool initNetSystem();

    ix::HttpClient httpClient;
    AudioCapture audioCapture;

    std::string configFilePath; // Path to the configuration file

    // Configuration members loaded from the JSON file
    SherpaOnnxVadModelConfig vadConfig;
    std::string promptTemplate;
    std::string modelName;
    std::string modelAuth;
    int modelMaxTokens = 512;
    float modelTemperature = 0.1;
    float modelTopP = 0.3;
    std::string llmServer;
    bool isLlamaCpp = false;
    bool isSakuraLLM = false;

    int modelSampleRate = 16000;
    float tail_paddings[4800] = {0.}; // 0.3 seconds at 16 kHz sample rate
    const SherpaOnnxOfflineRecognizer *recognizer = nullptr;
    SherpaOnnxOfflineRecognizerConfig recognizer_config;
    SherpaOnnxOfflineModelConfig offline_model_config;
    SherpaOnnxVoiceActivityDetector *vad = nullptr;
    std::function<void(short *, int32_t, int32_t)> asrCallback;

    bool running = false;
    std::queue<SpeechSubtitle> subtitles;
    std::mutex subtitlesMutex;
    std::thread captureThread;
};

#endif
