const { contextBridge, ipcRenderer } = require('electron');
contextBridge.exposeInMainWorld('lycan', {
  command: command => ipcRenderer.invoke('lycan:command', command),
  window: action => ipcRenderer.send('lycan:window', String(action || '')),
  gecko: url => ipcRenderer.invoke('lycan:gecko', url),
  installPackage: () => ipcRenderer.invoke('lycan:install-lypkg'),
  listPackages: () => ipcRenderer.invoke('lycan:list-packages'),
  launchPackage: id => ipcRenderer.invoke('lycan:launch-package', String(id || ''))
});
