#include <windows.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wrl.h>
#include <objbase.h>
#include <functional>
#include <iostream>
#include <fstream>
#include "WebView2.h"
#include "config.h"

using namespace Microsoft::WRL;

// GUID-y dla MinGW
static const GUID IID_EnvCompleted = { 0xe36784d1, 0x80d3, 0x4505, { 0x99, 0xbc, 0x24, 0x51, 0x33, 0x6c, 0x1b, 0x35 } };
static const GUID IID_CtrlCompleted = { 0x6348d660, 0xea79, 0x455b, { 0x80, 0x16, 0x1f, 0x31, 0xf9, 0x8d, 0x78, 0x07 } };
static const GUID IID_NavStarting = { 0x60346061, 0x4680, 0x482d, { 0x8b, 0x43, 0xb1, 0xd5, 0xc7, 0x55, 0x3f, 0x60 } };
static const GUID IID_TitleChanged = { 0x340a00ce, 0x4c3d, 0x44e6, { 0xba, 0x2d, 0x2f, 0x1a, 0x19, 0xd0, 0x92, 0x2f } };

ComPtr<ICoreWebView2Controller> webviewController;
ComPtr<ICoreWebView2> webviewWindow;
AppConfig g_Config;
std::wstring g_FallbackTitle = L"ExperienceFrame";

// Funkcja czytająca pierwszą linię z app.report
std::wstring GetBaseTitle() {
    std::ifstream file("app.report");
    std::string line;
    if (file.is_open() && std::getline(file, line)) {
        int len = MultiByteToWideChar(CP_UTF8, 0, line.c_str(), -1, NULL, 0);
        std::wstring wstr(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, line.c_str(), -1, &wstr[0], len);
        if (!wstr.empty() && wstr.back() == 0) wstr.pop_back(); // Remove null terminator
        return wstr;
    }
    return L"ExperienceFrame";
}

template <typename T, typename F>
class Handler : public T {
    F m_callback;
    long m_refCount = 1;
    GUID m_iid;
public:
    Handler(GUID iid, F callback) : m_iid(iid), m_callback(callback) {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (IsEqualIID(riid, m_iid) || IsEqualIID(riid, IID_IUnknown)) {
            *ppv = static_cast<T*>(this);
            AddRef(); return S_OK;
        }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_refCount); }
    STDMETHODIMP_(ULONG) Release() override {
        long ref = InterlockedDecrement(&m_refCount);
        if (ref == 0) delete this;
        return ref;
    }
    STDMETHODIMP Invoke(HRESULT res, ICoreWebView2Environment* env) { return m_callback(res, env); }
    STDMETHODIMP Invoke(HRESULT res, ICoreWebView2Controller* ctrl) { return m_callback(res, ctrl); }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) { return m_callback(sender, args); }
    STDMETHODIMP Invoke(ICoreWebView2* sender, IUnknown* args) { return m_callback(sender, args); }
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_SIZE:
            if (webviewController) {
                RECT b; GetClientRect(hWnd, &b);
                webviewController->put_Bounds(b);
            }
            break;
        case WM_DESTROY: PostQuitMessage(0); break;
        default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int main() {
    HINSTANCE hInst = GetModuleHandle(NULL);
    g_Config = LoadSettings();
    g_FallbackTitle = GetBaseTitle();
    std::wstring initialTitle = g_FallbackTitle + L" by codednex";

    std::cout << "[*] Start: " << std::string(initialTitle.begin(), initialTitle.end()) << std::endl;

    WNDCLASSEXW wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInst;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"EF_CLASS";
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowExW(0, L"EF_CLASS", initialTitle.c_str(), 
                               WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
                               NULL, NULL, hInst, NULL);

    wchar_t szPath[MAX_PATH];
    GetTempPathW(MAX_PATH, szPath);
    std::wstring userDataDir = szPath;
    userDataDir += L"ExperienceFrameData";

    std::cout << "[*] Inicjalizacja WebView2..." << std::endl;
    
    CreateCoreWebView2EnvironmentWithOptions(nullptr, userDataDir.c_str(), nullptr,
        new Handler<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, std::function<HRESULT(HRESULT, ICoreWebView2Environment*)>>(
            IID_EnvCompleted, [hWnd](HRESULT res, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(res)) { std::cerr << "[X] Blad Environment: " << std::hex << res << std::endl; return res; }
                
                env->CreateCoreWebView2Controller(hWnd,
                    new Handler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, std::function<HRESULT(HRESULT, ICoreWebView2Controller*)>>(
                        IID_CtrlCompleted, [hWnd](HRESULT res, ICoreWebView2Controller* ctrl) -> HRESULT {
                            if (FAILED(res) || !ctrl) return res;
                            std::cout << "[OK] Kontroler gotowy." << std::endl;

                            webviewController = ctrl;
                            webviewController->get_CoreWebView2(&webviewWindow);

                            // --- DYNAMICZNY TYTUŁ ---
                            webviewWindow->add_DocumentTitleChanged(
                                new Handler<ICoreWebView2DocumentTitleChangedEventHandler, std::function<HRESULT(ICoreWebView2*, IUnknown*)>>(
                                    IID_TitleChanged, [hWnd](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
                                        LPWSTR title;
                                        sender->get_DocumentTitle(&title);
                                        std::wstring newTitle = (title && wcslen(title) > 0) ? title : g_FallbackTitle;
                                        newTitle += L" by codednex";
                                        SetWindowTextW(hWnd, newTitle.c_str());
                                        CoTaskMemFree(title);
                                        return S_OK;
                                    }), NULL);

                            std::wstring wUrl = GetFormattedUrl(g_Config.url);
                            std::wcout << L"[*] Laduje: " << wUrl << std::endl;
                            webviewWindow->Navigate(wUrl.c_str());
                            return S_OK;
                        }));
                return S_OK;
            }));

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}