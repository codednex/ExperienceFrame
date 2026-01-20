# ExperienceFrame

Minimalistic web-view window for Windows (C++). No address bar, no tabs, just content.

## Configuration (config.json)
- `url`: The target website or local file (use `!` prefix to bypass filters).
- `distributedMode`: Set to `1` to disable dev shortcuts.
- `lockedURL`: List of patterns to block (supports `*.domain.com` and `domain.com/*`).

## Build
Compile using MinGW or GCC with WebView2 SDK:
`g++ main.cpp -o ExperienceFrame.exe -I./ -L./ -lWebView2Loader -mwindows`