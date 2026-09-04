const STORE_CATALOG = [
  { id: 'lycan-terminal', name: 'ARES Terminal', version: '1.0.0', type: 'CORE', description: 'Guest command processor for ARES runtime operations.' },
  { id: 'lycan-files', name: 'LYFS Files', version: '1.0.0', type: 'CORE', description: 'Guest-only filesystem navigator and file workspace.' },
  { id: 'lycan-web', name: 'Gecko Web', version: '1.0.0', type: 'CORE', description: 'Native Gecko browser bridge using a dedicated profile.' },
  { id: 'lycan-snapshots', name: 'State Snapshots', version: '1.0.0', type: 'CORE', description: 'Guest-state checkpoint manager.' },
  { id: 'lycan-diagnostics', name: 'ARES Diagnostics', version: '1.0.0', type: 'CORE', description: 'Runtime telemetry and integrity inspection.' },
  { id: 'lycan-settings', name: 'LYCAN Settings', version: '1.0.0', type: 'CORE', description: 'Persistent interface and guest configuration.', launchTarget: 'settings' },
  { id: 'crawford', name: 'Crawford', version: '1.0.0', type: 'CORE', description: 'Bounded control-plane and isolation inspector.' }
];

function storeEscape(value) {
  return String(value).replace(/[&<>\"']/g, c => ({ '&':'&amp;', '<':'&lt;', '>':'&gt;', '\"':'&quot;', "'":'&#039;' }[c]));
}

async function getRegistry() {
  try { return await window.lycan.listPackages(); } catch { return []; }
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
      <div class="eyebrow">LYPKG / APPLICATION ARRAY</div>
      <h3>LYCAN STORE</h3>
      <p>FREE GUEST SOFTWARE. VERIFIED BEFORE INSTALL.</p>
    </div>
    <div class="store-availability"><span></span><b id="storeAvailability">SCANNING REGISTRY</b></div>`;

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
  footer.innerHTML = '<span>PACKAGE POLICY</span><b>LYPKG</b><i>MANIFEST + SHA-256 + SANDBOXED LAUNCH</i>';

  body.insertBefore(head, body.firstChild);
  if (pre) body.insertBefore(toolbar, pre);
  else body.appendChild(toolbar);
  body.appendChild(grid);
  body.appendChild(footer);

  const renderCatalog = async () => {
    const registry = await getRegistry();
    const custom = registry.filter(item => !STORE_CATALOG.some(core => core.id === item.id));
    const catalog = [...STORE_CATALOG, ...custom.map(item => ({ ...item, type: 'LOCAL' }))];
    const countNode = head.querySelector('#storeAvailability');
    if (countNode) countNode.textContent = `${catalog.length} AVAILABLE`;
    grid.innerHTML = catalog.map((item, index) => `
      <article class="store-card ${item.type === 'LOCAL' ? 'local-package' : ''}">
        <div class="store-card-top"><span class="store-card-index">${String(index + 1).padStart(2,'0')}</span><span class="store-card-type">${storeEscape(item.type)}</span></div>
        <div class="store-card-icon">${storeEscape(item.name.slice(0,1).toUpperCase())}</div>
        <h4>${storeEscape(item.name)}</h4>
        <p>${storeEscape(item.description || 'Registered LYCAN guest application.')}</p>
        <div class="store-card-meta"><span>${storeEscape(item.id)}</span><b>v${storeEscape(item.version)}</b></div>
        <button class="store-card-action" ${item.type === 'LOCAL' ? `data-store-launch="${storeEscape(item.id)}"` : `data-store-core="${storeEscape(item.launchTarget || item.id)}"`}>${item.type === 'LOCAL' ? 'LAUNCH PACKAGE' : 'OPEN'}</button>
      </article>`).join('');
  };

  const renderInstalled = async () => {
    grid.innerHTML = '<div class="store-loading">SCANNING NATIVE PACKAGE REGISTRY...</div>';
    const packages = await getRegistry();
    if (!packages.length) {
      grid.innerHTML = '<div class="store-loading">NO .LYPKG APPLICATIONS INSTALLED.</div>';
      return;
    }
    const core = STORE_CATALOG.filter(x => packages.some(p => p.id === x.id));
    const custom = packages.filter(item => !STORE_CATALOG.some(x => x.id === item.id));
    const rows = [...core, ...custom];
    grid.innerHTML = rows.map((item, i) => {
      const isCore = item.type === 'CORE';
      const name = item.name || item.id;
      return `<article class="store-card installed-card ${isCore ? '' : 'local-package'}">
        <div class="store-card-top"><span class="store-card-index">${String(i + 1).padStart(2,'0')}</span><span class="store-card-type">${isCore ? 'CORE' : 'LOCAL PACKAGE'}</span></div>
        <div class="store-card-icon">${storeEscape(name.slice(0,1).toUpperCase())}</div>
        <h4>${storeEscape(name)}</h4>
        <p>${storeEscape(item.description || 'Registered guest application.')}</p>
        <div class="store-card-meta"><span>${storeEscape(item.id)}</span><b>${storeEscape(item.version || '—')}</b></div>
        <button class="store-card-action" ${isCore ? `data-store-core="${storeEscape(item.launchTarget || item.id)}"` : `data-store-launch="${storeEscape(item.id)}"`}>LAUNCH</button>
      </article>`;
    }).join('');
  };

  renderCatalog();

  toolbar.addEventListener('click', async event => {
    const tab = event.target.closest('[data-store-tab]')?.dataset.storeTab;
    if (!tab) return;
    toolbar.querySelectorAll('[data-store-tab]').forEach(button => button.classList.toggle('active', button.dataset.storeTab === tab));
    if (tab === 'installed') await renderInstalled();
    else await renderCatalog();
  });

  toolbar.querySelector('[data-store-import]')?.addEventListener('click', async () => {
    const result = await window.lycan.installPackage();
    if (result?.canceled) return;
    const message = result?.ok ? `INSTALLED ${result.name} ${result.version}` : `INSTALL FAILED: ${result?.error || 'UNKNOWN ERROR'}`;
    if (pre) pre.textContent = message;
    window.dispatchEvent(new CustomEvent('lycan:toast', { detail: message }));
    if (result?.ok) await renderCatalog();
  });

  grid.addEventListener('click', async event => {
    const core = event.target.closest('[data-store-core]')?.dataset.storeCore;
    if (core) {
      if (typeof window.openApp === 'function') window.openApp(core);
      return;
    }
    const launch = event.target.closest('[data-store-launch]')?.dataset.storeLaunch;
    if (launch) {
      const result = await window.lycan.launchPackage(launch);
      if (!result?.ok) window.dispatchEvent(new CustomEvent('lycan:toast', { detail: `LAUNCH FAILED: ${result?.error || 'UNKNOWN ERROR'}` }));
      else window.dispatchEvent(new CustomEvent('lycan:toast', { detail: `${result.name} LAUNCHED` }));
    }
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
