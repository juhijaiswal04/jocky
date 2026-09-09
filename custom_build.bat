@echo off
setlocal

:: Initialize MSVC build environment if cl.exe is not available
where cl.exe >nul 2>nul
if %errorlevel% neq 0 (
    echo [JOCKY] Initializing MSVC environment...
    :: Adjust this path if your VS installation is in a different folder
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    if %errorlevel% neq 0 (
        echo [ERROR] Could not find Visual Studio. Please run this from the Developer Command Prompt.
        exit /b 1
    )
)

echo ==========================================
echo [JOCKY] Building Compiler (v0.1)...
echo ==========================================

if not exist bin mkdir bin
if not exist temp mkdir temp

:: Compile the compiler itself
cl.exe /nologo /W3 /O2 /I src /I runtime src\*.c /Febin\jky.exe /Fotemp\ /Fdtemp\

if %errorlevel% neq 0 (
    echo [JOCKY] Build failed.
    exit /b %errorlevel%
)

:: Copy to root for easy access
copy bin\jky.exe jky.exe >nul

echo [JOCKY] Build succeeded! 
echo [JOCKY] Compiler ready: jky.exe
echo ==========================================
endlocal