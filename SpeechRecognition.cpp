#include <nlohmann/json.hpp>
#include "SpeechRecognition.h"

#include "AudioCapture.h"
#include "danmaku.h"

using json = nlohmann::json;


SpeechRecognition::SpeechRecognition(const std::string &configFilePath) : configFilePath(configFilePath) {
    init();
}

SpeechRecognition::~SpeechRecognition() {
    stop();
};

void SpeechRecognition::stop() {
    running = false;
    audioCapture.StopCapture();
    if (captureThread.joinable()) {
        captureThread.join();
    }
}

void SpeechRecognition::init() {
    // 加载配置文件
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Failed to open config file: " + configFilePath);
    }
    json config;
    configFile >> config;

    // 初始化加载 ASR 模型
    auto asr_model_path = config["asr"]["sense_voice"]["model_path"].get<std::string>();
    auto asr_model_lang = config["asr"]["sense_voice"]["language"].get<std::string>();
    SherpaOnnxOfflineSenseVoiceModelConfig sense_voice_config{
        asr_model_path.c_str(),
        asr_model_lang.c_str(),
        config["asr"]["sense_voice"]["num_threads"].get<int>()
    };
    // Offline model config
    SherpaOnnxOfflineModelConfig offline_model_config;
    memset(&offline_model_config, 0, sizeof(offline_model_config));
    offline_model_config.debug = 0;
    offline_model_config.num_threads = config["asr"]["num_threads"].get<int>();;
    auto asr_model_provider = config["asr"]["provider"].get<std::string>();
    offline_model_config.provider = asr_model_provider.c_str();
    auto asr_model_token_path = config["asr"]["token_path"].get<std::string>();
    offline_model_config.tokens = asr_model_token_path.c_str();
    offline_model_config.sense_voice = sense_voice_config;

    // Recognizer config
    SherpaOnnxOfflineRecognizerConfig recognizer_config;
    memset(&recognizer_config, 0, sizeof(recognizer_config));
    recognizer_config.decoding_method = "greedy_search";
    recognizer_config.model_config = offline_model_config;

    recognizer = SherpaOnnxCreateOfflineRecognizer(&recognizer_config);

    if (recognizer == nullptr) {
        throw std::runtime_error("Please check your recognizer config!\n");
    }

    // VAD Config
    SherpaOnnxVadModelConfig vadConfig;
    memset(&vadConfig, 0, sizeof(vadConfig));
    auto vad_model_path = config["vad"]["silero_vad"]["model"].get<std::string>();
    vadConfig.silero_vad.model = vad_model_path.c_str();
    vadConfig.interrupt_threshold = config["vad"]["interrupt_threshold"].get<float>();;
    vadConfig.silero_vad.threshold = config["vad"]["silero_vad"]["threshold"].get<float>();;
    vadConfig.silero_vad.min_silence_duration = config["vad"]["silero_vad"]["min_silence_duration"].get<float>();
    vadConfig.silero_vad.min_speech_duration = config["vad"]["silero_vad"]["min_speech_duration"].get<float>();;
    vadConfig.silero_vad.max_speech_duration = config["vad"]["silero_vad"]["max_speech_duration"].get<float>();;
    vadConfig.silero_vad.window_size = 512;
    vadConfig.sample_rate = modelSampleRate;
    vadConfig.num_threads =
    vadConfig.num_threads = config["vad"]["num_threads"].get<int>();;
    vadConfig.debug = 0;
    vadConfig.provider = "cpu";

    vad = SherpaOnnxCreateVoiceActivityDetector(&vadConfig, 30);

    if (vad == nullptr) {
        SherpaOnnxDestroyOfflineRecognizer(recognizer);
        throw std::runtime_error("Please check your vad config!\n");
    }
    std::cout << "VAD & ASR model loaded" << std::endl;

    // LLM Params
    promptTemplate = config["llm"]["prompt_template"].get<std::string>();
    modelName = config["llm"]["model_name"].get<std::string>();
    modelAuth = config["llm"]["auth_key"].get<std::string>();
    llmServer = config["llm"]["api_base"].get<std::string>();
    isLlamaCpp = config["llm"]["is_llama_cpp"].get<bool>();
    isSakuraLLM = config["llm"]["is_sakura_llm"].get<bool>();
    modelMaxTokens = config["llm"]["max_tokens"].get<int>();
    modelTemperature = config["llm"]["temperature"].get<float>();
    modelTopP = config["llm"]["top_p"].get<float>();

    // Init ASR Handler
    asrCallback = [this](short *input, int32_t n_samples,
                         int32_t sample_rate) -> void {
        // 转成单通道 float
        std::vector<float> samples;
        samples.resize(n_samples);
        for (int i = 0; i < n_samples; i++) {
            samples[i] = ((float) (input[2 * i] + input[2 * i + 1]) / 2.0f) / std::numeric_limits<short>::max();
        }

        // resample  to 16k hz;
        float min_freq = std::min(modelSampleRate, sample_rate);
        float lowpass_cutoff = 0.99 * 0.3 * min_freq;
        auto liner = SherpaOnnxCreateLinearResampler(
            sample_rate, modelSampleRate, lowpass_cutoff, 7);
        auto resample_out = SherpaOnnxLinearResamplerResample(liner, samples.data(),
                                                              samples.size(), true);

        SherpaOnnxVoiceActivityDetectorAcceptWaveform(vad, resample_out->samples, resample_out->n);

        while (!SherpaOnnxVoiceActivityDetectorEmpty(vad)) {
            const SherpaOnnxSpeechSegment *segment = SherpaOnnxVoiceActivityDetectorFront(vad);

            const SherpaOnnxOfflineStream *stream = SherpaOnnxCreateOfflineStream(recognizer);

            SherpaOnnxAcceptWaveformOffline(stream, modelSampleRate, tail_paddings, 4800);
            SherpaOnnxAcceptWaveformOffline(stream, modelSampleRate, segment->samples, segment->n);
            SherpaOnnxAcceptWaveformOffline(stream, modelSampleRate, tail_paddings, 4800);

            SherpaOnnxDecodeOfflineStream(recognizer, stream);

            auto *result = SherpaOnnxGetOfflineStreamResult(stream);
            if (auto text = std::string(result->text); text.size() > 1) {
                // Get the current time
                auto now = std::chrono::system_clock::now();
                // Convert the current time to a time_t object
                std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
                // Format the time using std::put_time and a stringstream
                std::tm localTime;
                // Use localtime_s for thread safety on Windows
                localtime_s(&localTime, &currentTime);

                std::cout << std::put_time(&localTime, "[%Y-%m-%d %H:%M:%S] ") << text << std::endl;
                std::lock_guard lock(subtitlesMutex);
                subtitles.emplace(text, result->lang);
            }


            SherpaOnnxDestroyOfflineRecognizerResult(result);
            SherpaOnnxDestroyOfflineStream(stream);
            SherpaOnnxDestroySpeechSegment(segment);
            SherpaOnnxVoiceActivityDetectorPop(vad);
            SherpaOnnxVoiceActivityDetectorFlush(vad);
        }

        SherpaOnnxDestroyLinearResampler(liner);
        if (resample_out) SherpaOnnxLinearResamplerResampleFree(resample_out);
    };

    // Init WinSocks
    initNetSystem();
}

std::string SpeechRecognition::getTranslate(const std::string &text) {
    try {
        std::stringstream ss;
        std::string nonSpaceText = text;
        nonSpaceText.erase(std::ranges::remove_if(nonSpaceText,
                                                  [](unsigned char c) { return std::isspace(c); }).begin(),
                           nonSpaceText.end());
        ss << llmServer << "/v1/completions";
        const std::string url = ss.str();
        ix::HttpRequestArgsPtr args = httpClient.createRequest();
        ix::WebSocketHttpHeaders headers;
        headers["Authorization"] = "Bearer " + modelAuth;
        headers["content-type"] = "application/json";
        args->extraHeaders = headers;
        json payload = {
            {"model", modelName},
            {"max_tokens", modelMaxTokens},
            {"temperature", modelTemperature},
            {"top_p", modelTopP}
        };

        // Use promptTemplate and format it
        std::string prompt = promptTemplate;
        size_t pos = prompt.find("%TEXT%");
        if (pos != std::string::npos) {
            prompt.replace(pos, 6, nonSpaceText);
        }
        if (isSakuraLLM) {
            payload["stop"] = {"<|im_end|>", "<|im_start|>"};
        }
        payload["prompt"] = prompt;

        ix::HttpResponsePtr out = httpClient.post(url, payload.dump(), args);
        if (out->errorCode == ix::HttpErrorCode::Ok) {
            json llm_result = json::parse(out->body);
            if (isLlamaCpp) {
                return llm_result["content"];
            }
            return llm_result["choices"][0]["text"];
        }
        std::cerr << "HTTP Error: " << static_cast<int>(out->errorCode) << ", Msg: " << out->errorMsg << std::endl;
        return "";
    } catch (const std::exception &e) {
        std::cerr << "Error in getTranslate: " << e.what() << std::endl;
        return "";
    }
}

SpeechSubtitle SpeechRecognition::getSubtitle() {
    std::lock_guard<std::mutex> lock(subtitlesMutex);
    if (subtitles.empty()) {
        return {"", ""};
    }
    const auto subtitle = subtitles.front();
    subtitles.pop();
    return subtitle;
}


void SpeechRecognition::start() {
    running = true;
    captureThread = std::thread(&SpeechRecognition::capture, this);
}

void SpeechRecognition::capture() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "CoInitializeEx failed: " << hr << std::endl;
        return;
    }

    try {
        audioCapture.SetIsLowLatency(false);
        hr = audioCapture.Initialize();
        if (FAILED(hr)) {
            std::cerr << "AudioCapture::Initialize failed: " << hr << std::endl;
            CoUninitialize();
            return;
        }

        std::cout << "Recording started. Press Enter to stop." << std::endl;

        hr = audioCapture.StartCapture(asrCallback);

        if (FAILED(hr)) {
            std::cerr << "AudioCapture::StartCapture failed: " << hr << std::endl;
            CoUninitialize();
            return;
        }

        std::cin.get();

        hr = audioCapture.StopCapture();
        if (FAILED(hr)) {
            std::cerr << "AudioCapture::StopCapture failed: " << hr << std::endl;
            CoUninitialize();
            return;
        }

        std::cout << "Recording stopped and saved to output.wav" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    CoUninitialize();
}

bool SpeechRecognition::initNetSystem() {
#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    // Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);

    return err == 0;
#else
    return true;
#endif
}
