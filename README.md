# Experience Frame

A lightweight and efficient system window frame built using **Microsoft Edge WebView2** and C++ WinAPI. It is designed to render modern web interfaces as native Windows applications with full system integration.

## Key Features
* **WebView2 Integration**: High-performance rendering using the Chromium engine.
* **Unsafe SSL Support**: Optional `allowUnsafe` flag to bypass certificate errors for local development.
* **Window Title**: You can change the window title in the Main .cpp file.
* **Process Management**: Build script automatically terminates existing instances to ensure successful compilation.
* **Modular Design**: Native support for **Files**, **PiLink**, and **Login/Register** modules.

## Requirements
* **Compiler**: MinGW-w64 (GCC 15.2.0 or newer).
* **Runtime**: Microsoft Edge WebView2 Runtime.
* **Libraries**: `WebView2Loader.dll`, `ole32`, `uuid`, `nlohmann_json`.

## Configuration (`config.json`)
The application is controlled by a JSON file located in the root directory:

```json
{
    "url": "./index.html",
    "allowUnsafe": 1,
    "defaultStrength": 1
}
