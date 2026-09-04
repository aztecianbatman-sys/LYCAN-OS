const { app, BrowserWindow, BrowserWindow: BW, ipcMain, Tray, Menu, nativeImage } = require('electron');
const path = require('path');
const { spawn } = require('child_process');
let backend = null;
let mainWindow = null;
let tray = null;
let isQuitting = false;

function trayIcon() {
  const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 64 64"><rect width="64" height="64" rx="14" fill="#05070b"/><path fill="#fff" d="M14 42 11 18l10 8L32 12l11 14 10-8-3 24-18 10z"/><path fill="#05070b" d="m18 29 8-6-4 12-7-4zm28 0-8-6 4 12 7-4zM18 39l12-13 2 3v14l-8-3zm28 0L34 26l-2 3v14l8-3z"/><path fill="#1597ff" d="m22 31 10-4-4 7-5 2zm20 0-10-4 4 7 5 2z"/></svg>`;
  return nativeImage.createFromDataURL(`data:image/svg+xml;base64,${Buffer.from(svg).toString('base64')}`);
}

function startBackend() {
  const exe = app.isPackaged ? path.join(process.resourcesPath, 'lycan-backend.exe') : path.join(__dirname, '..', 'build', 'Release', 'lycan-backend.exe');
  backend = spawn(exe, [], { stdio: ['pipe','pipe','pipe'], windowsHide: true });
  backend.stderr.on('data', b => console.error(String(b)));
}

function commandVm(command) {
  return new Promise((resolve, reject) => {
    if (!backend || backend.killed) return reject(new Error('LYCAN backend is offline'));
    let buffer = '';
    const onData = chunk => {
      buffer += String(chunk);
      const marker = buffer.indexOf('\n<<<LYCAN_END>>>');
      if (marker >= 0) {
        const output = buffer.slice(0, marker);
        backend.stdout.off('data', onData);
        resolve(output);
      }
    };
    backend.stdout.on('data', onData);
    backend.stdin.write(command.replace(/\r?\n/g, ' ') + '\n');
    setTimeout(() => { backend.stdout.off('data', onData); reject(new Error('LYCAN backend command timeout')); }, 10000);
  });
}

function createTray() {
  tray = new Tray(trayIcon());
  tray.setToolTip('LYCAN OS — Virtual Workspace');
  const restore = () => {
    if (!mainWindow) return;
    mainWindow.show();
    mainWindow.focus();
  };
  tray.setContextMenu(Menu.buildFromTemplate([
    { label: 'Open LYCAN OS', click: restore },
    { type: 'separator' },
    { label: 'Quit LYCAN OS', click: () => { isQuitting = true; app.quit(); } }
  ]));
  tray.on('double-click', restore);
}

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1500,
    height: 900,
    minWidth: 1000,
    minHeight: 650,
    frame: false,
    backgroundColor: '#04060b',
    show: false,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true
    }
  });
  mainWindow.loadFile(path.join(__dirname, 'index.html'));
  mainWindow.once('ready-to-show', () => mainWindow.show());
  mainWindow.on('close', event => {
    if (!isQuitting) {
      event.preventDefault();
      mainWindow.hide();
    }
  });
  mainWindow.on('closed', () => { mainWindow = null; });
}

app.whenReady().then(() => {
  startBackend();
  createWindow();
  createTray();
  ipcMain.handle('lycan:command', (_event, command) => commandVm(String(command || '')));
  ipcMain.on('lycan:window', (_event, action) => {
    if (!mainWindow) return;
    if (action === 'minimize') mainWindow.minimize();
    if (action === 'maximize') mainWindow.isMaximized() ? mainWindow.unmaximize() : mainWindow.maximize();
    if (action === 'close') mainWindow.hide();
  });
});

app.on('window-all-closed', event => event.preventDefault());
app.on('before-quit', () => { isQuitting = true; if (backend && !backend.killed) backend.stdin.write('__LYCAN_EXIT__\n'); });
