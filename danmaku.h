#ifndef DANMAKU_H
#define DANMAKU_H
#include <string>
#include <Windows.h>

#include "WTFDanmaku.h"

extern std::string current_channel;

void initDanmaku(uint32_t width, uint32_t height, const std::wstring &title);

void destroyDanmaku();

bool isDanmakuInited();

bool isWtfInited();

void addDanmaku(const std::wstring &text, int fontSize = 30);

bool needTranslate();

void initializeWTF(HWND hwnd);

void resizeWTF(uint32_t width, uint32_t height);

void releaseWTF();

void ShowContextMenu(HWND hWnd);

void onTopSwitch(HWND hWnd);

void HandleMenuSelection(HWND hWnd, int menuItemId);

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

std::wstring utf8_to_wstring(const std::string &utf8_str);

std::pair<int, int> getScreenResolution();

#endif //DANMAKU_H
