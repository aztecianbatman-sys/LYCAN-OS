const { app, BrowserWindow, ipcMain, Tray, Menu, nativeImage, dialog } = require('electron');
const path = require('path');
const fs = require('fs');
const crypto = require('crypto');
const { spawn, execFile } = require('child_process');

let backend = null;
let mainWindow = null;
let tray = null;
let isQuitting = false;

function trayIcon() {
  const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 64 64"><rect width="64" height="64" rx="14" fill="#05070b"/><path fill="#fff" d="M14 42 11 18l10 8L32 12l11 14 10-8-3 24-18 10z"/><path fill="#05070b" d="m18 29 8-6-4 12-7-4zm28 0-8-6 4 12 7-4zM18 39l12-13 2 3v14l-8-3zm28 0L34 26l-2 3v14l-8 3z"/><path fill="#1597ff" d="m22 31 10-4-4 7-5 2zm20 0-10-4 4 7 5 2z"/></svg>`;
  return nativeImage.createFromDataURL(`data:image/svg+xml;base64,${Buffer.from(svg).toString('base64')}`);
}

function startBackend() {
  const exe = app.isPackaged ? path.join(process.resourcesPath, 'lycan-backend.exe') : path.join(__dirname, '..', 'build', 'Release', 'lycan-backend.exe');
  backend = spawn(exe, [], { stdio: ['pipe', 'pipe', 'pipe'], windowsHide: true, detached: false });
  backend.stderr.on('data', b => console.error(String(b)));
  backend.on('exit', (code, signal) => { console.log(`LYCAN backend stopped (${code ?? 'null'}/${signal ?? 'none'})`); backend = null; });
  backend.on('error', err => console.error('LYCAN backend error:', err.message));
}

function commandVm(command) {
  return new Promise((resolve, reject) => {
    if (!backend || backend.killed || backend.exitCode !== null) return reject(new Error('LYCAN backend is offline'));
    let buffer = '';
    let settled = false;
    const finish = (fn, value) => { if (settled) return; settled = true; backend?.stdout?.off('data', onData); clearTimeout(timer); fn(value); };
    const onData = chunk => { buffer += String(chunk); const marker = buffer.indexOf('\n<<<LYCAN_END>>>'); if (marker >= 0) finish(resolve, buffer.slice(0, marker)); };
    const timer = setTimeout(() => finish(reject, new Error('LYCAN backend command timeout')), 10000);
    backend.stdout.on('data', onData);
    try { backend.stdin.write(String(command).replace(/\r?\n/g, ' ') + '\n'); } catch (err) { finish(reject, err); }
  });
}

function showMainWindow() { if (!mainWindow) return; mainWindow.show(); mainWindow.focus(); }
function hideMainWindow() { if (!mainWindow) return; mainWindow.hide(); }

function createTray() {
  tray = new Tray(trayIcon());
  tray.setToolTip('LYCAN OS — Guest environment running');
  const update = () => {
    const visible = !!mainWindow?.isVisible();
    tray.setContextMenu(Menu.buildFromTemplate([
      { label: visible ? 'LYCAN OS — OPEN' : 'LYCAN OS — RUNNING IN BACKGROUND', enabled: false },
      { type: 'separator' },
      { label: visible ? 'Hide LYCAN OS' : 'Open LYCAN OS', click: () => visible ? hideMainWindow() : showMainWindow() },
      { label: 'Restart guest runtime', click: async () => { try { await commandVm('ping'); } catch {} } },
      { type: 'separator' },
      { label: 'Quit LYCAN OS', click: () => { isQuitting = true; app.quit(); } }
    ]));
  };
  tray.on('click', () => { if (!mainWindow) return; mainWindow.isVisible() ? hideMainWindow() : showMainWindow(); update(); });
  tray.on('double-click', () => { showMainWindow(); update(); });
  update();
  return update;
}

function candidateFirefoxPaths() {
  const list = [];
  if (process.env.LYCAN_GECKO_PATH) list.push(process.env.LYCAN_GECKO_PATH);
  if (app.isPackaged) list.push(path.join(process.resourcesPath, 'gecko', 'firefox.exe'));
  if (process.env.PROGRAMFILES) list.push(path.join(process.env.PROGRAMFILES, 'Mozilla Firefox', 'firefox.exe'));
  if (process.env['PROGRAMFILES(X86)']) list.push(path.join(process.env['PROGRAMFILES(X86)'], 'Mozilla Firefox', 'firefox.exe'));
  if (process.env.LOCALAPPDATA) list.push(path.join(process.env.LOCALAPPDATA, 'Mozilla Firefox', 'firefox.exe'));
  return [...new Set(list)];
}

function findFirefox() {
  return candidateFirefoxPaths().find(p => fs.existsSync(p)) || null;
}

function openGecko(url = 'about:blank') {
  const firefox = findFirefox();
  if (!firefox) return { ok: false, error: 'GECKO NOT FOUND. Install Mozilla Firefox or place firefox.exe in resources/gecko/.' };
  const root = process.env.LOCALAPPDATA ? path.join(process.env.LOCALAPPDATA, 'LYCAN', 'gecko-profile') : path.join(app.getPath('userData'), 'gecko-profile');
  fs.mkdirSync(root, { recursive: true });
  const clean = String(url || 'about:blank').trim();
  const args = ['-profile', root, '-new-instance', '-private-window', clean];
  const child = spawn(firefox, args, { windowsHide: false, detached: true, stdio: 'ignore' });
  child.unref();
  return { ok: true, engine: 'Gecko', executable: firefox, profile: root };
}

function sha256(file) {
  const h = crypto.createHash('sha256');
  h.update(fs.readFileSync(file));
  return h.digest('hex');
}

function safePackageId(id) { return /^[a-z0-9][a-z0-9._-]{1,63}$/.test(id); }

async function installLypkg(filePath) {
  const source = path.resolve(String(filePath || ''));
  if (!source.toLowerCase().endsWith('.lypkg')) throw new Error('Not a .lypkg package');
  if (!fs.existsSync(source)) throw new Error('Package file not found');
  const staging = path.join(app.getPath('temp'), `lycan-lypkg-${crypto.randomBytes(8).toString('hex')}`);
  const packagesRoot = process.env.LOCALAPPDATA ? path.join(process.env.LOCALAPPDATA, 'LYCAN', 'apps') : path.join(app.getPath('userData'), 'apps');
  fs.mkdirSync(staging, { recursive: true });
  fs.mkdirSync(packagesRoot, { recursive: true });
  try {
    await new Promise((resolve, reject) => {
      execFile('powershell.exe', ['-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-Command',`Expand-Archive -LiteralPath ${JSON.stringify(source)} -DestinationPath ${JSON.stringify(staging)} -Force`], { windowsHide: true, timeout: 30000 }, (err, stdout, stderr) => err ? reject(new Error(String(stderr || err.message))) : resolve());
    });
    const manifestPath = path.join(staging, 'manifest.json');
    const checksumsPath = path.join(staging, 'checksums.sha256');
    const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
    if (!manifest.id || !safePackageId(manifest.id)) throw new Error('Invalid manifest id');
    if (!manifest.name || !manifest.version || manifest.type !== 'lycan-app') throw new Error('Invalid LYPKG manifest');
    if (!fs.existsSync(path.join(staging, 'app'))) throw new Error('Package is missing app/');
    if (!fs.existsSync(checksumsPath)) throw new Error('Package is missing checksums.sha256');
    const lines = fs.readFileSync(checksumsPath, 'utf8').split(/\r?\n/).map(x => x.trim()).filter(Boolean);
    for (const line of lines) {
      const m = line.match(/^([a-f0-9]{64})\s+(.+)$/i);
      if (!m) throw new Error(`Invalid checksum line: ${line}`);
      const rel = m[2].replace(/^\*?/, '').replace(/\\/g, '/');
      if (rel.startsWith('../') || rel.includes('/../')) throw new Error('Invalid checksum path');
      const file = path.resolve(staging, rel);
      if (!file.startsWith(path.resolve(staging) + path.sep)) throw new Error('Checksum path escaped staging');
      if (!fs.existsSync(file) || !fs.statSync(file).isFile()) throw new Error(`Missing package file: ${rel}`);
      if (sha256(file).toLowerCase() !== m[1].toLowerCase()) throw new Error(`SHA-256 mismatch: ${rel}`);
    }
    const destination = path.join(packagesRoot, manifest.id);
    if (!path.resolve(destination).startsWith(path.resolve(packagesRoot) + path.sep)) throw new Error('Invalid destination');
    fs.rmSync(destination, { recursive: true, force: true });
    fs.mkdirSync(destination, { recursive: true });
    fs.cpSync(path.join(staging, 'app'), path.join(destination, 'app'), { recursive: true });
    fs.copyFileSync(manifestPath, path.join(destination, 'manifest.json'));
    fs.copyFileSync(checksumsPath, path.join(destination, 'checksums.sha256'));
    return { ok: true, id: manifest.id, name: manifest.name, version: manifest.version, destination };
  } finally { fs.rmSync(staging, { recursive: true, force: true }); }
}

function createWindow(updateTrayState) {
  mainWindow = new BrowserWindow({ width: 1500, height: 900, minWidth: 1000, minHeight: 650, frame: false, backgroundColor: '#04060b', show: false, title: 'LYCAN OS', webPreferences: { preload: path.join(__dirname, 'preload.js'), contextIsolation: true, nodeIntegration: false, sandbox: true, devTools: !app.isPackaged } });
  mainWindow.setMenuBarVisibility(false);
  mainWindow.loadFile(path.join(__dirname, 'index.html'));
  mainWindow.once('ready-to-show', () => { showMainWindow(); updateTrayState?.(); });
  mainWindow.on('show', () => updateTrayState?.());
  mainWindow.on('hide', () => updateTrayState?.());
  mainWindow.on('minimize', event => { event.preventDefault(); hideMainWindow(); });
  mainWindow.on('close', event => { if (!isQuitting) { event.preventDefault(); hideMainWindow(); updateTrayState?.(); } });
  mainWindow.on('closed', () => { mainWindow = null; updateTrayState?.(); });
}

app.whenReady().then(() => {
  startBackend();
  const updateTrayState = createTray();
  createWindow(updateTrayState);
  ipcMain.handle('lycan:command', (_event, command) => commandVm(String(command || '')));
  ipcMain.handle('lycan:gecko', (_event, url) => openGecko(url));
  ipcMain.handle('lycan:install-lypkg', async () => {
    const picked = await dialog.showOpenDialog(mainWindow, { title: 'Install LYPKG', properties: ['openFile'], filters: [{ name: 'LYCAN Packages', extensions: ['lypkg'] }] });
    if (picked.canceled || !picked.filePaths[0]) return { ok: false, canceled: true };
    return installLypkg(picked.filePaths[0]).catch(error => ({ ok: false, error: error.message }));
  });
  ipcMain.on('lycan:window', (_event, action) => {
    if (!mainWindow) return;
    if (action === 'minimize' || action === 'close') hideMainWindow();
    if (action === 'maximize') mainWindow.isMaximized() ? mainWindow.unmaximize() : mainWindow.maximize();
    updateTrayState();
  });
  app.on('activate', () => showMainWindow());
});

app.on('window-all-closed', event => event.preventDefault());
app.on('before-quit', () => {
  isQuitting = true;
  if (backend && !backend.killed) {
    try { backend.stdin.write('__LYCAN_EXIT__\n'); } catch {}
    setTimeout(() => { if (backend && !backend.killed) backend.kill(); }, 1200);
  }
});
