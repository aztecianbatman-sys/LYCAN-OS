const { app, BrowserWindow, ipcMain, dialog, shell, session } = require('electron');
const path = require('path');
const fs = require('fs');
const crypto = require('crypto');
const https = require('https');
const { execFile } = require('child_process');
const { LycanVM } = require('./vm');

let mainWindow = null;
let isQuitting = false;
const packageContexts = new Map();
const CORE_IDS = new Set(['lycan-terminal','lycan-files','lycan-web','lycan-store','lycan-settings','lycan-diagnostics','lycan-snapshots','crawford']);
const PACKAGE_PERMISSIONS = new Set(['network','external','storage','notifications','clipboard-read','clipboard-write']);

function rootPath(){ return process.env.LOCALAPPDATA ? path.join(process.env.LOCALAPPDATA,'LYCAN') : path.join(app.getPath('userData'),'guest'); }
const vm = new LycanVM(rootPath());
function packageRoot(){ return path.join(rootPath(),'apps'); }
function safeId(id){ return /^[a-z0-9][a-z0-9._-]{1,63}$/.test(String(id||'')); }
function sha256(file){ const h=crypto.createHash('sha256'); h.update(fs.readFileSync(file)); return h.digest('hex'); }
function normalizePermissions(m){ const p=Array.isArray(m.permissions)?m.permissions.map(x=>String(x).trim().toLowerCase()):[]; if(p.some(x=>!PACKAGE_PERMISSIONS.has(x))) throw new Error('Unsupported package permission'); return [...new Set(p)]; }
function quota(m){ const n=Number(m.storageQuotaMB ?? 16); if(!Number.isInteger(n)||n<1||n>1024) throw new Error('storageQuotaMB must be 1-1024'); return n; }

function listPackages(){
  fs.mkdirSync(packageRoot(),{recursive:true}); const result=[];
  for(const e of fs.readdirSync(packageRoot(),{withFileTypes:true})){
    if(!e.isDirectory()||!safeId(e.name)) continue;
    try{ const m=JSON.parse(fs.readFileSync(path.join(packageRoot(),e.name,'manifest.json'),'utf8')); if(m.id!==e.name||m.type!=='lycan-app') continue; result.push({id:m.id,name:m.name,version:m.version,description:m.description||'LYCAN guest application.',entry:m.entry||'app/index.html',icon:m.icon||'',permissions:normalizePermissions(m),storageQuotaMB:quota(m),type:CORE_IDS.has(m.id)?'CORE':'LOCAL'}); }catch{}
  }
  return result.sort((a,b)=>String(a.name).localeCompare(String(b.name)));
}
function registerPackage(m){ normalizePermissions(m); quota(m); const out=vm.cmd(`app register ${m.id} ${m.version} ${quota(m)} ${normalizePermissions(m).join(',')}`); if(!out.startsWith('APP REGISTERED')) throw new Error(out); }
function packageEntry(id){
  if(!safeId(id)) throw new Error('Invalid package id'); const dir=path.resolve(packageRoot(),id); const root=path.resolve(packageRoot()); if(!dir.startsWith(root+path.sep)) throw new Error('Invalid package path');
  const manifest=JSON.parse(fs.readFileSync(path.join(dir,'manifest.json'),'utf8')); const entry=String(manifest.entry||'app/index.html').replace(/\\/g,'/'); if(entry.startsWith('/')||entry.includes('../')) throw new Error('Invalid package entry');
  const file=path.resolve(dir,entry); if(!file.startsWith(dir+path.sep)||!fs.existsSync(file)) throw new Error('Package entrypoint not found'); return {dir,file,manifest,permissions:normalizePermissions(manifest)};
}

function createWindow(){
  mainWindow = new BrowserWindow({width:1440,height:920,minWidth:1080,minHeight:700,show:false,frame:false,backgroundColor:'#030508',title:'LYCAN OS',webPreferences:{preload:path.join(__dirname,'preload.js'),contextIsolation:true,nodeIntegration:false,sandbox:false}});
  mainWindow.setMenuBarVisibility(false); mainWindow.loadFile(path.join(__dirname,'index.html'));
  mainWindow.once('ready-to-show',()=>mainWindow.show());
  mainWindow.on('close',event=>{ if(!isQuitting){ event.preventDefault(); mainWindow.hide(); } });
}

function openGecko(url='about:blank'){
  const candidates=[]; if(process.env.LYCAN_GECKO_PATH)candidates.push(process.env.LYCAN_GECKO_PATH); if(process.env.PROGRAMFILES)candidates.push(path.join(process.env.PROGRAMFILES,'Mozilla Firefox','firefox.exe')); if(process.env['PROGRAMFILES(X86)'])candidates.push(path.join(process.env['PROGRAMFILES(X86)'],'Mozilla Firefox','firefox.exe')); if(process.env.LOCALAPPDATA)candidates.push(path.join(process.env.LOCALAPPDATA,'Mozilla Firefox','firefox.exe'));
  const exe=candidates.find(p=>fs.existsSync(p)); if(!exe)return{ok:false,error:'Firefox/Gecko was not found. Install Firefox or set LYCAN_GECKO_PATH.'};
  if(!/^NETWORK ONLINE/m.test(vm.cmd('network status')))return{ok:false,error:'LYCAN guest network is offline.'};
  const profile=path.join(rootPath(),'gecko-profile'); fs.mkdirSync(profile,{recursive:true}); const child=execFile(exe,['-profile',profile,'-new-instance','-private-window',String(url||'about:blank')],{windowsHide:false}); child.unref(); return{ok:true,engine:'Gecko',profile};
}

function installPackage(filePath){
  return new Promise((resolve,reject)=>{
    const source=path.resolve(String(filePath||'')); if(!source.toLowerCase().endsWith('.lypkg'))return reject(new Error('Select a .lypkg file')); if(!fs.existsSync(source))return reject(new Error('Package file not found'));
    const staging=path.join(app.getPath('temp'),`lycan-${crypto.randomBytes(8).toString('hex')}`); fs.mkdirSync(staging,{recursive:true});
    const ps=`Expand-Archive -LiteralPath ${JSON.stringify(source)} -DestinationPath ${JSON.stringify(staging)} -Force`;
    execFile('powershell.exe',['-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-Command',ps],{windowsHide:true,timeout:30000},(err)=>{
      if(err){fs.rmSync(staging,{recursive:true,force:true});return reject(err);}
      try{
        const manifest=JSON.parse(fs.readFileSync(path.join(staging,'manifest.json'),'utf8')); if(manifest.type!=='lycan-app'||!safeId(manifest.id)||CORE_IDS.has(manifest.id))throw new Error('Invalid or reserved LYPKG'); normalizePermissions(manifest); quota(manifest);
        const checks=fs.readFileSync(path.join(staging,'checksums.sha256'),'utf8').split(/\r?\n/).map(x=>x.trim()).filter(Boolean);
        for(const line of checks){const m=line.match(/^([a-f0-9]{64})\s+(.+)$/i);if(!m)throw new Error('Invalid checksums.sha256');const rel=m[2].replace(/^\*/,'').replace(/\\/g,'/');if(rel.includes('..'))throw new Error('Unsafe package path');const f=path.resolve(staging,rel);if(!f.startsWith(path.resolve(staging)+path.sep)||!fs.existsSync(f)||sha256(f).toLowerCase()!==m[1].toLowerCase())throw new Error(`Checksum failed: ${rel}`);}
        const dest=path.join(packageRoot(),manifest.id); fs.rmSync(dest,{recursive:true,force:true}); fs.mkdirSync(dest,{recursive:true}); fs.cpSync(path.join(staging,'app'),path.join(dest,'app'),{recursive:true}); fs.copyFileSync(path.join(staging,'manifest.json'),path.join(dest,'manifest.json')); fs.copyFileSync(path.join(staging,'checksums.sha256'),path.join(dest,'checksums.sha256')); registerPackage(manifest); resolve({ok:true,id:manifest.id,name:manifest.name,version:manifest.version});
      }catch(e){reject(e);}finally{fs.rmSync(staging,{recursive:true,force:true});}
    });
  });
}
function downloadPackage(url){
  return new Promise((resolve,reject)=>{const u=new URL(url);if(u.protocol!=='https:')return reject(new Error('Only HTTPS repository URLs are allowed'));https.get(u,res=>{if(res.statusCode>=300&&res.statusCode<400&&res.headers.location){res.resume();return downloadPackage(new URL(res.headers.location,u).toString()).then(resolve,reject);}if(res.statusCode!==200){res.resume();return reject(new Error(`HTTP ${res.statusCode}`));}const chunks=[];let size=0;res.on('data',b=>{size+=b.length;if(size>50*1024*1024)res.destroy(new Error('Package exceeds 50 MB'));else chunks.push(b);});res.on('end',async()=>{try{const f=path.join(app.getPath('temp'),`lycan-${Date.now()}.lypkg`);fs.writeFileSync(f,Buffer.concat(chunks));const r=await installPackage(f);fs.rmSync(f,{force:true});resolve(r);}catch(e){reject(e);}});res.on('error',reject);}).on('error',reject);});
}
function launchPackage(id){
  const p=packageEntry(id), partition=`persist:lycan-${id}`, ses=session.fromPartition(partition), network=p.permissions.includes('network');
  ses.webRequest.onBeforeRequest({urls:['*://*/*']},(details,cb)=>{let protocol='';try{protocol=new URL(details.url).protocol;}catch{}cb({cancel:(protocol==='http:'||protocol==='https:')&&!network});});
  const win=new BrowserWindow({width:1120,height:760,minWidth:720,minHeight:480,backgroundColor:'#05070a',title:`LYCAN — ${p.manifest.name}`,webPreferences:{preload:path.join(__dirname,'package-preload.js'),contextIsolation:true,nodeIntegration:false,sandbox:true,partition,devTools:!app.isPackaged}});
  packageContexts.set(win.webContents.id,{id,dir:p.dir,permissions:p.permissions,window:win}); vm.cmd(`open ${id}`);
  win.webContents.setWindowOpenHandler(({url})=>{if(p.permissions.includes('external')){shell.openExternal(url);return{action:'deny'};}return{action:network?'allow':'deny'};});
  win.on('closed',()=>{packageContexts.delete(win.webContents.id);try{vm.cmd(`close ${id}`);}catch{}}); win.loadFile(p.file); return{ok:true,id,name:p.manifest.name,version:p.manifest.version,permissions:p.permissions};
}

app.whenReady().then(()=>{vm.boot();createWindow();app.on('activate',()=>mainWindow?.show());});
app.on('before-quit',()=>{isQuitting=true;});

ipcMain.handle('lycan:command',(_e,c)=>{try{return vm.cmd(String(c||''));}catch(e){throw new Error(e.message||String(e));}});
ipcMain.handle('lycan:open-gecko',(_e,url)=>openGecko(url));
ipcMain.handle('lycan:list-packages',()=>listPackages());
ipcMain.handle('lycan:install-package',async()=>{const r=await dialog.showOpenDialog({properties:['openFile'],filters:[{name:'LYCAN Package',extensions:['lypkg']}]});if(r.canceled||!r.filePaths[0])return{canceled:true};try{return await installPackage(r.filePaths[0]);}catch(e){return{ok:false,error:e.message};}});
ipcMain.handle('lycan:download-package',async(_e,url)=>{try{return await downloadPackage(url);}catch(e){return{ok:false,error:e.message};}});
ipcMain.handle('lycan:launch-package',(_e,id)=>{try{return launchPackage(String(id||''));}catch(e){return{ok:false,error:e.message};}});
ipcMain.on('lycan:window',(_e,action)=>{if(action==='minimize')mainWindow?.minimize();if(action==='maximize'){if(mainWindow?.isMaximized())mainWindow.unmaximize();else mainWindow?.maximize();}if(action==='close')mainWindow?.hide();if(action==='show')mainWindow?.show();if(action==='quit'){isQuitting=true;app.quit();}});
