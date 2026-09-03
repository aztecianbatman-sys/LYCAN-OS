#include "package_manager.h"
#include "package_archive.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>

namespace lycan {
namespace {
bool isSha256(const std::string& s) {
    if (s.size() != 64) return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isxdigit(c) != 0; });
}
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}
std::string jsonStringField(const std::string& object, const std::string& key) {
    const std::string needle="\""+key+"\"";
    auto k=object.find(needle); if(k==std::string::npos)return{};
    auto colon=object.find(':',k+needle.size()); if(colon==std::string::npos)return{};
    auto q=object.find('"',colon+1); if(q==std::string::npos)return{};
    std::string v; bool escaped=false;
    for(size_t i=q+1;i<object.size();++i){char c=object[i];if(escaped){v+=c;escaped=false;continue;}if(c=='\\'){escaped=true;continue;}if(c=='"')break;v+=c;}
    return v;
}
std::string securityFailure() { return "LYCAN SECURITY\n\nPackage rejected.\n\nReason:\nSHA-256 verification failed.\n\nThe package was NOT installed."; }
std::string transactionFailure(const std::string& reason) { return "LYCAN PACKAGE MANAGER\n\nInstallation failed.\n\nReason:\n"+reason+"\n\nThe previous version was restored."; }
}

PackageManager::PackageManager(Lyfs&fs,SecurityPolicy&s):fs_(fs),security_(s),db_(fs_.hostPath("/system/packages.db")){}

bool PackageManager::validId(const std::string&id){if(id.empty()||id.size()>80)return false;for(unsigned char c:id)if(!(std::isalnum(c)||c=='-'||c=='_'||c=='.'))return false;return true;}

bool PackageManager::install(const Package&p){
    if(!validId(p.id)||p.publisher.empty()||!security_.publisherTrusted(p.publisher))return false;
    auto target=fs_.hostPath("/apps/"+p.id),cache=fs_.hostPath("/package-cache");std::error_code ec;
    std::filesystem::create_directories(cache,ec);if(ec)return false;
    const auto token=std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto stage=cache/(p.id+"."+token+".tmp"),backup=cache/(p.id+"."+token+".backup");
    std::filesystem::create_directories(stage,ec);if(ec)return false;
    auto cleanup=[&]{std::error_code x;std::filesystem::remove_all(stage,x);std::filesystem::remove_all(backup,x);};
    std::string m="{\n  \"id\": \""+p.id+"\",\n  \"name\": \""+p.name+"\",\n  \"version\": \""+p.version+"\",\n  \"publisher\": \""+p.publisher+"\",\n  \"entry\": \""+p.entry+"\"\n}\n";
    if(!fs_.writeText("/package-cache/"+stage.filename().string()+"/manifest.json",m)){cleanup();return false;}
    const bool hadPrevious=std::filesystem::exists(target);
    if(hadPrevious){std::filesystem::rename(target,backup,ec);if(ec){cleanup();return false;}}
    std::filesystem::rename(stage,target,ec);if(ec){if(hadPrevious){std::error_code x;std::filesystem::rename(backup,target,x);}cleanup();return false;}
    std::ofstream db(db_,std::ios::app);
    if(!db||(db<<p.id<<"|"<<p.version<<"|"<<p.publisher<<"\n").fail()){std::error_code x;std::filesystem::remove_all(target,x);if(hadPrevious)std::filesystem::rename(backup,target,x);cleanup();return false;}
    cleanup();return true;
}

bool PackageManager::inspectArchive(const std::filesystem::path&path,Package&pkg,std::string*error)const{
    PackageManifest m;std::vector<PackageEntry>entries;std::string e;
    if(!PackageArchive::inspect(path,m,entries,e)){if(error)*error=e;return false;}
    pkg.id=m.id;pkg.name=m.name;pkg.version=m.version;pkg.publisher=m.publisher;pkg.entry=m.entry;pkg.permissions=m.permissions;pkg.source=path;
    if(!validId(pkg.id)){if(error)*error="invalid package id";return false;}
    if(pkg.entry.rfind("/apps/"+pkg.id+"/",0)!=0){if(error)*error="manifest entry must stay inside its app";return false;}
    return true;
}
std::string PackageManager::packageSha256(const std::filesystem::path&package){return PackageArchive::sha256File(package);}
bool PackageManager::verifySha256(const std::string&calculated,const std::string&expected){return isSha256(calculated)&&isSha256(expected)&&lower(calculated)==lower(expected);}
std::string PackageManager::catalogSha256(const std::string&catalogJson,const std::string&id,const std::string&version){size_t pos=0;while((pos=catalogJson.find("{",pos))!=std::string::npos){auto end=catalogJson.find('}',pos+1);if(end==std::string::npos)break;auto object=catalogJson.substr(pos,end-pos+1);if(jsonStringField(object,"id")==id&&jsonStringField(object,"version")==version){auto d=jsonStringField(object,"sha256");return isSha256(d)?lower(d):std::string{};}pos=end+1;}return{};}

bool PackageManager::installArchiveFromCatalog(const std::filesystem::path&package,const std::string&catalogJson,std::string*error){Package pkg;std::string e;if(!inspectArchive(package,pkg,&e)){if(error)*error=e;return false;}const auto expected=catalogSha256(catalogJson,pkg.id,pkg.version);if(!isSha256(expected)){if(error)*error=securityFailure();return false;}return installArchive(package,expected,error);}

bool PackageManager::installArchive(const std::filesystem::path&path,const std::string&expectedSha256,std::string*error){
    // TRANSACTION: verify -> stage -> validate -> backup -> commit -> persist -> cleanup.
    if(!isSha256(expectedSha256)||!verifySha256(packageSha256(path),expectedSha256)){if(error)*error=securityFailure();return false;}
    Package p;std::string e;if(!inspectArchive(path,p,&e)){if(error)*error=e;return false;}
    if(!security_.publisherTrusted(p.publisher)){if(error)*error="publisher is not trusted";return false;}

    const auto cache=fs_.hostPath("/package-cache"),target=fs_.hostPath("/apps/"+p.id);std::error_code ec;
    const auto token=std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto stage=cache/(p.id+"."+token+".tmp"),backup=cache/(p.id+"."+token+".backup"),dbBackup=cache/(p.id+"."+token+".dbbackup");
    std::filesystem::create_directories(cache,ec);if(ec){if(error)*error=transactionFailure(ec.message());return false;}
    std::filesystem::create_directories(stage,ec);if(ec){if(error)*error=transactionFailure(ec.message());return false;}
    const bool hadPrevious=std::filesystem::exists(target),hadDb=std::filesystem::exists(db_);
    if(hadDb){std::filesystem::copy_file(db_,dbBackup,std::filesystem::copy_options::overwrite_existing,ec);if(ec){std::filesystem::remove_all(stage,ec);if(error)*error=transactionFailure("cannot protect package database: "+ec.message());return false;}}

    auto rollback=[&]{
        std::error_code x;
        if(std::filesystem::exists(target))std::filesystem::remove_all(target,x);
        if(hadPrevious&&std::filesystem::exists(backup))std::filesystem::rename(backup,target,x);
        std::filesystem::remove_all(stage,x);
        if(hadDb&&std::filesystem::exists(dbBackup))std::filesystem::copy_file(dbBackup,db_,std::filesystem::copy_options::overwrite_existing,x);
        else if(!hadDb)std::filesystem::remove(db_,x);
        std::filesystem::remove(dbBackup,x);
    };

    PackageManifest stagedManifest;std::vector<PackageEntry> stagedEntries;
    if(!PackageArchive::extract(path,stage,stagedManifest,e)){rollback();if(error)*error=transactionFailure(e);return false;}
    if(stagedManifest.id!=p.id||stagedManifest.version!=p.version||stagedManifest.publisher!=p.publisher||!validId(stagedManifest.id)||stagedManifest.entry.rfind("/apps/"+p.id+"/",0)!=0||!security_.publisherTrusted(stagedManifest.publisher)){rollback();if(error)*error=transactionFailure("staged manifest validation failed");return false;}
    const std::string relativeEntry=stagedManifest.entry.substr(std::string("/apps/").size()+p.id.size()+1);
    if(relativeEntry.empty()||relativeEntry.find("..")!=std::string::npos||!std::filesystem::exists(stage/relativeEntry)){rollback();if(error)*error=transactionFailure("staged entry point is missing or unsafe");return false;}

    if(hadPrevious){std::filesystem::rename(target,backup,ec);if(ec){rollback();if(error)*error=transactionFailure("cannot stage previous version: "+ec.message());return false;}}
    std::filesystem::rename(stage,target,ec);if(ec){rollback();if(error)*error=transactionFailure("cannot commit staged application: "+ec.message());return false;}
    std::ofstream db(db_,std::ios::app);
    if(!db||(db<<p.id<<"|"<<p.version<<"|"<<p.publisher<<"\n").fail()){rollback();if(error)*error=transactionFailure("cannot update package database");return false;}
    db.close();
    std::filesystem::remove_all(backup,ec);std::filesystem::remove(dbBackup,ec);
    if(ec&&error)*error="LYCAN PACKAGE MANAGER\n\nInstallation completed with recoverable temporary files retained.\n\nReason:\n"+ec.message();
    return true;
}

bool PackageManager::uninstall(const std::string&id){return validId(id)&&fs_.remove("/apps/"+id);}
bool PackageManager::rollback(const std::string&id){return validId(id)&&fs_.ready();}
std::vector<Package> PackageManager::installed()const{std::vector<Package>v;for(auto&e:fs_.list("/apps"))if(e.directory){Package p;p.id=e.path.substr(6);p.name=p.id;p.version="1.0.0";p.publisher="LYCAN";std::string j;if(fs_.readText("/apps/"+p.id+"/manifest.json",j)){auto value=[&](const std::string&k){auto n="\""+k+"\"";auto a=j.find(n);if(a==std::string::npos)return std::string{};a=j.find(':',a+n.size());auto q=j.find('"',a);auto z=j.find('"',q+1);return q==std::string::npos||z==std::string::npos?std::string{}:j.substr(q+1,z-q-1);};auto x=value("name");if(!x.empty())p.name=x;x=value("version");if(!x.empty())p.version=x;x=value("publisher");if(!x.empty())p.publisher=x;x=value("entry");if(!x.empty())p.entry=x;}v.push_back(p);}return v;}
}
