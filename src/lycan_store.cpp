#include "lycan_store.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

namespace lycan {
namespace {
std::string unescape(std::string s) {
    std::string out;
    for(size_t i=0;i<s.size();++i){
        if(s[i]=='\\' && i+1<s.size()) { out += s[++i]; }
        else out += s[i];
    }
    return out;
}
std::string field(const std::string& object, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    auto p=object.find(needle); if(p==std::string::npos) return {};
    p=object.find(':',p); if(p==std::string::npos) return {};
    p=object.find('"',p); if(p==std::string::npos) return {};
    auto e=p+1;
    while(e<object.size()) { if(object[e]=='"' && object[e-1]!='\\') break; ++e; }
    return unescape(object.substr(p+1,e-p-1));
}
}

bool LycanStore::loadCatalog(const std::string& json) {
    apps_.clear();
    size_t pos=0;
    while((pos=json.find('{',pos))!=std::string::npos) {
        size_t end=json.find('}',pos); if(end==std::string::npos) break;
        std::string o=json.substr(pos,end-pos+1);
        StoreApp a;
        a.id=field(o,"id"); a.name=field(o,"name"); a.version=field(o,"version");
        a.description=field(o,"description"); a.author=field(o,"author");
        a.downloadUrl=field(o,"downloadUrl"); a.sha256=field(o,"sha256");
        if(!a.id.empty() && !a.name.empty()) apps_.push_back(std::move(a));
        pos=end+1;
    }
    return !apps_.empty();
}

std::optional<StoreApp> LycanStore::find(const std::string& id) const {
    auto it=std::find_if(apps_.begin(),apps_.end(),[&](const StoreApp& a){return a.id==id;});
    return it==apps_.end()?std::nullopt:std::optional<StoreApp>(*it);
}

bool LycanStore::verifyPackage(const std::string&, const std::string& expectedSha256) const {
    // Cryptographic verification is intentionally a separate backend boundary.
    // An empty digest means the catalog did not publish a digest and verification is skipped.
    return expectedSha256.empty();
}

bool LycanStore::download(const StoreApp& app, const std::string& destination, std::string& error) const {
#ifdef _WIN32
    URL_COMPONENTSW uc{}; uc.dwStructSize=sizeof(uc);
    std::wstring url(app.downloadUrl.begin(),app.downloadUrl.end());
    wchar_t host[256]{}, path[2048]{}; uc.lpszHostName=host; uc.dwHostNameLength=256; uc.lpszUrlPath=path; uc.dwUrlPathLength=2048;
    if(!WinHttpCrackUrl(url.c_str(),0,0,&uc)){ error="Invalid download URL"; return false; }
    HINTERNET session=WinHttpOpen(L"LYCAN-OS/0.5",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,nullptr,nullptr,0);
    if(!session){error="WinHTTP initialization failed";return false;}
    HINTERNET conn=WinHttpConnect(session,host,uc.nPort,0);
    HINTERNET req=conn?WinHttpOpenRequest(conn,L"GET",path,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,(uc.nScheme==INTERNET_SCHEME_HTTPS?WINHTTP_FLAG_SECURE:0)):nullptr;
    bool ok=false;
    if(req && WinHttpSendRequest(req,WINHTTP_NO_ADDITIONAL_HEADERS,0,nullptr,0,0,0) && WinHttpReceiveResponse(req,nullptr)) {
        std::ofstream out(destination,std::ios::binary);
        if(out) { char buf[64*1024]; DWORD got=0; ok=true; while(WinHttpReadData(req,buf,sizeof(buf),&got) && got){out.write(buf,got); if(!out){ok=false;break;}} }
    }
    if(!ok) error="Download failed";
    if(req) WinHttpCloseHandle(req); if(conn) WinHttpCloseHandle(conn); WinHttpCloseHandle(session);
    return ok;
#else
    (void)app; (void)destination; error="Internet downloads are currently implemented for Windows."; return false;
#endif
}

bool GeckoRuntime::discover(const std::string& explicitPath) {
    if(!explicitPath.empty() && std::filesystem::exists(explicitPath)) { runtimePath_=explicitPath; return true; }
#ifdef _WIN32
    const char* candidates[]={"C:/Program Files/Mozilla Firefox","C:/Program Files (x86)/Mozilla Firefox"};
    for(const char* c:candidates) if(std::filesystem::exists(c)){ runtimePath_=c; return true; }
#endif
    runtimePath_.clear(); return false;
}

}
