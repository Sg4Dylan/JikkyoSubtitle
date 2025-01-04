#include "danmaku.h"

#ifndef WS_EX_NOREDIRECTIONBITMAP
    #define WS_EX_NOREDIRECTIONBITMAP 0x00200000L
#endif

#define WM_USER_NOTIFYICON (WM_USER + 100)
#define IDM_EXIT 1001
#define IDM_TOP 1002
#define IDM_EDGE 1003
#define IDM_RENDER_TYPE 1004
#define IDM_LANG_SETTING 1007


NOTIFYICONDATAW nid = {sizeof(NOTIFYICONDATAW)};
bool wtfInited = false;
bool onTop = true;
bool hideFrame = false;
bool exited = false;
bool rollingDanmaku = true;
bool forceTranslate = true;
WTF_Window *window = nullptr;
WTF_Instance *wtf = nullptr;
std::string current_channel;

void initDanmaku(uint32_t width, uint32_t height, const std::wstring &title) {
    // HiDPI 感知
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    window = WTFWindow_Create(GetModuleHandle(nullptr), SW_SHOWNORMAL);
    WTFWindow_SetCustomWndProc(window, WndProc);
    WTFWindow_Initialize(window, WS_EX_WINDOWEDGE | WS_EX_NOREDIRECTIONBITMAP, width, height, title.c_str());

    // 获取窗体 & DPI
    auto hwnd = static_cast<HWND>(WTFWindow_GetHwnd(window));
    auto dpiFactor = static_cast<float>(GetDpiForWindow(hwnd)) / 96.0f;
    // 设置窗体大小
    SetWindowPos(hwnd, HWND_TOPMOST,
                 static_cast<int>(dpiFactor * (width - width / 3.0) / 2.0), // 水平居中
                 static_cast<int>((height - height / 12.0 - 48) * dpiFactor), // 底部，预留任务栏高度
                 static_cast<int>(dpiFactor * width / 3.0), // 宽度为屏幕宽度的 1/3
                 static_cast<int>(dpiFactor * height / 12.0), // 高度约为屏幕高度的 1/12
                 SWP_FRAMECHANGED);

    // 修改主窗口图标
    auto hIcon = LoadIcon(GetModuleHandle(nullptr), "IDI_ICON1");
    SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(hIcon));

    // 添加托盘图标
    nid.hWnd = hwnd;
    nid.uID = 100;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.hIcon = hIcon;
    nid.uCallbackMessage = WM_USER_NOTIFYICON;
    wcscpy_s(nid.szTip, L"实况字幕");
    Shell_NotifyIconW(NIM_ADD, &nid);

    WTFWindow_SetHitTestOverEnabled(window, 0);
    WTFWindow_RunMessageLoop(window);
}

void destroyDanmaku() {
    WTFWindow_Release(window);
}

bool isWtfInited() {
    return wtfInited;
}

void addDanmaku(const std::wstring &text, int fontSize) {
    WTF_AddLiveDanmaku(wtf, rollingDanmaku ? 1 : 5, 0, text.c_str(), fontSize, 0xFFFFFFFF, 0, 0);
}

bool needTranslate() {
    return forceTranslate;
}

void initializeWTF(HWND hwnd) {
    if (!wtfInited) {
        wtf = WTF_CreateInstance();
        WTF_InitializeWithHwnd(wtf, hwnd);
        WTF_SetFontName(wtf, L"Microsoft Yahei UI Bold");
        WTF_SetFontWeight(wtf, 700);
        WTF_SetFontScaleFactor(wtf, 1.0f);
        WTF_SetCompositionOpacity(wtf, 0.5f);
        wtfInited = true;
    }
    WTF_Start(wtf);
}

void resizeWTF(uint32_t width, uint32_t height) {
    if (wtf) {
        WTF_Resize(wtf, width, height);
    }
}

void releaseWTF() {
    if (wtf) {
        if (WTF_IsRunning(wtf)) {
            WTF_Stop(wtf);
        }
        WTF_ReleaseInstance(wtf);
        wtf = nullptr;
        wtfInited = false;
        exited = true;
    }
}

void ShowContextMenu(HWND hWnd) {
    // 创建主菜单
    HMENU hMenu = CreatePopupMenu();

    // 创建子菜单
    HMENU hRenderMenu = CreatePopupMenu();
    HMENU hLangMenu = CreatePopupMenu();

    // 渲染设置
    AppendMenuW(hRenderMenu, MF_STRING, IDM_RENDER_TYPE + 1, rollingDanmaku ? L"滚动 ✓" : L"滚动");
    AppendMenuW(hRenderMenu, MF_STRING, IDM_RENDER_TYPE + 2, rollingDanmaku ? L"顶部" : L"顶部 ✓");

    // 语言设置
    AppendMenuW(hLangMenu, MF_STRING, IDM_LANG_SETTING + 1, forceTranslate ? L"翻译为中文 ✓" : L"翻译为中文");
    AppendMenuW(hLangMenu, MF_STRING, IDM_LANG_SETTING + 2, forceTranslate ? L"保持原文" : L"保持原文 ✓");

    // 添加子菜单到主菜单
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hRenderMenu), L"呈现方式");
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hLangMenu), L"展示语言");

    // 添加隐藏边框选项
    AppendMenuW(hMenu, MF_STRING, IDM_EDGE, hideFrame ? L"隐藏边框 ✓" : L"隐藏边框 ✕");

    // 添加置顶选项
    AppendMenuW(hMenu, MF_STRING, IDM_TOP, onTop ? L"窗口置顶 ✓" : L"窗口置顶 ✕");

    // 添加退出选项
    AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"退出");

    // 获取鼠标位置
    POINT pt;
    GetCursorPos(&pt);

    // 显示上下文菜单
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
    PostMessage(hWnd, WM_NULL, 0, 0);

    // 销毁菜单
    DestroyMenu(hMenu);
}

void onTopSwitch(HWND hWnd) {
    if (onTop) {
        SetWindowPos(
            hWnd, // 窗口句柄
            HWND_NOTOPMOST, // 置顶标志
            0, 0, 0, 0, // x, y, width, height（保持不变）
            SWP_NOMOVE | SWP_NOSIZE // 不移动和改变窗口大小
        );
        onTop = false;
    } else {
        SetWindowPos(
            hWnd, // 窗口句柄
            HWND_TOPMOST, // 置顶标志
            0, 0, 0, 0, // x, y, width, height（保持不变）
            SWP_NOMOVE | SWP_NOSIZE // 不移动和改变窗口大小
        );
        onTop = true;
    }
}

void onHideFrame(HWND hWnd) {
    if (hideFrame) {
        SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) | WS_CAPTION | WS_THICKFRAME);
        hideFrame = false;
    } else {
        // 去除外侧标题框
        SetWindowLong(
            hWnd, GWL_STYLE,
            GetWindowLong(hWnd, GWL_STYLE) & ~WS_CAPTION & ~WS_THICKFRAME);
        hideFrame = true;
    }
}

void HandleMenuSelection(HWND hWnd, int menuItemId) {
    // 使用 switch 语句处理不同的菜单项
    switch (menuItemId) {
        case IDM_RENDER_TYPE + 1:
            rollingDanmaku = true;
            break;

        case IDM_RENDER_TYPE + 2:
            rollingDanmaku = false;
            break;

        case IDM_LANG_SETTING + 1:
            forceTranslate = true;
            break;

        case IDM_LANG_SETTING + 2:
            forceTranslate = false;
            break;

        case IDM_EDGE:
            onHideFrame(hWnd);
            break;

        case IDM_TOP:
            onTopSwitch(hWnd);
            break;

        // 退出处理
        case IDM_EXIT:
            PostQuitMessage(0);
            destroyDanmaku();
            ExitProcess(0);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_USER_NOTIFYICON:
            if (lParam == WM_RBUTTONUP) {
                ShowContextMenu(hwnd);
            }
            break;
        case WM_SIZE:
            if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) {
                resizeWTF(LOWORD(lParam), HIWORD(lParam));
            }
            break;
        case WM_COMMAND:
            HandleMenuSelection(hwnd, LOWORD(wParam));
            break;
        case WM_CREATE:
            initializeWTF(hwnd);
            break;
        case WM_DESTROY:
            releaseWTF();
            destroyDanmaku();
            Shell_NotifyIconW(NIM_DELETE, &nid);
            ExitProcess(0);
    }
    return WTFWindow_DefaultWindowProc(window, (void *) hwnd, message, (WPARAM) wParam, (LPARAM) lParam);
}

std::wstring utf8_to_wstring(const std::string &utf8_str) {
    // バッファサイズの取得
    int iBufferSize = ::MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1,
                                            (wchar_t *) nullptr, 0);
    // バッファの取得
    wchar_t *wpBufWString = (wchar_t *) new wchar_t[iBufferSize];
    // UTF8 → wstring
    ::MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, wpBufWString,
                          iBufferSize);
    // wstringの生成
    std::wstring oRet(wpBufWString, wpBufWString + iBufferSize - 1);
    // バッファの破棄
    delete [] wpBufWString;
    // 変換結果を返す
    return oRet;
}

std::pair<int, int> getScreenResolution() {
    RECT desktop;
    HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    return std::make_pair(desktop.right, desktop.bottom);
}
