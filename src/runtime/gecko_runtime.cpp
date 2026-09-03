#include "gecko_runtime.h"
#include <algorithm>
#include <fstream>
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif
namespace lycan {
GeckoRuntime::GeckoRuntime(std::filesystem::path root):root_(std::move(root)),profile_(root_/"profile"),downloads_(root_/"downloads"){}
bool GeckoRuntime::configure(std::filesystem::path e){if(e.empty()||!std::filesystem::exists(e))return false;executable_=std::move(e);return true;}
bool GeckoRuntime::httpsOrLocal(const std::string&u){return u.rfind("https://",0)==0||u.rfind("about:",0)==0||u.rfind("file://",0)==0;}
bool GeckoRuntime::start(std::string*e){if(running_)return true;if(executable_.empty()){if(e)*e="Gecko executable not configured";return false;}std::filesystem::create_directories(profile_);std::filesystem::create_directories(downloads_);return launchProcess(e);}
bool GeckoRuntime::launchProcess(std::string*e){
#if defined(_WIN32)
 std::wstring exe=executable_.wstring(); std::wstring prof=profile_.wstring();
 std::wstring cmd=L"\""+exe+L"\" -profile \""+prof+L"\" -new-instance"; std::vector<wchar_t>b(cmd.begin(),cmd.end()); b.push_back(0); STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{}; if(!CreateProcessW(nullptr,b.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi)){if(e)*e="CreateProcessW failed";return false;} CloseHandle(pi.hThread);CloseHandle(pi.hProcess); running_=true;return true;
#else
 if(e)*e="Gecko process launch is currently implemented for the Windows host build";return false;
#endif
}
bool GeckoRuntime::stop(){running_=false;return true;}
bool GeckoRuntime::running()const noexcept{return running_;}
uint32_t GeckoRuntime::newTab(const std::string&url){if(!httpsOrLocal(url))return 0;for(auto&t:tabs_)t.active=false;uint32_t id=nextTab_++;tabs_.push_back({id,url,true});return id;}
bool GeckoRuntime::closeTab(uint32_t id){auto it=std::remove_if(tabs_.begin(),tabs_.end(),[&](const auto&t){return t.id==id;});if(it==tabs_.end())return false;tabs_.erase(it,tabs_.end());if(!tabs_.empty())tabs_.back().active=true;return true;}
bool GeckoRuntime::navigate(uint32_t id,const std::string&url){if(!httpsOrLocal(url))return false;for(auto&t:tabs_)if(t.id==id){t.url=url;t.active=true;return true;}return false;}
bool GeckoRuntime::setPermission(const std::string&o,const std::string&p,bool g){if(o.empty()||p.empty())return false;for(auto&w:permissions_)if(w.origin==o&&w.permission==p){w.granted=g;return true;}permissions_.push_back({o,p,g});return true;}
bool GeckoRuntime::permission(const std::string&o,const std::string&p)const{for(const auto&w:permissions_)if(w.origin==o&&w.permission==p)return w.granted;return false;}
std::vector<GeckoTab> GeckoRuntime::tabs()const{return tabs_;} std::vector<WebPermission> GeckoRuntime::permissions()const{return permissions_;}
std::filesystem::path GeckoRuntime::profilePath()const noexcept{return profile_;} std::filesystem::path GeckoRuntime::downloadPath()const noexcept{return downloads_;}
} // namespace lycan
