const { contextBridge, ipcRenderer } = require('electron');
contextBridge.exposeInMainWorld('lycan', {
  command: command => ipcRenderer.invoke('lycan:command', command),
  window: action => ipcRenderer.send('lycan:window', String(action || ''))
});
