#include <thread>


#include "danmaku.h"
#include "SpeechRecognition.h"

std::thread danmakuThread;

void syncDanmaku(SpeechRecognition &asr) {
    while (!isWtfInited()) {
        std::cout << "Waiting for danmaku render to start...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    std::cout << "Danmaku render initialized.\n";
    while (true) {
        const auto subtitle = asr.getSubtitle();
        if (auto text = subtitle.getText(); !text.empty()) {
            if (isWtfInited()) {
                if (subtitle.getLang() == "<|zh|>") {
                    addDanmaku(utf8_to_wstring(text)); // 中文使用默认字体大小 30
                } else {
                    if (auto translation = asr.getTranslate(text); !translation.empty() && text != translation && needTranslate()) {
                        addDanmaku(utf8_to_wstring(translation)); // 中文翻译使用默认字体大小 30
                    } else {
                        addDanmaku(utf8_to_wstring(text), 20); // 没有翻译时，原文也使用小号字体 20
                    }
                }
            }
        }
        // Add a small delay to prevent busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    // 切换代码页
    SetConsoleOutputCP(65001);

    // 初始化 ASR 线程
    SpeechRecognition asr("./config.json");
    asr.init();
    asr.start();

    // 初始化同步线程
    danmakuThread = std::thread(&syncDanmaku, std::ref(asr));

    // 初始化弹幕渲染线程
    auto screen_size = getScreenResolution();
    initDanmaku(screen_size.first, screen_size.second, L"RealtimeBilingualSubtitle");

    // 结束线程
    if (danmakuThread.joinable()) {
        danmakuThread.join();
    }
    asr.stop();
    destroyDanmaku();
    return 0;
}
