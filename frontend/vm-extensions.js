const fs = require('fs');
const path = require('path');
const { LycanVM } = require('./vm');

function guest(vm, input = '/home') {
  return vm.guestPath(input);
}

function safeName(value) {
  return String(value || '').replace(/[^a-zA-Z0-9._-]/g, '_').slice(0, 160);
}

function extendVM() {
  if (LycanVM.prototype.__lycanExtended) return;
  const originalCmd = LycanVM.prototype.cmd;

  LycanVM.prototype.cmd = function(command) {
    const line = String(command || '').trim();
    const [verb, ...rest] = line.split(/\s+/);
    const arg = rest.join(' ').trim();

    switch (String(verb || '').toLowerCase()) {
      case 'pwd':
        return '/home';
      case 'whoami':
        return 'ares';
      case 'hostname':
        return 'lycan-guest';
      case 'date':
        return new Date().toISOString();
      case 'uptime': {
        const started = this.state.bootTime ? Date.parse(this.state.bootTime) : Date.now();
        const seconds = Math.max(0, Math.floor((Date.now() - started) / 1000));
        return `UPTIME ${seconds}s\nHOSTNAME lycan-guest\nINIT ARES`;
      }
      case 'echo':
        return arg;
      case 'env':
        return 'USER=ares\nHOME=/home\nSHELL=/bin/lysh\nHOSTNAME=lycan-guest\nLANG=en_US.UTF-8\nLYCAN_VERSION=2.1.0';
      case 'df': {
        const bytes = this.directoryBytes(this.root);
        return `FILESYSTEM  LYFS\nUSED        ${bytes} bytes\nROOT        /\nMODE        GUEST_RW\nHOST        BLOCKED`;
      }
      case 'mkdir': {
        const target = guest(this, rest[0] || '');
        fs.mkdirSync(target, { recursive: false });
        return `CREATED DIRECTORY ${rest[0]}`;
      }
      case 'touch': {
        const target = guest(this, rest[0] || '');
        fs.mkdirSync(path.dirname(target), { recursive: true });
        if (!fs.existsSync(target)) fs.writeFileSync(target, '', 'utf8');
        else fs.utimesSync(target, new Date(), new Date());
        return `TOUCHED ${rest[0]}`;
      }
      case 'cat': {
        const target = guest(this, rest[0] || '');
        if (!fs.existsSync(target)) throw new Error(`Path not found: ${rest[0]}`);
        if (!fs.statSync(target).isFile()) throw new Error('CAT requires a file');
        return fs.readFileSync(target, 'utf8');
      }
      case 'rm': {
        const target = guest(this, rest[0] || '');
        if (!fs.existsSync(target)) throw new Error(`Path not found: ${rest[0]}`);
        const relative = path.relative(this.root, target).replace(/\\/g, '/');
        if (!relative || relative === 'home' || relative === 'system') throw new Error('Protected guest path');
        fs.rmSync(target, { recursive: true, force: true });
        return `REMOVED ${rest[0]}`;
      }
      case 'mv': {
        if (rest.length < 2) throw new Error('Usage: mv <source> <destination>');
        const source = guest(this, rest[0]);
        const destination = guest(this, rest[1]);
        if (!fs.existsSync(source)) throw new Error(`Path not found: ${rest[0]}`);
        fs.mkdirSync(path.dirname(destination), { recursive: true });
        fs.renameSync(source, destination);
        return `MOVED ${rest[0]} -> ${rest[1]}`;
      }
      case 'cp': {
        if (rest.length < 2) throw new Error('Usage: cp <source> <destination>');
        const source = guest(this, rest[0]);
        const destination = guest(this, rest[1]);
        if (!fs.existsSync(source)) throw new Error(`Path not found: ${rest[0]}`);
        fs.mkdirSync(path.dirname(destination), { recursive: true });
        const stat = fs.statSync(source);
        if (stat.isDirectory()) fs.cpSync(source, destination, { recursive: true });
        else fs.copyFileSync(source, destination);
        return `COPIED ${rest[0]} -> ${rest[1]}`;
      }
      case 'clear':
        return '\u0000CLEAR_TERMINAL';
      case 'shutdown':
        this.state.power = 'HALTED';
        this.persist();
        return 'SYSTEM HALTED\nARES OFFLINE\nELECTRON SHELL REMAINS AVAILABLE';
      case 'reboot':
        this.state.power = 'ONLINE';
        this.state.bootTime = new Date().toISOString();
        this.persist();
        return 'REBOOTING GUEST...\nARES CORE ONLINE\nLYFS ONLINE\nVNET0 ONLINE';
      case 'sysinfo':
        return [
          'LYCAN GUEST SYSTEM',
          '-------------------',
          `VERSION     2.1.0`,
          `HOST        ${process.platform}`,
          'KERNEL      ELECTRON GUEST RUNTIME',
          'INIT        ARES',
          'FILESYSTEM  LYFS',
          'NETWORK     VNET0',
          `RAM         ${this.pageCount * this.pageSize} bytes`,
          `PROCESSES   ${this.processes.size}`
        ].join('\n');
      case 'mkfile': {
        const name = safeName(rest[0]);
        if (!name) throw new Error('Usage: mkfile <name> [text]');
        const p = guest(this, `/home/${name}`);
        fs.writeFileSync(p, rest.slice(1).join(' '), 'utf8');
        return `CREATED /home/${name}`;
      }
      default:
        return originalCmd.call(this, command);
    }
  };

  const originalEnsure = LycanVM.prototype.ensure;
  LycanVM.prototype.ensure = function() {
    originalEnsure.call(this);
    for (const dir of ['etc', 'tmp', 'var', 'var/log']) {
      fs.mkdirSync(path.join(this.root, dir), { recursive: true });
    }
    if (!this.state.bootTime) this.state.bootTime = new Date().toISOString();
    if (!this.state.power) this.state.power = 'ONLINE';
    this.persist();
  };

  LycanVM.prototype.__lycanExtended = true;
}

extendVM();
module.exports = { extendVM };
