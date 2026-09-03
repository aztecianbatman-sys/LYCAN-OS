# LYCAN OS

LYCAN is a Windows-hosted virtual operating environment with a radial, cinematic desktop shell.

## Experience

The primary desktop is intentionally empty. A LYCAN mark anchors the center of the screen. Moving toward the left or right edge reveals application nodes; selecting one opens a glassy app surface over the workspace.

The shell is built with Electron + HTML + CSS + JavaScript. The VM backend is native C++ and communicates through a local stdin/stdout command bridge.

## Safety

LYCAN is a normal Windows application. It does not replace Windows, change boot configuration, repartition disks, or install another operating system.

## Build on Windows

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
cd frontend
npm install
npm run build:win
```

The frontend build expects the native `build/Release/lycan-backend.exe` beside the repository root.
