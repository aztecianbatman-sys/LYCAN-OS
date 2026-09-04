#include "vm.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
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
bool validIdChar(char c){ return std::isalnum(static_cast<unsigned char>(c)) || c=='_' || c=='-' || c=='.'; }
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

bool VirtualMachine::validSnapshotName(const std::string& name) const{
    if(name.empty() || name.size()>64 || name=="." || name=="..") return false;
    for(char c:name) if(!validIdChar(c) || isPathSeparator(c)) return false;
    return true;
}

std::filesystem::path VirtualMachine::snapshotPath(const std::string& name) const{
    return root_/"snapshots"/(name+".snap");
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

std::string VirtualMachine::tree(const std::string&p) const{
    const auto base=guestPath(p);
    if(!validGuestPath(base) && !isGuestRoot(base)) return "ACCESS DENIED: OUTSIDE LYFS";
    if(!std::filesystem::is_directory(base)) return "NOT A DIRECTORY";
    std::string o;
    std::function<void(const std::filesystem::path&,std::string)> walk=[&](const auto& dir,std::string prefix){
        std::vector<std::filesystem::directory_entry> entries;
        for(const auto& e:std::filesystem::directory_iterator(dir)) entries.push_back(e);
        std::sort(entries.begin(),entries.end(),[](const auto&a,const auto&b){return a.path().filename().string()<b.path().filename().string();});
        for(std::size_t i=0;i<entries.size();++i){
            const auto& e=entries[i]; const bool last=i+1==entries.size();
            o+=prefix+(last?"└─ ":"├─ ")+e.path().filename().string();
            if(e.is_directory() && !e.is_symlink()){o+="/\n";walk(e.path(),prefix+(last?"   ":"│  "));}
            else{o+="\n";}
        }
    };
    o=base.filename().string()+"/\n";
    walk(base," ");
    return o;
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
    std::vector<std::string> names;
    for(auto&i:std::filesystem::directory_iterator(root_/"snapshots"))
        if(i.path().extension()==".snap") names.push_back(i.path().stem().string());
    std::sort(names.begin(),names.end());
    if(names.empty()) return o+"(none)\n";
    for(const auto& n:names) o+=n+"\n";
    return o;
}

std::string VirtualMachine::diagnostics() const{
    std::size_t fileCount=0,dirCount=0,bytes=0;
    std::error_code ec;
    if(std::filesystem::exists(root_,ec)){
        for(auto it=std::filesystem::recursive_directory_iterator(root_,std::filesystem::directory_options::skip_permission_denied,ec); it!=std::filesystem::recursive_directory_iterator(); it.increment(ec)){
            if(ec){ec.clear();continue;}
            if(it->is_regular_file(ec)){++fileCount;bytes += static_cast<std::size_t>(it->file_size(ec));}
            else if(it->is_directory(ec)) ++dirCount;
        }
    }
    std::ostringstream o;
    o<<asciiLogo()
     <<"LYCAN DIAGNOSTIC CORE\n"
     <<"=====================\n"
     <<"RUNTIME             ONLINE\n"
     <<"ARES CPU            ONLINE\n"
     <<"VIRTUAL RAM         "<<ramBytes_/1048576ULL<<" MB\n"
     <<"LYFS                ISOLATED\n"
     <<"GUEST ROOT          LOCALIZED\n"
     <<"HOST ACCESS         DENIED\n"
     <<"SYMLINK POLICY      RESTRICTED\n"
     <<"NETWORK             "<<(network_?"ONLINE":"OFFLINE")<<"\n"
     <<"PROCESSES           "<<processes_.size()<<"\n"
     <<"PACKAGES            "<<installed_.size()<<"\n"
     <<"GUEST FILES         "<<fileCount<<"\n"
     <<"GUEST DIRECTORIES   "<<dirCount<<"\n"
     <<"GUEST DATA          "<<bytes/1024<<" KB\n"
     <<"SNAPSHOTS           ";
    std::size_t snaps=0;
    for(auto&i:std::filesystem::directory_iterator(root_/"snapshots",ec)) if(i.path().extension()==".snap") ++snaps;
    o<<snaps<<"\n"
     <<"VM CYCLES           "<<cycles_<<"\n"
     <<"SECURITY            ENFORCED\n";
    return o.str();
}

std::string VirtualMachine::snapshotCreate(const std::string& name){
    if(!validSnapshotName(name)) return "INVALID SNAPSHOT NAME";
    std::ofstream f(snapshotPath(name),std::ios::trunc);
    if(!f) return "SNAPSHOT WRITE FAILED";
    f<<"LYCAN-SNAPSHOT 1\n"
     <<"cycles="<<cycles_<<"\n"
     <<"network="<<(network_?"on":"off")<<"\n"
     <<"next_pid="<<nextPid_<<"\n"
     <<"processes=";
    for(std::size_t i=0;i<processes_.size();++i){ if(i) f<<";"; f<<processes_[i].pid<<","<<processes_[i].name<<","<<processes_[i].state; }
    f<<"\n";
    f<<"packages=";
    bool first=true; for(const auto& [id,v]:installed_){ if(!first) f<<";"; first=false; f<<id<<","<<v; }
    f<<"\n";
    return "SNAPSHOT SAVED "+name;
}

std::string VirtualMachine::snapshotInfo(const std::string& name) const{
    if(!validSnapshotName(name)) return "INVALID SNAPSHOT NAME";
    std::ifstream f(snapshotPath(name)); if(!f) return "SNAPSHOT NOT FOUND";
    std::string content((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
    return content;
}

std::string VirtualMachine::snapshotRestore(const std::string& name){
    if(!validSnapshotName(name)) return "INVALID SNAPSHOT NAME";
    std::ifstream f(snapshotPath(name)); if(!f) return "SNAPSHOT NOT FOUND";
    std::string line,header; std::getline(f,header);
    if(header!="LYCAN-SNAPSHOT 1") return "INVALID SNAPSHOT FORMAT";
    std::vector<Process> restored;
    std::map<std::string,std::string> packages=installed_;
    uint64_t cycles=cycles_; bool network=network_; uint32_t nextPid=nextPid_;
    while(std::getline(f,line)){
        auto eq=line.find('='); if(eq==std::string::npos) continue;
        const auto key=line.substr(0,eq),value=line.substr(eq+1);
        try{
            if(key=="cycles") cycles=std::stoull(value);
            else if(key=="network") network=value=="on";
            else if(key=="next_pid") nextPid=static_cast<uint32_t>(std::stoul(value));
            else if(key=="processes"){
                restored.clear(); std::stringstream ss(value); std::string item;
                while(std::getline(ss,item,';')){std::stringstream row(item);std::string a,b,c;if(std::getline(row,a,',')&&std::getline(row,b,',')&&std::getline(row,c,',')) restored.push_back({static_cast<uint32_t>(std::stoul(a)),b,c});}
            } else if(key=="packages"){
                packages.clear(); std::stringstream ss(value); std::string item;
                while(std::getline(ss,item,';')){auto comma=item.find(',');if(comma!=std::string::npos) packages[item.substr(0,comma)]=item.substr(comma+1);}
            }
        }catch(...){ return "INVALID SNAPSHOT DATA"; }
    }
    if(restored.empty()) return "INVALID SNAPSHOT DATA";
    cycles_=cycles; network_=network; nextPid_=nextPid; processes_=std::move(restored); installed_=std::move(packages);
    return "SNAPSHOT RESTORED "+name;
}

std::string VirtualMachine::snapshotDelete(const std::string& name){
    if(!validSnapshotName(name)) return "INVALID SNAPSHOT NAME";
    std::error_code ec; if(!std::filesystem::remove(snapshotPath(name),ec)) return ec?"SNAPSHOT DELETE FAILED":"SNAPSHOT NOT FOUND";
    return "SNAPSHOT DELETED "+name;
}

std::string VirtualMachine::execute(const std::string&c){
    ++cycles_;
    if(c=="ping") return "LYCAN VM ONLINE";
    if(c=="version") return "LYCAN OS 1.0.0\nARES VIRTUAL CORE 1.0\nGUEST ABI 1";
    if(c=="help") return "logo | ping | version | help | diagnostics | pwd | ls [path] | tree [path] | cat <path> | write <path> <text> | mkdir <path> | touch <path> | rm <path> | apps | ps | network [on|off] | open <app> | close <app> | snapshots | snapshot <name> | snapshot-info <name> | restore <name> | delete-snapshot <name> | web start | web tab <url>";
    if(c=="logo") return asciiLogo();
    if(c=="pwd") return "/home";
    if(c=="diagnostics") return diagnostics();
    if(c=="ls") return ls("/home");
    if(c.rfind("ls ",0)==0) return ls(c.substr(3));
    if(c=="tree") return tree("/home");
    if(c.rfind("tree ",0)==0) return tree(c.substr(5));
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

    if(c.rfind("snapshot-info ",0)==0) return snapshotInfo(c.substr(15));
    if(c.rfind("restore ",0)==0) return snapshotRestore(c.substr(8));
    if(c.rfind("delete-snapshot ",0)==0) return snapshotDelete(c.substr(16));
    if(c.rfind("snapshot ",0)==0) return snapshotCreate(c.substr(9));

    return "UNKNOWN COMMAND";
}
}
