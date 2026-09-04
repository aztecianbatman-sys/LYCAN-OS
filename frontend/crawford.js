(() => {
  const QUICK = [
    ['STATUS', 'diagnostics'],
    ['FILES', 'ls /home'],
    ['PROCESSES', 'ps'],
    ['SNAPSHOTS', 'snapshots'],
    ['NETWORK', 'network'],
    ['VERSION', 'version']
  ];

  const escapeHtml = value => String(value).replace(/[&<>\"']/g, c => ({
    '&':'&amp;','<':'&lt;','>':'&gt;','\"':'&quot;',"'":'&#039;'
  }[c]));

  function commandFor(text) {
    const q = text.trim().toLowerCase();
    if (!q) return null;
    if (/^(status|system|health|diagnostic|diagnostics)$/.test(q)) return 'diagnostics';
    if (/^(files|file system|filesystem|home|list files)$/.test(q)) return 'ls /home';
    if (/^(processes|process|tasks|running)$/.test(q)) return 'ps';
    if (/^(snapshots|snapshot|checkpoints)$/.test(q)) return 'snapshots';
    if (/^(network|net)$/.test(q)) return 'network';
    if (/^(version|about|build)$/.test(q)) return 'version';
    if (/^tree(?:\s+(.+))?$/.test(q)) return q;
    if (/^ls(?:\s+(.+))?$/.test(q)) return q;
    if (/^cat\s+(.+)$/.test(q)) return q;
    return null;
  }

  function buildAssistant(shell) {
    if (shell.querySelector('.crawford-assistant')) return;
    const wrap = document.createElement('section');
    wrap.className = 'crawford-assistant';
    wrap.innerHTML = `
      <div class="crawford-assistant-head">
        <div>
          <div class="eyebrow">CRAWFORD / LOCAL REASONER</div>
          <strong>CONTROL-PLANE ASSISTANT</strong>
        </div>
        <span class="crawford-lock">HOST CONTROL // LOCKED</span>
      </div>
      <div class="crawford-terminal" data-crawford-log>
        <div class="crawford-line system"><span>CRAWFORD</span><b>Local guest-context mode active.</b></div>
        <div class="crawford-line system"><span>BOUNDARY</span><b>Only approved LYCAN guest commands are exposed.</b></div>
      </div>
      <div class="crawford-quick">${QUICK.map(([label, cmd]) => `<button data-crawford-command="${escapeHtml(cmd)}">${label}</button>`).join('')}</div>
      <form class="crawford-input-row">
        <span>ASK //</span><input aria-label="Ask Crawford" autocomplete="off" placeholder="status, files, processes, snapshots…"><button type="submit">RUN</button>
      </form>
      <div class="crawford-foot"><span>LOCAL ONLY</span><span>NO HOST CONTROL</span><span>COMMANDS ARE FILTERED</span></div>
    `;
    shell.appendChild(wrap);

    const log = wrap.querySelector('[data-crawford-log]');
    const input = wrap.querySelector('input');
    const run = async text => {
      const command = commandFor(text);
      const label = text.trim();
      if (!command) {
        log.insertAdjacentHTML('beforeend', `<div class="crawford-line warn"><span>CRAWFORD</span><b>Command not allowed. Try status, files, processes, snapshots, network, version, ls, tree, or cat.</b></div>`);
        log.scrollTop = log.scrollHeight;
        return;
      }
      log.insertAdjacentHTML('beforeend', `<div class="crawford-line user"><span>YOU</span><b>${escapeHtml(label)}</b></div><div class="crawford-line pending"><span>ARES</span><b>EXECUTING ${escapeHtml(command)}</b></div>`);
      log.scrollTop = log.scrollHeight;
      try {
        const output = await window.lycan.command(command);
        const pending = [...log.querySelectorAll('.pending')].pop();
        if (pending) pending.remove();
        log.insertAdjacentHTML('beforeend', `<div class="crawford-output">${escapeHtml(output)}</div>`);
      } catch (error) {
        const pending = [...log.querySelectorAll('.pending')].pop();
        if (pending) pending.remove();
        log.insertAdjacentHTML('beforeend', `<div class="crawford-line error"><span>ARES</span><b>${escapeHtml(error?.message || 'Backend unavailable')}</b></div>`);
      }
      log.scrollTop = log.scrollHeight;
    };

    wrap.querySelector('.crawford-input-row').addEventListener('submit', event => {
      event.preventDefault();
      run(input.value);
      input.value = '';
      input.focus();
    });
    wrap.querySelectorAll('[data-crawford-command]').forEach(button => {
      button.addEventListener('click', () => run(button.dataset.crawfordCommand));
    });
  }

  const scan = () => document.querySelectorAll('.app-window .boundary-shell').forEach(boundary => {
    const shell = boundary.parentElement;
    if (shell) buildAssistant(shell);
  });

  const observer = new MutationObserver(scan);
  observer.observe(document.body, { childList: true, subtree: true });
  scan();
})();
