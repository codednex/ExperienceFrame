@echo off
setlocal enabledelayedexpansion

:: 1. Ustawienia
set "PROJ_DIR=%~dp0"
cd /d "%PROJ_DIR%"
set "PATH=%PROJ_DIR%mingw64\bin;%PATH%"
set "OUT_EXE=ExperienceFrame.exe"
set "TEMP_EXE=ef_runtime_tmp.exe"

:: 2. Sprawdzenie kompilatora
if not exist "mingw64\bin\g++.exe" (
    echo [X] Nie znaleziono folderu mingw64! 
    echo Pobierz ZIP, rozpakuj go tutaj i upewnij sie, ze sciezka to: %PROJ_DIR%mingw64\bin\g++.exe
    pause
    exit
)

:: 3. Pobieranie WebView2 SDK (Male pliki - powinno przejsc)
if exist "WebView2.h" goto :check_json
echo [!] Pobieranie WebView2 SDK...
powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; (New-Object System.Net.WebClient).DownloadFile('https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2', 'v.zip')"
powershell -Command "Expand-Archive -Path 'v.zip' -DestinationPath 'vt' -Force"
copy "vt\build\native\include\*.h" ".\" >nul
copy "vt\build\native\x64\WebView2Loader.dll*" ".\" >nul
rd /s /q "vt"
rd /s /q "build" 2>nul
del v.zip

:check_json
:: 4. Pobieranie nlohmann/json
if exist "nlohmann\json.hpp" goto :start_build
echo [!] Pobieranie biblioteki JSON...
if not exist "nlohmann" mkdir nlohmann
powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; (New-Object System.Net.WebClient).DownloadFile('https://github.com/nlohmann/json/releases/latest/download/json.hpp', 'nlohmann\json.hpp')"

:start_build
:: 5. Kompilacja
if "%~1"=="--build-exe" (
    echo [*] Kompilacja do EXE...
    g++.exe main.cpp -o %OUT_EXE% -I. -L. -lWebView2Loader -lole32 -luuid -mwindows -std=c++17
    if !errorlevel! equ 0 (echo [OK] Gotowe: %OUT_EXE%) else (echo [X] Blad kompilacji!)
    pause
) else (
    echo [*] Tryb: On-the-fly...
    g++.exe main.cpp -o %OUT_EXE% -I. -L. -lWebView2Loader -lole32 -luuid -mwindows -std=c++17
    if !errorlevel! equ 0 (
        start /wait %TEMP_EXE%
        del %TEMP_EXE%
    ) else (
        echo [X] Blad kompilacji!
        pause
    )
)