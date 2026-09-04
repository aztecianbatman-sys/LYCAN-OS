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
    if(!std::filesystem::exists(root_/"home"/"Welcome.txt")) std::ofstream(root_/"home"/"Welcome.txt")<<"LYCAN OS\nA Windows-hosted virtual workspace.\n\nType help for commands.\n";
    processes_.clear(); processes_.push_back({1,"init","RUNNING"}); processes_.push_back({2,"desktop","RUNNING"});
    installed_={{"lycan-terminal","1.0.0"},{"lycan-files","1.0.0"},{"lycan-web","1.0.0"},{"lycan-store","1.0.0"},{"lycan-settings","1.0.0"},{"lycan-diagnostics","1.0.0"},{"lycan-snapshots","1.0.0"},{"crawford","1.0.0"}};
}
std::filesystem::path VirtualMachine::guestPath(const std::string&p) const{std::string q=p.empty()?"/home":p;if(q.rfind("/",0)!=0)q="/home/"+q;return root_/q.substr(1);}
bool VirtualMachine::validGuestPath(const std::filesystem::path&p) const{std::error_code a,b;auto base=std::filesystem::weakly_canonical(root_,a),target=std::filesystem::weakly_canonical(p,b);if(a||b)return false;auto m=std::mismatch(base.begin(),base.end(),target.begin(),target.end());return m.first==base.end();}
std::string VirtualMachine::ls(const std::string&p) const{auto d=guestPath(p);if(!validGuestPath(d))return "ACCESS DENIED";if(!std::filesystem::exists(d))return "PATH NOT FOUND";std::string o;for(auto&i:std::filesystem::directory_iterator(d))o+=(i.is_directory()?"DIR ":"FILE")+std::string("  ")+i.path().filename().string()+"\n";return o.empty()?"(empty)":o;}
std::string VirtualMachine::apps() const{std::string o="INSTALLED APPLICATIONS\n----------------------\n";for(auto&[id,v]:installed_)o+=id+"  "+v+"\n";return o;}
std::string VirtualMachine::ps() const{std::string o="PID   NAME              STATE\n-------------------------------\n";for(auto&p:processes_)o+=std::to_string(p.pid)+"   "+p.name+"              "+p.state+"\n";return o;}
std::string VirtualMachine::snapshots() const{std::string o="SNAPSHOTS\n---------\n";bool any=false;for(auto&i:std::filesystem::directory_iterator(root_/"snapshots"))if(i.path().extension()==".snap"){any=true;o+=i.path().stem().string()+"\n";}return any?o:o+"(none)\n";}
std::string VirtualMachine::diagnostics() const{return "LYCAN SYSTEM\n----------------\nARES CPU      ONLINE\nVIRTUAL RAM   "+std::to_string(ramBytes_/1048576ULL)+" MB\nLYFS          ONLINE\nSECURITY      ENFORCED\nNETWORK       "+std::string(network_?"ONLINE":"OFFLINE")+"\nCYCLES        "+std::to_string(cycles_)+"\nPROCESSES     "+std::to_string(processes_.size())+"\nAPPS          "+std::to_string(installed_.size())+"\n";}
std::string VirtualMachine::execute(const std::string&c){
 ++cycles_;
 if(c=="help")return "help | diagnostics | ls [path] | cat <path> | write <path> <text> | mkdir <path> | touch <path> | rm <path> | apps | ps | network [on|off] | open <app> | close <app> | snapshots | snapshot <name> | web start | web tab <url>";
 if(c=="diagnostics")return diagnostics();if(c=="ls")return ls("/home");if(c.rfind("ls ",0)==0)return ls(c.substr(3));if(c=="apps")return apps();if(c=="ps")return ps();if(c=="snapshots")return snapshots();if(c=="network")return network_?"NETWORK ONLINE":"NETWORK OFFLINE";if(c=="network on"){network_=true;return "NETWORK ONLINE";}if(c=="network off"){network_=false;return "NETWORK OFFLINE";}
 if(c=="web start")return network_?"GECKO RUNTIME READY":"NETWORK OFFLINE";if(c.rfind("web tab ",0)==0){auto u=c.substr(8);if(u.empty())return "URL REQUIRED";return network_?"GECKO TAB OPENED\n"+u:"NETWORK OFFLINE";}
 if(c.rfind("open ",0)==0){auto id=c.substr(5);if(!installed_.contains(id))return "APP NOT FOUND";if(std::find_if(processes_.begin(),processes_.end(),[&](const Process&p){return p.name==id&&p.pid>2;})!=processes_.end())return "APP ALREADY RUNNING";processes_.push_back({nextPid_++,id,"RUNNING"});return "OPENED "+id;}
 if(c.rfind("close ",0)==0){auto id=c.substr(6);auto it=std::find_if(processes_.begin(),processes_.end(),[&](const Process&p){return p.name==id&&p.pid>2;});if(it==processes_.end())return "APP NOT RUNNING";processes_.erase(it);return "CLOSED "+id;}
 if(c.rfind("cat ",0)==0){auto p=guestPath(c.substr(4));if(!validGuestPath(p))return "ACCESS DENIED";if(!std::filesystem::is_regular_file(p))return "FILE NOT FOUND";std::ifstream f(p);std::ostringstream s;s<<f.rdbuf();return s.str();}
 if(c.rfind("mkdir ",0)==0){auto p=guestPath(c.substr(6));if(!validGuestPath(p))return "ACCESS DENIED";std::error_code e;std::filesystem::create_directories(p,e);return e?"MKDIR FAILED":"DIRECTORY CREATED";}
 if(c.rfind("touch ",0)==0){auto p=guestPath(c.substr(6));if(!validGuestPath(p))return "ACCESS DENIED";std::filesystem::create_directories(p.parent_path());std::ofstream f(p,std::ios::app);return f?"FILE CREATED":"TOUCH FAILED";}
 if(c.rfind("write ",0)==0){auto rest=c.substr(6),sep=rest.find(' ');if(sep==std::string::npos)return "USAGE: write <path> <text>";auto p=guestPath(rest.substr(0,sep));if(!validGuestPath(p))return "ACCESS DENIED";std::filesystem::create_directories(p.parent_path());std::ofstream f(p);if(!f)return "WRITE FAILED";f<<rest.substr(sep+1);return "WROTE "+std::to_string(rest.size()-sep-1)+" BYTES";}
 if(c.rfind("rm ",0)==0){auto p=guestPath(c.substr(3));if(!validGuestPath(p))return "ACCESS DENIED";std::error_code e;auto n=std::filesystem::remove_all(p,e);return e?"REMOVE FAILED":"REMOVED "+std::to_string(n)+" ITEM(S)";}
 if(c.rfind("snapshot ",0)==0){auto n=c.substr(9);if(n.empty())return "SNAPSHOT NAME REQUIRED";std::ofstream(root_/"snapshots"/(n+".snap"))<<"LYCAN SNAPSHOT\ncycles="<<cycles_<<"\nnetwork="<<(network_?"on":"off")<<"\nprocesses="<<processes_.size()<<"\n";return "SNAPSHOT SAVED "+n;}
 return "UNKNOWN COMMAND";
}
}
