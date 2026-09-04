const notes=document.getElementById('notes');const status=document.getElementById('status');const save=document.getElementById('save');
async function boot(){try{notes.value=await window.lycanApp.storage.read('data','notes.txt')||'';const q=await window.lycanApp.storage.quota();status.textContent=q.replace(/\n/g,'  •  ');}catch(e){status.textContent=e.message;}}
save.onclick=async()=>{try{await window.lycanApp.storage.write('data','notes.txt',notes.value);if(window.lycanApp.notify)await window.lycanApp.notify('Notes saved','LYCAN Notes wrote the guest workspace.');status.textContent='SAVED TO LYFS GUEST STORAGE';}catch(e){status.textContent='SAVE FAILED: '+e.message;}};
boot();
