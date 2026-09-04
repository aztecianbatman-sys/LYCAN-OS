(()=>{
  const scene=document.getElementById('scene');
  const apps=[
    ['lycan-terminal','Terminal','ARES COMMAND','>_'],['lycan-files','Files','LYFS FILESPACE','⌁'],['lycan-web','Web','GECKO BROWSER','◎'],['store','Store','FREE APPLICATIONS','◫'],
    ['lycan-snapshots','Snapshots','GUEST STATE','◒'],['lycan-diagnostics','Diagnostics','ARES TELEMETRY','◈'],['settings','Settings','SYSTEM CONTROL','⚙'],['crawford','Crawford','AI BOUNDARY','✦']
  ];
  const shell=document.createElement('section');shell.className='launcher-shell';shell.innerHTML=`<div class="launcher-backdrop"></div><div class="launcher-panel"><div class="launcher-top"><div><span class="launcher-kicker">LYCAN COMMAND</span><h1>Open a module</h1></div><button class="launcher-close" aria-label="Close">ESC</button></div><div class="launcher-search"><span>⌕</span><input id="launcherInput" autocomplete="off" spellcheck="false" placeholder="Search applications, tools, runtime…"><kbd>ENTER</kbd></div><div id="launcherResults" class="launcher-results"></div><div class="launcher-foot"><span>↑↓ NAVIGATE</span><span>ENTER OPEN</span><span>ESC CLOSE</span><span>CTRL+K COMMAND</span></div></div>`;document.body.appendChild(shell);
  const input=shell.querySelector('#launcherInput'),results=shell.querySelector('#launcherResults');
  const render=q=>{const term=q.trim().toLowerCase();results.innerHTML='';apps.filter(a=>!term||a.some(x=>x.toLowerCase().includes(term))).forEach((a,i)=>{const b=document.createElement('button');b.className='launcher-item';b.dataset.app=a[0];b.innerHTML=`<span class="launcher-icon">${a[3]}</span><span><b>${a[1]}</b><small>${a[2]}</small></span><em>${String(i+1).padStart(2,'0')}</em>`;results.appendChild(b)});const first=results.querySelector('.launcher-item');first?.classList.add('selected')};
  let selected=0;
  const visible=()=>[...results.querySelectorAll('.launcher-item')];
  const move=d=>{const list=visible();if(!list.length)return;selected=(selected+d+list.length)%list.length;list.forEach((x,i)=>x.classList.toggle('selected',i===selected));list[selected].scrollIntoView({block:'nearest'})};
  const close=()=>{shell.classList.remove('visible');input.value='';render('');scene.classList.remove('launcher-open')};
  const open=()=>{shell.classList.add('visible');scene.classList.add('launcher-open');selected=0;render('');setTimeout(()=>input.focus(),30)};
  shell.addEventListener('click',e=>{if(e.target.classList.contains('launcher-backdrop')||e.target.classList.contains('launcher-close'))close();const item=e.target.closest('.launcher-item');if(item){window.openApp?.(item.dataset.app);close()}});
  input.addEventListener('input',()=>{selected=0;render(input.value)});
  input.addEventListener('keydown',e=>{if(e.key==='ArrowDown'){e.preventDefault();move(1)}if(e.key==='ArrowUp'){e.preventDefault();move(-1)}if(e.key==='Enter'){e.preventDefault();const item=visible()[selected];if(item){window.openApp?.(item.dataset.app);close()}}if(e.key==='Escape'){e.preventDefault();close()}});
  document.addEventListener('keydown',e=>{if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='k'){e.preventDefault();open()}else if(e.key==='/'&&!e.target.matches('input,textarea')){e.preventDefault();open()}else if(e.key==='Escape'&&shell.classList.contains('visible'))close()});
  window.lycanLauncher={open,close};
})();
