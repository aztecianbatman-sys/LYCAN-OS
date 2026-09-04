const STORE_CATALOG = [
  { id:'lycan-terminal',name:'ARES Terminal',version:'1.1.0',type:'CORE',category:'SYSTEM',description:'Guest command processor for ARES runtime operations.' },
  { id:'lycan-files',name:'LYFS Files',version:'1.1.0',type:'CORE',category:'SYSTEM',description:'Guest-only filesystem navigator and file workspace.' },
  { id:'lycan-web',name:'Gecko Web',version:'1.1.0',type:'CORE',category:'INTERNET',description:'Native Gecko browser bridge using a dedicated profile.' },
  { id:'lycan-snapshots',name:'State Snapshots',version:'1.1.0',type:'CORE',category:'SYSTEM',description:'Guest-state checkpoint manager.' },
  { id:'lycan-diagnostics',name:'ARES Diagnostics',version:'1.1.0',type:'CORE',category:'SYSTEM',description:'Runtime telemetry and integrity inspection.' },
  { id:'lycan-settings',name:'LYCAN Settings',version:'1.1.0',type:'CORE',category:'SYSTEM',description:'Persistent interface and guest configuration.',launchTarget:'settings' },
  { id:'crawford',name:'Crawford',version:'1.1.0',type:'CORE',category:'SYSTEM',description:'Bounded control-plane and isolation inspector.' }
];

const STORE_CATEGORIES=['ALL','SYSTEM','PRODUCTIVITY','INTERNET','TOOLS'];
function storeEscape(value){return String(value).replace(/[&<>\"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','\"':'&quot;',"'":'&#039;'}[c]));}
async function getRegistry(){try{return await window.lycan.listPackages();}catch{return[];}}
async function getRepository(){try{const r=await fetch('./store-repository.json',{cache:'no-store'});if(!r.ok)throw new Error('repository unavailable');const data=await r.json();return Array.isArray(data.packages)?data.packages:[];}catch{return[];}}
function compareVersions(a,b){const aa=String(a||'0').split('.').map(Number),bb=String(b||'0').split('.').map(Number);for(let i=0;i<3;i++){const x=aa[i]||0,y=bb[i]||0;if(x!==y)return x>y?1:-1;}return 0;}

function storeMount(win){
  if(!win||win.dataset.storeMounted==='1')return;const body=win.querySelector('.window-body');if(!body)return;const pre=body.querySelector('pre');win.dataset.storeMounted='1';
  const head=document.createElement('section');head.className='store-hub-head';head.innerHTML=`<div><div class="eyebrow">LYPKG / OFFICIAL + REPOSITORY ARRAY</div><h3>LYCAN STORE</h3><p>FREE GUEST SOFTWARE. VERIFY. INSTALL. UPDATE.</p></div><div class="store-availability"><span></span><b id="storeAvailability">SCANNING</b></div>`;
  const toolbar=document.createElement('div');toolbar.className='store-toolbar';toolbar.innerHTML=`<div class="store-tabs"><button class="store-tab active" data-store-tab="catalog">CATALOG</button><button class="store-tab" data-store-tab="installed">INSTALLED</button><button class="store-tab" data-store-tab="updates">UPDATES</button></div><div class="store-filters"><input class="store-search" data-store-search placeholder="SEARCH APPLICATIONS"><select class="store-category" data-store-category>${STORE_CATEGORIES.map(c=>`<option>${c}</option>`).join('')}</select><button class="store-import" data-store-import>IMPORT .LYPKG</button></div>`;
  const grid=document.createElement('div');grid.className='store-grid';const footer=document.createElement('div');footer.className='store-footer-note';footer.innerHTML='<span>PACKAGE POLICY</span><b>LYPKG</b><i>MANIFEST + SHA-256 + SANDBOXED RUNTIME</i>';
  body.insertBefore(head,body.firstChild);if(pre)body.insertBefore(toolbar,pre);else body.appendChild(toolbar);body.appendChild(grid);body.appendChild(footer);
  let repository=[];let registry=[];let tab='catalog';let query='';let category='ALL';
  const refreshData=async()=>{[repository,registry]=await Promise.all([getRepository(),getRegistry()]);};
  const installedMap=()=>new Map(registry.map(x=>[x.id,x]));
  const catalog=()=>[...STORE_CATALOG,...repository.map(x=>({...x,type:'REPOSITORY'}))];
  const card=(item,index,actionLabel,actionAttr,extra='')=>`<article class="store-card ${item.type==='REPOSITORY'?'remote-package':''}"><div class="store-card-top"><span class="store-card-index">${String(index+1).padStart(2,'0')}</span><span class="store-card-type">${storeEscape(item.type||'PACKAGE')}</span></div><div class="store-card-icon">${storeEscape((item.name||item.id).slice(0,1).toUpperCase())}</div><h4>${storeEscape(item.name||item.id)}</h4><p>${storeEscape(item.description||'LYCAN guest application.')}</p><div class="store-card-meta"><span>${storeEscape(item.category||'TOOLS')}</span><b>v${storeEscape(item.version||'—')}</b></div>${extra}<button class="store-card-action" ${actionAttr||''}>${storeEscape(actionLabel)}</button></article>`;
  const render=async()=>{
    await refreshData();const map=installedMap();let items=[];
    if(tab==='installed')items=registry.map(x=>({...x,type:x.type==='CORE'?'CORE':'LOCAL',category:x.category||'TOOLS'}));
    else if(tab==='updates')items=[...repository].filter(x=>{const installed=map.get(x.id);return installed&&compareVersions(x.version,installed.version)>0}).map(x=>({...x,type:'UPDATE'}));
    else items=catalog();
    items=items.filter(x=>category==='ALL'||(x.category||'TOOLS')===category).filter(x=>!query||`${x.name} ${x.id} ${x.description} ${x.category}`.toLowerCase().includes(query.toLowerCase()));
    head.querySelector('#storeAvailability').textContent=`${items.length} ${tab.toUpperCase()}`;
    grid.innerHTML=items.length?items.map((item,i)=>{const installed=map.get(item.id);let label='OPEN',attr=`data-store-core="${storeEscape(item.launchTarget||item.id)}"`,extra='';
      if(item.type==='REPOSITORY'){label=installed?'INSTALLED':'DOWNLOAD';attr=installed?'data-store-launch="'+storeEscape(item.id)+'"':'data-store-download="'+storeEscape(item.download||'')+'"';if(installed&&compareVersions(item.version,installed.version)>0){label='UPDATE';attr='data-store-update="'+storeEscape(item.download||'')+'"';}extra=`<div class="store-card-meta"><span>${storeEscape(item.id)}</span><b>${installed?'INSTALLED '+storeEscape(installed.version):'REMOTE'}</b></div>`;}
      if(item.type==='LOCAL'){label='LAUNCH';attr=`data-store-launch="${storeEscape(item.id)}"`;extra=`<div class="store-card-meta"><span>${storeEscape(item.id)}</span><b>${storeEscape(item.version||'—')}</b></div>`;}
      if(item.type==='UPDATE'){label='UPDATE';attr=`data-store-update="${storeEscape(item.download||'')}"`;extra=`<div class="store-card-meta"><span>INSTALLED ${storeEscape(installed?.version||'?')}</span><b>→ ${storeEscape(item.version)}</b></div>`;}
      return card(item,i,label,attr,extra);
    }).join(''):'<div class="store-loading">NO MATCHING APPLICATIONS.</div>';
  };
  const showCatalog=async next=>{tab=next||'catalog';toolbar.querySelectorAll('[data-store-tab]').forEach(b=>b.classList.toggle('active',b.dataset.storeTab===tab));await render();};
  toolbar.addEventListener('input',async e=>{if(e.target.matches('[data-store-search]')){query=e.target.value;await render();}});
  toolbar.addEventListener('change',async e=>{if(e.target.matches('[data-store-category]')){category=e.target.value;await render();}});
  toolbar.addEventListener('click',async e=>{const t=e.target.closest('[data-store-tab]')?.dataset.storeTab;if(t)await showCatalog(t);});
  toolbar.querySelector('[data-store-import]')?.addEventListener('click',async()=>{const result=await window.lycan.installPackage();if(result?.canceled)return;const message=result?.ok?`INSTALLED ${result.name} ${result.version}`:`INSTALL FAILED: ${result?.error||'UNKNOWN ERROR'}`;if(pre)pre.textContent=message;window.dispatchEvent(new CustomEvent('lycan:toast',{detail:message}));await render();});
  grid.addEventListener('click',async e=>{
    const core=e.target.closest('[data-store-core]')?.dataset.storeCore;if(core){if(typeof window.openApp==='function')window.openApp(core);return;}
    const launch=e.target.closest('[data-store-launch]')?.dataset.storeLaunch;if(launch){const r=await window.lycan.launchPackage(launch);window.dispatchEvent(new CustomEvent('lycan:toast',{detail:r?.ok?`${r.name} LAUNCHED`:`LAUNCH FAILED: ${r?.error||'UNKNOWN ERROR'}`}));return;}
    const download=e.target.closest('[data-store-download]')?.dataset.storeDownload;const update=e.target.closest('[data-store-update]')?.dataset.storeUpdate;const url=download||update;if(url){if(!/^https:\/\//i.test(url)){window.dispatchEvent(new CustomEvent('lycan:toast',{detail:'INVALID REPOSITORY URL'}));return;}const r=await window.lycan.downloadPackage(url);window.dispatchEvent(new CustomEvent('lycan:toast',{detail:r?.ok?`${r.name} ${update?'UPDATED':'INSTALLED'}`:`DOWNLOAD FAILED: ${r?.error||'UNKNOWN ERROR'}`}));await render();}
  });
  render();
}
function storeObserve(){const observer=new MutationObserver(()=>{document.querySelectorAll('.app-window').forEach(win=>{const title=win.querySelector('.window-title')?.textContent||'';if(title.includes('STORE'))storeMount(win);});});observer.observe(document.getElementById('desktopWindows')||document.body,{childList:true,subtree:true});document.querySelectorAll('.app-window').forEach(win=>{const title=win.querySelector('.window-title')?.textContent||'';if(title.includes('STORE'))storeMount(win);});}
document.addEventListener('DOMContentLoaded',storeObserve);window.addEventListener('lycan:store-ready',storeObserve);
