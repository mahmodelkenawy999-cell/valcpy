@echo off
REM Game Launcher Script for valcpy
REM This script compiles and runs the C++ game program

cd /d "%~dp0"

REM Check if build directory exists, if not create it
if not exist "build" mkdir build
cd build

REM Compile the C++ program (adjust based on your main file)
REM Assumes you have g++ or MSVC installed
echo Compiling game...

if exist "game.exe" (
    echo Starting game...
    game.exe
) else (
    echo Attempting to compile...
    REM Try with g++ (MinGW)
    g++ -o game.exe ../src/*.cpp ../src/*.c 2>nul
    
    REM If g++ fails, try MSVC
    if errorlevel 1 (
        cl.exe /Fe:game.exe ..\src\*.cpp ..\src\*.c 2>nul
    )
    
    REM If compilation succeeded, run the game
    if exist "game.exe" (
        echo Compilation successful! Starting game...
        game.exe
    ) else (
        echo Error: Could not compile the game.
        echo Make sure you have a C++ compiler installed (MinGW or MSVC).
        pause
    )
)

pause
