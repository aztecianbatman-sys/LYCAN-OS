#include "package_manager.h"
#include <cctype>
#include <fstream>
namespace lycan {
PackageManager::PackageManager(Lyfs&fs,SecurityPolicy&s):fs_(fs),security_(s),db_(fs_.hostPath("/system/packages.db")){}
bool PackageManager::validId(const std::string&id){if(id.empty()||id.size()>80)return false;for(unsigned char c:id)if(!(std::isalnum(c)||c=='-'||c=='_'||c=='.'))return false;return true;}
bool PackageManager::install(const Package&p){if(!validId(p.id)||p.publisher.empty()||!security_.publisherTrusted(p.publisher)||!fs_.createDirectory("/apps/"+p.id))return false;fs_.writeText("/apps/"+p.id+"/manifest.txt","id="+p.id+"\nname="+p.name+"\nversion="+p.version+"\npublisher="+p.publisher+"\ndescription="+p.description+"\nsha256="+p.sha256+"\n");std::ofstream db(db_,std::ios::app);return(bool)db&&(db<<p.id<<"|"<<p.version<<"|"<<p.publisher<<"\n");}
bool PackageManager::uninstall(const std::string&id){return validId(id)&&fs_.remove("/apps/"+id);}
bool PackageManager::rollback(const std::string&id){return validId(id)&&fs_.ready();}
std::vector<Package> PackageManager::installed()const{std::vector<Package>v;for(auto&e:fs_.list("/apps"))if(e.directory)v.push_back({e.path.substr(6),e.path.substr(6),"1.0.0","LYCAN","Installed package",""});return v;}
}
