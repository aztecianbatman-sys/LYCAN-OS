#include "package_manager.h"
#include "package_archive.h"
#include <algorithm>
#include <cctype>
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
    for(size_t i=q+1;i<object.size();++i){
        char c=object[i];
        if(escaped){v+=c;escaped=false;continue;}
        if(c=='\\'){escaped=true;continue;}
        if(c=='"')break;
        v+=c;
    }
    return v;
}
}

PackageManager::PackageManager(Lyfs&fs,SecurityPolicy&s):fs_(fs),security_(s),db_(fs_.hostPath("/system/packages.db")){}

bool PackageManager::validId(const std::string&id){
    if(id.empty()||id.size()>80)return false;
    for(unsigned char c:id)if(!(std::isalnum(c)||c=='-'||c=='_'||c=='.'))return false;
    return true;
}

bool PackageManager::install(const Package&p){
    if(!validId(p.id)||p.publisher.empty()||!security_.publisherTrusted(p.publisher)||!fs_.createDirectory("/apps/"+p.id))return false;
    std::string m="{\n  \"id\": \""+p.id+"\",\n  \"name\": \""+p.name+"\",\n  \"version\": \""+p.version+"\",\n  \"publisher\": \""+p.publisher+"\",\n  \"entry\": \""+p.entry+"\"\n}\n";
    if(!fs_.writeText("/apps/"+p.id+"/manifest.json",m))return false;
    std::ofstream db(db_,std::ios::app);return(bool)db&&(db<<p.id<<"|"<<p.version<<"|"<<p.publisher<<"\n");
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

bool PackageManager::verifySha256(const std::string&calculated,const std::string&expected){
    if(!isSha256(calculated)||!isSha256(expected))return false;
    return lower(calculated)==lower(expected);
}

std::string PackageManager::catalogSha256(const std::string&catalogJson,const std::string&id,const std::string&version){
    // The Store catalog is JSON, but LYCAN intentionally accepts only the small
    // object shape it publishes: an app object containing id, version and sha256.
    size_t pos=0;
    while((pos=catalogJson.find("{",pos))!=std::string::npos){
        auto end=catalogJson.find('}',pos+1); if(end==std::string::npos)break;
        auto object=catalogJson.substr(pos,end-pos+1);
        if(jsonStringField(object,"id")==id && jsonStringField(object,"version")==version){
            auto digest=jsonStringField(object,"sha256");
            return isSha256(digest)?lower(digest):std::string{};
        }
        pos=end+1;
    }
    return{};
}

bool PackageManager::installArchiveFromCatalog(const std::filesystem::path&package,const std::string&catalogJson,std::string*error){
    // Read-only inspection identifies the catalog record. It does not install anything.
    Package pkg;std::string e;
    if(!inspectArchive(package,pkg,&e)){if(error)*error=e;return false;}
    const auto expected=catalogSha256(catalogJson,pkg.id,pkg.version);
    if(!isSha256(expected)){
        if(error)*error="LYCAN SECURITY\n\nPackage rejected.\n\nReason:\nSHA-256 verification failed.\n\nThe package was NOT installed.";
        return false;
    }
    return installArchive(package,expected,error);
}

bool PackageManager::installArchive(const std::filesystem::path&path,const std::string&expectedSha256,std::string*error){
    // SECURITY ORDER: hash the downloaded archive first, compare with the catalog,
    // and only then create/extract the guest application directory.
    if(!isSha256(expectedSha256)){
        if(error)*error="LYCAN SECURITY\n\nPackage rejected.\n\nReason:\nSHA-256 verification failed.\n\nThe package was NOT installed.";
        return false;
    }
    const std::string calculated=packageSha256(path);
    if(!verifySha256(calculated,expectedSha256)){
        if(error)*error="LYCAN SECURITY\n\nPackage rejected.\n\nReason:\nSHA-256 verification failed.\n\nThe package was NOT installed.";
        return false;
    }

    Package p;std::string e;
    if(!inspectArchive(path,p,&e)){if(error)*error=e;return false;}
    if(!security_.publisherTrusted(p.publisher)){if(error)*error="publisher is not trusted";return false;}
    auto target=fs_.hostPath("/apps/"+p.id);std::error_code ec;
    std::filesystem::create_directories(target,ec);if(ec){if(error)*error=ec.message();return false;}
    PackageManifest manifest;std::vector<PackageEntry>entries;
    if(!PackageArchive::inspect(path,manifest,entries,e)||!PackageArchive::extract(path,target,manifest,e)){if(error)*error=e;return false;}
    std::ofstream db(db_,std::ios::app);
    if(!db||(db<<p.id<<"|"<<p.version<<"|"<<p.publisher<<"\n").fail()){if(error)*error="cannot update package database";return false;}
    return true;
}

bool PackageManager::uninstall(const std::string&id){return validId(id)&&fs_.remove("/apps/"+id);}
bool PackageManager::rollback(const std::string&id){return validId(id)&&fs_.ready();}

std::vector<Package> PackageManager::installed()const{
    std::vector<Package>v;
    for(auto&e:fs_.list("/apps"))if(e.directory){
        Package p;p.id=e.path.substr(6);p.name=p.id;p.version="1.0.0";p.publisher="LYCAN";std::string j;
        if(fs_.readText("/apps/"+p.id+"/manifest.json",j)){
            auto value=[&](const std::string&k){auto n="\""+k+"\"";auto a=j.find(n);if(a==std::string::npos)return std::string{};a=j.find(':',a+n.size());auto q=j.find('"',a);auto z=j.find('"',q+1);return q==std::string::npos||z==std::string::npos?std::string{}:j.substr(q+1,z-q-1);};
            auto x=value("name");if(!x.empty())p.name=x;x=value("version");if(!x.empty())p.version=x;x=value("publisher");if(!x.empty())p.publisher=x;x=value("entry");if(!x.empty())p.entry=x;
        }
        v.push_back(p);
    }
    return v;
}
}
