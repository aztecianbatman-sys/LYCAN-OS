const { contextBridge, ipcRenderer } = require('electron');
contextBridge.exposeInMainWorld('lycan', {
  command: command => ipcRenderer.invoke('lycan:command', command),
  window: action => ipcRenderer.send('lycan:window', String(action || '')),
  gecko: url => ipcRenderer.invoke('lycan:open-gecko', url),
  openGecko: url => ipcRenderer.invoke('lycan:open-gecko', url),
  installPackage: () => ipcRenderer.invoke('lycan:install-package'),
  listPackages: () => ipcRenderer.invoke('lycan:list-packages'),
  downloadPackage: url => ipcRenderer.invoke('lycan:download-package', String(url || '')),
  launchPackage: id => ipcRenderer.invoke('lycan:launch-package', String(id || ''))
});
