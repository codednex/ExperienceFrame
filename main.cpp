#include <windows.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wrl.h>
#include <objbase.h>
#include <functional>
#include "WebView2.h"
#include "config.h"
// Ręczna definicja UUID dla MinGW
struct __declspec(uuid("e36784d1-80d3-4505-99bc-2451336c1b35")) ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;
struct __declspec(uuid("6348d660-ea79-455b-8016-1f31f98d7807")) ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
struct __declspec(uuid("60346061-4680-482d-8b43-b1d5c7553f60")) ICoreWebView2NavigationStartingEventHandler;

using namespace Microsoft::WRL;

ComPtr<ICoreWebView2Controller> webviewController;
ComPtr<ICoreWebView2> webviewWindow;
AppConfig g_Config;

// Klasa Handler dostosowana pod MinGW - bez variadic templates
template <typename T, typename F>
class Handler : public T {
    F m_callback;
    long m_refCount = 1;
public:
    Handler(F callback) : m_callback(callback) {}

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == __uuidof(T) || riid == __uuidof(IUnknown)) {
            *ppv = static_cast<T*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_refCount); }
    STDMETHODIMP_(ULONG) Release() override {
        long ref = InterlockedDecrement(&m_refCount);
        if (ref == 0) delete this;
        return ref;
    }

    // Przeciazenia dla konkretnych interfejsow WebView2
    STDMETHODIMP Invoke(HRESULT res, ICoreWebView2Environment* env) { return m_callback(res, env); }
    STDMETHODIMP Invoke(HRESULT res, ICoreWebView2Controller* ctrl) { return m_callback(res, ctrl); }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) { return m_callback(sender, args); }
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_SIZE:
            if (webviewController) {
                RECT b; GetClientRect(hWnd, &b);
                webviewController->put_Bounds(b);
            }
            break;
        case WM_KEYDOWN:
            if (g_Config.distributedMode == 0 && (GetKeyState(VK_CONTROL) & 0x8000) && 
                (GetKeyState(VK_SHIFT) & 0x8000) && (wParam == 'I')) {
                MessageBox(hWnd, _T("Dev Mode Active - ExperienceFrame"), _T("Info"), MB_OK);
            }
            break;
        case WM_DESTROY: PostQuitMessage(0); break;
        default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    g_Config = LoadSettings(); // Automatycznie wygeneruje config.json jesli go nie ma

    WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInst, 
                        LoadCursor(NULL, IDC_ARROW), NULL, NULL, NULL, _T("EF_CLASS"), NULL };
    RegisterClassEx(&wcex);

    HWND hWnd = CreateWindowEx(WS_EX_TOPMOST, _T("EF_CLASS"), _T("ExperienceFrame"), 
                               WS_POPUP | WS_VISIBLE, 0, 0, GetSystemMetrics(SM_CXSCREEN), 
                               GetSystemMetrics(SM_CYSCREEN), NULL, NULL, hInst, NULL);

    // Inicjalizacja srodowiska
    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        new Handler<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, std::function<HRESULT(HRESULT, ICoreWebView2Environment*)>>(
            [hWnd](HRESULT res, ICoreWebView2Environment* env) -> HRESULT {
                env->CreateCoreWebView2Controller(hWnd,
                    new Handler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, std::function<HRESULT(HRESULT, ICoreWebView2Controller*)>>(
                        [hWnd](HRESULT res, ICoreWebView2Controller* ctrl) -> HRESULT {
                            if (ctrl) {
                                webviewController = ctrl;
                                webviewController->get_CoreWebView2(&webviewWindow);
                                
                                // Filtracja URL
                                webviewWindow->add_NavigationStarting(
                                    new Handler<ICoreWebView2NavigationStartingEventHandler, std::function<HRESULT(ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs*)>>(
                                        [](ICoreWebView2* s, ICoreWebView2NavigationStartingEventArgs* e) -> HRESULT {
                                            LPWSTR uri; e->get_Uri(&uri);
                                            std::wstring ws(uri);
                                            std::string sUrl(ws.begin(), ws.end());
                                            if (!g_Config.bypassEnabled) {
                                                for (auto& p : g_Config.lockedURL) {
                                                    if (IsUrlBlocked(p, sUrl)) { e->put_Cancel(TRUE); break; }
                                                }
                                            }
                                            CoTaskMemFree(uri);
                                            return S_OK;
                                        }), NULL);

                                // Zaladowanie URL
                                std::string target = g_Config.url;
                                if (target.find("./") == 0) {
                                    char p[MAX_PATH]; GetFullPathNameA(target.c_str(), MAX_PATH, p, NULL);
                                    target = "file:///" + std::string(p);
                                }
                                std::wstring wUrl(target.begin(), target.end());
                                webviewWindow->Navigate(wUrl.c_str());
                            }
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