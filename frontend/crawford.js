(() => {
  const QUICK = [
    ['STATUS', 'diagnostics', 'System health'],
    ['MEMORY', 'memory', 'Virtual memory'],
    ['PROCESSES', 'ps', 'Process table'],
    ['FILES', 'tree /home', 'LYFS home tree'],
    ['NETWORK', 'network', 'Network state'],
    ['SNAPSHOTS', 'snapshots', 'Saved checkpoints'],
    ['VERSION', 'version', 'Runtime identity']
  ];

  const ALIASES = new Map([
    ['status', 'diagnostics'], ['health', 'diagnostics'], ['system', 'diagnostics'],
    ['memory', 'memory'], ['ram', 'memory'], ['processes', 'ps'], ['process', 'ps'],
    ['tasks', 'ps'], ['files', 'tree /home'], ['filesystem', 'tree /home'],
    ['home', 'ls /home'], ['network', 'network'], ['net', 'network'],
    ['snapshots', 'snapshots'], ['checkpoints', 'snapshots'], ['version', 'version'],
    ['about', 'version']
  ]);

  const esc = value => String(value ?? '').replace(/[&<>\"']/g, c => ({
    '&':'&amp;', '<':'&lt;', '>':'&gt;', '\"':'&quot;', "'":'&#039;'
  }[c]));

  function commandFor(text) {
    const q = String(text || '').trim();
    if (!q) return null;
    const lower = q.toLowerCase();
    if (ALIASES.has(lower)) return ALIASES.get(lower);
    if (/^(?:ls)(?:\s+.+)?$/i.test(q)) return q;
    if (/^tree(?:\s+.+)?$/i.test(q)) return q;
    if (/^cat\s+\S+$/i.test(q)) return q;
    if (/^vm\s+(?:pages|page\s+\d+)$/i.test(q)) return q;
    if (/^app\s+state\s+[a-z0-9._-]+$/i.test(q)) return q;
    if (/^storage-(?:usage|quota)\s+[a-z0-9._-]+$/i.test(q)) return q;
    if (/^network\s+(?:status|interfaces|on|off|app\s+[a-z0-9._-]+)$/i.test(q)) return q;
    if (/^vnet\s+(?:status|interfaces|routes|dns|on|off|up\s+\w+|down\s+\w+|config\s+\w+\s+\S+\s+\S+\s+\S+)$/i.test(q)) return q;
    return null;
  }

  function buildAssistant(shell) {
    if (shell.querySelector('.crawford-assistant')) return;
    const wrap = document.createElement('section');
    wrap.className = 'crawford-assistant';
    wrap.innerHTML = `
      <div class="crawford-topbar">
        <div class="crawford-brand">
          <div class="crawford-sigil">C</div>
          <div>
            <div class="eyebrow">LYCAN CONTROL PLANE / CRAWFORD</div>
            <strong>LOCAL GUEST CONSOLE</strong>
            <span>Operational interface for the ARES virtual runtime</span>
          </div>
        </div>
        <div class="crawford-health">
          <span class="health-dot" data-crawford-health-dot></span>
          <div><b data-crawford-health>READY</b><small data-crawford-clock>--:--:--</small></div>
        </div>
      </div>

      <div class="crawford-grid">
        <aside class="crawford-sidebar">
          <div class="crawford-sidebar-title">QUICK TELEMETRY</div>
          <div class="crawford-metrics">
            <div><span>RUNTIME</span><b data-crawford-runtime>ONLINE</b></div>
            <div><span>MEMORY</span><b data-crawford-memory>—</b></div>
            <div><span>NETWORK</span><b data-crawford-network>—</b></div>
            <div><span>PROCESSES</span><b data-crawford-processes>—</b></div>
          </div>
          <div class="crawford-sidebar-title">CAPABILITIES</div>
          <div class="crawford-capabilities">
            <span>LYFS</span><span>PROCESS</span><span>SNAPSHOT</span><span>VNET</span><span>DIAGNOSTICS</span><span>SECURITY</span>
          </div>
          <div class="crawford-boundary">
            <b>SECURITY BOUNDARY</b>
            <p>Guest context only. Host filesystem, Windows processes, and host control remain unavailable to this panel.</p>
          </div>
        </aside>

        <main class="crawford-main">
          <div class="crawford-commandbar">
            <div class="crawford-quick" data-crawford-quick>
              ${QUICK.map(([label, cmd, hint]) => `<button data-crawford-command="${esc(cmd)}" title="${esc(hint)}"><span>${esc(label)}</span></button>`).join('')}
            </div>
            <button class="crawford-refresh" data-crawford-refresh title="Refresh telemetry">↻</button>
          </div>

          <div class="crawford-terminal" data-crawford-log aria-live="polite">
            <div class="crawford-line system"><span>CRAWFORD</span><b>Control console initialized.</b></div>
            <div class="crawford-line system"><span>BOUNDARY</span><b>Only approved LYCAN guest commands are exposed.</b></div>
          </div>

          <form class="crawford-input-row" data-crawford-form>
            <span>COMMAND //</span>
            <input data-crawford-input aria-label="Ask Crawford" autocomplete="off" spellcheck="false" placeholder="status, memory, ps, tree /home, network…">
            <button type="submit">EXECUTE</button>
          </form>

          <div class="crawford-footer">
            <div><span class="led"></span> LOCAL GUEST MODE</div>
            <div>NO HOST CONTROL</div>
            <div>COMMAND FILTER ACTIVE</div>
            <div>ESC CLEARS INPUT</div>
          </div>
        </main>
      </div>
    `;
    shell.appendChild(wrap);

    const log = wrap.querySelector('[data-crawford-log]');
    const input = wrap.querySelector('[data-crawford-input]');
    const health = wrap.querySelector('[data-crawford-health]');
    const healthDot = wrap.querySelector('[data-crawford-health-dot]');
    let busy = false;

    const setHealth = (label, state = 'ready') => {
      health.textContent = label;
      healthDot.dataset.state = state;
    };

    const appendLine = (kind, label, text) => {
      const el = document.createElement('div');
      el.className = `crawford-line ${kind}`;
      el.innerHTML = `<span>${esc(label)}</span><b>${esc(text)}</b>`;
      log.appendChild(el);
      log.scrollTop = log.scrollHeight;
      return el;
    };

    const appendOutput = text => {
      const el = document.createElement('pre');
      el.className = 'crawford-output';
      el.textContent = String(text ?? '');
      log.appendChild(el);
      log.scrollTop = log.scrollHeight;
    };

    async function run(text) {
      const label = String(text || '').trim();
      const command = commandFor(label);
      if (!command) {
        appendLine('warn', 'CRAWFORD', 'Command not allowed. Try one of the approved telemetry commands or a filtered LYFS/VNET query.');
        input.focus();
        return;
      }
      if (busy) {
        appendLine('warn', 'CRAWFORD', 'A guest query is already running.');
        return;
      }
      busy = true;
      appendLine('user', 'YOU', label);
      const pending = appendLine('pending', 'ARES', `EXECUTING ${command}`);
      setHealth('RUNNING', 'running');
      try {
        const output = await window.lycan.command(command);
        pending.remove();
        appendOutput(output);
        setHealth('READY', 'ready');
        updateMetrics(String(output));
      } catch (error) {
        pending.remove();
        appendLine('error', 'ARES', error?.message || 'Backend unavailable');
        setHealth('ERROR', 'error');
      } finally {
        busy = false;
        input.focus();
      }
    }

    function updateMetrics(text) {
      const memory = text.match(/TOTAL\s+(\d+) MB[\s\S]*?USED\s+(\d+) MB[\s\S]*?FREE\s+(\d+) MB/i);
      if (memory) wrap.querySelector('[data-crawford-memory]').textContent = `${memory[2]}/${memory[1]} MB`;
      const processRows = text.match(/^[0-9]+\s+/gm);
      if (processRows) wrap.querySelector('[data-crawford-processes]').textContent = String(processRows.length);
      const network = /NETWORK\s+(ONLINE|OFFLINE)/i.exec(text);
      if (network) wrap.querySelector('[data-crawford-network]').textContent = network[1];
      const runtime = /RUNTIME\s+(ONLINE|OFFLINE)/i.exec(text);
      if (runtime) wrap.querySelector('[data-crawford-runtime]').textContent = runtime[1];
    }

    async function refresh() {
      if (busy) return;
      busy = true;
      try {
        const [diag, memory, network] = await Promise.all([
          window.lycan.command('diagnostics'),
          window.lycan.command('memory'),
          window.lycan.command('network')
        ]);
        updateMetrics(`${diag}\n${memory}\n${network}`);
        setHealth('READY', 'ready');
      } catch {
        setHealth('DEGRADED', 'error');
      } finally {
        busy = false;
      }
    }

    wrap.querySelector('[data-crawford-form]').addEventListener('submit', event => {
      event.preventDefault();
      run(input.value);
      input.value = '';
    });
    wrap.querySelectorAll('[data-crawford-command]').forEach(button => button.addEventListener('click', () => run(button.dataset.crawfordCommand)));
    wrap.querySelector('[data-crawford-refresh]').addEventListener('click', refresh);
    input.addEventListener('keydown', event => {
      if (event.key === 'Escape') input.value = '';
      if (event.key === 'Enter' && event.shiftKey) { event.preventDefault(); refresh(); }
    });

    setInterval(() => {
      const clock = wrap.querySelector('[data-crawford-clock]');
      if (clock) clock.textContent = new Date().toLocaleTimeString();
    }, 1000);
    refresh();
  }

  const scan = () => document.querySelectorAll('.app-window .boundary-shell').forEach(boundary => {
    const shell = boundary.parentElement;
    if (shell) buildAssistant(shell);
  });

  const observer = new MutationObserver(scan);
  observer.observe(document.body, { childList: true, subtree: true });
  scan();
})();
