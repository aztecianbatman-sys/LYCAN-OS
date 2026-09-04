const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('lycanApp', {
  id: () => ipcRenderer.sendSync('lycan:package-id'),
  permissions: () => ipcRenderer.sendSync('lycan:package-permissions'),
  requestExternal: url => ipcRenderer.invoke('lycan:package-external', String(url || '')),
  storagePath: () => ipcRenderer.sendSync('lycan:package-storage-path')
});
