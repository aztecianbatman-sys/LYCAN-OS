#include "settings.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
namespace lycan {
SettingsManager::SettingsManager(std::filesystem::path file):file_(std::move(file)){}
static bool booleanValue(const std::string& v,bool& out){std::string x=v;std::transform(x.begin(),x.end(),x.begin(),[](unsigned char c){return char(std::tolower(c));});if(x=="true"||x=="1"||x=="on"){out=true;return true;}if(x=="false"||x=="0"||x=="off"){out=false;return true;}return false;}
bool SettingsManager::load(){std::ifstream f(file_);if(!f)return save();std::string line;while(std::getline(f,line)){auto p=line.find('=');if(p==std::string::npos)continue;set(line.substr(0,p),line.substr(p+1));}return true;}
bool SettingsManager::save() const{std::error_code ec;std::filesystem::create_directories(file_.parent_path(),ec);if(ec)return false;std::ofstream f(file_,std::ios::trunc);if(!f)return false;f<<"cpus="<<settings_.virtualCpus<<"\nram="<<settings_.ramBytes<<"\nwidth="<<settings_.width<<"\nheight="<<settings_.height<<"\nfullscreen="<<(settings_.fullscreen?"true":"false")<<"\nnetwork="<<(settings_.networkEnabled?"true":"false")<<"\nstrict_packages="<<(settings_.strictPackages?"true":"false")<<"\n";return bool(f);}
bool SettingsManager::set(const std::string& key,const std::string& value){try{if(key=="cpus"){auto n=std::stoul(value);if(n<1||n>64)return false;settings_.virtualCpus=n;}else if(key=="ram"){auto n=std::stoull(value);if(n<64ull*1024*1024||n>64ull*1024*1024*1024)return false;settings_.ramBytes=n;}else if(key=="width"){auto n=std::stoul(value);if(n<640||n>7680)return false;settings_.width=n;}else if(key=="height"){auto n=std::stoul(value);if(n<480||n>4320)return false;settings_.height=n;}else if(key=="fullscreen"){if(!booleanValue(value,settings_.fullscreen))return false;}else if(key=="network"){if(!booleanValue(value,settings_.networkEnabled))return false;}else if(key=="strict_packages"){if(!booleanValue(value,settings_.strictPackages))return false;}else return false;return true;}catch(...){return false;}}
std::string SettingsManager::describe() const{std::ostringstream s;s<<"CPU: "<<settings_.virtualCpus<<" virtual cores\nRAM: "<<settings_.ramBytes<<" bytes\nDISPLAY: "<<settings_.width<<"x"<<settings_.height<<(settings_.fullscreen?" fullscreen":" windowed")<<"\nNETWORK: "<<(settings_.networkEnabled?"enabled":"disabled")<<"\nPACKAGE SECURITY: "<<(settings_.strictPackages?"strict":"permissive");return s.str();}
}
