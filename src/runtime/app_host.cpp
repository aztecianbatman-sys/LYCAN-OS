#include "app_host.h"
#include <algorithm>
#include <array>
#include <sstream>
#include <cctype>
#include <fstream>
namespace lycan {
namespace {
Package builtin(const char* id,const char* name,const char* description,const char* entry,const char* permissions){Package p;p.id=id;p.name=name;p.version="1.0.0";p.publisher="LYCAN";p.description=description;p.entry=entry;std::stringstream ss(permissions);std::string x;while(std::getline(ss,x,','))if(!x.empty())p.permissions.push_back(x);return p;}
const std::array<Package,6>& builtins(){static const std::array<Package,6> apps={builtin("lycan-terminal","Terminal","Native ARES guest command workspace","builtin://terminal","lyfs.read,lyfs.write,process.launch"),builtin("lycan-files","Files","LYFS guest file manager","builtin://files","builtin://files","lyfs.read,lyfs.write,process.launch"),builtin("lycan-web","Web","Mozilla Gecko web surface","builtin://web","network,lyfs.read,process.launch"),builtin("lycan-snapshots","Snapshots","Save and restore guest state","builtin://snapshots","snapshot.read,snapshot.write,process.launch"),builtin("lycan-diagnostics","Diagnostics","VM health and security inspection","builtin://diagnostics","vm.read,security.read,process.launch"),builtin("crawford","Crawford","AI integration boundary for LYCAN apps","builtin://crawford","network,process.launch")};return apps;}
std::string upperState(const std::string&s){std::string r=s;std::transform(r.begin(),r.end(),r.begin(),[](unsigned char c){return char(std::toupper(c));});return r;}
std::string rest(const std::string& c,size_t n){return c.size()>n?c.substr(n):std::string{};}
}
AppHost::AppHost(std::filesystem::path root):root_(std::move(root)),fs_(root_/"lyfs"),packages_(fs_,security_,&processes_),snapshots_(root_/"snapshots"),settings_(root_/"system"/"settings.conf"),applications_(processes_,packages_),store_(packages_){}
void AppHost::boot(){fs_.format();security_.trustPublisher("LYCAN");settings_.load();for(const auto& app:builtins()){bool present=false;for(const auto&p:packages_.installed())if(p.id==app.id){present=true;break;}if(!present)packages_.install(app);applications_.registerPackageSurface(app);}if(!processes_.findApp("init"))processes_.spawn("init",8192);if(!processes_.findApp("desktop"))processes_.spawn("desktop",16384);vm_.boot();std::string welcome;if(!fs_.readText("/home/Welcome.txt",welcome))fs_.writeText("/home/Welcome.txt","Welcome to LYCAN OS 1.0\nThis is a virtual guest environment hosted by Windows.\n");}
static std::string loadText(const std::filesystem::path& p){std::ifstream f(p);return f?std::string((std::istreambuf_iterator<char>(f)),{}):std::string{};}
std::string AppHost::execute(const std::string&cmd){
 if(cmd=="help")return "help  pwd  ls [path]  cat <path>  write <path> <text>  mkdir <path>  rm <path>  rename <from> <to>  ps  launcher  apps  sessions  open <id>  suspend <pid>  resume <pid>  close <id>  install <id>  uninstall <id>  store install <id>  snapshot <name>  snapshots  restore <name>  delete-snapshot <name>  diagnostics  settings  set <key> <value>  vm  clear";
 if(cmd=="pwd")return "/home";
 if(cmd=="ls"||cmd.rfind("ls ",0)==0){auto path=cmd=="ls"?"/home":cmd.substr(3);std::string s;for(auto&e:fs_.list(path))s+=(e.directory?"DIR  ":"FILE ")+e.path+"  "+std::to_string(e.bytes)+" bytes\n";return s.empty()?"(empty)":s;}
 if(cmd.rfind("cat ",0)==0){std::string s;return fs_.readText(cmd.substr(4),s)?s:"file not found";}
 if(cmd.rfind("write ",0)==0){auto p=cmd.find(' ',6);if(p==std::string::npos)return"usage: write /path text";return fs_.writeText(cmd.substr(6,p-6),cmd.substr(p+1))?"written":"write failed";}
 if(cmd.rfind("mkdir ",0)==0)return fs_.createDirectory(rest(cmd,6))?"directory created":"mkdir failed";
 if(cmd.rfind("rm ",0)==0)return fs_.remove(rest(cmd,3))?"removed":"remove failed";
 if(cmd.rfind("rename ",0)==0){auto p=cmd.find(' ',7);if(p==std::string::npos)return"usage: rename /from /to";return fs_.rename(cmd.substr(7,p-7),cmd.substr(p+1))?"renamed":"rename failed";}
 if(cmd=="ps"){std::string s="PID   NAME                 STATE       APP\n-----------------------------------------------\n";for(const auto&p:processes_.list())s+=std::to_string(p.pid)+"   "+p.name+"                 "+upperState(p.state)+"   "+(p.appId.empty()?"-":p.appId)+"\n";return s;}
 if(cmd=="sessions"){auto list=applications_.sessions();if(list.empty())return"(no application sessions)";std::string s="PID   APP                  SURFACE      WINDOW\n------------------------------------------------\n";for(const auto&a:list)s+=std::to_string(a.pid)+"   "+a.appId+"   "+appSurfaceName(a.surface)+"   "+(a.windowOpen?"OPEN":"CLOSED")+"\n";return s;}
 if(cmd=="launcher"){auto list=packages_.launcherApps();if(list.empty())return"(launcher empty)";std::string s="LYCAN LAUNCHER\n------------------------------\n";for(const auto&p:list)s+="Installed  "+p.name+"  ["+p.id+"]\n";return s;}
 if(cmd=="apps"){auto list=packages_.installed();if(list.empty())return"(no packages installed)";std::string s="ID                 VERSION    PUBLISHER    LOCATION                 PERMISSIONS\n--------------------------------------------------------------------------------\n";for(const auto&p:list){std::string perms;for(size_t i=0;i<p.permissions.size();++i){if(i)perms+=", ";perms+=p.permissions[i];}s+=p.id+"  "+p.version+"  "+p.publisher+"  "+p.installLocation+"  "+(perms.empty()?"(none)":perms)+"\n";}return s;}
 if(cmd.rfind("open ",0)==0||cmd.rfind("launch ",0)==0){auto id=cmd.substr(cmd.find(' ')+1);for(const auto&p:packages_.launcherApps())if(p.id==id){auto pid=applications_.open(p.id);return pid?"opened "+p.name+" (PID "+std::to_string(pid)+", surface "+appSurfaceName(applications_.surfaceFor(p.id))+")":"launch failed";}return"app not installed or not registered with launcher";}
 if(cmd.rfind("suspend ",0)==0){try{return applications_.suspend(uint32_t(std::stoul(cmd.substr(8))))?"suspended":"process not found";}catch(...){return"invalid pid";}}
 if(cmd.rfind("resume ",0)==0){try{return applications_.resume(uint32_t(std::stoul(cmd.substr(7))))?"resumed":"process not found";}catch(...){return"invalid pid";}}
 if(cmd.rfind("close ",0)==0)return applications_.close(cmd.substr(6))?"closed":"app not running";
 if(cmd=="settings")return settings_.describe();
 if(cmd.rfind("set ",0)==0){auto p=cmd.find(' ',4);if(p==std::string::npos)return"usage: set key value";if(!settings_.set(cmd.substr(4,p-4),cmd.substr(p+1)))return"invalid setting";return settings_.save()?"setting saved":"setting changed but persistence failed";}
 if(cmd.rfind("store install ",0)==0){auto catalog=loadText(root_/"store"/"catalog.json");if(catalog.empty())catalog=loadText("store/catalog.json");if(catalog.empty())return"store catalog unavailable";auto r=store_.installFromCatalog(catalog,cmd.substr(14),root_/"package-cache");return r.ok?r.message:"store install failed: "+r.message;}
 if(cmd=="snapshot"||cmd.rfind("snapshot ",0)==0){auto name=cmd=="snapshot"?"manual":cmd.substr(9);return snapshots_.save(name,vm_.bootStage(),vm_.cpu().cycles())?"snapshot saved: "+name:"snapshot failed";}
 if(cmd=="snapshots"){auto list=snapshots_.list();if(list.empty())return"(no snapshots)";std::string s="NAME                 STAGE                 CYCLES\n-----------------------------------------------\n";for(const auto&i:list)s+=i.name+"                 "+i.stage+"                 "+std::to_string(i.cycles)+"\n";return s;}
 if(cmd.rfind("restore ",0)==0){SnapshotInfo i{};return snapshots_.load(cmd.substr(8),i)?"snapshot loaded: "+i.name+" (stage="+i.stage+", cycles="+std::to_string(i.cycles)+")":"snapshot not found";}
 if(cmd.rfind("delete-snapshot ",0)==0)return snapshots_.remove(cmd.substr(16))?"snapshot deleted":"snapshot not found";
 if(cmd=="diagnostics"){return "LYCAN DIAGNOSTICS\n------------------\nBOOT: "+vm_.bootStage()+"\nCPU CYCLES: "+std::to_string(vm_.cpu().cycles())+"\nLYFS USED: "+std::to_string(fs_.usedBytes())+" / "+std::to_string(fs_.capacityBytes())+" bytes\nPROCESSES: "+std::to_string(processes_.list().size())+"\nPACKAGES: "+std::to_string(packages_.installed().size())+"\nSESSIONS: "+std::to_string(applications_.sessions().size())+"\nSECURITY: "+security_.describe();}
 if(cmd=="vm")return vm_.cpu().state();
 if(cmd.rfind("install ",0)==0){auto id=cmd.substr(8);for(const auto&p:builtins())if(p.id==id){if(!packages_.install(p))return"installation rejected";applications_.registerPackageSurface(p);return"installed "+id;}return"package not found";}
 if(cmd.rfind("uninstall ",0)==0){auto id=cmd.substr(10);applications_.close(id);return packages_.uninstall(id)?"uninstalled":"uninstall failed";}
 if(cmd=="clear")return "\x1b[2J\x1b[H";return"unknown command";
}
AresVm& AppHost::vm(){return vm_;} Lyfs& AppHost::fs(){return fs_;} ProcessManager& AppHost::processes(){return processes_;} SecurityPolicy& AppHost::security(){return security_;} PackageManager& AppHost::packages(){return packages_;} SnapshotManager& AppHost::snapshots(){return snapshots_;} ApplicationManager& AppHost::apps(){return applications_;} SettingsManager& AppHost::settings(){return settings_;} StoreClient& AppHost::store(){return store_;}
}
