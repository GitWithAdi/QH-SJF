@echo off
setlocal enabledelayedexpansion

if not exist bin mkdir bin

set CC=gcc
set CFLAGS=-std=c11 -O2 -Wall -Wextra -Iinclude
set LIBS=-lm -lws2_32

if "%1"=="clean" (
    if exist bin rmdir /s /q bin
    echo Cleaned build artifacts.
    goto end
)

if "%1"=="test" (
    echo Building test runner...
    %CC% %CFLAGS% -o bin\test_runner.exe src\main_test.c src\tests.c src\sim.c src\workload.c -lm
    if errorlevel 1 (
        echo Compilation failed!
        exit /b 1
    )
    echo Running paper test suite...
    bin\test_runner.exe
    goto end
)

if "%1"=="run" (
    echo Building C HTTP server...
    %CC% %CFLAGS% -o bin\qhsjf-server.exe src\server.c src\sim.c src\workload.c src\tests.c %LIBS%
    if errorlevel 1 (
        echo Server build failed!
        exit /b 1
    )
    echo Starting server at http://127.0.0.1:5173...
    bin\qhsjf-server.exe 5173
    goto end
)

echo [1/2] Building C HTTP server...
%CC% %CFLAGS% -o bin\qhsjf-server.exe src\server.c src\sim.c src\workload.c src\tests.c %LIBS%
if errorlevel 1 exit /b 1

echo [2/2] Building test runner...
%CC% %CFLAGS% -o bin\test_runner.exe src\main_test.c src\tests.c src\sim.c src\workload.c -lm
if errorlevel 1 exit /b 1

echo.
echo Build successful!
echo - Run server:     build.bat run
echo - Run test suite: build.bat test
echo.

:end
