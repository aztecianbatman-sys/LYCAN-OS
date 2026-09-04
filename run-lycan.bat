@echo off
setlocal
cd /d "%~dp0"

title LYCAN OS 2.1.0 - Pure Electron Guest Runtime

if not exist "frontend\package.json" (
  echo [LYCAN] frontend package.json not found.
  pause
  exit /b 1
)

where node >nul 2>nul
if errorlevel 1 (
  echo [LYCAN] Node.js was not found.
  echo Install Node.js 20+ and run this file again.
  pause
  exit /b 1
)

where npm >nul 2>nul
if errorlevel 1 (
  echo [LYCAN] npm was not found.
  pause
  exit /b 1
)

cd frontend
if not exist node_modules\electron\dist\electron.exe (
  echo [LYCAN] First run: installing Electron dependencies...
  call npm install
  if errorlevel 1 (
    echo.
    echo [LYCAN] Dependency installation failed. No Windows system changes were made.
    pause
    exit /b 1
  )
)

echo [LYCAN] Validating pure Electron guest runtime...
call npm run check
if errorlevel 1 (
  echo.
  echo [LYCAN] Runtime validation failed. Nothing was installed into Windows.
  pause
  exit /b 1
)

echo.
echo [LYCAN] Starting LYCAN OS 2.1.0...
echo [LYCAN] Guest data: %%LOCALAPPDATA%%\LYCAN
echo [LYCAN] Windows is not replaced, repartitioned, or boot-modified.
echo.
call npm start
set ERR=%ERRORLEVEL%
if not "%ERR%"=="0" (
  echo.
  echo [LYCAN] Electron exited with code %ERR%.
  pause
)
exit /b %ERR%
