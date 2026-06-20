@echo off
REM Build script for GameServerManager (Qt 6 / MinGW, Windows).
REM Runs the full toolchain: CMake configure -> build -> Qt DLL deployment.
REM Run from a native cmd.exe (the Qt MinGW g++ fails when invoked from WSL).
REM Usage: build.bat   (from the repo root, or via cmd.exe /c path\to\build.bat)

setlocal
set QT_ROOT=C:\Qt\6.7.3\mingw_64
set MINGW=C:\Qt\Tools\mingw1310_64\bin
set CMAKE_BIN=C:\Qt\Tools\CMake_64\bin
set PATH=%MINGW%;%CMAKE_BIN%;%QT_ROOT%\bin;%PATH%

cd /d "%~dp0"

cmake.exe -B build -G "MinGW Makefiles" ^
  -DCMAKE_MAKE_PROGRAM=%MINGW%/mingw32-make.exe ^
  -DCMAKE_C_COMPILER=%MINGW%/gcc.exe ^
  -DCMAKE_CXX_COMPILER=%MINGW%/g++.exe ^
  -DCMAKE_PREFIX_PATH=%QT_ROOT%
if errorlevel 1 exit /b 1

cmake.exe --build build -j 4
if errorlevel 1 exit /b 1

"%QT_ROOT%\bin\windeployqt.exe" build\app-qt.exe --no-translations
if errorlevel 1 exit /b 1

echo.
echo Build complete: build\app-qt.exe
endlocal
