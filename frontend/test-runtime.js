const fs = require('fs');
const os = require('os');
const path = require('path');
const assert = require('assert');
const { LycanVM } = require('./vm');
require('./vm-extensions');

const root = fs.mkdtempSync(path.join(os.tmpdir(), 'lycan-test-'));
try {
  const vm = new LycanVM(root);
  assert.match(vm.boot(), /LYCAN VM ONLINE/);
  assert.match(vm.cmd('version'), /ELECTRON RUNTIME|LYCAN OS/);
  assert.strictEqual(vm.cmd('whoami'), 'ares');
  assert.strictEqual(vm.cmd('pwd'), '/home');
  assert.match(vm.cmd('sysinfo'), /LYCAN GUEST SYSTEM/);
  vm.cmd('mkdir /home/test');
  vm.cmd('write /home/test/hello.txt hello-lycan');
  assert.strictEqual(vm.cmd('cat /home/test/hello.txt'), 'hello-lycan');
  vm.cmd('cp /home/test/hello.txt /home/test/copied.txt');
  vm.cmd('mv /home/test/copied.txt /home/test/moved.txt');
  assert.match(vm.cmd('ls /home/test'), /moved\.txt/);
  vm.cmd('rm /home/test/moved.txt');
  const snap = 'runtime-test';
  assert.match(vm.cmd(`snapshot create ${snap}`), /SNAPSHOT CREATED/);
  vm.cmd('write /home/test/restored.txt before');
  assert.match(vm.cmd(`snapshot restore ${snap}`), /SNAPSHOT RESTORED/);
  assert.throws(() => vm.cmd('cat /home/test/restored.txt'), /Path not found/);
  assert.match(vm.cmd('network status'), /NETWORK ONLINE/);
  console.log('LYCAN pure Electron runtime test: PASS');
} finally {
  fs.rmSync(root, { recursive: true, force: true });
}
