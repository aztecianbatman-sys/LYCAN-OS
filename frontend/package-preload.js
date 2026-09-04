const { contextBridge, ipcRenderer } = require('electron');

const call = (channel, ...args) => ipcRenderer.invoke(channel, ...args);
contextBridge.exposeInMainWorld('lycanApp', {
  id: () => ipcRenderer.sendSync('lycan:package-id'),
  permissions: () => ipcRenderer.sendSync('lycan:package-permissions'),
  requestExternal: url => call('lycan:package-external', String(url || '')),
  storage: {
    list: (bucket = 'data', path = '') => call('lycan:app-storage-list', bucket, path),
    read: (bucket = 'data', path = '') => call('lycan:app-storage-read', bucket, path),
    write: (bucket = 'data', path = '', text = '') => call('lycan:app-storage-write', bucket, path, String(text)),
    delete: (bucket = 'data', path = '') => call('lycan:app-storage-delete', bucket, path),
    usage: () => call('lycan:app-storage-usage'),
    quota: () => call('lycan:app-storage-quota')
  },
  network: {
    status: () => call('lycan:app-network-status')
  },
  notify: (title, body) => call('lycan:package-notify', String(title || 'LYCAN'), String(body || ''))
});
