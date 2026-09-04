const { app, BrowserWindow, ipcMain, Tray, Menu, nativeImage, shell } = require('electron');
const path = require('path');
const { spawn } = require('child_process');

let backend = null;
let mainWindow = null;
let tray = null;
let isQuitting = false;
let firstWindowShown = false;

function trayIcon() {
  const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 64 64"><rect width="64" height="64" rx="14" fill="#05070b"/><path fill="#fff" d="M14 42 11 18l10 8L32 12l11 14 10-8-3 24-18 10z"/><path fill="#05070b" d="m18 29 8-6-4 12-7-4zm28 0-8-6 4 12 7-4zM18 39l12-13 2 3v14l-8-3zm28 0L34 26l-2 3v14l8-3z"/><path fill="#1597ff" d="m22 31 10-4-4 7-5 2zm20 0-10-4 4 7 5 2z"/></svg>`;
  return nativeImage.createFromDataURL(`data:image/svg+xml;base64,${Buffer.from(svg).toString('base64')}`);
}

function startBackend() {
  const exe = app.isPackaged
    ? path.join(process.resourcesPath, 'lycan-backend.exe')
    : path.join(__dirname, '..', 'build', 'Release', 'lycan-backend.exe');
  backend = spawn(exe, [], {
    stdio: ['pipe', 'pipe', 'pipe'],
    windowsHide: true,
    detached: false
  });
  backend.stderr.on('data', b => console.error(String(b)));
  backend.on('exit', (code, signal) => {
    console.log(`LYCAN backend stopped (${code ?? 'null'}/${signal ?? 'none'})`);
    backend = null;
  });
  backend.on('error', err => console.error('LYCAN backend error:', err.message));
}

function commandVm(command) {
  return new Promise((resolve, reject) => {
    if (!backend || backend.killed || backend.exitCode !== null) {
      return reject(new Error('LYCAN backend is offline'));
    }
    let buffer = '';
    let settled = false;
    const finish = (fn, value) => {
      if (settled) return;
      settled = true;
      backend?.stdout?.off('data', onData);
      clearTimeout(timer);
      fn(value);
    };
    const onData = chunk => {
      buffer += String(chunk);
      const marker = buffer.indexOf('\n<<<LYCAN_END>>>');
      if (marker >= 0) finish(resolve, buffer.slice(0, marker));
    };
    const timer = setTimeout(() => finish(reject, new Error('LYCAN backend command timeout')), 10000);
    backend.stdout.on('data', onData);
    try {
      backend.stdin.write(String(command).replace(/\r?\n/g, ' ') + '\n');
    } catch (err) {
      finish(reject, err);
    }
  });
}

function showMainWindow() {
  if (!mainWindow) return;
  mainWindow.show();
  mainWindow.focus();
  firstWindowShown = true;
}

function hideMainWindow() {
  if (!mainWindow) return;
  mainWindow.hide();
}

function createTray() {
  tray = new Tray(trayIcon());
  tray.setToolTip('LYCAN OS — Guest environment running');

  const updateTrayState = () => {
    const visible = !!mainWindow?.isVisible();
    tray.setContextMenu(Menu.buildFromTemplate([
      { label: visible ? 'LYCAN OS — OPEN' : 'LYCAN OS — RUNNING IN BACKGROUND', enabled: false },
      { type: 'separator' },
      { label: visible ? 'Hide LYCAN OS' : 'Open LYCAN OS', click: () => visible ? hideMainWindow() : showMainWindow() },
      { label: 'Restart guest runtime', click: async () => {
          try { await commandVm('ping'); } catch {}
        } },
      { type: 'separator' },
      { label: 'Quit LYCAN OS', click: () => { isQuitting = true; app.quit(); } }
    ]));
  };

  updateTrayState();
  tray.on('click', () => {
    if (!mainWindow) return;
    if (mainWindow.isVisible()) hideMainWindow();
    else showMainWindow();
    updateTrayState();
  });
  tray.on('double-click', () => { showMainWindow(); updateTrayState(); });
  return updateTrayState;
}

function createWindow(updateTrayState) {
  mainWindow = new BrowserWindow({
    width: 1500,
    height: 900,
    minWidth: 1000,
    minHeight: 650,
    frame: false,
    backgroundColor: '#04060b',
    show: false,
    skipTaskbar: false,
    title: 'LYCAN OS',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
      devTools: !app.isPackaged
    }
  });

  mainWindow.setMenuBarVisibility(false);
  mainWindow.loadFile(path.join(__dirname, 'index.html'));

  mainWindow.once('ready-to-show', () => {
    // The first launch is visible. After that, LYCAN behaves like a resident
    // desktop environment: closing/minimizing the shell leaves the guest alive
    // in the Windows notification area.
    showMainWindow();
    updateTrayState?.();
  });

  mainWindow.on('show', () => updateTrayState?.());
  mainWindow.on('hide', () => updateTrayState?.());
  mainWindow.on('minimize', event => {
    // Do not kill the guest or the Electron process when minimized.
    event.preventDefault();
    hideMainWindow();
  });

  mainWindow.on('close', event => {
    if (!isQuitting) {
      event.preventDefault();
      hideMainWindow();
      updateTrayState?.();
    }
  });
  mainWindow.on('closed', () => { mainWindow = null; updateTrayState?.(); });
}

app.whenReady().then(() => {
  startBackend();
  const updateTrayState = createTray();
  createWindow(updateTrayState);

  ipcMain.handle('lycan:command', (_event, command) => commandVm(String(command || '')));
  ipcMain.on('lycan:window', (_event, action) => {
    if (!mainWindow) return;
    if (action === 'minimize') hideMainWindow();
    if (action === 'maximize') mainWindow.isMaximized() ? mainWindow.unmaximize() : mainWindow.maximize();
    if (action === 'close') hideMainWindow();
    updateTrayState();
  });

  app.on('activate', () => showMainWindow());
});

app.on('window-all-closed', event => event.preventDefault());

app.on('before-quit', () => {
  isQuitting = true;
  if (backend && !backend.killed) {
    try { backend.stdin.write('__LYCAN_EXIT__\n'); } catch {}
    setTimeout(() => {
      if (backend && !backend.killed) backend.kill();
    }, 1200);
  }
});
