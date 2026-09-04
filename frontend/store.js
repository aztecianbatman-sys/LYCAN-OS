const STORE_CATALOG = [
  { id: 'lycan-terminal', name: 'ARES Terminal', version: '1.0.0', type: 'CORE', description: 'Guest command processor for ARES runtime operations.' },
  { id: 'lycan-files', name: 'LYFS Files', version: '1.0.0', type: 'CORE', description: 'Guest-only filesystem navigator and file workspace.' },
  { id: 'lycan-web', name: 'Gecko Web', version: '1.0.0', type: 'CORE', description: 'Native Gecko browser bridge using a dedicated profile.' },
  { id: 'lycan-snapshots', name: 'State Snapshots', version: '1.0.0', type: 'CORE', description: 'Guest-state checkpoint manager.' },
  { id: 'lycan-diagnostics', name: 'ARES Diagnostics', version: '1.0.0', type: 'CORE', description: 'Runtime telemetry and integrity inspection.' },
  { id: 'lycan-settings', name: 'LYCAN Settings', version: '1.0.0', type: 'CORE', description: 'Persistent interface and guest configuration.' },
  { id: 'crawford', name: 'Crawford', version: '1.0.0', type: 'CORE', description: 'Bounded control-plane and isolation inspector.' }
];

function storeEscape(value) {
  return String(value).replace(/[&<>\"']/g, c => ({ '&':'&amp;', '<':'&lt;', '>':'&gt;', '\"':'&quot;', "'":'&#039;' }[c]));
}

function storeMount(win) {
  if (!win || win.dataset.storeMounted === '1') return;
  const body = win.querySelector('.window-body');
  if (!body) return;
  const pre = body.querySelector('pre');
  win.dataset.storeMounted = '1';

  const head = document.createElement('section');
  head.className = 'store-hub-head';
  head.innerHTML = `
    <div>
      <div class="eyebrow">LYPKG / OFFICIAL PACKAGE ARRAY</div>
      <h3>LYCAN STORE</h3>
      <p>FREE GUEST SOFTWARE. VERIFIED BEFORE INSTALL.</p>
    </div>
    <div class="store-availability"><span></span><b>LOCAL CATALOG</b></div>`;

  const toolbar = document.createElement('div');
  toolbar.className = 'store-toolbar';
  toolbar.innerHTML = `
    <div class="store-tabs">
      <button class="store-tab active" data-store-tab="catalog">CATALOG</button>
      <button class="store-tab" data-store-tab="installed">INSTALLED</button>
    </div>
    <button class="store-import" data-store-import>IMPORT .LYPKG</button>`;

  const grid = document.createElement('div');
  grid.className = 'store-grid';

  const footer = document.createElement('div');
  footer.className = 'store-footer-note';
  footer.innerHTML = '<span>PACKAGE POLICY</span><b>LYPKG</b><i>MANIFEST + SHA-256 + GUEST CONTAINMENT</i>';

  body.insertBefore(head, body.firstChild);
  if (pre) body.insertBefore(toolbar, pre);
  else body.appendChild(toolbar);
  body.appendChild(grid);
  body.appendChild(footer);

  const renderCatalog = () => {
    grid.innerHTML = STORE_CATALOG.map((item, index) => `
      <article class="store-card">
        <div class="store-card-top"><span class="store-card-index">${String(index + 1).padStart(2,'0')}</span><span class="store-card-type">${item.type}</span></div>
        <div class="store-card-icon">${storeEscape(item.name.slice(0,1).toUpperCase())}</div>
        <h4>${storeEscape(item.name)}</h4>
        <p>${storeEscape(item.description)}</p>
        <div class="store-card-meta"><span>${storeEscape(item.id)}</span><b>v${storeEscape(item.version)}</b></div>
        <button class="store-card-action" data-store-core="${storeEscape(item.id)}">OPEN</button>
      </article>`).join('');
  };

  const renderInstalled = async () => {
    grid.innerHTML = '<div class="store-loading">SCANNING GUEST REGISTRY...</div>';
    try {
      const output = await window.lycan.command('apps');
      const rows = String(output).split('\n').slice(2).filter(Boolean);
      grid.innerHTML = rows.length ? rows.map((row, i) => {
        const parts = row.trim().split(/\s+/);
        const id = parts.shift() || 'unknown';
        const version = parts.join(' ') || '—';
        const catalog = STORE_CATALOG.find(x => x.id === id);
        return `<article class="store-card installed-card">
          <div class="store-card-top"><span class="store-card-index">${String(i + 1).padStart(2,'0')}</span><span class="store-card-type">INSTALLED</span></div>
          <div class="store-card-icon">${storeEscape((catalog?.name || id).slice(0,1).toUpperCase())}</div>
          <h4>${storeEscape(catalog?.name || id)}</h4>
          <p>${storeEscape(catalog?.description || 'Registered guest application.')}</p>
          <div class="store-card-meta"><span>${storeEscape(id)}</span><b>${storeEscape(version)}</b></div>
          <button class="store-card-action" data-store-open="${storeEscape(id)}">LAUNCH</button>
        </article>`;
      }).join('') : '<div class="store-loading">NO APPLICATIONS REGISTERED.</div>';
    } catch (error) {
      grid.innerHTML = `<div class="store-loading">REGISTRY ERROR: ${storeEscape(error.message)}</div>`;
    }
  };

  renderCatalog();

  toolbar.addEventListener('click', async event => {
    const tab = event.target.closest('[data-store-tab]')?.dataset.storeTab;
    if (!tab) return;
    toolbar.querySelectorAll('[data-store-tab]').forEach(button => button.classList.toggle('active', button.dataset.storeTab === tab));
    if (tab === 'installed') await renderInstalled();
    else renderCatalog();
  });

  toolbar.querySelector('[data-store-import]')?.addEventListener('click', async () => {
    const result = await window.lycan.installPackage();
    if (result?.canceled) return;
    const message = result?.ok ? `INSTALLED ${result.name} ${result.version}` : `INSTALL FAILED: ${result?.error || 'UNKNOWN ERROR'}`;
    if (pre) pre.textContent = message;
    window.dispatchEvent(new CustomEvent('lycan:toast', { detail: message }));
  });

  grid.addEventListener('click', async event => {
    const core = event.target.closest('[data-store-core]')?.dataset.storeCore;
    if (core) {
      if (typeof window.openApp === 'function') window.openApp(core);
      return;
    }
    const launch = event.target.closest('[data-store-open]')?.dataset.storeOpen;
    if (launch && typeof window.openApp === 'function') window.openApp(launch);
  });
}

function storeObserve() {
  const observer = new MutationObserver(() => {
    document.querySelectorAll('.app-window').forEach(win => {
      const title = win.querySelector('.window-title')?.textContent || '';
      if (title.includes('STORE')) storeMount(win);
    });
  });
  observer.observe(document.getElementById('desktopWindows') || document.body, { childList: true, subtree: true });
  document.querySelectorAll('.app-window').forEach(win => {
    const title = win.querySelector('.window-title')?.textContent || '';
    if (title.includes('STORE')) storeMount(win);
  });
}

document.addEventListener('DOMContentLoaded', storeObserve);
window.addEventListener('lycan:store-ready', storeObserve);
