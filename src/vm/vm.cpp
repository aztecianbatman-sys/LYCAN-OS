#include "vm.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>
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
bool hasParentTraversal(const std::filesystem::path& p){ for(const auto& part:p) if(part=="..") return true; return false; }
bool isPathSeparator(char c){ return c=='/'||c=='\\'; }
bool validIdChar(char c){ return std::isalnum(static_cast<unsigned char>(c))||c=='_'||c=='-'||c=='.'; }
std::string clean(const std::string& s){ auto a=s.find_first_not_of(" \t\r\n"); auto b=s.find_last_not_of(" \t\r\n"); return a==std::string::npos?"":s.substr(a,b-a+1); }
bool csvHas(const std::string& csv,const std::string& needle){ std::stringstream ss(csv); std::string x; while(std::getline(ss,x,',')) if(clean(x)==needle) return true; return false; }
}

VirtualMachine::VirtualMachine(std::filesystem::path root):root_(std::move(root)){}

void VirtualMachine::boot(){
    std::filesystem::create_directories(root_/"home");
    std::filesystem::create_directories(root_/"apps");
    std::filesystem::create_directories(root_/"system");
    std::filesystem::create_directories(root_/"snapshots");
    std::filesystem::create_directories(root_/"appdata");
    loadState();
    if(!std::filesystem::exists(root_/"home"/"Welcome.txt"))
        std::ofstream(root_/"home"/"Welcome.txt")<<"LYCAN OS\nA Windows-hosted virtual workspace.\n\nLYFS isolation boundary: ACTIVE\nHOST FILESYSTEM: INACCESSIBLE\nType help for commands.\n";
    if(!std::filesystem::exists(root_/"system"/"identity")) std::ofstream(root_/"system"/"identity")<<"LYCAN-GUEST-1\n";
    processes_.clear();
    processes_.push_back({1,"init","RUNNING","",8ULL*1024*1024});
    processes_.push_back({2,"desktop","RUNNING","",24ULL*1024*1024});
    rebuildMemoryMap();
    if(installed_.empty()){
        const uint64_t q=16ULL*1024*1024;
        installed_={{"lycan-terminal",{"lycan-terminal","1.0.0","storage",q}},{"lycan-files",{"lycan-files","1.0.0","storage",q}},{"lycan-web",{"lycan-web","1.0.0","network,external",q}},{"lycan-store",{"lycan-store","1.0.0","storage,network",q}},{"lycan-settings",{"lycan-settings","1.0.0","storage",q}},{"lycan-diagnostics",{"lycan-diagnostics","1.0.0","storage",q}},{"lycan-snapshots",{"lycan-snapshots","1.0.0","storage",q}},{"crawford",{"crawford","1.0.0","storage",q}}};
        for(const auto& [id,a]:installed_) ensureAppStorage(id);
        persistState();
    } else for(const auto& [id,a]:installed_) ensureAppStorage(id);
}

bool VirtualMachine::validAppId(const std::string& id) const{ if(id.empty()||id.size()>64||!std::isalnum(static_cast<unsigned char>(id[0]))) return false; for(char c:id) if(!validIdChar(c)) return false; return true; }
bool VirtualMachine::validBucket(const std::string& bucket) const{ return bucket=="data"||bucket=="config"||bucket=="cache"; }
std::filesystem::path VirtualMachine::appDataRoot(const std::string& id) const{ return root_/"appdata"/id; }
void VirtualMachine::ensureAppStorage(const std::string& id) const{ if(!validAppId(id)) return; for(const char* b:{"data","config","cache"}) std::filesystem::create_directories(appDataRoot(id)/b); }

bool VirtualMachine::validStoragePath(const std::filesystem::path& p) const{
    std::error_code a,b;
    const auto base=std::filesystem::weakly_canonical(root_/"appdata",a),target=std::filesystem::weakly_canonical(p,b);
    if(a||b)return false;
    return !hasParentTraversal(target.lexically_relative(base));
}
std::filesystem::path VirtualMachine::storagePath(const std::string& id,const std::string&bucket,const std::string&path) const{
    if(!validAppId(id)||!validBucket(bucket))return {};
    std::string q=path.empty()?".":path;
    while(!q.empty()&&(q[0]=='/'||q[0]=='\\'))q.erase(q.begin());
    const std::filesystem::path relative(q);
    if(relative.is_absolute()||hasParentTraversal(relative))return {};
    const auto bucketRoot=appDataRoot(id)/bucket;
    const auto target=bucketRoot/relative;
    std::error_code ec;
    const auto base=std::filesystem::weakly_canonical(bucketRoot,ec);
    if(ec)return {};
    const auto canonical=std::filesystem::weakly_canonical(target,ec);
    if(ec||hasParentTraversal(canonical.lexically_relative(base)))return {};
    return target;
}
uint64_t VirtualMachine::appStorageUsage(const std::string& id) const{
    if(!validAppId(id))return 0; ensureAppStorage(id); uint64_t bytes=0; std::error_code ec;
    for(auto it=std::filesystem::recursive_directory_iterator(appDataRoot(id),std::filesystem::directory_options::skip_permission_denied,ec);it!=std::filesystem::recursive_directory_iterator();it.increment(ec)){
        if(ec){ec.clear();continue;} if(it->is_regular_file(ec))bytes+=static_cast<uint64_t>(it->file_size(ec));
    }
    return bytes;
}
bool VirtualMachine::hasPermission(const std::string& id,const std::string& permission) const{ auto it=installed_.find(id); return it!=installed_.end()&&csvHas(it->second.permissions,permission); }

std::string VirtualMachine::storageList(const std::string&id,const std::string&bucket,const std::string&path) const{ if(installed_.find(id)==installed_.end())return"APP NOT REGISTERED"; if(!hasPermission(id,"storage"))return"PERMISSION DENIED: STORAGE"; ensureAppStorage(id); auto p=storagePath(id,bucket,path); if(p.empty()||!validStoragePath(p)||std::filesystem::is_symlink(p))return"ACCESS DENIED"; if(!std::filesystem::exists(p))return"PATH NOT FOUND"; if(!std::filesystem::is_directory(p))return"NOT A DIRECTORY"; std::string o; for(const auto&e:std::filesystem::directory_iterator(p))o+=std::string(e.is_directory()?"DIR  ":"FILE ")+"  "+e.path().filename().string()+"\n"; return o.empty()?"(empty)":o; }
std::string VirtualMachine::storageRead(const std::string&id,const std::string&bucket,const std::string&path) const{ if(installed_.find(id)==installed_.end())return"APP NOT REGISTERED"; if(!hasPermission(id,"storage"))return"PERMISSION DENIED: STORAGE"; auto p=storagePath(id,bucket,path); if(p.empty()||!validStoragePath(p)||std::filesystem::is_symlink(p))return"ACCESS DENIED"; if(!std::filesystem::is_regular_file(p))return"FILE NOT FOUND"; std::ifstream f(p,std::ios::binary); std::ostringstream s;s<<f.rdbuf();return s.str(); }
std::string VirtualMachine::storageWrite(const std::string&id,const std::string&bucket,const std::string&path,const std::string&text){ if(installed_.find(id)==installed_.end())return"APP NOT REGISTERED"; if(!hasPermission(id,"storage"))return"PERMISSION DENIED: STORAGE"; auto p=storagePath(id,bucket,path); if(p.empty()||!validStoragePath(p)||std::filesystem::is_symlink(p))return"ACCESS DENIED"; ensureAppStorage(id); if(std::filesystem::is_directory(p))return"NOT A FILE"; uint64_t before=std::filesystem::exists(p)?static_cast<uint64_t>(std::filesystem::file_size(p)):0,after=static_cast<uint64_t>(text.size()),used=appStorageUsage(id); if(used<before)used=0; else used-=before; if(used+after>installed_.at(id).quotaBytes)return"STORAGE QUOTA EXCEEDED"; std::filesystem::create_directories(p.parent_path()); std::ofstream f(p,std::ios::binary|std::ios::trunc);if(!f)return"WRITE FAILED";f<<text;return f?"STORAGE WROTE":"WRITE FAILED"; }
std::string VirtualMachine::storageDelete(const std::string&id,const std::string&bucket,const std::string&path){ if(installed_.find(id)==installed_.end())return"APP NOT REGISTERED"; if(!hasPermission(id,"storage"))return"PERMISSION DENIED: STORAGE"; auto p=storagePath(id,bucket,path); if(p.empty()||!validStoragePath(p)||std::filesystem::is_symlink(p))return"ACCESS DENIED"; ensureAppStorage(id); if(p==appDataRoot(id)/bucket)return"REFUSED: STORAGE ROOT"; if(!std::filesystem::exists(p))return"PATH NOT FOUND"; std::error_code ec;std::filesystem::remove_all(p,ec);return ec?"DELETE FAILED":"STORAGE DELETED"; }
std::string VirtualMachine::storageUsage(const std::string&id) const{ if(installed_.find(id)==installed_.end())return"APP NOT REGISTERED"; std::ostringstream o;o<<"STORAGE USAGE\nAPP                 "<<id<<"\nUSED                "<<appStorageUsage(id)/1024<<" KB\nQUOTA               "<<installed_.at(id).quotaBytes/1024<<" KB";return o.str(); }
std::string VirtualMachine::storageQuota(const std::string&id) const{ if(installed_.find(id)==installed_.end())return"APP NOT REGISTERED";auto used=appStorageUsage(id),q=installed_.at(id).quotaBytes;std::ostringstream o;o<<"APP STORAGE QUOTA\nAPP                 "<<id<<"\nQUOTA               "<<q/1024<<" KB\nUSED                "<<used/1024<<" KB\nAVAILABLE           "<<(q>used?(q-used)/1024:0)<<" KB";return o.str(); }

void VirtualMachine::persistState() const{ std::ofstream f(root_/"system"/"runtime.state",std::ios::trunc);if(!f)return;f<<"LYCAN-RUNTIME 3\nram_bytes="<<ramBytes_<<"\nnetwork="<<(network_?"on":"off")<<"\nnext_pid="<<nextPid_<<"\ngeneration="<<generation_<<"\npackages=";bool first=true;for(const auto&[id,a]:installed_){if(!first)f<<';';first=false;f<<id<<','<<a.version<<','<<a.permissions<<','<<a.quotaBytes;}f<<"\n"; }
void VirtualMachine::loadState(){ installed_.clear();std::ifstream f(root_/"system"/"runtime.state");if(!f)return;std::string line,header;std::getline(f,header);if(header!="LYCAN-RUNTIME 3"&&header!="LYCAN-RUNTIME 2")return;while(std::getline(f,line)){auto eq=line.find('=');if(eq==std::string::npos)continue;auto key=line.substr(0,eq),v=line.substr(eq+1);try{if(key=="ram_bytes")ramBytes_=std::max<uint64_t>(128ULL*1024*1024,std::min<uint64_t>(8ULL*1024*1024*1024,std::stoull(v)));else if(key=="network")network_=v=="on";else if(key=="next_pid")nextPid_=static_cast<uint32_t>(std::stoul(v));else if(key=="generation")generation_=std::stoull(v);else if(key=="packages"){std::stringstream ss(v);std::string item;while(std::getline(ss,item,';')){std::stringstream row(item);std::string id,ver,perms,q;if(std::getline(row,id,',')&&std::getline(row,ver,',')&&std::getline(row,perms,',')&&std::getline(row,q,','))installed_[id]={id,ver,perms,std::stoull(q)};}}}catch(...){installed_.clear();return;}} }

std::string VirtualMachine::apps() const{std::string o="APPLICATION REGISTRY\n===================\n";for(const auto&[id,a]:installed_)o+=id+"  "+a.version+"  "+(a.permissions.empty()?"none":a.permissions)+"  quota="+std::to_string(a.quotaBytes/1024)+"KB\n";return o;}
std::string VirtualMachine::ps() const{std::string o="PID   NAME              STATE       MEMORY\n-----------------------------------------------\n";for(const auto&p:processes_)o+=std::to_string(p.pid)+"   "+p.name+"              "+p.state+"       "+std::to_string(p.memoryBytes/1024)+" KB  PAGE="+std::to_string(p.pageStart)+(p.error.empty()?"":"  ERROR="+p.error)+"\n";return o;}

bool VirtualMachine::allocateProcessMemory(Process& process){
    const uint64_t pages=(process.memoryBytes+4095ULL)/4096ULL;
    if(pages==0)return true;
    const uint64_t total=ramBytes_/4096ULL;
    if(total==0||pages>total)return false;
    if(pageMap_.size()!=total)pageMap_.assign(static_cast<std::size_t>(total),0);
    uint64_t run=0,start=0;
    for(uint64_t i=0;i<total;++i){
        if(pageMap_[static_cast<std::size_t>(i)]==0){ if(run==0)start=i; if(++run==pages){ for(uint64_t p=start;p<start+pages;++p)pageMap_[static_cast<std::size_t>(p)]=1; process.pageStart=start; usedMemoryBytes_+=pages*4096ULL; return true; } }
        else run=0;
    }
    return false;
}
void VirtualMachine::releaseProcessMemory(const Process& process){
    const uint64_t pages=(process.memoryBytes+4095ULL)/4096ULL;
    if(pages==0||process.pageStart>=pageMap_.size()||pages>pageMap_.size()-process.pageStart)return;
    for(uint64_t p=process.pageStart;p<process.pageStart+pages;++p)pageMap_[static_cast<std::size_t>(p)]=0;
    const uint64_t bytes=pages*4096ULL; usedMemoryBytes_=usedMemoryBytes_>=bytes?usedMemoryBytes_-bytes:0;
}
void VirtualMachine::rebuildMemoryMap(){
    const uint64_t total=ramBytes_/4096ULL;
    pageMap_.assign(static_cast<std::size_t>(total),0); usedMemoryBytes_=0;
    for(auto& p:processes_){
        p.pageStart=0;
        if(!allocateProcessMemory(p)){ p.state="CRASHED"; p.error="MEMORY RESTORE FAILED"; p.memoryBytes=0; }
    }
}
std::string VirtualMachine::networkStatus() const{std::ostringstream o;o<<"NETWORK "<<(network_?"ONLINE":"OFFLINE")<<"\nVNET0    "<<(network_?"UP":"DOWN")<<"\nIP       10.42.0.2\nGATEWAY  10.42.0.1\nDNS      10.42.0.1";return o.str();}
std::string VirtualMachine::networkInterfaces() const{return std::string("INTERFACE  STATE  ADDRESS      TYPE\nVNET0       ")+(network_?"UP   ":"DOWN ")+"10.42.0.2    VIRTUAL\nLOOP0       UP    127.0.0.1    LOOPBACK";}
std::string VirtualMachine::memoryStatus() const{std::ostringstream o;o<<"MEMORY\nTOTAL       "<<ramBytes_/1048576ULL<<" MB\nUSED        "<<usedMemoryBytes_/1048576ULL<<" MB\nFREE        "<<(ramBytes_>usedMemoryBytes_?(ramBytes_-usedMemoryBytes_)/1048576ULL:0)<<" MB\nPAGE SIZE   4096 B\nPAGES       "<<pageMap_.size()<<" TOTAL / "<<(pageMap_.size()-(std::count(pageMap_.begin(),pageMap_.end(),static_cast<uint8_t>(1))))<<" FREE\nGENERATION  "<<generation_;return o.str();}
std::string VirtualMachine::setRam(const std::string&mb){try{auto n=std::stoull(mb)*1024ULL*1024ULL;if(n<128ULL*1024*1024||n>8ULL*1024*1024*1024)return"RAM MUST BE 128-8192 MB";if(n<usedMemoryBytes_)return"RAM BELOW CURRENT USAGE";ramBytes_=n;rebuildMemoryMap();++generation_;persistState();return"RAM SET "+std::to_string(n/1048576ULL)+" MB";}catch(...){return"INVALID RAM";}}

std::string VirtualMachine::registerApp(const std::string&spec){std::stringstream ss(spec);std::string id,v,q,perms;if(!(ss>>id>>v>>q))return"USAGE: app register <id> <version> <quota-mb> [permissions]";std::getline(ss,perms);perms=clean(perms);if(!perms.empty()&&perms.front()=='['&&perms.back()==']')perms=perms.substr(1,perms.size()-2);if(!validAppId(id))return"INVALID APP ID";try{auto quota=std::stoull(q)*1024ULL*1024ULL;if(quota<1024ULL*1024||quota>1024ULL*1024*1024)return"APP QUOTA MUST BE 1-1024 MB";for(char c:perms)if(!(std::isalnum(static_cast<unsigned char>(c))||c==','||c=='-'||c=='_'))return"INVALID PERMISSIONS";if(perms.empty())perms="storage";installed_[id]={id,v,perms,quota};ensureAppStorage(id);++generation_;persistState();return"APP REGISTERED "+id+" "+v;}catch(...){return"INVALID APP REGISTRATION";}}
std::string VirtualMachine::unregisterApp(const std::string&id){if(installed_.find(id)==installed_.end())return"APP NOT REGISTERED";if(id.rfind("lycan-",0)==0||id=="crawford")return"REFUSED: CORE APP";for(const auto&p:processes_)if(p.name==id&&p.pid>2)return"REFUSED: APP RUNNING";installed_.erase(id);std::error_code ec;std::filesystem::remove_all(appDataRoot(id),ec);++generation_;persistState();return"APP UNREGISTERED "+id;}
std::string VirtualMachine::appState(const std::string&id) const{auto ai=installed_.find(id);if(ai==installed_.end())return"APP NOT REGISTERED";auto it=std::find_if(processes_.begin(),processes_.end(),[&](const Process&p){return p.name==id&&p.pid>2;});if(it==processes_.end())return"APP STATE           STOPPED\nAPP                 "+id+"\nVERSION             "+ai->second.version;std::ostringstream o;o<<"APP STATE           "<<it->state<<"\nAPP                 "<<id<<"\nVERSION             "<<ai->second.version<<"\nPID                 "<<it->pid<<"\nMEMORY              "<<it->memoryBytes/1024<<" KB\nPAGE START          "<<it->pageStart;if(!it->error.empty())o<<"\nERROR               "<<it->error;return o.str();}

std::string VirtualMachine::diagnostics() const{size_t files=0,dirs=0;uint64_t bytes=0;std::error_code ec;if(std::filesystem::exists(root_,ec)){for(auto it=std::filesystem::recursive_directory_iterator(root_,std::filesystem::directory_options::skip_permission_denied,ec);it!=std::filesystem::recursive_directory_iterator();it.increment(ec)){if(ec){ec.clear();continue;}if(it->is_regular_file(ec)){++files;bytes+=static_cast<uint64_t>(it->file_size(ec));}else if(it->is_directory(ec))++dirs;}}size_t snaps=0;if(std::filesystem::exists(root_/"snapshots",ec))for(const auto&i:std::filesystem::directory_iterator(root_/"snapshots",ec))if(i.path().extension()==".snap")++snaps;auto usedPages=std::count(pageMap_.begin(),pageMap_.end(),static_cast<uint8_t>(1));std::ostringstream o;o<<asciiLogo()<<"LYCAN DIAGNOSTIC CORE\n=====================\nRUNTIME             ONLINE\nARES CPU            ONLINE\nVIRTUAL RAM         "<<ramBytes_/1048576ULL<<" MB\nMEMORY USED        "<<usedMemoryBytes_/1048576ULL<<" MB\nPAGE SIZE           4096 B\nVIRTUAL PAGES       "<<pageMap_.size()<<"\nALLOCATED PAGES     "<<usedPages<<"\nLYFS                ISOLATED\nGUEST ROOT          LOCALIZED\nHOST ACCESS         DENIED\nSYMLINK POLICY      RESTRICTED\nNETWORK             "<<(network_?"ONLINE":"OFFLINE")<<"\nVNET                "<<(network_?"UP":"DOWN")<<"\nPROCESSES           "<<processes_.size()<<"\nPACKAGES            "<<installed_.size()<<"\nGUEST FILES         "<<files<<"\nGUEST DIRECTORIES   "<<dirs<<"\nGUEST DATA          "<<bytes/1024<<" KB\nSNAPSHOTS           "<<snaps<<"\nVM CYCLES           "<<cycles_<<"\nRUNTIME GENERATION  "<<generation_<<"\nSECURITY            ENFORCED\n";return o.str();}

std::string VirtualMachine::snapshots() const{std::string o="SNAPSHOTS\n---------\n";std::vector<std::string>names;std::error_code ec;for(const auto&i:std::filesystem::directory_iterator(root_/"snapshots",ec))if(i.path().extension()==".snap")names.push_back(i.path().stem().string());std::sort(names.begin(),names.end());if(names.empty())return o+"(none)\n";for(const auto&n:names)o+=n+"\n";return o;}
std::string VirtualMachine::snapshotCreate(const std::string&name){if(!validSnapshotName(name))return"INVALID SNAPSHOT NAME";auto meta=snapshotPath(name),fs=snapshotFsPath(name);std::error_code ec;std::filesystem::remove(meta,ec);std::filesystem::remove_all(fs,ec);std::filesystem::create_directories(fs,ec);if(ec)return"SNAPSHOT WRITE FAILED";std::ofstream f(meta);if(!f)return"SNAPSHOT WRITE FAILED";f<<"LYCAN-SNAPSHOT 4\ncycles="<<cycles_<<"\nnetwork="<<(network_?"on":"off")<<"\nram_bytes="<<ramBytes_<<"\nnext_pid="<<nextPid_<<"\ngeneration="<<generation_<<"\npackages=";bool first=true;for(const auto&[id,a]:installed_){if(!first)f<<';';first=false;f<<id<<','<<a.version<<','<<a.permissions<<','<<a.quotaBytes;}f<<"\n";for(const char*top:{"home","apps","appdata","system"}){if(!std::filesystem::exists(root_/top,ec))continue;std::filesystem::copy(root_/top,fs/top,std::filesystem::copy_options::recursive|std::filesystem::copy_options::overwrite_existing,ec);if(ec){std::filesystem::remove_all(fs,ec);return"SNAPSHOT FILESYSTEM COPY FAILED";}}return"SNAPSHOT SAVED "+name;}
std::string VirtualMachine::snapshotInfo(const std::string&name) const{if(!validSnapshotName(name))return"INVALID SNAPSHOT NAME";std::ifstream f(snapshotPath(name));if(!f)return"SNAPSHOT NOT FOUND";return std::string((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());}
std::string VirtualMachine::snapshotRestore(const std::string&name){if(!validSnapshotName(name))return"INVALID SNAPSHOT NAME";std::ifstream f(snapshotPath(name));if(!f)return"SNAPSHOT NOT FOUND";std::string line,header;std::getline(f,header);if(header!="LYCAN-SNAPSHOT 4"&&header!="LYCAN-SNAPSHOT 3"&&header!="LYCAN-SNAPSHOT 2")return"INVALID SNAPSHOT FORMAT";uint64_t cycles=cycles_,ram=ramBytes_,gen=generation_;uint32_t pid=nextPid_;bool net=network_;std::map<std::string,AppRecord>packages;while(std::getline(f,line)){auto eq=line.find('=');if(eq==std::string::npos)continue;auto k=line.substr(0,eq),v=line.substr(eq+1);try{if(k=="cycles")cycles=std::stoull(v);else if(k=="network")net=v=="on";else if(k=="ram_bytes")ram=std::stoull(v);else if(k=="next_pid")pid=static_cast<uint32_t>(std::stoul(v));else if(k=="generation")gen=std::stoull(v);else if(k=="packages"){std::stringstream ss(v);std::string item;while(std::getline(ss,item,';')){std::stringstream row(item);std::string id,a,b,c;if(std::getline(row,id,',')&&std::getline(row,a,',')&&std::getline(row,b,',')&&std::getline(row,c,','))packages[id]={id,a,b,std::stoull(c)};}}}catch(...){return"INVALID SNAPSHOT DATA";}}auto fs=snapshotFsPath(name);if(!std::filesystem::exists(fs))return"SNAPSHOT FILESYSTEM MISSING";std::error_code ec;for(const char*top:{"home","apps","appdata","system"}){std::filesystem::remove_all(root_/top,ec);if(std::filesystem::exists(fs/top,ec))std::filesystem::copy(fs/top,root_/top,std::filesystem::copy_options::recursive|std::filesystem::copy_options::overwrite_existing,ec);if(ec)return"SNAPSHOT FILESYSTEM RESTORE FAILED";}cycles_=cycles;ramBytes_=ram;nextPid_=pid;generation_=gen;network_=net;installed_=std::move(packages);processes_.clear();processes_.push_back({1,"init","RUNNING","",8ULL*1024*1024});processes_.push_back({2,"desktop","RUNNING","",24ULL*1024*1024});rebuildMemoryMap();persistState();return"SNAPSHOT RESTORED "+name;}
std::string VirtualMachine::snapshotDelete(const std::string&name){if(!validSnapshotName(name))return"INVALID SNAPSHOT NAME";std::error_code ec;if(!std::filesystem::remove(snapshotPath(name),ec))return ec?"SNAPSHOT DELETE FAILED":"SNAPSHOT NOT FOUND";std::filesystem::remove_all(snapshotFsPath(name),ec);return"SNAPSHOT DELETED "+name;}

std::filesystem::path VirtualMachine::guestPath(const std::string&p) const{std::string q=p.empty()?"/home":p;if(q.rfind("/",0)!=0)q="/home/"+q;return root_/q.substr(1);}
bool VirtualMachine::validGuestPath(const std::filesystem::path&p) const{std::error_code a,b;auto base=std::filesystem::weakly_canonical(root_,a),target=std::filesystem::weakly_canonical(p,b);if(a||b)return false;auto rel=target.lexically_relative(base);if(hasParentTraversal(rel))return false;auto m=std::mismatch(base.begin(),base.end(),target.begin(),target.end());return m.first==base.end();}
bool VirtualMachine::isGuestRoot(const std::filesystem::path&p) const{std::error_code a,b;auto base=std::filesystem::weakly_canonical(root_,a),target=std::filesystem::weakly_canonical(p,b);return!a&&!b&&base==target;}
bool VirtualMachine::validSnapshotName(const std::string&name) const{if(name.empty()||name.size()>64||name=="."||name=="..")return false;for(char c:name)if(!validIdChar(c)||isPathSeparator(c))return false;return true;}
std::filesystem::path VirtualMachine::snapshotPath(const std::string&name) const{return root_/"snapshots"/(name+".snap");}
std::filesystem::path VirtualMachine::snapshotFsPath(const std::string&name) const{return root_/"snapshots"/(name+".fs");}

std::string VirtualMachine::execute(const std::string&c){
 ++cycles_;
 if(c=="ping")return"LYCAN VM ONLINE";
 if(c=="version")return"LYCAN OS 1.2.0\nARES VIRTUAL CORE 1.2\nGUEST ABI 3";
 if(c=="help")return"logo | ping | version | help | diagnostics | memory | vm ram <mb> | vm pages | vm page <index> | pwd | ls [path] | tree [path] | cat <path> | write <path> <text> | mkdir <path> | touch <path> | rm <path> | apps | app register <id> <version> <quota-mb> [permissions] | app state <id> | app unregister <id> | ps | suspend <app> | resume <app> | crash <app> [error] | network [on|off|status|interfaces] | network app <id> | storage <id> <data|config|cache> <path> | storage-read <id> <bucket> <path> | storage-write <id> <bucket> <path> <text> | storage-delete <id> <bucket> <path> | storage-usage <id> | storage-quota <id> | open <app> | close <app> | snapshots | snapshot <name> | snapshot-info <name> | restore <name> | delete-snapshot <name> | web start | web tab <url>";
 if(c=="logo")return asciiLogo(); if(c=="pwd")return"/home"; if(c=="diagnostics")return diagnostics(); if(c=="memory")return memoryStatus();
 if(c=="vm pages"){std::ostringstream o;auto used=std::count(pageMap_.begin(),pageMap_.end(),static_cast<uint8_t>(1));o<<"VM PAGES\nPAGE SIZE       4096 B\nTOTAL PAGES     "<<pageMap_.size()<<"\nUSED PAGES      "<<used<<"\nFREE PAGES      "<<(pageMap_.size()-used);return o.str();}
 if(c.rfind("vm page ",0)==0){try{auto n=std::stoull(clean(c.substr(8)));if(n>=pageMap_.size())return"PAGE OUT OF RANGE";std::ostringstream o;o<<"PAGE "<<n<<"\nSTATE "<<(pageMap_[static_cast<std::size_t>(n)]?"ALLOCATED":"FREE")<<"\nFRAME 0x"<<std::hex<<(n*4096ULL);return o.str();}catch(...){return"INVALID PAGE";}}
 if(c=="ls")return ls("/home");if(c.rfind("ls ",0)==0)return ls(c.substr(3));if(c=="tree")return tree("/home");if(c.rfind("tree ",0)==0)return tree(c.substr(5));if(c=="apps")return apps();if(c=="ps")return ps();
 if(c=="network"||c=="network status")return networkStatus();if(c=="network interfaces")return networkInterfaces();if(c=="network on"){network_=true;++generation_;persistState();return"NETWORK ONLINE";}if(c=="network off"){network_=false;++generation_;persistState();return"NETWORK OFFLINE";}if(c.rfind("network app ",0)==0){auto id=clean(c.substr(12));if(installed_.find(id)==installed_.end())return"APP NOT REGISTERED";return hasPermission(id,"network")?"NETWORK PERMISSION GRANTED\n"+id:"NETWORK PERMISSION DENIED\n"+id;}
 if(c.rfind("vm ram ",0)==0)return setRam(clean(c.substr(7)));if(c.rfind("app register ",0)==0)return registerApp(c.substr(13));if(c.rfind("app unregister ",0)==0)return unregisterApp(clean(c.substr(15)));if(c.rfind("app state ",0)==0)return appState(clean(c.substr(10)));
 if(c.rfind("storage-read ",0)==0){std::stringstream ss(c.substr(13));std::string id,b,p;ss>>id>>b>>p;if(id.empty()||b.empty()||p.empty())return"USAGE: storage-read <id> <bucket> <path>";return storageRead(id,b,p);}if(c.rfind("storage-write ",0)==0){std::stringstream ss(c.substr(14));std::string id,b,p;ss>>id>>b>>p;std::string text;std::getline(ss,text);text=clean(text);if(id.empty()||b.empty()||p.empty())return"USAGE: storage-write <id> <bucket> <path> <text>";return storageWrite(id,b,p,text);}if(c.rfind("storage-delete ",0)==0){std::stringstream ss(c.substr(15));std::string id,b,p;ss>>id>>b>>p;if(id.empty()||b.empty()||p.empty())return"USAGE: storage-delete <id> <bucket> <path>";return storageDelete(id,b,p);}if(c.rfind("storage-usage ",0)==0)return storageUsage(clean(c.substr(14)));if(c.rfind("storage-quota ",0)==0)return storageQuota(clean(c.substr(14)));if(c.rfind("storage ",0)==0){std::stringstream ss(c.substr(9));std::string id,b,p;ss>>id>>b>>p;if(id.empty()||b.empty())return"USAGE: storage <id> <bucket> <path>";return storageList(id,b,p);}
 if(c.rfind("mkdir ",0)==0){auto p=guestPath(c.substr(6));if(!validGuestPath(p)||isGuestRoot(p))return"ACCESS DENIED";std::error_code e;std::filesystem::create_directories(p,e);return e?"CREATE FAILED":"DIRECTORY CREATED";}if(c.rfind("touch ",0)==0){auto p=guestPath(c.substr(6));if(!validGuestPath(p)||isGuestRoot(p))return"ACCESS DENIED";std::ofstream f(p);return f?"FILE CREATED":"CREATE FAILED";}if(c.rfind("write ",0)==0){auto s=c.substr(6);auto sp=s.find(' ');if(sp==std::string::npos)return"USAGE: write <path> <text>";auto p=guestPath(s.substr(0,sp));if(!validGuestPath(p)||isGuestRoot(p)||std::filesystem::is_symlink(p))return"ACCESS DENIED";std::filesystem::create_directories(p.parent_path());std::ofstream f(p,std::ios::trunc);if(!f)return"WRITE FAILED";f<<s.substr(sp+1);return"WROTE "+p.filename().string();}
 if(c.rfind("cat ",0)==0){auto p=guestPath(c.substr(4));if(!validGuestPath(p)||std::filesystem::is_symlink(p))return"ACCESS DENIED: OUTSIDE LYFS";if(!std::filesystem::is_regular_file(p))return"FILE NOT FOUND";std::ifstream f(p);std::ostringstream s;s<<f.rdbuf();return s.str();}if(c.rfind("rm ",0)==0){auto p=guestPath(c.substr(3));if(isGuestRoot(p))return"REFUSED: LYFS ROOT IS IMMUTABLE";if(!validGuestPath(p)||std::filesystem::is_symlink(p))return"ACCESS DENIED";if(!std::filesystem::exists(p))return"PATH NOT FOUND";std::error_code e;std::filesystem::remove_all(p,e);return e?"DELETE FAILED":"DELETED";}
 if(c.rfind("open ",0)==0){auto id=clean(c.substr(5));auto ai=installed_.find(id);if(ai==installed_.end())return"APP NOT FOUND";auto it=std::find_if(processes_.begin(),processes_.end(),[&](const Process&p){return p.name==id&&p.pid>2;});if(it!=processes_.end()){if(it->state=="SUSPENDED"){it->state="RUNNING";it->error.clear();persistState();return"APP RESUMED "+id;}if(it->state=="CRASHED")return"APP CRASHED";return"APP ALREADY RUNNING";}Process p{nextPid_++,id,"RUNNING","",16ULL*1024*1024,0};if(!allocateProcessMemory(p))return"OUT OF VIRTUAL MEMORY";processes_.push_back(p);persistState();return"OPENED "+id;}
 if(c.rfind("suspend ",0)==0){auto id=clean(c.substr(8));auto it=std::find_if(processes_.begin(),processes_.end(),[&](const Process&p){return p.name==id&&p.pid>2;});if(it==processes_.end())return"APP NOT RUNNING";if(it->state=="CRASHED")return"APP CRASHED";it->state="SUSPENDED";persistState();return"SUSPENDED "+id;}if(c.rfind("resume ",0)==0){auto id=clean(c.substr(7));auto it=std::find_if(processes_.begin(),processes_.end(),[&](const Process&p){return p.name==id&&p.pid>2;});if(it==processes_.end())return"APP NOT RUNNING";if(it->state=="CRASHED")return"APP CRASHED";it->state="RUNNING";persistState();return"RESUMED "+id;}if(c.rfind("crash ",0)==0){auto s=c.substr(6);auto sp=s.find(' ');std::string id=sp==std::string::npos?s:s.substr(0,sp);std::string err=sp==std::string::npos?"Unspecified application failure":clean(s.substr(sp+1));auto it=std::find_if(processes_.begin(),processes_.end(),[&](const Process&p){return p.name==id&&p.pid>2;});if(it==processes_.end())return"APP NOT RUNNING";it->state="CRASHED";it->error=err;persistState();return"APP CRASHED "+id;}
 if(c.rfind("close ",0)==0){auto id=clean(c.substr(6));auto it=std::find_if(processes_.begin(),processes_.end(),[&](const Process&p){return p.name==id&&p.pid>2;});if(it==processes_.end())return"APP NOT RUNNING";releaseProcessMemory(*it);processes_.erase(it);persistState();return"CLOSED "+id;}
 if(c=="snapshots")return snapshots();if(c.rfind("snapshot-info ",0)==0)return snapshotInfo(clean(c.substr(14)));if(c.rfind("snapshot ",0)==0)return snapshotCreate(clean(c.substr(9)));if(c.rfind("restore ",0)==0)return snapshotRestore(clean(c.substr(8)));if(c.rfind("delete-snapshot ",0)==0)return snapshotDelete(clean(c.substr(16)));
 if(c=="web start")return network_?"GECKO RUNTIME READY":"NETWORK OFFLINE";if(c.rfind("web tab ",0)==0){auto u=clean(c.substr(8));if(u.empty())return"URL REQUIRED";return network_?"GECKO TAB OPENED\n"+u:"NETWORK OFFLINE";}
 return"UNKNOWN COMMAND. Type help.";
}

}
