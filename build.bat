@echo off
setlocal
echo [*] Zamykanie starych procesow...
taskkill /f /im ExperienceFrame.exe >nul 2>&1

set "PATH=%~dp0mingw64\bin;%PATH%"

echo [*] Kompilacja...
g++.exe main.cpp -o ExperienceFrame.exe -I. -L. -lWebView2Loader -lole32 -luuid -lgdi32 -luser32 -mwindows -std=c++17

if exist ExperienceFrame.exe (
    echo [OK] Uruchamianie...
    start ExperienceFrame.exe
) else (
    echo [X] Blad kompilacji!
    pause
)