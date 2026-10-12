@echo off
rem Builds dev\patch_src\src into dev\patch_src\build\DDraw.dll (32-bit). Needs the Visual Studio 2019 Build Tools.
rem   src\core      startup, patch helpers, log, settings, diagnostics
rem   src\patches   the patches of buzz.exe, by topic: system input graphics hud gameplay menus
rem   src\dx6_wrapper  DirectX 6 wrapper: the DirectX 6 objects the game talks to
rem   src\render    the real work: vertex math, Direct3D 11, the window and the shader
setlocal
rem Another Visual Studio: set VCVARS to its vcvars32.bat before running this
if not defined VCVARS set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
if not exist "%VCVARS%" goto novs
call "%VCVARS%" >nul 2>&1
set SRC=%~dp0src
set OUT=%~dp0build
if not exist "%OUT%" mkdir "%OUT%"
set P=%SRC%\patches
set INC=/I"%SRC%\core" /I"%SRC%\dx6_wrapper" /I"%SRC%\render" /I"%P%\input" /I"%P%\menus"
rem "build.bat log": a DDraw.dll that writes buzzpatch.log (for testing; the one handed out writes nothing)
set DEF=
if /i "%~1"=="log" set DEF=/DBUZZ_LOG
rem The shader (src\render\shader.hlsl) goes into the DLL as a resource
rem Build number in the version resource (core\version.h): days since 2026-01-01 and minute of the day
for /f %%d in ('powershell -NoProfile -Command "[int][math]::Floor(((Get-Date) - (Get-Date -Year 2026 -Month 1 -Day 1 -Hour 0 -Minute 0 -Second 0)).TotalDays)"') do set BUILD_DAY=%%d
for /f %%m in ('powershell -NoProfile -Command "[int][math]::Floor((Get-Date).TimeOfDay.TotalMinutes)"') do set BUILD_MINUTE=%%m
rc /nologo /DPATCH_BUILD_DAY=%BUILD_DAY% /DPATCH_BUILD_MINUTE=%BUILD_MINUTE% /I"%SRC%\render" /I"%SRC%\core" /fo"%OUT%\buzzpatch.res" "%SRC%\buzzpatch.rc" || exit /b 1
cl /nologo /LD /O2 /W3 /EHsc /std:c++17 /MT /D_CRT_SECURE_NO_WARNINGS %DEF% %INC% /Fo"%OUT%\\" ^
   "%SRC%\core\*.c" "%SRC%\core\*.cpp" "%P%\system\*.c" "%P%\system\*.cpp" "%P%\input\*.cpp" "%P%\graphics\*.cpp" "%P%\hud\*.cpp" ^
   "%P%\gameplay\*.cpp" "%P%\menus\*.c*" "%SRC%\dx6_wrapper\*.cpp" "%SRC%\render\*.cpp" ^
   /Fe"%OUT%\DDraw.dll" "%OUT%\buzzpatch.res" /link /DEF:"%SRC%\ddraw.def" ^
   user32.lib gdi32.lib kernel32.lib d3d11.lib dxgi.lib d3dcompiler.lib dxguid.lib
exit /b %errorlevel%
:novs
echo Visual Studio 2019 Build Tools not found: set VCVARS to your vcvars32.bat
exit /b 1
