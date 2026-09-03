#include "app_host.h"
namespace lycan {
AppHost::AppHost(std::filesystem::path root):root_(std::move(root)),fs_(root_/"lyfs"),packages_(fs_,security_),snapshots_(root_/"snapshots"){}
void AppHost::boot(){
    fs_.format(); security_.trustPublisher("LYCAN");
    processes_.spawn("init",8192); processes_.spawn("desktop",16384); vm_.boot();
    std::string welcome;
    if(!fs_.readText("/home/Welcome.txt",welcome)) fs_.writeText("/home/Welcome.txt","Welcome to LYCAN OS 1.0\nThis is a virtual guest environment hosted by Windows.\n");
}
std::string AppHost::execute(const std::string&cmd){
    if(cmd=="help")return "help  ls  cat <path>  write <path> <text>  ps  vm  clear";
    if(cmd=="ls"){std::string s;for(auto&e:fs_.list("/home"))s+=e.path+"\n";return s.empty()?"(empty)":s;}
    if(cmd.rfind("cat ",0)==0){std::string s;return fs_.readText(cmd.substr(4),s)?s:"file not found";}
    if(cmd.rfind("write ",0)==0){auto p=cmd.find(' ',6);if(p==std::string::npos)return"usage: write /path text";return fs_.writeText(cmd.substr(6,p-6),cmd.substr(p+1))?"written":"write failed";}
    if(cmd=="ps"){std::string s;for(auto&p:processes_.list())s+=std::to_string(p.pid)+"  "+p.name+"  "+p.state+"\n";return s;}
    if(cmd=="vm")return vm_.cpu().state();
    return "unknown command";
}
AresVm&AppHost::vm(){return vm_;} Lyfs&AppHost::fs(){return fs_;} ProcessManager&AppHost::processes(){return processes_;} SecurityPolicy&AppHost::security(){return security_;} PackageManager&AppHost::packages(){return packages_;} SnapshotManager&AppHost::snapshots(){return snapshots_;}
}
