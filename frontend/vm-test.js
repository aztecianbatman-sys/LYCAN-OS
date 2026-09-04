const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { LycanVM } = require('./vm');

const root = fs.mkdtempSync(path.join(os.tmpdir(), 'lycan-vm-test-'));
try {
  const vm = new LycanVM(root);
  assert.match(vm.boot(), /LYCAN VM ONLINE/);
  assert.match(vm.cmd('ping'), /PONG/);
  assert.match(vm.cmd('version'), /ELECTRON RUNTIME/);
  assert.match(vm.cmd('ls /home'), /documents/);
  assert.match(vm.cmd('write /home/documents/proof.txt hello-from-lycan'), /WROTE/);
  assert.match(vm.cmd('ls /home/documents'), /proof\.txt/);
  const before = vm.cmd('diagnostics');
  assert.match(before, /GUEST FILES\s+1/);
  assert.match(vm.cmd('snapshot create test'), /SNAPSHOT CREATED test/);
  assert.match(vm.cmd('write /home/documents/proof.txt changed'), /WROTE/);
  assert.match(vm.cmd('snapshot restore test'), /SNAPSHOT RESTORED test/);
  assert.strictEqual(fs.readFileSync(path.join(root, 'home', 'documents', 'proof.txt'), 'utf8'), 'hello-from-lycan');
  assert.match(vm.cmd('network off'), /NETWORK OFFLINE/);
  assert.match(vm.cmd('network status'), /NETWORK OFFLINE/);
  assert.match(vm.cmd('network on'), /NETWORK ONLINE/);
  assert.match(vm.cmd('open demo'), /OPEN demo PID=/);
  assert.match(vm.cmd('suspend demo'), /SUSPEND demo/);
  assert.match(vm.cmd('resume demo'), /RESUME demo/);
  assert.match(vm.cmd('close demo'), /CLOSE demo/);
  console.log('LYCAN pure Electron VM test: PASS');
} finally {
  fs.rmSync(root, { recursive: true, force: true });
}
