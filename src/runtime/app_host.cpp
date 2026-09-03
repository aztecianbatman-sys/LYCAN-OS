#include "app_host.h"
#include <sstream>
namespace lycan {
AppHost::AppHost(std::filesystem::path root):root_(std::move(root)),fs_(root_/"lyfs"),packages_(fs_,security_),snapshots_(root_/"snapshots"){}
void AppHost::boot(){
    // Directory initialization is idempotent and never erases existing guest data.
    fs_.format();
    security_.trustPublisher("LYCAN");
    processes_.spawn("init",8192); processes_.spawn("desktop",16384); vm_.boot();
    std::string welcome;
    if(!fs_.readText("/home/Welcome.txt",welcome)) fs_.writeText("/home/Welcome.txt","Welcome to LYCAN OS 1.0\nThis is a virtual guest environment hosted by Windows.\n");
}
std::string AppHost::execute(const std::string&cmd){
    if(cmd=="help")return "help  ls  cat <path>  write <path> <text>  ps  vm  apps  install <id>  uninstall <id>  clear";
    if(cmd=="ls"){std::string s;for(auto&e:fs_.list("/home"))s+=e.path+"\n";return s.empty()?"(empty)":s;}
    if(cmd.rfind("cat ",0)==0){std::string s;return fs_.readText(cmd.substr(4),s)?s:"file not found";}
    if(cmd.rfind("write ",0)==0){auto p=cmd.find(' ',6);if(p==std::string::npos)return"usage: write /path text";return fs_.writeText(cmd.substr(6,p-6),cmd.substr(p+1))?"written":"write failed";}
    if(cmd=="ps"){std::string s;for(auto&p:processes_.list())s+=std::to_string(p.pid)+"  "+p.name+"  "+p.state+"\n";return s;}
    if(cmd=="vm")return vm_.cpu().state();
    if(cmd=="apps"){
        std::string s; for(const auto&p:packages_.installed()) s+=p.id+"  "+p.version+"  "+p.publisher+"\n";
        return s.empty()?"(no packages installed)":s;
    }
    if(cmd.rfind("install ",0)==0){
        const auto id=cmd.substr(8);
        const Package catalog[]={{"terminal","Terminal","1.0.0","LYCAN","Command workspace","builtin"},{"files","Files","1.0.0","LYCAN","LYFS file manager","builtin"},{"web","Web","1.0.0","LYCAN","Mozilla Gecko surface","builtin"},{"snapshots","Snapshots","1.0.0","LYCAN","Save guest state","builtin"},{"diagnostics","Diagnostics","1.0.0","LYCAN","VM and security tools","builtin"},{"crawford","Crawford","1.0.0","LYCAN","AI integration","builtin"}};
        for(const auto&p:catalog) if(p.id==id) return packages_.install(p)?"installed "+id:"installation rejected";
        return "package not found";
    }
    if(cmd.rfind("uninstall ",0)==0)return packages_.uninstall(cmd.substr(10))?"uninstalled":"uninstall failed";
    if(cmd=="clear")return "\x1b[2J\x1b[H";
    return "unknown command";
}
AresVm&AppHost::vm(){return vm_;} Lyfs&AppHost::fs(){return fs_;} ProcessManager&AppHost::processes(){return processes_;} SecurityPolicy&AppHost::security(){return security_;} PackageManager&AppHost::packages(){return packages_;} SnapshotManager&AppHost::snapshots(){return snapshots_;}
}
