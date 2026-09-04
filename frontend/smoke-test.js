const fs = require('fs');
const path = require('path');

const root = __dirname;
const read = file => fs.readFileSync(path.join(root, file), 'utf8');
const fail = message => { throw new Error(`LYCAN packaging smoke test failed: ${message}`); };

const pkg = JSON.parse(read('package.json'));
if (pkg.main !== 'bootstrap.js') fail(`package main is ${pkg.main}, expected bootstrap.js`);
if (pkg.build?.executableName !== 'LYCAN') fail('electron-builder executableName must be LYCAN');
if (pkg.build?.appId !== 'com.lycan.os') fail('electron-builder appId changed unexpectedly');
if (pkg.build?.nsis?.createDesktopShortcut !== false) fail('default desktop shortcut creation must stay disabled');
if (pkg.build?.nsis?.createStartMenuShortcut !== false) fail('default Start Menu shortcut creation must stay disabled');

for (const file of ['bootstrap.js','main.js','preload.js','package-preload.js','index.html','renderer.js','windowing.js','launcher.js','store.js','crawford.js','runtime.js']) {
  if (!fs.existsSync(path.join(root, file))) fail(`missing packaged frontend file: ${file}`);
}

const bootstrap = read('bootstrap.js');
if (!bootstrap.includes("path.join(__dirname, 'index.html')")) fail('bootstrap does not validate index.html');
if (!bootstrap.includes("require('./main.js')")) fail('bootstrap does not delegate to main.js');

const main = read('main.js');
if (!main.includes("mainWindow.loadFile(path.join(__dirname, 'index.html'))")) fail('main window does not load index.html');
if (main.includes("loadFile(path.join(__dirname, 'assets/lycan-mark.svg'))")) fail('main window is configured to load the logo asset');

const installer = fs.readFileSync(path.join(root, '..', 'installer', 'lycan.nsh'), 'utf8');
if (!installer.includes('$INSTDIR\\LYCAN.exe')) fail('NSIS script does not target LYCAN.exe');
if (installer.includes('$INSTDIR\\assets\\lycan-mark.svg')) fail('NSIS script targets the logo asset');

console.log('LYCAN packaging smoke test: PASS');
