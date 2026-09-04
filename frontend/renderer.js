const scene=document.getElementById('scene');
const leftRail=document.getElementById('leftRail');
const rightRail=document.getElementById('rightRail');
const windows=document.getElementById('desktopWindows');
const toastStack=document.getElementById('toastStack');
const vmState=document.getElementById('vmState');
const clock=document.getElementById('clock');
const netState=document.getElementById('netState');

const apps=[
 {id:'lycan-terminal',name:'Terminal',side:'left',icon:'A',meta:'ARES COMMAND'},
 {id:'lycan-files',name:'Files',side:'left',icon:'F',meta:'LYFS FILESPACE'},
 {id:'lycan-web',name:'Web',side:'left',icon:'G',meta:'GECKO BROWSER'},
 {id:'store',name:'Store',side:'left',icon:'S',meta:'PACKAGE ARRAY'},
 {id:'lycan-snapshots',name:'Snapshots',side:'right',icon:'Q',meta:'GUEST STATE'},
 {id:'lycan-diagnostics',name:'Diagnostics',side:'right',icon:'D',meta:'ARES TELEMETRY'},
 {id:'settings',name:'Settings',side:'right',icon:'C',meta:'SYSTEM CONTROL'},
 {id:'crawford',name:'Crawford',side:'right',icon:'K',meta:'AI BOUNDARY'}
];
const open=new Map();let lastEdge='';let z=100;
const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#039;'}[c]));
function nodeHTML(a){return `<button class="node" data-app="${a.id}"><span class="node-orb">${a.icon}</span><span class="node-copy"><span class="node-name">${a.name.toUpperCase()}</span><span class="node-meta">${a.meta}</span></span><span class="node-index">${String(apps.indexOf(a)+1).padStart(2,'0')}</span></button>`}
for(const a of apps)(a.side==='left'?leftRail:rightRail).insertAdjacentHTML('beforeend',nodeHTML(a));

function updateClock(){clock.textContent=new Date().toLocaleTimeString([], {hour:'2-digit',minute:'2-digit',second:'2-digit',hour12:false})}
setInterval(updateClock,1000);updateClock();
function toast(t){const e=document.createElement('div');e.className='toast';e.textContent=t;toastStack.appendChild(e);setTimeout(()=>e.remove(),2600)}
async function vm(c){try{return await window.lycan.command(c)}catch(e){throw new Error(e.message||'Backend offline')}}

const surfaces={
 'lycan-terminal':{title:'TERMINAL',tag:'ARES',render:o=>`<div class="eyebrow">ARES / COMMAND PROCESSOR</div><div class="command-row"><input id="term" autocomplete="off" placeholder="enter command"><button data-action="terminal">RUN</button></div><pre id="termOut">${esc(o)}</pre>`},
 'lycan-files':{title:'FILES',tag:'LYFS',render:o=>`<div class="eyebrow">LYFS / GUEST FILESYSTEM</div><div class="status-cards"><div class="status-card"><small>VOLUME</small><strong>LYFS</strong></div><div class="status-card"><small>ROOT</small><strong>/home</strong></div><div class="status-card"><small>MODE</small><strong>GUEST RW</strong></div><div class="status-card"><small>HOST</small><strong>BLOCKED</strong></div></div><div class="command-row"><input id="path" value="/home" autocomplete="off"><button data-action="files">INSPECT</button></div><pre id="fileOut">${esc(o)}</pre>`},
 'lycan-web':{title:'WEB',tag:'GECKO',render:o=>`<div class="eyebrow">NETWORK / BROWSER RUNTIME</div><div class="command-row"><input id="url" autocomplete="off" placeholder="https://example.com"><button data-action="web">OPEN</button></div><pre>${esc(o)}</pre><div class="runtime-chip">GECKO BOUNDARY <span>ISOLATED</span></div>`},
 'store':{title:'STORE',tag:'PACKAGES',render:o=>`<div class="eyebrow">LYPKG / APPLICATION ARRAY</div><div class="status-cards"><div class="status-card"><small>CATALOG</small><strong>FREE</strong></div><div class="status-card"><small>FORMAT</small><strong>.LYPKG</strong></div><div class="status-card"><small>VERIFY</small><strong>SHA-256</strong></div><div class="status-card"><small>HOST</small><strong>NONE</strong></div></div><pre>${esc(o)}</pre><div class="quick-grid"><button class="quick" data-action="apps"><b>INSTALLED</b><span>Read the guest package registry.</span></button><button class="quick" data-action="launcher"><b>APP INDEX</b><span>Inspect launcher registrations.</span></button></div>`},
 'lycan-snapshots':{title:'SNAPSHOTS',tag:'STATE',render:o=>`<div class="eyebrow">GUEST STATE / CHECKPOINTS</div><div class="command-row"><input id="snap" autocomplete="off" placeholder="checkpoint name"><button data-action="snapshot">SAVE</button></div><pre id="snapOut">${esc(o)}</pre><div class="quick-grid"><button class="quick" data-action="snapshots"><b>REFRESH</b><span>Read stored checkpoints.</span></button><button class="quick" data-action="snapshot-manual"><b>QUICK SAVE</b><span>Save the current guest state.</span></button></div>`},
 'lycan-diagnostics':{title:'DIAGNOSTICS',tag:'ARES',render:o=>`<div class="eyebrow">ARES / SYSTEM TELEMETRY</div><div class="status-cards"><div class="status-card"><small>ARES</small><strong>ONLINE</strong></div><div class="status-card"><small>LYFS</small><strong>SEALED</strong></div><div class="status-card"><small>HOST</small><strong>DENIED</strong></div><div class="status-card"><small>VM</small><strong>ACTIVE</strong></div></div><pre>${esc(o)}</pre>`},
 'settings':{title:'SETTINGS',tag:'CONTROL',render:o=>`<div class="eyebrow">SYSTEM / GUEST CONFIGURATION</div><div class="status-cards"><div class="status-card"><small>CPU</small><strong>2 CORES</strong></div><div class="status-card"><small>MEMORY</small><strong>512 MB</strong></div><div class="status-card"><small>NETWORK</small><strong>GUEST</strong></div><div class="status-card"><small>FILES</small><strong>SEALED</strong></div></div><pre>${esc(o)}</pre>`},
 'crawford':{title:'CRAWFORD',tag:'BOUNDARY',render:o=>`<div class="eyebrow">AI INTEGRATION / HOST BOUNDARY</div><div class="hero"><div><h2>CRAWFORD</h2><p>LOCAL HOST BRIDGE REMAINS DISABLED.</p><pre>${esc(o)}</pre></div></div>`}
};

function focus(win){win.style.zIndex=++z;document.querySelectorAll('.app-window.is-focused').forEach(w=>w.classList.remove('is-focused'));win.classList.add('is-focused')}
async function openApp(id){
 if(open.has(id)){focus(open.get(id));return}
 const meta=surfaces[id];if(!meta)return;
 const win=document.createElement('article');win.className='app-window';win.style.zIndex=++z;
 win.innerHTML=`<div class="window-bar"><div class="window-title"><span class="window-dot"></span>${meta.title}<span class="window-tag">${meta.tag}</span></div><button class="window-close" aria-label="Close">×</button></div><div class="window-body"><div class="hero"><div><div class="eyebrow">LYCAN RUNTIME</div><h2>INITIALIZING</h2><p>ATTACHING TO GUEST PROCESS</p></div></div></div>`;
 windows.appendChild(win);open.set(id,win);focus(win);
 try{
   const command=id==='store'?'apps':id==='settings'?'diagnostics':id==='crawford'?'version':id==='lycan-files'?'ls /home':id==='lycan-web'?'web start':id==='lycan-snapshots'?'snapshots':id==='lycan-diagnostics'?'diagnostics':'help';
   const out=await vm(command);win.querySelector('.window-body').innerHTML=meta.render(out);focus(win);toast(`${meta.title} ONLINE`);
 }catch(e){win.querySelector('.window-body').innerHTML=`<div class="hero"><div><div class="eyebrow">RUNTIME FAULT</div><h2>BACKEND ERROR</h2><p>${esc(e.message)}</p></div></div>`;toast(e.message)}
 win.addEventListener('mousedown',()=>focus(win));
 win.addEventListener('click',e=>{if(e.target.closest('.window-close')){win.classList.add('closing');setTimeout(()=>{win.remove();open.delete(id)},180);return}const action=e.target.closest('[data-action]')?.dataset.action;if(action)runAction(id,action)});
}
window.openApp=openApp;

async function runAction(id,a){
 try{
  const win=open.get(id);let out='';
  if(a==='terminal'){const v=win?.querySelector('#term')?.value.trim();if(!v)return;out=await vm(v);win.querySelector('#termOut').textContent=out;win.querySelector('#term').value='';return}
  if(a==='files'){const v=win?.querySelector('#path')?.value.trim()||'/home';win.querySelector('#fileOut').textContent=await vm(`ls ${v}`);return}
  if(a==='web'){const v=win?.querySelector('#url')?.value.trim();if(!v)return;out=await vm(`web tab ${v}`);win.querySelector('pre').textContent=out;return}
  if(a==='snapshot'){const v=win?.querySelector('#snap')?.value.trim()||'manual';out=await vm(`snapshot ${v}`);win.querySelector('#snapOut').textContent=out;return}
  if(a==='snapshot-manual')out=await vm('snapshot manual');
  if(a==='snapshots')out=await vm('snapshots');
  if(a==='apps')out=await vm('apps');
  if(a==='launcher')out=await vm('apps');
  const pre=win?.querySelector('pre');if(pre&&out)pre.textContent=out;
  toast(out.split('\n')[0]||'DONE');
 }catch(e){toast(e.message)}
}

document.addEventListener('click',e=>{const a=e.target.closest('[data-app]')?.dataset.app;if(a)openApp(a);if(e.target.id==='logo')scene.classList.toggle('node-open')});
document.addEventListener('mousemove',e=>{const threshold=70;const edge=e.clientX<threshold?'left':e.clientX>innerWidth-threshold?'right':'';if(edge!==lastEdge){lastEdge=edge;scene.classList.toggle('edge-left',edge==='left');scene.classList.toggle('edge-right',edge==='right')}});
document.addEventListener('keydown',e=>{if(e.key==='Escape'){if(document.querySelector('.launcher-shell.visible'))return;for(const [id,w] of open){w.classList.add('closing');setTimeout(()=>w.remove(),140)}open.clear();scene.classList.remove('node-open')}if(e.key===' '&&!e.target.matches('input,textarea'))scene.classList.toggle('node-open')});

async function refreshNetwork(){try{const status=await vm('network');netState.textContent=status.endsWith('ONLINE')?'ON':'OFF'}catch(_){netState.textContent='--'}}
async function bootHandshake(){
 vmState.textContent='BOOTING';scene.classList.add('vm-booting');
 try{await new Promise(r=>setTimeout(r,300));const pong=await vm('ping');if(pong!=='LYCAN VM ONLINE')throw new Error('Unexpected VM response');await new Promise(r=>setTimeout(r,320));vmState.textContent='ONLINE';scene.classList.remove('vm-booting');await refreshNetwork();toast('ARES / GUEST ENVIRONMENT READY')}
 catch(e){vmState.textContent='OFFLINE';scene.classList.remove('vm-booting');toast(`VM CONNECTION FAILED: ${e.message}`)}
}
setInterval(refreshNetwork,5000);bootHandshake();