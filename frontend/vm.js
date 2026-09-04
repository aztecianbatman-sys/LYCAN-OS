const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

class LycanVM {
  constructor(root) {
    this.root = path.resolve(root);
    this.home = path.join(this.root, 'home');
    this.system = path.join(this.root, 'system');
    this.stateFile = path.join(this.system, 'state.json');
    this.appsFile = path.join(this.system, 'apps.json');
    this.snapshotDir = path.join(this.system, 'snapshots');
    this.processes = new Map();
    this.nextPid = 100;
    this.pageSize = 4096;
    this.pageCount = 256;
    this.pages = new Uint8Array(this.pageCount);
    this.booted = false;
    this.state = {
      version: '2.0.0',
      network: { online: true, interface: 'VNET0', address: '10.42.0.2', gateway: '10.42.0.1', dns: '10.42.0.1' },
      memoryPages: this.pageCount,
      openFiles: 0,
      createdAt: new Date().toISOString()
    };
    this.apps = {};
    this.processes.set(1, { pid: 1, id: 'ares-core', name: 'ARES Core', state: 'RUNNING', pages: 8 });
    this.processes.set(2, { pid: 2, id: 'lyfs', name: 'LYFS', state: 'RUNNING', pages: 6 });
    this.processes.set(3, { pid: 3, id: 'vnet', name: 'VNET0', state: 'RUNNING', pages: 4 });
  }

  ensure() {
    fs.mkdirSync(this.home, { recursive: true });
    fs.mkdirSync(this.system, { recursive: true });
    fs.mkdirSync(this.snapshotDir, { recursive: true });
    for (const dir of ['home', 'home/documents', 'home/downloads', 'home/desktop']) fs.mkdirSync(path.join(this.root, dir), { recursive: true });
    if (fs.existsSync(this.stateFile)) {
      try { this.state = { ...this.state, ...JSON.parse(fs.readFileSync(this.stateFile, 'utf8')) }; } catch {}
    }
    if (fs.existsSync(this.appsFile)) {
      try { this.apps = JSON.parse(fs.readFileSync(this.appsFile, 'utf8')) || {}; } catch { this.apps = {}; }
    }
    this.persist();
  }

  persist() {
    fs.mkdirSync(this.system, { recursive: true });
    const tmp = `${this.stateFile}.tmp`;
    fs.writeFileSync(tmp, JSON.stringify(this.state, null, 2));
    fs.renameSync(tmp, this.stateFile);
    fs.writeFileSync(this.appsFile, JSON.stringify(this.apps, null, 2));
  }

  boot() { this.ensure(); this.booted = true; return 'LYCAN VM ONLINE\nARES CORE ONLINE\nLYFS ONLINE\nVNET0 ONLINE'; }

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
    const entries = fs.readdirSync(dir, { withFileTypes: true }).sort((a,b)=>a.name.localeCompare(b.name));
    return entries.map((entry, i) => {
      const last = i === entries.length - 1;
      const branch = `${prefix}${last ? '└── ' : '├── '}`;
      if (entry.isDirectory()) return `${branch}${entry.name}/\n${this.formatTree(path.join(dir, entry.name), `${prefix}${last ? '    ' : '│   '}`)}`;
      return `${branch}${entry.name}`;
    }).join('\n');
  }

  ls(target) {
    const dir = this.guestPath(target || '/home');
    if (!fs.existsSync(dir)) throw new Error(`Path not found: ${target}`);
    const stat = fs.statSync(dir);
    if (stat.isFile()) return path.posix.relative(this.root, dir).replace(/\\/g, '/');
    return fs.readdirSync(dir, { withFileTypes: true }).sort((a,b)=>a.name.localeCompare(b.name)).map(x => `${x.isDirectory() ? '[DIR] ' : '      '}${x.name}`).join('\n') || '(empty)';
  }

  writeGuest(file, text) {
    const p = this.guestPath(file);
    fs.mkdirSync(path.dirname(p), { recursive: true });
    fs.writeFileSync(p, String(text), 'utf8');
    return `WROTE ${file}`;
  }

  alloc(id, pages = 1) {
    let start = -1;
    for (let i = 0; i <= this.pageCount - pages; i++) {
      let free = true;
      for (let j = 0; j < pages; j++) if (this.pages[i + j]) { free = false; break; }
      if (free) { start = i; break; }
    }
    if (start < 0) throw new Error('Out of guest memory');
    for (let i = 0; i < pages; i++) this.pages[start + i] = 1;
    return { id, start, pages };
  }

  free(start, pages) { for (let i = 0; i < pages; i++) this.pages[start + i] = 0; }

  cmd(command) {
    if (!this.booted) this.boot();
    const line = String(command || '').trim();
    if (!line) return '';
    const [verb, ...rest] = line.split(/\s+/);
    const arg = rest.join(' ').trim();

    switch (verb.toLowerCase()) {
      case 'ping': return 'PONG ARES/JVM 2';
      case 'version': return 'LYCAN OS 2.0.0\nARES JAVASCRIPT VIRTUAL CORE 2.0\nGUEST ABI 4\nELECTRON RUNTIME';
      case 'help': return 'help, version, memory, vm pages, ps, ls <path>, tree <path>, write <path> <text>, network status|on|off, diagnostics, apps, app register|unregister, open <id>, close <id>, suspend <id>, resume <id>, crash <id> <reason>, snapshots, snapshot create|restore|info|delete';
      case 'memory': { const used=[...this.pages].filter(Boolean).length; return `PAGE SIZE  ${this.pageSize}\nTOTAL PAGES  ${this.pageCount}\nUSED PAGES   ${used}\nFREE PAGES   ${this.pageCount-used}\nTOTAL RAM    ${this.pageCount*this.pageSize}`; }
      case 'vm': if (rest[0] === 'pages') return [...this.pages].map((v,i)=>`${String(i).padStart(3,'0')}  ${v?'ALLOC':'FREE'}`).join('\n'); return 'VM STATUS ONLINE';
      case 'ps': return [...this.processes.values()].map(p=>`${String(p.pid).padEnd(5)} ${p.id.padEnd(20)} ${p.state.padEnd(9)} ${String(p.pages).padStart(3)}`).join('\n');
      case 'ls': return this.ls(rest[0] || '/home');
      case 'tree': { const d=this.guestPath(rest[0]||'/home'); return this.formatTree(d)||'(empty)'; }
      case 'write': { const m=line.match(/^write\s+(\S+)\s+([\s\S]*)$/i); if(!m) throw new Error('Usage: write <path> <text>'); return this.writeGuest(m[1], m[2]); }
      case 'network': {
        const sub=(rest[0]||'status').toLowerCase();
        if (sub==='on') this.state.network.online=true;
        else if (sub==='off') this.state.network.online=false;
        if (sub==='status') return `NETWORK ${this.state.network.online?'ONLINE':'OFFLINE'}\nINTERFACE  ${this.state.network.interface}\nADDRESS    ${this.state.network.address}\nGATEWAY    ${this.state.network.gateway}\nDNS        ${this.state.network.dns}`;
        this.persist(); return `NETWORK ${this.state.network.online?'ONLINE':'OFFLINE'}`;
      }
      case 'diagnostics': return this.diagnostics();
      case 'apps': { const list=Object.values(this.apps); return list.length ? list.map(a=>`${a.id}  v${a.version}  ${a.state||'REGISTERED'}`).join('\n') : 'NO THIRD-PARTY APPS REGISTERED'; }
      case 'app': return this.appCommand(rest);
      case 'open': return this.processCommand('open', rest);
      case 'close': return this.processCommand('close', rest);
      case 'suspend': return this.processCommand('suspend', rest);
      case 'resume': return this.processCommand('resume', rest);
      case 'crash': return this.processCommand('crash', rest);
      case 'snapshots': return this.snapshotCommand(rest);
      default: return `UNKNOWN COMMAND: ${verb}`;
    }
  }

  diagnostics() {
    const used=[...this.pages].filter(Boolean).length;
    const files = fs.readdirSync(this.root, { recursive: true }).length;
    return `LYFS  ISOLATED\nHOST ACCESS  DENIED\nNETWORK  ${this.state.network.online?'ONLINE':'OFFLINE'}\nPROCESSES  ${this.processes.size}\nPACKAGES  ${Object.keys(this.apps).length}\nGUEST FILES  ${files}\nGUEST DATA  ${Math.round(this.directoryBytes(this.root)/1024)} KB\nRAM USED  ${used}/${this.pageCount} PAGES`;
  }

  directoryBytes(dir) {
    let total = 0;
    for (const entry of fs.readdirSync(dir, {withFileTypes:true})) {
      const p=path.join(dir,entry.name);
      try { total += entry.isDirectory()?this.directoryBytes(p):fs.statSync(p).size; } catch {}
    }
    return total;
  }

  appCommand(args) {
    const sub=(args[0]||'').toLowerCase();
    const id=args[1];
    if (sub==='register') { const [version='1.0.0', quota='16', perms=''] = args.slice(2); if(!/^[a-z0-9][a-z0-9._-]{1,63}$/.test(id)) throw new Error('Invalid app id'); this.apps[id]={id,version,quotaMB:Number(quota)||16,permissions:perms?perms.split(','):[],state:'REGISTERED'}; this.persist(); return `APP REGISTERED ${id} ${version}`; }
    if (sub==='unregister') { if(!this.apps[id]) throw new Error('APP NOT REGISTERED'); delete this.apps[id]; this.persist(); return `APP UNREGISTERED ${id}`; }
    return 'APP COMMANDS: register <id> <version> <quota> <permissions>, unregister <id>';
  }

  processCommand(action,args) {
    const id=args[0]; if(!id) throw new Error(`${action} requires an id`);
    let p=[...this.processes.values()].find(x=>x.id===id);
    if(!p){ if(action!=='open') throw new Error('PROCESS NOT FOUND'); const alloc=this.alloc(id,4); const pid=this.nextPid++; p={pid,id,name:id,state:'RUNNING',pages:alloc.pages,startPage:alloc.start}; this.processes.set(pid,p); return `OPEN ${id} PID=${pid}`; }
    if(action==='close'){ if(p.pid<=3) return 'CORE PROCESS CANNOT CLOSE'; this.free(p.startPage||0,p.pages||0);this.processes.delete(p.pid);return `CLOSE ${id}`; }
    if(action==='suspend'){p.state='SUSPENDED';return `SUSPEND ${id}`;}
    if(action==='resume'){p.state='RUNNING';return `RESUME ${id}`;}
    if(action==='crash'){p.state='CRASHED';return `CRASH ${id} ${args.slice(1).join('_')||'unknown'}`;}
    return `OPEN ${id} PID=${p.pid}`;
  }

  snapshotCommand(args) {
    const sub=(args[0]||'').toLowerCase();
    if(sub==='create'){
      const name=(args.slice(1).join('-')||`checkpoint-${Date.now()}`).replace(/[^a-zA-Z0-9._-]/g,'-');
      const target=path.join(this.snapshotDir,name+'.json');
      const payload={format:2,createdAt:new Date().toISOString(),state:this.state,apps:this.apps,pages:Array.from(this.pages),files:this.captureFiles()};
      fs.writeFileSync(target,JSON.stringify(payload)); return `SNAPSHOT CREATED ${name}`;
    }
    const snaps=fs.readdirSync(this.snapshotDir).filter(x=>x.endsWith('.json')).sort();
    if(sub==='restore'){const name=args[1];if(!name)throw new Error('RESTORE requires snapshot name');const file=path.join(this.snapshotDir,name.endsWith('.json')?name:`${name}.json`);const data=JSON.parse(fs.readFileSync(file,'utf8'));this.restoreFiles(data.files||{});this.state=data.state||this.state;this.apps=data.apps||{};this.pages=Uint8Array.from(data.pages||this.pages);this.persist();return `SNAPSHOT RESTORED ${name.replace(/\.json$/,'')}`;}
    if(sub==='info'){const name=args[1];const file=path.join(this.snapshotDir,name.endsWith('.json')?name:`${name}.json`);const s=fs.statSync(file);const data=JSON.parse(fs.readFileSync(file,'utf8'));return `NAME  ${name.replace(/\.json$/,'')}\nCREATED  ${data.createdAt}\nBYTES  ${s.size}\nFILES  ${Object.keys(data.files||{}).length}`;}
    if(sub==='delete'){const name=args[1];const file=path.join(this.snapshotDir,name.endsWith('.json')?name:`${name}.json`);fs.rmSync(file);return `SNAPSHOT DELETED ${name}`;}
    return snaps.length ? snaps.map(x=>x.replace(/\.json$/,'')).join('\n') : 'NO SNAPSHOTS';
  }

  captureFiles() {
    const out={};
    const walk=(dir)=>{for(const entry of fs.readdirSync(dir,{withFileTypes:true})){const p=path.join(dir,entry.name);const rel=path.relative(this.root,p).replace(/\\/g,'/');if(rel.startsWith('system/snapshots'))continue;if(entry.isDirectory())walk(p);else {try{out[rel]=fs.readFileSync(p,'base64');}catch{}}}};
    walk(this.root);return out;
  }
  restoreFiles(files){
    const keep=new Set(Object.keys(files));
    for(const rel of Object.keys(this.captureFiles())) if(!rel.startsWith('system/snapshots')){const p=this.guestPath('/'+rel);if(!keep.has(rel))fs.rmSync(p,{force:true,recursive:true});}
    for(const [rel,b64] of Object.entries(files)){if(rel.startsWith('system/snapshots'))continue;const p=this.guestPath('/'+rel);fs.mkdirSync(path.dirname(p),{recursive:true});fs.writeFileSync(p,Buffer.from(b64,'base64'));}
  }
}

module.exports = { LycanVM };
