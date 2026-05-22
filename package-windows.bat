@echo off
set PATH=C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\CMake_64\bin;%PATH%

echo === Building ===
cd /d %~dp0build
mingw32-make -j8
if errorlevel 1 goto :error

echo === Packaging ===
set PKG=%~dp0package\LMAO
rmdir /s /q "%PKG%" 2>nul
mkdir "%PKG%"

:: Copy exe
copy lmao.exe "%PKG%\"

:: Copy mpv dll
copy libmpv-2.dll "%PKG%\"

:: Copy ffmpeg tools if present
if exist ffprobe.exe copy ffprobe.exe "%PKG%\"
if exist ffmpeg.exe copy ffmpeg.exe "%PKG%\"

:: Deploy Qt DLLs
C:\Qt\6.11.1\mingw_64\bin\windeployqt.exe "%PKG%\lmao.exe"

:: Remove unnecessary Qt stuff
rmdir /s /q "%PKG%\translations" 2>nul
rmdir /s /q "%PKG%\tls" 2>nul
rmdir /s /q "%PKG%\networkinformation" 2>nul
rmdir /s /q "%PKG%\generic" 2>nul
del "%PKG%\opengl32sw.dll" 2>nul
del "%PKG%\D3Dcompiler_47.dll" 2>nul

echo === Done ===
echo Package ready in: %PKG%
echo.
dir "%PKG%" /s /-c | find "File(s)"
pause
exit /b 0

:error
echo Build failed!
pause
exit /b 1