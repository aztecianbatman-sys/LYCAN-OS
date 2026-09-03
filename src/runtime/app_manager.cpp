#include "app_manager.h"
#include <algorithm>
namespace lycan {
namespace { AppSurface fromId(const std::string&id) noexcept { if(id=="lycan-terminal")return AppSurface::Terminal;if(id=="lycan-files")return AppSurface::Files;if(id=="lycan-web")return AppSurface::Web;if(id=="lycan-snapshots")return AppSurface::Snapshots;if(id=="lycan-diagnostics")return AppSurface::Diagnostics;if(id=="crawford")return AppSurface::Crawford;return AppSurface::Unknown; } }
const char* appSurfaceName(AppSurface s) noexcept {switch(s){case AppSurface::Terminal:return"terminal";case AppSurface::Files:return"files";case AppSurface::Web:return"web";case AppSurface::Snapshots:return"snapshots";case AppSurface::Diagnostics:return"diagnostics";case AppSurface::Crawford:return"crawford";case AppSurface::Native:return"native";default:return"unknown";}}
ApplicationManager::ApplicationManager(ProcessManager&p,PackageManager&pm):processes_(p),packages_(pm){}
AppSurface ApplicationManager::inferSurface(const Package&p)noexcept{auto s=fromId(p.id);if(s!=AppSurface::Unknown)return s;if(p.entry.rfind("builtin://",0)==0){auto n=p.entry.substr(10);if(n=="terminal")return AppSurface::Terminal;if(n=="files")return AppSurface::Files;if(n=="web")return AppSurface::Web;if(n=="snapshots")return AppSurface::Snapshots;if(n=="diagnostics")return AppSurface::Diagnostics;if(n=="crawford")return AppSurface::Crawford;}return AppSurface::Native;}
bool ApplicationManager::registerBuiltInSurface(const std::string&id,AppSurface s){if(id.empty()||s==AppSurface::Unknown)return false;for(auto&i:registry_)if(i.first==id){i.second=s;return true;}registry_.push_back({id,s});return true;}
bool ApplicationManager::registerPackageSurface(const Package&p){auto s=inferSurface(p);for(auto&i:registry_)if(i.first==p.id){i.second=s;return true;}registry_.push_back({p.id,s});return true;}
uint32_t ApplicationManager::open(const std::string&id){if(id.empty())return 0;if(auto*e=const_cast<AppSession*>(find(id))){processes_.resume(e->pid);e->windowOpen=true;return e->pid;}auto installed=packages_.installed();auto it=std::find_if(installed.begin(),installed.end(),[&](const Package&p){return p.id==id;});if(it==installed.end())return 0;if(surfaceFor(id)==AppSurface::Unknown)registerPackageSurface(*it);auto pid=processes_.launchApp(it->id,it->entry,8192);if(!pid)return 0;sessions_.push_back({pid,it->id,it->name,it->entry,surfaceFor(it->id),true});return pid;}
bool ApplicationManager::close(const std::string&id){auto*s=const_cast<AppSession*>(find(id));if(!s)return false;if(!processes_.closeApp(id))return false;sessions_.erase(std::remove_if(sessions_.begin(),sessions_.end(),[&](const auto&x){return x.appId==id;}),sessions_.end());return true;}
bool ApplicationManager::suspend(const std::string&id){auto*s=const_cast<AppSession*>(find(id));return s&&processes_.suspend(s->pid);}
bool ApplicationManager::resume(const std::string&id){auto*s=const_cast<AppSession*>(find(id));return s&&processes_.resume(s->pid);}
bool ApplicationManager::suspend(uint32_t pid){return processes_.suspend(pid);}
bool ApplicationManager::resume(uint32_t pid){return processes_.resume(pid);}
bool ApplicationManager::isOpen(const std::string&id)const{return find(id)!=nullptr;}
const AppSession*ApplicationManager::find(const std::string&id)const noexcept{for(const auto&s:sessions_)if(s.appId==id)return&s;return nullptr;}
const AppSession*ApplicationManager::find(uint32_t pid)const noexcept{for(const auto&s:sessions_)if(s.pid==pid)return&s;return nullptr;}
std::vector<AppSession>ApplicationManager::sessions()const{return sessions_;}
AppSurface ApplicationManager::surfaceFor(const std::string&id)const noexcept{for(const auto&i:registry_)if(i.first==id)return i.second;return AppSurface::Unknown;}
} // namespace lycan
