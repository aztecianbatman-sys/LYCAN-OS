class Lysh {
  constructor(vm){this.vm=vm;this.cwd='/home';this.vars={USER:'guest',HOME:'/home',SHELL:'/bin/lysh',TERM:'lycan'};}
  expand(s){return String(s).replace(/\$([A-Z_][A-Z0-9_]*)/gi,(_,k)=>this.vars[k]??'');}
  split(line){const out=[];let cur='',quote='';for(let i=0;i<line.length;i++){const c=line[i];if(quote){if(c===quote)quote='';else cur+=c;}else if(c==='"'||c==="'"){quote=c;}else if(/\s/.test(c)){if(cur){out.push(cur);cur='';}}else cur+=c;}if(cur)out.push(cur);return out;}
  resolve(p){p=this.expand(p||this.cwd).replace(/\\/g,'/');if(!p.startsWith('/'))p=`${this.cwd}/${p}`;const parts=[];for(const x of p.split('/')){if(!x||x==='.')continue;if(x==='..')parts.pop();else parts.push(x);}return '/'+parts.join('/');}
  run(line){const raw=String(line||'').trim();if(!raw)return '';const a=this.split(raw),c=(a[0]||'').toLowerCase();const arg=a.slice(1);
    if(c==='cd'){this.cwd=this.resolve(arg[0]||this.vars.HOME);return this.cwd;}
    if(c==='pwd')return this.cwd;
    if(c==='export'){const m=arg.join(' ').match(/^([A-Z_][A-Z0-9_]*)=(.*)$/i);if(!m)throw new Error('usage: export NAME=value');this.vars[m[1]]=m[2];return '';}
    if(c==='echo')return arg.join(' ');
    if(c==='clear')return '\u001b[2J\u001b[H';
    if(c==='history')return this.history?.length?this.history.map((x,i)=>`${String(i+1).padStart(3,'0')}  ${x}`).join('\n'):'(no history)';
    const map={ls:()=>this.vm.cmd(`ls ${this.resolve(arg[0]||'.')}`),tree:()=>this.vm.cmd(`tree ${this.resolve(arg[0]||'.')}`),cat:()=>this.vm.cmd(`cat ${this.resolve(arg[0]||'')}`),touch:()=>this.vm.cmd(`touch ${this.resolve(arg[0]||'')}`),mkdir:()=>this.vm.cmd(`mkdir ${this.resolve(arg[0]||'')}`),rm:()=>this.vm.cmd(`rm ${this.resolve(arg[0]||'')}`),mv:()=>this.vm.cmd(`mv ${this.resolve(arg[0]||'')} ${this.resolve(arg[1]||'')}`),cp:()=>this.vm.cmd(`cp ${this.resolve(arg[0]||'')} ${this.resolve(arg[1]||'')}`),write:()=>this.vm.cmd(`write ${this.resolve(arg[0]||'')} ${arg.slice(1).join(' ')}`),memory:()=>this.vm.cmd('memory'),ps:()=>this.vm.cmd('ps'),top:()=>this.vm.cmd('ps'),df:()=>this.vm.cmd('df'),uname:()=>this.vm.cmd('uname'),whoami:()=>this.vm.cmd('whoami'),hostname:()=>this.vm.cmd('hostname'),uptime:()=>this.vm.cmd('uptime'),sysinfo:()=>this.vm.cmd('sysinfo'),network:()=>this.vm.cmd(`network ${arg.join(' ')}`),reboot:()=>this.vm.cmd('reboot'),shutdown:()=>this.vm.cmd('shutdown'),snapshots:()=>this.vm.cmd('snapshots'),snapshot:()=>this.vm.cmd(`snapshot ${arg.join(' ')}`),apps:()=>this.vm.cmd('apps'),diagnostics:()=>this.vm.cmd('diagnostics')};
    if(map[c])return map[c](); return this.vm.cmd(raw);
  }
}
if(typeof module!=='undefined')module.exports={Lysh};
