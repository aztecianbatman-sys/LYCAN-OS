const fs = require('fs');
const path = require('path');

class LycanVM {
  constructor(root) {
    this.root = path.resolve(root);
    this.home = path.join(this.root, 'home');
    this.system = path.join(this.root, 'system');
    this.stateFile = path.join(this.system, 'state.json');
    this.appsFile = path.join(this.system, 'apps.json');
    this.snapshotDir = path.join(this.system, 'snapshots');
    this.usersFile = path.join(this.system, 'users.json');
    this.servicesFile = path.join(this.system, 'services.json');
    this.logFile = path.join(this.root, 'var', 'log', 'lycan.log');
    this.processes = new Map();
    this.nextPid = 100;
    this.pageSize = 4096;
    this.pageCount = 256;
    this.pages = new Uint8Array(this.pageCount);
    this.booted = false;
    this.currentUser = 'guest';
    this.users = {};
    this.services = {};
    this.state = {
      version: '2.2.0',
      hostname: 'lycan-guest',
      bootId: 0,
      bootedAt: null,
      power: 'RUNNING',
      network: { online: true, interface: 'VNET0', address: '10.42.0.2', gateway: '10.42.0.1', dns: '10.42.0.1' },
      memoryPages: this.pageCount,
      createdAt: new Date().toISOString()
    };
    this.apps = {};
    this.processes.set(1, { pid: 1, id: 'ares-core', name: 'ARES Core', state: 'RUNNING', pages: 8, startPage: 0 });
    this.processes.set(2, { pid: 2, id: 'lyfs', name: 'LYFS', state: 'RUNNING', pages: 6, startPage: 8 });
    this.processes.set(3, { pid: 3, id: 'vnet', name: 'VNET0', state: 'RUNNING', pages: 4, startPage: 14 });
    for (let i = 0; i < 18; i++) this.pages[i] = 1;
  }

  ensure() {
    for (const dir of ['home','home/documents','home/downloads','home/desktop','etc','tmp','var','var/log','var/run','bin']) fs.mkdirSync(path.join(this.root, dir), { recursive: true });
    fs.mkdirSync(this.system, { recursive: true });
    fs.mkdirSync(this.snapshotDir, { recursive: true });
    if (fs.existsSync(this.stateFile)) { try { this.state = { ...this.state, ...JSON.parse(fs.readFileSync(this.stateFile, 'utf8')) }; } catch {} }
    if (fs.existsSync(this.appsFile)) { try { this.apps = JSON.parse(fs.readFileSync(this.appsFile, 'utf8')) || {}; } catch { this.apps = {}; } }
    if (fs.existsSync(this.usersFile)) { try { this.users = JSON.parse(fs.readFileSync(this.usersFile, 'utf8')) || {}; } catch { this.users = {}; } }
    if (fs.existsSync(this.servicesFile)) { try { this.services = JSON.parse(fs.readFileSync(this.servicesFile, 'utf8')) || {}; } catch { this.services = {}; } }
    if (!this.users.guest) this.users.guest = { name: 'Guest', uid: 1000, groups: ['users'], home: '/home' };
    if (!this.users.root) this.users.root = { name: 'root', uid: 0, groups: ['root','users'], home: '/root' };
    for (const [id, defaults] of Object.entries({ ares:{state:'RUNNING',enabled:true}, lyfs:{state:'RUNNING',enabled:true}, vnet:{state:this.state.network.online?'RUNNING':'STOPPED',enabled:true}, logger:{state:'RUNNING',enabled:true} })) {
      if (!this.services[id]) this.services[id] = defaults;
    }
    this.persist();
    this.log('kernel: guest filesystem initialized');
  }

  persist() {
    fs.mkdirSync(this.system, { recursive: true });
    const atomic = (file, value) => { const tmp = `${file}.tmp`; fs.writeFileSync(tmp, JSON.stringify(value, null, 2)); fs.renameSync(tmp, file); };
    atomic(this.stateFile, this.state);
    atomic(this.appsFile, this.apps);
    atomic(this.usersFile, this.users);
    atomic(this.servicesFile, this.services);
  }

  log(message) {
    try { fs.mkdirSync(path.dirname(this.logFile), { recursive: true }); fs.appendFileSync(this.logFile, `[${new Date().toISOString()}] ${message}\n`); } catch {}
  }

  boot() {
    this.ensure();
    this.booted = true;
    this.state.power = 'RUNNING';
    this.state.bootId = Number(this.state.bootId || 0) + 1;
    this.state.bootedAt = new Date().toISOString();
    for (const id of Object.keys(this.services)) this.services[id].state = this.services[id].enabled ? 'RUNNING' : 'STOPPED';
    this.services.vnet.state = this.state.network.online ? 'RUNNING' : 'STOPPED';
    this.persist();
    this.log(`boot: bootId=${this.state.bootId}`);
    return 'LYCAN VM ONLINE\nARES CORE ONLINE\nLYFS ONLINE\nVNET0 ONLINE\nLYSH ONLINE';
  }

  shutdown() { this.state.power = 'HALTED'; this.booted = false; this.log('power: shutdown'); this.persist(); return 'LYCAN POWER OFFLINE'; }
  reboot() { this.log('power: reboot'); this.state.power = 'REBOOTING'; this.persist(); this.booted = false; return this.boot(); }

  guestPath(input = '/home') {
    let raw = String(input || '/home').replace(/\\/g, '/');
    if (!raw.startsWith('/')) raw = `/home/${raw}`;
    const rel = path.posix.normalize(raw).replace(/^\/+/, '');
    if (rel === '..' || rel.startsWith('../') || rel.includes('/../')) throw new Error('Guest path escaped LYFS boundary');
    const out = path.resolve(this.root, rel);
    if (!(out === this.root || out.startsWith(`${this.root}${path.sep}`))) throw new Error('Guest path escaped LYFS boundary');
    return out;
  }

  formatTree(dir, prefix = '') {
    const entries = fs.readdirSync(dir, { withFileTypes: true }).sort((a, b) => a.name.localeCompare(b.name));
    return entries.map((entry, i) => {
      const last = i === entries.length - 1, branch = `${prefix}${last ? '└── ' : '├── '}`;
      if (entry.isDirectory()) return `${branch}${entry.name}/\n${this.formatTree(path.join(dir, entry.name), `${prefix}${last ? '    ' : '│   '}`)}`;
      return `${branch}${entry.name}`;
    }).join('\n');
  }

  ls(target) {
    const dir = this.guestPath(target || '/home');
    if (!fs.existsSync(dir)) throw new Error(`Path not found: ${target}`);
    if (fs.statSync(dir).isFile()) return path.posix.relative(this.root, dir).replace(/\\/g, '/');
    return fs.readdirSync(dir, { withFileTypes: true }).sort((a, b) => a.name.localeCompare(b.name)).map(x => `${x.isDirectory() ? '[DIR] ' : '      '}${x.name}`).join('\n') || '(empty)';
  }

  cat(file) { const p = this.guestPath(file); if (!fs.existsSync(p) || !fs.statSync(p).isFile()) throw new Error(`File not found: ${file}`); return fs.readFileSync(p, 'utf8'); }
  touch(file) { const p = this.guestPath(file); fs.mkdirSync(path.dirname(p), { recursive: true }); if (!fs.existsSync(p)) fs.writeFileSync(p, ''); this.log(`fs: touch ${file}`); return `TOUCHED ${file}`; }
  mkdir(dir) { const p = this.guestPath(dir); fs.mkdirSync(p, { recursive: true }); this.log(`fs: mkdir ${dir}`); return `CREATED ${dir}`; }
  rm(file) { const p = this.guestPath(file); if (!fs.existsSync(p)) throw new Error(`Path not found: ${file}`); fs.rmSync(p, { recursive: true, force: true }); this.log(`fs: rm ${file}`); return `REMOVED ${file}`; }
  mv(src,dst) { const a=this.guestPath(src),b=this.guestPath(dst); if(!fs.existsSync(a)) throw new Error(`Path not found: ${src}`); fs.mkdirSync(path.dirname(b),{recursive:true}); fs.renameSync(a,b); this.log(`fs: mv ${src} ${dst}`); return `MOVED ${src} -> ${dst}`; }
  cp(src,dst) { const a=this.guestPath(src),b=this.guestPath(dst); if(!fs.existsSync(a)) throw new Error(`Path not found: ${src}`); fs.mkdirSync(path.dirname(b),{recursive:true}); fs.cpSync(a,b,{recursive:true}); this.log(`fs: cp ${src} ${dst}`); return `COPIED ${src} -> ${dst}`; }
  writeGuest(file, text) { const p=this.guestPath(file); fs.mkdirSync(path.dirname(p),{recursive:true}); fs.writeFileSync(p,String(text),'utf8'); this.log(`fs: write ${file}`); return `WROTE ${file}`; }

  alloc(id, pages=1) { let start=-1; for(let i=0;i<=this.pageCount-pages;i++){let free=true;for(let j=0;j<pages;j++)if(this.pages[i+j]){free=false;break;}if(free){start=i;break;}}if(start<0)throw new Error('Out of guest memory');for(let i=0;i<pages;i++)this.pages[start+i]=1;return{id,start,pages}; }
  free(start,pages){for(let i=0;i<pages;i++)this.pages[start+i]=0;}

  cmd(command) {
    if (!this.booted && !/^\s*(ping|version|boot)\b/i.test(String(command||''))) this.boot();
    const line=String(command||'').trim(); if(!line)return '';
    const [verb,...rest]=line.split(/\s+/), arg=rest.join(' '), sub=(rest[0]||'').toLowerCase();
    switch(verb.toLowerCase()) {
      case 'boot': return this.boot();
      case 'ping': return 'PONG ARES/JVM 2.2';
      case 'version': return 'LYCAN OS 2.2.0\nARES JAVASCRIPT VIRTUAL CORE 2.2\nGUEST ABI 5\nELECTRON RUNTIME';
      case 'help': return 'help | version | boot | memory | vm pages | ps | top | ls <path> | tree <path> | pwd | cat <file> | touch <file> | mkdir <dir> | rm <path> | mv <src> <dst> | cp <src> <dst> | write <path> <text> | df | uname | whoami | hostname | uptime | sysinfo | users | user add|remove|list | login <user> | services | service start|stop|restart|status <id> | network status|on|off | web status | diagnostics | logs [lines] | apps | app register|unregister | open|close|suspend|resume|crash <id> | snapshots | snapshot create|restore|info|delete | reboot | shutdown';
      case 'memory': { const used=[...this.pages].filter(Boolean).length; return `PAGE SIZE  ${this.pageSize}\nTOTAL PAGES  ${this.pageCount}\nUSED PAGES   ${used}\nFREE PAGES   ${this.pageCount-used}\nTOTAL RAM    ${this.pageCount*this.pageSize}`; }
      case 'vm': if(sub==='pages')return[...this.pages].map((v,i)=>`${String(i).padStart(3,'0')}  ${v?'ALLOC':'FREE'}`).join('\n');return 'VM STATUS ONLINE';
      case 'ps': return [...this.processes.values()].map(p=>`${String(p.pid).padEnd(5)} ${p.id.padEnd(20)} ${p.state.padEnd(9)} ${String(p.pages).padStart(3)}`).join('\n');
      case 'top': return this.cmd('ps') + `\n\nLOAD  ${(this.processes.size/16).toFixed(2)}\nMEMORY ${(this.cmd('memory').match(/USED PAGES\s+(\d+)/)||[])[1]||0}/${this.pageCount} pages`;
      case 'ls': return this.ls(rest[0]||'/home');
      case 'tree': return this.formatTree(this.guestPath(rest[0]||'/home'))||'(empty)';
      case 'pwd': return '/'+path.posix.relative(this.root,this.home).replace(/\\/g,'/');
      case 'cat': return this.cat(rest[0]||'');
      case 'touch': return this.touch(rest[0]||'');
      case 'mkdir': return this.mkdir(rest[0]||'');
      case 'rm': return this.rm(rest[0]||'');
      case 'mv': return this.mv(rest[0]||'',rest[1]||'');
      case 'cp': return this.cp(rest[0]||'',rest[1]||'');
      case 'write': { const m=line.match(/^write\s+(\S+)\s+([\s\S]*)$/i);if(!m)throw new Error('Usage: write <path> <text>');return this.writeGuest(m[1],m[2]); }
      case 'df': return `FILESYSTEM  LYFS\nTOTAL  ${Math.round(this.directoryBytes(this.root)/1024)+1024} KB\nUSED   ${Math.round(this.directoryBytes(this.root)/1024)} KB\nFREE   guest-managed`;
      case 'uname': return 'LYCAN/2.2 ARES/5 x64 ELECTRON';
      case 'whoami': return this.currentUser;
      case 'hostname': return this.state.hostname;
      case 'uptime': return this.state.bootedAt ? `UP ${Math.max(0,Math.floor((Date.now()-Date.parse(this.state.bootedAt))/1000))} seconds` : 'DOWN';
      case 'sysinfo': return `HOSTNAME  ${this.state.hostname}\nUSER      ${this.currentUser}\nVERSION   2.2.0\nPOWER     ${this.state.power}\nNETWORK   ${this.state.network.online?'ONLINE':'OFFLINE'}\nPROCESSES ${this.processes.size}\nSERVICES  ${Object.keys(this.services).length}`;
      case 'network': { if(sub==='on'){this.state.network.online=true;this.services.vnet.state='RUNNING';}else if(sub==='off'){this.state.network.online=false;this.services.vnet.state='STOPPED';}if(sub==='status')return `NETWORK ${this.state.network.online?'ONLINE':'OFFLINE'}\nINTERFACE  ${this.state.network.interface}\nADDRESS    ${this.state.network.address}\nGATEWAY    ${this.state.network.gateway}\nDNS        ${this.state.network.dns}`;this.persist();this.log(`network: ${this.state.network.online?'on':'off'}`);return `NETWORK ${this.state.network.online?'ONLINE':'OFFLINE}`; }
      case 'web': return `WEB BRIDGE READY\nENGINE GECKO\nNETWORK ${this.state.network.online?'ONLINE':'OFFLINE'}`;
      case 'diagnostics': return this.diagnostics();
      case 'logs': { const lines=fs.existsSync(this.logFile)?fs.readFileSync(this.logFile,'utf8').trimEnd().split(/\r?\n/):[];const n=Math.min(200,Math.max(1,Number(rest[0]||30)));return lines.slice(-n).join('\n')||'(no logs)'; }
      case 'users': return this.userCommand(rest);
      case 'login': if(!rest[0]||!this.users[rest[0]])throw new Error('Unknown user');this.currentUser=rest[0];this.log(`auth: login ${this.currentUser}`);return `LOGGED IN AS ${this.currentUser}`;
      case 'services': return this.serviceCommand(rest);
      case 'apps': { const list=Object.values(this.apps); return list.length?list.map(a=>`${a.id}  v${a.version}  ${a.state||'REGISTERED'}`).join('\n'):'NO THIRD-PARTY APPS REGISTERED'; }
      case 'app': return this.appCommand(rest);
      case 'open': return this.processCommand('open',rest);
      case 'close': return this.processCommand('close',rest);
      case 'suspend': return this.processCommand('suspend',rest);
      case 'resume': return this.processCommand('resume',rest);
      case 'crash': return this.processCommand('crash',rest);
      case 'snapshot':
      case 'snapshots': return this.snapshotCommand(verb.toLowerCase()==='snapshot'?rest:[]);
      case 'reboot': return this.reboot();
      case 'shutdown': return this.shutdown();
      default: return `UNKNOWN COMMAND: ${verb}`;
    }
  }

  userCommand(args){ const sub=(args[0]||'').toLowerCase(); if(sub==='list'||!sub)return Object.values(this.users).map(u=>`${u.uid}\t${u.name}\t${u.home}\t${u.groups.join(',')}`).join('\n'); if(sub==='add'){const id=args[1];if(!/^[a-z][a-z0-9_-]{1,31}$/.test(id))throw new Error('Invalid username');if(this.users[id])throw new Error('User exists');this.users[id]={name:id,uid:Math.max(...Object.values(this.users).map(u=>u.uid))+1,groups:['users'],home:`/home/${id}`};fs.mkdirSync(this.guestPath(this.users[id].home),{recursive:true});this.persist();this.log(`auth: user add ${id}`);return `USER CREATED ${id}`;}if(sub==='remove'){const id=args[1];if(!id||id==='root'||id==='guest')throw new Error('Protected user');delete this.users[id];this.persist();return `USER REMOVED ${id}`;}throw new Error('users: list|add|remove'); }
  serviceCommand(args){ const sub=(args[0]||'status').toLowerCase(),id=args[1];if(sub==='status')return Object.entries(this.services).map(([k,v])=>`${k.padEnd(12)} ${v.state.padEnd(9)} ${v.enabled?'ENABLED':'DISABLED'}`).join('\n');if(!id||!this.services[id])throw new Error('Service not found');if(sub==='start'||sub==='restart'){this.services[id].state='RUNNING';if(id==='vnet')this.state.network.online=true;this.persist();this.log(`service: ${sub} ${id}`);return `SERVICE ${sub.toUpperCase()} ${id}`;}if(sub==='stop'){this.services[id].state='STOPPED';if(id==='vnet')this.state.network.online=false;this.persist();this.log(`service: stop ${id}`);return `SERVICE STOP ${id}`;}throw new Error('service: start|stop|restart|status <id>'); }

  diagnostics(){const used=[...this.pages].filter(Boolean).length;let files=0;const walk=d=>{for(const e of fs.readdirSync(d,{withFileTypes:true})){const p=path.join(d,e.name);try{if(e.isDirectory())walk(p);else files++;}catch{}}};walk(this.root);return `LYFS  ISOLATED\nHOST ACCESS  DENIED\nNETWORK  ${this.state.network.online?'ONLINE':'OFFLINE'}\nUSER      ${this.currentUser}\nPROCESSES  ${this.processes.size}\nSERVICES  ${Object.keys(this.services).length}\nPACKAGES  ${Object.keys(this.apps).length}\nGUEST FILES  ${files}\nGUEST DATA  ${Math.round(this.directoryBytes(this.root)/1024)} KB\nRAM USED  ${used}/${this.pageCount} PAGES`;}
  directoryBytes(dir){let total=0;for(const entry of fs.readdirSync(dir,{withFileTypes:true})){const p=path.join(dir,entry.name);try{total+=entry.isDirectory()?this.directoryBytes(p):fs.statSync(p).size;}catch{}}return total;}
  appCommand(args){const sub=(args[0]||'').toLowerCase(),id=args[1];if(sub==='register'){const[version='1.0.0',quota='16',perms='']=args.slice(2);if(!/^[a-z0-9][a-z0-9._-]{1,63}$/.test(id))throw new Error('Invalid app id');this.apps[id]={id,version,quotaMB:Number(quota)||16,permissions:perms?perms.split(','):[],state:'REGISTERED'};this.persist();return`APP REGISTERED ${id} ${version}`;}if(sub==='unregister'){if(!this.apps[id])throw new Error('APP NOT REGISTERED');delete this.apps[id];this.persist();return`APP UNREGISTERED ${id}`;}return'APP COMMANDS: register <id> <version> <quota> <permissions>, unregister <id>';}
  processCommand(action,args){const id=args[0];if(!id)throw new Error(`${action} requires an id`);let p=[...this.processes.values()].find(x=>x.id===id);if(!p){if(action!=='open')throw new Error('PROCESS NOT FOUND');const a=this.alloc(id,4),pid=this.nextPid++;p={pid,id,name:id,state:'RUNNING',pages:a.pages,startPage:a.start};this.processes.set(pid,p);return`OPEN ${id} PID=${pid}`;}if(action==='close'){if(p.pid<=3)return'CORE PROCESS CANNOT CLOSE';this.free(p.startPage||0,p.pages||0);this.processes.delete(p.pid);return`CLOSE ${id}`;}if(action==='suspend'){p.state='SUSPENDED';return`SUSPEND ${id}`;}if(action==='resume'){p.state='RUNNING';return`RESUME ${id}`;}if(action==='crash'){p.state='CRASHED';return`CRASH ${id} ${args.slice(1).join('_')||'unknown'}`;}return`OPEN ${id} PID=${p.pid}`;}
  snapshotCommand(args){const sub=(args[0]||'').toLowerCase();if(sub==='create'){const name=(args.slice(1).join('-')||`checkpoint-${Date.now()}`).replace(/[^a-zA-Z0-9._-]/g,'-');const target=path.join(this.snapshotDir,name+'.json');const payload={format:3,createdAt:new Date().toISOString(),currentUser:this.currentUser,state:this.state,users:this.users,services:this.services,apps:this.apps,pages:Array.from(this.pages),files:this.captureFiles()};fs.writeFileSync(target,JSON.stringify(payload));return`SNAPSHOT CREATED ${name}`;}const snaps=fs.readdirSync(this.snapshotDir).filter(x=>x.endsWith('.json')).sort();if(sub==='restore'){const name=args[1];if(!name)throw new Error('RESTORE requires snapshot name');const file=path.join(this.snapshotDir,name.endsWith('.json')?name:`${name}.json`),data=JSON.parse(fs.readFileSync(file,'utf8'));this.restoreFiles(data.files||{});this.state=data.state||this.state;this.users=data.users||this.users;this.services=data.services||this.services;this.apps=data.apps||{};this.currentUser=data.currentUser||'guest';this.pages=Uint8Array.from(data.pages||this.pages);this.persist();return`SNAPSHOT RESTORED ${name.replace(/\.json$/,'')}`;}if(sub==='info'){const name=args[1];if(!name)throw new Error('INFO requires snapshot name');const file=path.join(this.snapshotDir,name.endsWith('.json')?name:`${name}.json`),s=fs.statSync(file),data=JSON.parse(fs.readFileSync(file,'utf8'));return`NAME  ${name.replace(/\.json$/,'')}\nCREATED  ${data.createdAt}\nBYTES  ${s.size}\nFILES  ${Object.keys(data.files||{}).length}\nUSER    ${data.currentUser||'guest'}`;}if(sub==='delete'){const name=args[1];if(!name)throw new Error('DELETE requires snapshot name');fs.rmSync(path.join(this.snapshotDir,name.endsWith('.json')?name:`${name}.json`));return`SNAPSHOT DELETED ${name}`;}return snaps.length?snaps.map(x=>x.replace(/\.json$/,'')).join('\n'):'NO SNAPSHOTS';}
  captureFiles(){const out={};const walk=dir=>{for(const e of fs.readdirSync(dir,{withFileTypes:true})){const p=path.join(dir,e.name),rel=path.relative(this.root,p).replace(/\\/g,'/');if(rel.startsWith('system/snapshots'))continue;if(e.isDirectory())walk(p);else{try{out[rel]=fs.readFileSync(p,'base64');}catch{}}}};walk(this.root);return out;}
  restoreFiles(files){const keep=new Set(Object.keys(files));for(const rel of Object.keys(this.captureFiles()))if(!rel.startsWith('system/snapshots')){const p=this.guestPath('/'+rel);if(!keep.has(rel))fs.rmSync(p,{force:true,recursive:true});}for(const[rel,b64]of Object.entries(files)){if(rel.startsWith('system/snapshots'))continue;const p=this.guestPath('/'+rel);fs.mkdirSync(path.dirname(p),{recursive:true});fs.writeFileSync(p,Buffer.from(b64,'base64'));}}
}
module.exports={LycanVM};
