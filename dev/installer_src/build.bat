@echo off
rem Builds dev\installer_src\src into dev\installer_src\build\BuzzSetup.exe (32-bit). Needs the Visual Studio 2019
rem Build Tools. The engine (dev\patch_src\build\DDraw.dll), ffmpeg (build\ffmpeg.exe, made by ffmpeg.sh in the
rem MSYS2 MINGW64 shell) and dev\lang\*.json go inside: build the engine and ffmpeg first.
setlocal
rem Another Visual Studio: set VCVARS to its vcvars32.bat before running this
if not defined VCVARS set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
if not exist "%VCVARS%" goto novs
call "%VCVARS%" >nul 2>&1
set SRC=%~dp0src
set OUT=%~dp0build
set OBJ=%~dp0build\obj
if not exist "%OBJ%" mkdir "%OBJ%"
if not exist "%~dp0..\patch_src\build\DDraw.dll" (
	echo dev\patch_src\build\DDraw.dll is missing: build the engine first
	exit /b 1
)
if not exist "%~dp0build\ffmpeg.exe" (
	echo build\ffmpeg.exe is missing: run ffmpeg.sh in the MSYS2 MINGW64 shell first
	exit /b 1
)
rem The help's style: refresh dev\help\runig.css from the shared one when it is next to this project
if exist "%~dp0..\..\..\Patch_HTML_Base\runig.css" copy /y "%~dp0..\..\..\Patch_HTML_Base\runig.css" "%~dp0..\help\runig.css" >nul
pushd "%SRC%"
rc /nologo /fo"%OBJ%\BuzzSetup.res" BuzzSetup.rc || (popd & exit /b 1)
popd
cl /nologo /O2 /W3 /EHsc /std:c++17 /MT /utf-8 /DUNICODE /D_UNICODE /D_CRT_SECURE_NO_WARNINGS /Fo"%OBJ%\\" ^
   "%SRC%\*.cpp" /Fe"%OUT%\BuzzSetup.exe" "%OBJ%\BuzzSetup.res" /link /SUBSYSTEM:WINDOWS ^
   user32.lib gdi32.lib shell32.lib ole32.lib advapi32.lib comdlg32.lib winmm.lib comctl32.lib version.lib || exit /b 1
rem What is handed out is BuzzSetup.exe alone (ffmpeg goes inside), with ffmpeg's source next to it in the release
exit /b %errorlevel%
:novs
echo Visual Studio 2019 Build Tools not found: set VCVARS to your vcvars32.bat
exit /b 1
