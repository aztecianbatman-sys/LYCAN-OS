const { app, dialog } = require('electron');
const fs = require('fs');
const path = require('path');

const SHELL_ENTRY = path.join(__dirname, 'index.html');
const LOG_DIR = path.join(app.getPath('userData'), 'logs');
const LOG_FILE = path.join(LOG_DIR, 'startup.log');

function log(message, error) {
  try {
    fs.mkdirSync(LOG_DIR, { recursive: true });
    const detail = error ? `\n${error.stack || error.message || String(error)}` : '';
    fs.appendFileSync(LOG_FILE, `[${new Date().toISOString()}] ${message}${detail}\n`);
  } catch (_) {}
}

function failStartup(message) {
  log(message);
  app.whenReady().then(() => {
    dialog.showErrorBox(
      'LYCAN OS could not start',
      `${message}\n\nStartup log:\n${LOG_FILE}`
    );
    app.quit();
  });
}

if (!app.requestSingleInstanceLock()) {
  app.quit();
} else {
  app.setAppUserModelId('com.lycan.os');
  log(`Launching LYCAN OS ${app.getVersion()} from ${__dirname}`);

  if (!fs.existsSync(SHELL_ENTRY) || !fs.statSync(SHELL_ENTRY).isFile()) {
    failStartup(`The packaged desktop shell is missing:\n${SHELL_ENTRY}\n\nReinstall LYCAN OS using the official installer.`);
  } else {
    // main.js owns the BrowserWindow/backend lifecycle. Keep bootstrap as the
    // only package entry so the installer can never target index.html directly.
    require('./main.js');

    app.on('second-instance', (_event, commandLine) => {
      log(`Second launch ignored: ${commandLine.join(' ')}`);
    });

    process.on('uncaughtException', error => log('Uncaught bootstrap exception', error));
    process.on('unhandledRejection', error => log('Unhandled bootstrap rejection', error));
  }
}
