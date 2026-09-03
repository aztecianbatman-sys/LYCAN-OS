#include "app_host.h"
#include <algorithm>
#include <array>
#include <sstream>
#include <cctype>
#include <fstream>
namespace lycan {
namespace {
Package builtin(const char* id,const char* name,const char* description,const char* entry,const char* permissions){Package p;p.id=id;p.name=name;p.version="1.0.0";p.publisher="LYCAN";p.description=description;p.entry=entry;std::stringstream ss(permissions);std::string x;while(std::getline(ss,x,','))if(!x.empty())p.permissions.push_back(x);return p;}
const std::array<Package,6>& builtins(){static const std::array<Package,6> apps={builtin("lycan-terminal","Terminal","Native ARES guest command workspace","builtin://terminal","lyfs.read,lyfs.write,process.launch"),builtin("lycan-files","Files","LYFS guest file manager","builtin://files","lyfs.read,lyfs.write,process.launch"),builtin("lycan-web","Web","Mozilla Gecko web surface","builtin://web","network,lyfs.read,process.launch"),builtin("lycan-snapshots","Snapshots","Save and restore guest state","builtin://snapshots","snapshot.read,snapshot.write,process.launch"),builtin("lycan-diagnostics","Diagnostics","VM health and security inspection","builtin://diagnostics","vm.read,security.read,process.launch"),builtin("crawford","Crawford","AI integration boundary for LYCAN apps","builtin://crawford","network,process.launch")};return apps;}
std::string upperState(const std::string&s){std::string r=s;std::transform(r.begin(),r.end(),r.begin(),[](unsigned char c){return char(std::toupper(c));});return r;}
std::string rest(const std::string& c,size_t n){return c.size()>n?c.substr(n):std::string{};}
std::string loadText(const std::filesystem::path& p){std::ifstream f(p);return f?std::string((std::istreambuf_iterator<char>(f)),{}):std::string{};}
Capability capabilityFrom(const std::string&s){if(s=="lyfs.read")return Capability::ReadGuestFs;if(s=="lyfs.write")return Capability::WriteGuestFs;if(s=="network")return Capability::Network;if(s=="process.launch")return Capability::LaunchProcess;return Capability::HostBridge;}
}
AppHost::AppHost(std::filesystem::path root):root_(std::move(root)),fs_(root_/"lyfs"),packages_(fs_,security_,&processes_),snapshots_(root_/"snapshots"),settings_(root_/"system"/"settings.conf"),applications_(processes_,packages_),store_(packages_),runtime_(fs_,processes_,security_,packages_),gecko_(root_/"gecko"),hardware_(vm_),windows_(1280,720),recovery_(fs_,vm_,root_){}
void AppHost::boot(){
 if(!fs_.ready()) fs_.format();
 security_.trustPublisher("LYCAN");
 settings_.load();
 const auto&s=settings_.get(); hardware_.configure(s.virtualCpus,s.ramBytes,s.width,s.height); windows_.setDesktopSize(s.width,s.height);
 for(const auto& app:builtins()){
  bool present=false; for(const auto&p:packages_.installed())if(p.id==app.id){present=true;break;}
  if(!present)packages_.install(app);
  applications_.registerPackageSurface(app);
  runtime_.installEnvironment(app);
  for(const auto&perm:app.permissions)security_.grant(app.id,capabilityFrom(perm));
 }
 if(!processes_.findApp("init"))processes_.spawn("init",8192);
 if(!processes_.findApp("desktop"))processes_.spawn("desktop",16384);
 vm_.boot();
 std::string welcome;if(!fs_.readText("/home/Welcome.txt",welcome))fs_.writeText("/home/Welcome.txt","Welcome to LYCAN OS 1.0\nThis is a virtual guest environment hosted by Windows.\n");
}
std::string AppHost::execute(const std::string&cmd){
 if(cmd=="help")return "help  pwd  ls [path]  cat <path>  write <path> <text>  mkdir <path>  rm <path>  rename <from> <to>  ps  launcher  apps  sessions  runtime  ipc  open <id>  suspend <pid>  resume <pid>  close <id>  install <id>  uninstall <id>  store install <id>  web start  web tab <url>  web tabs  web close <id>  snapshot <name>  snapshots  restore <name>  delete-snapshot <name>  diagnostics  hardware  security  settings  set <key> <value>  recover verify  recover safe  recover backup <host-path>  vm  clear";
 if(cmd=="pwd")return "/home";
 if(cmd=="ls"||cmd.rfind("ls ",0)==0){auto path=cmd=="ls"?"/home":cmd.substr(3);std::string s;for(auto&e:fs_.list(path))s+=(e.directory?"DIR  ":"FILE ")+e.path+"  "+std::to_string(e.bytes)+" bytes\n";return s.empty()?"(empty)":s;}
 if(cmd.rfind("cat ",0)==0){std::string s;return fs_.readText(cmd.substr(4),s)?s:"file not found";}
 if(cmd.rfind("write ",0)==0){auto p=cmd.find(' ',6);if(p==std::string::npos)return"usage: write /path text";return fs_.writeText(cmd.substr(6,p-6),cmd.substr(p+1))?"written":"write failed";}
 if(cmd.rfind("mkdir ",0)==0)return fs_.createDirectory(rest(cmd,6))?"directory created":"mkdir failed";
 if(cmd.rfind("rm ",0)==0)return fs_.remove(rest(cmd,3))?"removed":"remove failed";
 if(cmd.rfind("rename ",0)==0){auto p=cmd.find(' ',7);if(p==std::string::npos)return"usage: rename /from /to";return fs_.rename(cmd.substr(7,p-7),cmd.substr(p+1))?"renamed":"rename failed";}
 if(cmd=="ps"){std::string s="PID   NAME                 STATE       APP\n-----------------------------------------------\n";for(const auto&p:processes_.list())s+=std::to_string(p.pid)+"   "+p.name+"                 "+upperState(p.state)+"   "+(p.appId.empty()?"-":p.appId)+"\n";return s;}
 if(cmd=="sessions"){auto list=applications_.sessions();if(list.empty())return"(no application sessions)";std::string s="PID   APP                  SURFACE      WINDOW\n------------------------------------------------\n";for(const auto&a:list)s+=std::to_string(a.pid)+"   "+a.appId+"   "+appSurfaceName(a.surface)+"   "+(a.windowOpen?"OPEN":"CLOSED")+"\n";return s;}
 if(cmd=="runtime"){std::string s="LYCAN APP RUNTIME\n------------------\n";for(const auto&a:runtime_.list())s+=a.id+"  "+a.version+"  "+appStateName(a.state)+"  PID="+std::to_string(a.pid)+"  sandbox="+a.sandboxRoot+"\n";return s;}
 if(cmd=="ipc"){return "IPC pending messages: "+std::to_string(runtime_.ipc().size());}
 if(cmd=="launcher"){auto list=packages_.launcherApps();if(list.empty())return"(launcher empty)";std::string s="LYCAN LAUNCHER\n------------------------------\n";for(const auto&p:list)s+="Installed  "+p.name+"  ["+p.id+"]\n";return s;}
 if(cmd=="apps"){auto list=packages_.installed();if(list.empty())return"(no packages installed)";std::string s="ID                 VERSION    PUBLISHER    LOCATION                 PERMISSIONS\n--------------------------------------------------------------------------------\n";for(const auto&p:list){std::string perms;for(size_t i=0;i<p.permissions.size();++i){if(i)perms+=", ";perms+=p.permissions[i];}s+=p.id+"  "+p.version+"  "+p.publisher+"  "+p.installLocation+"  "+(perms.empty()?"(none)":perms)+"\n";}return s;}
 if(cmd.rfind("open ",0)==0||cmd.rfind("launch ",0)==0){auto id=cmd.substr(cmd.find(' ')+1);for(const auto&p:packages_.launcherApps())if(p.id==id){auto pid=applications_.open(p.id);runtime_.start(p.id);if(pid)windows_.create(pid,p.id,p.name,0);return pid?"opened "+p.name+" (PID "+std::to_string(pid)+")":"launch failed";}return"app not installed or not registered with launcher";}
 if(cmd.rfind("suspend ",0)==0){try{return applications_.suspend(uint32_t(std::stoul(cmd.substr(8))))?"suspended":"process not found";}catch(...){return"invalid pid";}}
 if(cmd.rfind("resume ",0)==0){try{return applications_.resume(uint32_t(std::stoul(cmd.substr(7))))?"resumed":"process not found";}catch(...){return"invalid pid";}}
 if(cmd.rfind("close ",0)==0){auto id=cmd.substr(6);runtime_.stop(id);applications_.close(id);return packages_.launcherContains(id)?"closed":"app not running";}
 if(cmd=="settings")return settings_.describe();
 if(cmd.rfind("set ",0)==0){auto p=cmd.find(' ',4);if(p==std::string::npos)return"usage: set key value";if(!settings_.set(cmd.substr(4,p-4),cmd.substr(p+1)))return"invalid setting";return settings_.save()?"setting saved":"setting changed but persistence failed";}
 if(cmd.rfind("store install ",0)==0){auto catalog=loadText(root_/"store"/"catalog.json");if(catalog.empty())catalog=loadText("store/catalog.json");if(catalog.empty())return"store catalog unavailable";auto r=store_.installFromCatalog(catalog,cmd.substr(14),root_/"package-cache");if(r.ok){Package p{};for(const auto&x:packages_.installed())if(x.id==cmd.substr(14))p=x;if(!p.id.empty()){runtime_.installEnvironment(p);for(const auto&perm:p.permissions)security_.grant(p.id,capabilityFrom(perm));}}return r.ok?r.message:"store install failed: "+r.message;}
 if(cmd.rfind("web ",0)==0){if(cmd=="web start"){std::string e;return gecko_.start(&e)?"Gecko started":"Gecko start failed: "+e;}if(cmd=="web tabs"){std::string s="ID  URL\n";for(const auto&t:gecko_.tabs())s+=std::to_string(t.id)+"  "+t.url+(t.active?"  *":"")+"\n";return s;}if(cmd.rfind("web tab ",0)==0){auto id=gecko_.newTab(cmd.substr(8));return id?"tab "+std::to_string(id)+" opened":"invalid URL or Gecko tab rejected";}if(cmd.rfind("web close ",0)==0){try{return gecko_.closeTab(uint32_t(std::stoul(cmd.substr(9))))?"tab closed":"tab not found";}catch(...){return"invalid tab";}}return"web start|tab <url>|tabs|close <id>";}
 if(cmd.rfind("snapshot",0)==0){auto name=cmd=="snapshot"?"manual":cmd.substr(9);return snapshots_.save(name,vm_.bootStage(),vm_.cpu().cycles())?"snapshot saved: "+name:"snapshot failed";}
 if(cmd=="snapshots"){auto list=snapshots_.list();if(list.empty())return"(no snapshots)";std::string s="NAME                 STAGE                 CYCLES\n-----------------------------------------------\n";for(const auto&i:list)s+=i.name+"                 "+i.stage+"                 "+std::to_string(i.cycles)+"\n";return s;}
 if(cmd.rfind("restore ",0)==0){SnapshotInfo i{};return snapshots_.load(cmd.substr(8),i)?"snapshot metadata loaded: "+i.name+" (stage="+i.stage+", cycles="+std::to_string(i.cycles)+")":"snapshot not found";}
 if(cmd.rfind("delete-snapshot ",0)==0)return snapshots_.remove(cmd.substr(16))?"snapshot deleted":"snapshot not found";
 if(cmd=="hardware")return hardware_.diagnostics();
 if(cmd=="security"){std::string s=security_.describe()+"\nAUDIT\n";for(const auto&e:security_.auditLog())s+=std::to_string(e.sequence)+" "+e.appId+" "+e.action+" => "+e.result+"\n";return s;}
 if(cmd.rfind("recover ",0)==0){if(cmd=="recover verify")return recovery_.verifyFilesystem().message;if(cmd=="recover safe"){recovery_.enterSafeMode();return"LYCAN Safe Mode enabled";}if(cmd.rfind("recover backup ",0)==0){auto r=recovery_.backup(cmd.substr(15));return r.message;}return"recover verify|safe|backup <host-path>";}
 if(cmd=="diagnostics")return "LYCAN DIAGNOSTICS\n------------------\nBOOT: "+vm_.bootStage()+"\nCPU CYCLES: "+std::to_string(vm_.cpu().cycles())+"\nLYFS USED: "+std::to_string(fs_.usedBytes())+" / "+std::to_string(fs_.capacityBytes())+" bytes\nPROCESSES: "+std::to_string(processes_.list().size())+"\nPACKAGES: "+std::to_string(packages_.installed().size())+"\nSESSIONS: "+std::to_string(applications_.sessions().size())+"\nWINDOWS: "+std::to_string(windows_.windows().size())+"\n"+security_.describe();
 if(cmd=="vm")return vm_.cpu().state();
 if(cmd.rfind("install ",0)==0){auto id=cmd.substr(8);for(const auto&p:builtins())if(p.id==id){if(!packages_.install(p))return"installation rejected";runtime_.installEnvironment(p);applications_.registerPackageSurface(p);return"installed "+id;}return"package not found";}
 if(cmd.rfind("uninstall ",0)==0){auto id=cmd.substr(10);runtime_.stop(id);applications_.close(id);return packages_.uninstall(id)?"uninstalled":"uninstall failed";}
 if(cmd=="clear")return "\x1b[2J\x1b[H";return"unknown command";
}
AresVm& AppHost::vm(){return vm_;} AresHardware& AppHost::hardware(){return hardware_;} Lyfs& AppHost::fs(){return fs_;} ProcessManager& AppHost::processes(){return processes_;} SecurityPolicy& AppHost::security(){return security_;} PackageManager& AppHost::packages(){return packages_;} SnapshotManager& AppHost::snapshots(){return snapshots_;} ApplicationManager& AppHost::apps(){return applications_;} AppRuntime& AppHost::runtime(){return runtime_;} GeckoRuntime& AppHost::gecko(){return gecko_;} WindowManager& AppHost::windows(){return windows_;} RecoveryManager& AppHost::recovery(){return recovery_;} SettingsManager& AppHost::settings(){return settings_;} StoreClient& AppHost::store(){return store_;}
} // namespace lycan
