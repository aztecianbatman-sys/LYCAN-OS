@echo off
setlocal
cd /d "%~dp0frontend"

echo.
echo  LYCAN OS 2.0 - PURE ELECTRON GUEST RUNTIME
echo  =============================================
echo.

where node >nul 2>nul
if errorlevel 1 (
  echo Node.js was not found.
  echo Install Node.js 20+ and run this file again.
  pause
  exit /b 1
)

if not exist node_modules\electron\dist\electron.exe (
  echo Installing Electron dependencies for the first run...
  call npm install
  if errorlevel 1 (
    echo.
    echo Dependency installation failed.
    pause
    exit /b 1
  )
)

call npm run check
if errorlevel 1 (
  echo.
  echo LYCAN source validation failed. See the output above.
  pause
  exit /b 1
)

echo.
echo Starting the LYCAN guest environment...
echo Guest data is stored under %%LOCALAPPDATA%%\LYCAN.
echo Windows is not replaced, repartitioned, or boot-modified.
echo.
call npm start
