@echo off
setlocal enabledelayedexpansion

:: Konfiguracja ścieżek
set "PROJ_DIR=%~dp0"
cd /d "%PROJ_DIR%"
set "PATH=%PROJ_DIR%mingw64\bin;%PATH%"
set "OUT_EXE=ExperienceFrame.exe"
set "TEMP_EXE=ef_runtime_tmp.exe"

:: Sprawdzenie trybu
if "%~1"=="--build-exe" goto :compile_final

:compile_tmp
echo [*] Kompilacja tymczasowa (On-the-fly)...
if exist "%TEMP_EXE%" del "%TEMP_EXE%"
g++.exe main.cpp -o %TEMP_EXE% -I. -L. -lWebView2Loader -lole32 -luuid -mwindows -std=c++17
if not exist "%TEMP_EXE%" goto :error
echo [OK] Uruchamianie...
start /wait %TEMP_EXE%
del %TEMP_EXE%
goto :end

:compile_final
echo [*] Kompilacja finalna do %OUT_EXE%...
if exist "%OUT_EXE%" del "%OUT_EXE%"
g++.exe main.cpp -o %OUT_EXE% -I. -L. -lWebView2Loader -lole32 -luuid -mwindows -std=c++17
if not exist "%OUT_EXE%" goto :error
echo [OK] Plik %OUT_EXE% gotowy.
goto :end

:error
echo.
echo [X] BLAD: Kompilacja nie powiodla sie.
echo Sprawdz czy g++ wyswietlil bledy powyzej.
pause
exit /b 1

:end
echo Gotowe.