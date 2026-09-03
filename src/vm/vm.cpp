#include "vm.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace lycan {
VirtualMachine::VirtualMachine(std::filesystem::path root):root_(std::move(root)){}
void VirtualMachine::boot(){
    std::filesystem::create_directories(root_/"home");
    std::filesystem::create_directories(root_/"apps");
    std::filesystem::create_directories(root_/"system");
    std::filesystem::create_directories(root_/"snapshots");
    std::ofstream(root_/"home"/"Welcome.txt")<<"LYCAN OS\nA Windows-hosted virtual workspace.\n";
    processes_.clear(); processes_.push_back({1,"init","RUNNING"}); processes_.push_back({2,"desktop","RUNNING"});
    installed_={{"lycan-terminal","1.0.0"},{"lycan-files","1.0.0"},{"lycan-web","1.0.0"},{"lycan-store","1.0.0"},{"lycan-settings","1.0.0"},{"lycan-diagnostics","1.0.0"},{"lycan-snapshots","1.0.0"},{"crawford","1.0.0"}};
}
std::filesystem::path VirtualMachine::guestPath(const std::string&p) const{ std::string q=p.empty()?"/home":p; if(q.rfind("/",0)!=0)q="/home/"+q; return root_/q.substr(1); }
std::string VirtualMachine::ls(const std::string&p) const{auto d=guestPath(p);if(!std::filesystem::exists(d))return "PATH NOT FOUND";std::string out;for(auto&i:std::filesystem::directory_iterator(d))out+=(i.is_directory()?"DIR ":"FILE")+std::string("  ")+i.path().filename().string()+"\n";return out.empty()?"(empty)":out;}
std::string VirtualMachine::apps() const{std::string o="INSTALLED APPLICATIONS\n----------------------\n";for(auto&[id,v]:installed_)o+=id+"  "+v+"\n";return o;}
std::string VirtualMachine::diagnostics() const{std::string o="LYCAN SYSTEM\n----------------\nARES CPU      ONLINE\nVIRTUAL RAM   512 MB\nLYFS          ONLINE\nSECURITY      ENFORCED\nNETWORK       "+std::string(network_?"ONLINE":"OFFLINE")+"\nCYCLES        "+std::to_string(cycles_)+"\nPROCESSES     "+std::to_string(processes_.size())+"\nAPPS          "+std::to_string(installed_.size())+"\n";return o;}
std::string VirtualMachine::execute(const std::string&c){++cycles_;if(c=="help")return "help | diagnostics | ls [path] | apps | ps | network | open <app> | close <app> | snapshot <name>";if(c=="diagnostics")return diagnostics();if(c=="ls")return ls("/home");if(c.rfind("ls ",0)==0)return ls(c.substr(3));if(c=="apps")return apps();if(c=="network")return network_?"NETWORK ONLINE":"NETWORK OFFLINE";if(c.rfind("open ",0)==0){auto id=c.substr(5);auto it=installed_.find(id);if(it==installed_.end())return "APP NOT FOUND";processes_.push_back({nextPid_++,id,"RUNNING"});return "OPENED "+id;}if(c.rfind("close ",0)==0){auto id=c.substr(6);auto it=std::find_if(processes_.begin(),processes_.end(),[&](const Process&p){return p.name==id&&p.pid>2;});if(it==processes_.end())return "APP NOT RUNNING";processes_.erase(it);return "CLOSED "+id;}if(c=="ps"){std::string o="PID   NAME              STATE\n-------------------------------\n";for(auto&p:processes_)o+=std::to_string(p.pid)+"   "+p.name+"              "+p.state+"\n";return o;}if(c.rfind("snapshot ",0)==0){auto n=c.substr(9);std::ofstream(root_/"snapshots"/(n+".snap"))<<"LYCAN SNAPSHOT\ncycles="<<cycles_<<"\n";return "SNAPSHOT SAVED "+n;}return "UNKNOWN COMMAND";}
}
