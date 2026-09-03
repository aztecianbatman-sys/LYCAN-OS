const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const { spawn } = require('child_process');
let backend = null;
let mainWindow = null;

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

function createWindow() {
  mainWindow = new BrowserWindow({width: 1500,height: 900,minWidth:1000,minHeight:650,frame:false,backgroundColor:'#04060b',webPreferences:{preload:path.join(__dirname,'preload.js'),contextIsolation:true,nodeIntegration:false}});
  mainWindow.loadFile(path.join(__dirname, 'index.html'));
  mainWindow.on('closed', () => { mainWindow = null; });
}

app.whenReady().then(() => {
  startBackend();
  ipcMain.handle('lycan:command', (_event, command) => commandVm(String(command || '')));
  ipcMain.on('lycan:window', (_event, action) => {
    if (!mainWindow) return;
    if (action === 'minimize') mainWindow.minimize();
    if (action === 'maximize') mainWindow.isMaximized() ? mainWindow.unmaximize() : mainWindow.maximize();
    if (action === 'close') mainWindow.close();
  });
  createWindow();
});
app.on('window-all-closed', () => { if (backend && !backend.killed) backend.stdin.write('__LYCAN_EXIT__\n'); if (process.platform !== 'darwin') app.quit(); });
