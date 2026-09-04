#include "vm.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace lycan {
namespace {
std::string asciiLogo(){
    return
        "       /\\       /\\\n"
        "      /  \\_____/  \\     LYCAN\n"
        "     /             \\    VIRTUAL CORE\n"
        "    /   <\\\"  \">   \\\n"
        "   |      \\____/      |\n"
        "   |   /\\        /\\   |\n"
        "    \\_/  \\______/  \\_/\n"
        "       \\__LYCAN__/\n";
}

bool hasParentTraversal(const std::filesystem::path& p){
    for(const auto& part : p) if(part == "..") return true;
    return false;
}

bool isPathSeparator(char c){ return c=='/' || c=='\\'; }
}

VirtualMachine::VirtualMachine(std::filesystem::path root):root_(std::move(root)){}

void VirtualMachine::boot(){
    std::filesystem::create_directories(root_/"home");
    std::filesystem::create_directories(root_/"apps");
    std::filesystem::create_directories(root_/"system");
    std::filesystem::create_directories(root_/"snapshots");
    if(!std::filesystem::exists(root_/"home"/"Welcome.txt"))
        std::ofstream(root_/"home"/"Welcome.txt")
            << "LYCAN OS\nA Windows-hosted virtual workspace.\n\n"
               "LYFS isolation boundary: ACTIVE\n"
               "HOST FILESYSTEM: INACCESSIBLE\n"
               "Type help for commands.\n";
    if(!std::filesystem::exists(root_/"system"/"identity"))
        std::ofstream(root_/"system"/"identity")<<"LYCAN-GUEST-1\n";
    processes_.clear();
    processes_.push_back({1,"init","RUNNING"});
    processes_.push_back({2,"desktop","RUNNING"});
    installed_={{"lycan-terminal","1.0.0"},{"lycan-files","1.0.0"},{"lycan-web","1.0.0"},{"lycan-store","1.0.0"},{"lycan-settings","1.0.0"},{"lycan-diagnostics","1.0.0"},{"lycan-snapshots","1.0.0"},{"crawford","1.0.0"}};
}

std::filesystem::path VirtualMachine::guestPath(const std::string&p) const{
    std::string q=p.empty()?"/home":p;
    if(q.rfind("/",0)!=0) q="/home/"+q;
    return root_/q.substr(1);
}

bool VirtualMachine::validGuestPath(const std::filesystem::path&p) const{
    std::error_code a,b;
    const auto base=std::filesystem::weakly_canonical(root_,a);
    const auto target=std::filesystem::weakly_canonical(p,b);
    if(a||b) return false;
    const auto relative=target.lexically_relative(base);
    if(hasParentTraversal(relative)) return false;
    auto m=std::mismatch(base.begin(),base.end(),target.begin(),target.end());
    return m.first==base.end();
}

bool VirtualMachine::isGuestRoot(const std::filesystem::path&p) const{
    std::error_code a,b;
    const auto base=std::filesystem::weakly_canonical(root_,a);
    const auto target=std::filesystem::weakly_canonical(p,b);
    return !a&&!b&&base==target;
}

std::string VirtualMachine::ls(const std::string&p) const{
    const auto d=guestPath(p);
    if(!validGuestPath(d) && !isGuestRoot(d)) return "ACCESS DENIED: OUTSIDE LYFS";
    if(!std::filesystem::exists(d)) return "PATH NOT FOUND";
    if(!std::filesystem::is_directory(d)) return "NOT A DIRECTORY";
    std::string o;
    for(const auto&i:std::filesystem::directory_iterator(d)){
        if(i.is_symlink()) o+="LINK ";
        else o+=(i.is_directory()?"DIR  ":"FILE ");
        o+="  "+i.path().filename().string()+"\n";
    }
    return o.empty()?"(empty)":o;
}

std::string VirtualMachine::apps() const{
    std::string o="INSTALLED APPLICATIONS\n----------------------\n";
    for(auto&[id,v]:installed_) o+=id+"  "+v+"\n";
    return o;
}

std::string VirtualMachine::ps() const{
    std::string o="PID   NAME              STATE\n-------------------------------\n";
    for(auto&p:processes_) o+=std::to_string(p.pid)+"   "+p.name+"              "+p.state+"\n";
    return o;
}

std::string VirtualMachine::snapshots() const{
    std::string o="SNAPSHOTS\n---------\n";
    bool any=false;
    for(auto&i:std::filesystem::directory_iterator(root_/"snapshots"))
        if(i.path().extension()==".snap"){any=true;o+=i.path().stem().string()+"\n";}
    return any?o:o+"(none)\n";
}

std::string VirtualMachine::diagnostics() const{
    return asciiLogo()+"LYCAN SYSTEM\n----------------\n"
        "ARES CPU      ONLINE\n"
        "VIRTUAL RAM   "+std::to_string(ramBytes_/1048576ULL)+" MB\n"
        "LYFS          ISOLATED\n"
        "GUEST ROOT    LOCALIZED\n"
        "HOST ACCESS   DENIED\n"
        "SYMLINKS      RESTRICTED\n"
        "SECURITY      ENFORCED\n"
        "NETWORK       "+std::string(network_?"ONLINE":"OFFLINE")+"\n"
        "CYCLES        "+std::to_string(cycles_)+"\n"
        "PROCESSES     "+std::to_string(processes_.size())+"\n"
        "APPS          "+std::to_string(installed_.size())+"\n";
}

std::string VirtualMachine::execute(const std::string&c){
    ++cycles_;
    if(c=="ping") return "LYCAN VM ONLINE";
    if(c=="version") return "LYCAN OS 1.0.0\nARES VIRTUAL CORE 1.0\nGUEST ABI 1";
    if(c=="help") return "logo | ping | version | help | diagnostics | pwd | ls [path] | cat <path> | write <path> <text> | mkdir <path> | touch <path> | rm <path> | apps | ps | network [on|off] | open <app> | close <app> | snapshots | snapshot <name> | web start | web tab <url>";
    if(c=="logo") return asciiLogo();
    if(c=="pwd") return "/home";
    if(c=="diagnostics") return diagnostics();
    if(c=="ls") return ls("/home");
    if(c.rfind("ls ",0)==0) return ls(c.substr(3));
    if(c=="apps") return apps();
    if(c=="ps") return ps();
    if(c=="snapshots") return snapshots();
    if(c=="network") return network_?"NETWORK ONLINE":"NETWORK OFFLINE";
    if(c=="network on"){network_=true;return "NETWORK ONLINE";}
    if(c=="network off"){network_=false;return "NETWORK OFFLINE";}
    if(c=="web start") return network_?"GECKO RUNTIME READY":"NETWORK OFFLINE";
    if(c.rfind("web tab ",0)==0){auto u=c.substr(8);if(u.empty())return "URL REQUIRED";return network_?"GECKO TAB OPENED\n"+u:"NETWORK OFFLINE";}

    if(c.rfind("open ",0)==0){
        auto id=c.substr(5);
        if(!installed_.contains(id)) return "APP NOT FOUND";
        if(std::find_if(processes_.begin(),processes_.end(),[&](const Process&p){return p.name==id&&p.pid>2;})!=processes_.end()) return "APP ALREADY RUNNING";
        processes_.push_back({nextPid_++,id,"RUNNING"});
        return "OPENED "+id;
    }
    if(c.rfind("close ",0)==0){
        auto id=c.substr(6);
        auto it=std::find_if(processes_.begin(),processes_.end(),[&](const Process&p){return p.name==id&&p.pid>2;});
        if(it==processes_.end()) return "APP NOT RUNNING";
        processes_.erase(it);
        return "CLOSED "+id;
    }

    if(c.rfind("cat ",0)==0){
        auto p=guestPath(c.substr(4));
        if(!validGuestPath(p) || std::filesystem::is_symlink(p)) return "ACCESS DENIED: OUTSIDE LYFS";
        if(!std::filesystem::is_regular_file(p)) return "FILE NOT FOUND";
        std::ifstream f(p); std::ostringstream s; s<<f.rdbuf(); return s.str();
    }

    if(c.rfind("mkdir ",0)==0){
        auto p=guestPath(c.substr(6));
        if(!validGuestPath(p) || isGuestRoot(p)) return "ACCESS DENIED";
        if(p.filename()=="." || p.filename()=="..") return "ACCESS DENIED";
        std::error_code e; std::filesystem::create_directories(p,e);
        return e?"MKDIR FAILED":"DIRECTORY CREATED";
    }

    if(c.rfind("touch ",0)==0){
        auto p=guestPath(c.substr(6));
        if(!validGuestPath(p) || isGuestRoot(p)) return "ACCESS DENIED";
        if(std::filesystem::exists(p) && std::filesystem::is_symlink(p)) return "ACCESS DENIED: SYMLINK";
        std::filesystem::create_directories(p.parent_path());
        std::ofstream f(p,std::ios::app);
        return f?"FILE CREATED":"TOUCH FAILED";
    }

    if(c.rfind("write ",0)==0){
        auto rest=c.substr(6),sep=rest.find(' ');
        if(sep==std::string::npos) return "USAGE: write <path> <text>";
        auto p=guestPath(rest.substr(0,sep));
        if(!validGuestPath(p) || isGuestRoot(p)) return "ACCESS DENIED";
        if(std::filesystem::exists(p) && std::filesystem::is_symlink(p)) return "ACCESS DENIED: SYMLINK";
        std::filesystem::create_directories(p.parent_path());
        std::ofstream f(p);
        if(!f) return "WRITE FAILED";
        f<<rest.substr(sep+1);
        return "WROTE "+std::to_string(rest.size()-sep-1)+" BYTES";
    }

    if(c.rfind("rm ",0)==0){
        auto p=guestPath(c.substr(3));
        if(!validGuestPath(p)) return "ACCESS DENIED: OUTSIDE LYFS";
        if(isGuestRoot(p)) return "REFUSED: LYFS ROOT IS IMMUTABLE";
        if(std::filesystem::exists(p) && std::filesystem::is_symlink(p)) return "REFUSED: SYMLINK";
        std::error_code e; auto n=std::filesystem::remove_all(p,e);
        return e?"REMOVE FAILED":"REMOVED "+std::to_string(n)+" ITEM(S)";
    }

    if(c.rfind("snapshot ",0)==0){
        auto n=c.substr(9);
        if(n.empty()) return "SNAPSHOT NAME REQUIRED";
        for(char ch:n) if(isPathSeparator(ch)) return "INVALID SNAPSHOT NAME";
        std::ofstream(root_/"snapshots"/(n+".snap"))
            <<"LYCAN SNAPSHOT\ncycles="<<cycles_<<"\nnetwork="<<(network_?"on":"off")<<"\nprocesses="<<processes_.size()<<"\n";
        return "SNAPSHOT SAVED "+n;
    }
    return "UNKNOWN COMMAND";
}
}
