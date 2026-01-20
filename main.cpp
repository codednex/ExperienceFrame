#include <windows.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <wrl.h>
#include <objbase.h>
#include <functional>
#include "WebView2.h"
#include "config.h"

using namespace Microsoft::WRL;

// GUID-y dla zdarzeń
static const GUID _IID_EnvCompleted = { 0xe36784d1, 0x80d3, 0x4505, { 0x99, 0xbc, 0x24, 0x51, 0x33, 0x6c, 0x1b, 0x35 } };
static const GUID _IID_CtrlCompleted = { 0x6348d660, 0x0ea79, 0x455b, { 0x80, 0x16, 0x1f, 0x31, 0xf9, 0x8d, 0x78, 0x07 } };
static const GUID _IID_NavStarting = { 0x6972e391, 0x3f1a, 0x4763, { 0x99, 0x27, 0x51, 0x93, 0x02, 0xb8, 0x28, 0xa5 } };

ComPtr<ICoreWebView2Controller> webviewController;
ComPtr<ICoreWebView2> webviewWindow;
AppConfig g_Config;

// Pomocnicza funkcja do sprawdzania wildcards (*.google.com | google.com/*)
bool WildcardMatch(const std::string& pattern, const std::string& str) {
    if (pattern == "*") return true;
    size_t wildPos = pattern.find('*');
    if (wildPos == std::string::npos) return str == pattern;
    if (wildPos == 0) return str.size() >= pattern.size() - 1 && str.compare(str.size() - pattern.size() + 1, pattern.size() - 1, pattern.substr(1)) == 0;
    if (wildPos == pattern.size() - 1) return str.compare(0, pattern.size() - 1, pattern.substr(0, pattern.size() - 1)) == 0;
    return false;
}

bool IsUrlAllowed(std::wstring wUrl) {
    std::string url(wUrl.begin(), wUrl.end());
    
    // 1. Sprawdź priorytet ALLOW (!)
    for (const auto& pattern : g_Config.lockedURL) {
        if (pattern[0] == '!') {
            if (url.find(pattern.substr(1)) != std::string::npos) return true;
        }
    }
    
    // 2. Sprawdź blokady (*)
    for (const auto& pattern : g_Config.lockedURL) {
        if (pattern[0] != '!' && WildcardMatch(pattern, url)) return false;
    }
    
    return true;
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
            *ppv = static_cast<T*>(this); AddRef(); return S_OK;
        }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_refCount); }
    STDMETHODIMP_(ULONG) Release() override {
        long ref = InterlockedDecrement(&m_refCount);
        if (ref == 0) delete this; return ref;
    }
    STDMETHODIMP Invoke(HRESULT res, ICoreWebView2Environment* env) { return m_callback(res, env); }
    STDMETHODIMP Invoke(HRESULT res, ICoreWebView2Controller* ctrl) { return m_callback(res, ctrl); }
    STDMETHODIMP Invoke(ICoreWebView2* s, ICoreWebView2NavigationStartingEventArgs* e) { return m_callback(s, e); }
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_SIZE && webviewController) {
        RECT b; GetClientRect(hWnd, &b);
        webviewController->put_Bounds(b);
    } else if (message == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    g_Config = LoadSettings();
    if (g_Config.allowUnsafe == 1) SetEnvironmentVariableW(L"WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", L"--ignore-certificate-errors");

    WNDCLASSEXW wcex = {sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInst, LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, L"EF_CLASS", NULL};
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowExW(0, L"EF_CLASS", L"ExperienceFrame Default Webpage", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768, NULL, NULL, hInst, NULL);

    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        new Handler<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, std::function<HRESULT(HRESULT, ICoreWebView2Environment*)>>(
            _IID_EnvCompleted, [hWnd](HRESULT res, ICoreWebView2Environment* env) -> HRESULT {
                env->CreateCoreWebView2Controller(hWnd,
                    new Handler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, std::function<HRESULT(HRESULT, ICoreWebView2Controller*)>>(
                        _IID_CtrlCompleted, [hWnd](HRESULT res, ICoreWebView2Controller* ctrl) -> HRESULT {
                            webviewController = ctrl;
                            webviewController->get_CoreWebView2(&webviewWindow);

                            // REGUŁY BLOKOWANIA
                            webviewWindow->add_NavigationStarting(
                                new Handler<ICoreWebView2NavigationStartingEventHandler, std::function<HRESULT(ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs*)>>(
                                    _IID_NavStarting, [](ICoreWebView2* s, ICoreWebView2NavigationStartingEventArgs* e) -> HRESULT {
                                        LPWSTR uri;
                                        e->get_Uri(&uri);
                                        if (!IsUrlAllowed(uri)) e->put_Cancel(TRUE);
                                        CoTaskMemFree(uri);
                                        return S_OK;
                                    }), NULL);

                            RECT bounds; GetClientRect(hWnd, &bounds);
                            webviewController->put_Bounds(bounds);
                            webviewController->put_IsVisible(TRUE);
                            webviewWindow->Navigate(GetFormattedUrl(g_Config.url).c_str());
                            return S_OK;
                        }));
                return S_OK;
            }));

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}