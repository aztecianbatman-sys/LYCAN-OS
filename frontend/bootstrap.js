const { app, dialog } = require('electron');
const fs = require('fs');
const path = require('path');

const SHELL_ENTRY = path.join(__dirname, 'index.html');

// Only allow one LYCAN process. A second click on the shortcut should never
// create a second guest runtime or a second tray instance.
if (!app.requestSingleInstanceLock()) {
  app.quit();
} else {
  app.setAppUserModelId('com.lycan.os');

  // Fail loudly and safely when packaging is incomplete instead of opening a
  // frontend asset or presenting a blank window.
  if (!fs.existsSync(SHELL_ENTRY) || !fs.statSync(SHELL_ENTRY).isFile()) {
    app.whenReady().then(() => {
      dialog.showErrorBox(
        'LYCAN OS could not start',
        `The packaged desktop shell is missing:\n${SHELL_ENTRY}\n\nReinstall LYCAN OS using the official installer.`
      );
      app.quit();
    });
  } else {
    require('./main.js');
  }
}
