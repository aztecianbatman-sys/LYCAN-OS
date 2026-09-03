#include "app_host.h"
#include <algorithm>
#include <array>
#include <sstream>

namespace lycan {
namespace {
Package builtin(const char* id,const char* name,const char* description,const char* entry,const char* permissions){
    Package p; p.id=id;p.name=name;p.version="1.0.0";p.publisher="LYCAN";p.description=description;p.entry=entry;
    std::stringstream ss(permissions);std::string x;while(std::getline(ss,x,','))if(!x.empty())p.permissions.push_back(x);return p;
}
const std::array<Package,6>& builtins(){
    static const std::array<Package,6> apps={
        builtin("lycan-terminal","Terminal","Native ARES guest command workspace","builtin://terminal","lyfs.read,lyfs.write,process.launch"),
        builtin("lycan-files","Files","LYFS guest file manager","builtin://files","lyfs.read,lyfs.write,process.launch"),
        builtin("lycan-web","Web","Mozilla Gecko web surface","builtin://web","network,lyfs.read,process.launch"),
        builtin("lycan-snapshots","Snapshots","Save and restore guest state","builtin://snapshots","snapshot.read,snapshot.write,process.launch"),
        builtin("lycan-diagnostics","Diagnostics","VM health and security inspection","builtin://diagnostics","vm.read,security.read,process.launch"),
        builtin("crawford","Crawford","AI integration boundary for LYCAN apps","builtin://crawford","network,process.launch")
    }; return apps;
}
std::string upperState(const std::string&s){std::string r=s;std::transform(r.begin(),r.end(),r.begin(),[](unsigned char c){return static_cast<char>(std::toupper(c));});return r;}
}

AppHost::AppHost(std::filesystem::path root):root_(std::move(root)),fs_(root_/"lyfs"),packages_(fs_,security_,&processes_),snapshots_(root_/"snapshots"){}

void AppHost::boot(){
    fs_.format(); security_.trustPublisher("LYCAN");
    for(const auto& app:builtins()){
        bool present=false;for(const auto&p:packages_.installed())if(p.id==app.id){present=true;break;}
        if(!present)packages_.install(app);
    }
    if(!processes_.findApp("init"))processes_.spawn("init",8192);
    if(!processes_.findApp("desktop"))processes_.spawn("desktop",16384);
    vm_.boot();
    std::string welcome;if(!fs_.readText("/home/Welcome.txt",welcome))fs_.writeText("/home/Welcome.txt","Welcome to LYCAN OS 1.0\nThis is a virtual guest environment hosted by Windows.\n");
}

std::string AppHost::execute(const std::string&cmd){
    if(cmd=="help")return "help  ls  cat <path>  write <path> <text>  ps  launcher  apps  open <id>  suspend <pid>  resume <pid>  close <id>  install <id>  uninstall <id>  clear";
    if(cmd=="ls"){std::string s;for(auto&e:fs_.list("/home"))s+=e.path+"\n";return s.empty()?"(empty)":s;}
    if(cmd.rfind("cat ",0)==0){std::string s;return fs_.readText(cmd.substr(4),s)?s:"file not found";}
    if(cmd.rfind("write ",0)==0){auto p=cmd.find(' ',6);if(p==std::string::npos)return"usage: write /path text";return fs_.writeText(cmd.substr(6,p-6),cmd.substr(p+1))?"written":"write failed";}
    if(cmd=="ps"){
        std::string s="PID   NAME                 STATE       APP\n";s+="-----------------------------------------------\n";
        for(const auto&p:processes_.list())s+=std::to_string(p.pid)+"   "+p.name+"                 "+upperState(p.state)+"   "+(p.appId.empty()?"-":p.appId)+"\n";return s;
    }
    if(cmd=="launcher"){
        auto list=packages_.launcherApps();if(list.empty())return "(launcher empty)";std::string s="LYCAN LAUNCHER\n------------------------------\n";
        for(const auto&p:list){bool pin=p.id=="lycan-terminal"||p.id=="lycan-files"||p.id=="lycan-web";s+=(pin?"Pinned  ":"Installed ")+p.name+"  ["+p.id+"]\n";}return s;
    }
    if(cmd=="apps"){
        auto list=packages_.installed();if(list.empty())return "(no packages installed)";std::string s="ID                 VERSION    PUBLISHER    LOCATION                 PERMISSIONS\n";s+="--------------------------------------------------------------------------------\n";
        for(const auto&p:list){std::string perms;for(size_t i=0;i<p.permissions.size();++i){if(i)perms+=", ";perms+=p.permissions[i];}s+=p.id+"  "+p.version+"  "+p.publisher+"  "+p.installLocation+"  "+(perms.empty()?"(none)":perms)+"\n";}return s;
    }
    if(cmd.rfind("open ",0)==0||cmd.rfind("launch ",0)==0){auto pos=cmd.find(' ');auto id=cmd.substr(pos+1);for(const auto&p:packages_.launcherApps())if(p.id==id){auto pid=processes_.launchApp(p.id,p.entry);return pid?"opened "+p.name+" (PID "+std::to_string(pid)+")":"launch failed";}return "app not installed or not registered with launcher";}
    if(cmd.rfind("suspend ",0)==0){try{return processes_.suspend(static_cast<uint32_t>(std::stoul(cmd.substr(8))))?"suspended":"process not found";}catch(...){return"invalid pid";}}
    if(cmd.rfind("resume ",0)==0){try{return processes_.resume(static_cast<uint32_t>(std::stoul(cmd.substr(7))))?"resumed":"process not found";}catch(...){return"invalid pid";}}
    if(cmd.rfind("close ",0)==0)return processes_.closeApp(cmd.substr(6))?"closed":"app not running";
    if(cmd=="vm")return vm_.cpu().state();
    if(cmd.rfind("install ",0)==0){const auto id=cmd.substr(8);for(const auto&p:builtins())if(p.id==id)return packages_.install(p)?"installed "+id:"installation rejected";return "package not found";}
    if(cmd.rfind("uninstall ",0)==0)return packages_.uninstall(cmd.substr(10))?"uninstalled":"uninstall failed";
    if(cmd=="clear")return "\x1b[2J\x1b[H";
    return "unknown command";
}
AresVm& AppHost::vm(){return vm_;} Lyfs& AppHost::fs(){return fs_;} ProcessManager& AppHost::processes(){return processes_;} SecurityPolicy& AppHost::security(){return security_;} PackageManager& AppHost::packages(){return packages_;} SnapshotManager& AppHost::snapshots(){return snapshots_;}
}
