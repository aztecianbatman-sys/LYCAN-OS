(() => {
  const esc = s => String(s).replace(/[&<>\"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','\"':'&quot;',"'":'&#039;'}[c]));
  const desktop = () => document.getElementById('desktopWindows');
  const watch = new MutationObserver(() => enhance());

  async function refreshDiagnostics(win) {
    try {
      const out = await window.lycan.command('diagnostics');
      const pre = win.querySelector('.runtime-diagnostics-pre');
      if (pre) pre.textContent = out;
      const mem = await window.lycan.command('memory');
      const memNode = win.querySelector('[data-runtime-memory]');
      if (memNode) memNode.textContent = mem;
    } catch (e) { window.dispatchEvent(new CustomEvent('lycan:toast', {detail:e.message})); }
  }

  function enhanceWindow(win) {
    if (win.dataset.runtimeEnhanced === '1') return;
    const title = win.querySelector('.window-title')?.textContent || '';
    if (!title) return;
    win.dataset.runtimeEnhanced = '1';

    if (title.includes('DIAGNOSTICS')) {
      const body = win.querySelector('.window-body');
      if (!body) return;
      const panel = document.createElement('section');
      panel.className = 'runtime-control-panel';
      panel.innerHTML = `<div class="runtime-panel-head"><div><span>ARES / LIVE CONTROL PLANE</span><b>RUNTIME TELEMETRY</b></div><button data-runtime-refresh>REFRESH</button></div><div class="runtime-pill-grid"><div><small>VIRTUAL MEMORY</small><pre data-runtime-memory>READING…</pre></div><div><small>NETWORK</small><pre>VNET0 / 10.42.0.2</pre></div><div><small>STORAGE</small><pre>LYFS / APP-ISOLATED</pre></div><div><small>SNAPSHOTS</small><pre>FILESYSTEM-AWARE</pre></div></div>`;
      body.insertBefore(panel, body.querySelector('pre:last-child') || null);
      panel.querySelector('[data-runtime-refresh]').onclick = () => refreshDiagnostics(win);
      refreshDiagnostics(win);
    }

    if (title.includes('SETTINGS')) {
      const body = win.querySelector('.window-body'); if (!body) return;
      const section = document.createElement('section'); section.className='runtime-control-panel';
      section.innerHTML=`<div class="runtime-panel-head"><div><span>ARES / MACHINE PROFILE</span><b>VIRTUAL HARDWARE</b></div><button data-runtime-read>READ</button></div><div class="runtime-hardware-row"><label>GUEST RAM<input type="number" min="128" max="8192" step="64" value="512" data-runtime-ram><span>MB</span></label><button data-runtime-apply>APPLY</button></div><pre class="runtime-hardware-output" data-runtime-hardware>Read the current virtual hardware profile.</pre>`;
      body.appendChild(section);
      const read=async()=>{try{const out=await window.lycan.command('memory');const total=out.match(/TOTAL\s+(\d+) MB/);if(total)section.querySelector('[data-runtime-ram]').value=total[1];section.querySelector('[data-runtime-hardware]').textContent=out;}catch(e){section.querySelector('[data-runtime-hardware]').textContent=e.message;}};
      section.querySelector('[data-runtime-read]').onclick=read;
      section.querySelector('[data-runtime-apply]').onclick=async()=>{try{const value=section.querySelector('[data-runtime-ram]').value;section.querySelector('[data-runtime-hardware]').textContent=await window.lycan.command(`vm ram ${value}`);window.dispatchEvent(new CustomEvent('lycan:toast',{detail:'VIRTUAL RAM UPDATED'}));await read();}catch(e){section.querySelector('[data-runtime-hardware]').textContent=e.message;}};
      read();
    }

    if (title.includes('STORE')) {
      const body=win.querySelector('.window-body'); if(!body)return;
      const managed=document.createElement('div');managed.className='runtime-package-manager';managed.innerHTML=`<div class="runtime-panel-head"><div><span>ARES / PACKAGE LIFECYCLE</span><b>REGISTERED APPLICATIONS</b></div><button data-runtime-appscan>SCAN</button></div><div data-runtime-applist class="runtime-app-list">SCANNING…</div>`;
      body.appendChild(managed);
      const scan=async()=>{const host=managed.querySelector('[data-runtime-applist]');try{const packages=await window.lycan.listPackages();host.innerHTML=packages.map(p=>`<div class="runtime-app-row"><div><b>${esc(p.name)}</b><span>${esc(p.id)} / v${esc(p.version)} / ${esc(p.permissions?.join(', ')||'no permissions')}</span></div><strong>${esc(p.type||'APP')}</strong>${p.type==='LOCAL'?`<button data-runtime-uninstall="${esc(p.id)}">UNINSTALL</button>`:'<em>CORE</em>'}</div>`).join('')||'NO REGISTERED APPS';}catch(e){host.textContent=e.message;}};
      managed.querySelector('[data-runtime-appscan]').onclick=scan;
      managed.addEventListener('click',async e=>{const id=e.target.closest('[data-runtime-uninstall]')?.dataset.runtimeUninstall;if(!id)return;if(!confirm(`Uninstall ${id}?`))return;const r=await window.lycan.uninstallPackage(id);window.dispatchEvent(new CustomEvent('lycan:toast',{detail:r?.ok?`${id} UNINSTALLED`:`UNINSTALL FAILED: ${r?.error||'UNKNOWN ERROR'}`}));await scan();});
      scan();
    }
  }

  function enhance(){ desktop()?.querySelectorAll('.app-window').forEach(enhanceWindow); }
  document.addEventListener('DOMContentLoaded',()=>{enhance();watch.observe(desktop()||document.body,{childList:true,subtree:true});});
})();
