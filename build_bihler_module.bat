@echo off
echo Starting clang-tidy build with Bihler module...

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to set up Visual Studio environment!
    goto :error
)

REM Get script directory and define paths relative to it
set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%..\llvm-build"
set "SOURCE_DIR=%SCRIPT_DIR%llvm"
set "VS_CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA_DIR=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"

REM Add Ninja to PATH
set "PATH=%NINJA_DIR%;%PATH%"

REM Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    echo Creating build directory: %BUILD_DIR%
    mkdir "%BUILD_DIR%"
)

REM Change to build directory
cd /d "%BUILD_DIR%"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to change to build directory!
    goto :error
)

echo.
echo Current directory: %CD%
echo Source directory: %SOURCE_DIR%
echo.

REM Check if CMake configuration exists
if not exist "CMakeCache.txt" (
    echo CMake not configured yet, running configuration...
    echo Using Ninja generator...
    echo.
    "%VS_CMAKE%" -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_ENABLE_ASSERTIONS=ON "%SOURCE_DIR%"
    
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: CMake configuration failed!
        goto :error
    )
    echo CMake configuration successful!
    echo.
) else (
    echo CMake already configured, skipping configuration step.
    echo.
)

REM Build clang-tidy with Ninja
echo Building clang-tidy with Ninja...
"%VS_CMAKE%" --build . --target clang-tidy
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    goto :error
)

echo Build successful!

:check_binary
echo.
echo Checking for clang-tidy binary...
if exist "bin\clang-tidy.exe" (
    echo SUCCESS: clang-tidy.exe found in bin!
    echo Testing binary...
    bin\clang-tidy.exe --version
    echo.
    echo Checking for bihler module...
    bin\clang-tidy.exe --list-checks | findstr /i "bihler"
    if %ERRORLEVEL% EQU 0 (
        echo SUCCESS: Bihler module is available!
    ) else (
        echo WARNING: Bihler module not found in check list
    )
) else (
    echo WARNING: clang-tidy.exe not found in bin\
    echo Searching for clang-tidy.exe...
    dir /s /b clang-tidy.exe 2>nul
)

echo.
echo Build completed successfully!
goto :end

:error
echo.
echo Build failed! Please check the error messages above.
exit /b 1

:end
echo.
echo Build script completed.
