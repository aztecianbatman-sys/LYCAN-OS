#include "lycan_store.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif
namespace lycan { namespace {
std::string unescape(std::string s){std::string o;for(size_t i=0;i<s.size();++i){if(s[i]=='\\'&&i+1<s.size())o+=s[++i];else o+=s[i];}return o;}
std::string field(const std::string&o,const std::string&k){const std::string n="\""+k+"\"";auto p=o.find(n);if(p==std::string::npos)return{};p=o.find(':',p);if(p==std::string::npos)return{};p=o.find('"',p);if(p==std::string::npos)return{};auto e=p+1;while(e<o.size()){if(o[e]=='"'&&(e==p+1||o[e-1]!='\\'))break;++e;}return unescape(o.substr(p+1,e-p-1));}
class Sha256{std::array<uint32_t,8>h_{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};std::array<uint8_t,64>b_{};uint64_t bits_=0;size_t n_=0;static uint32_t rotr(uint32_t x,int n){return(x>>n)|(x<<(32-n));}void block(const uint8_t*p){static const uint32_t k[64]={0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b69c1,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x27b70a85,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};uint32_t w[64];for(int i=0;i<16;i++)w[i]=(uint32_t(p[i*4])<<24)|(uint32_t(p[i*4+1])<<16)|(uint32_t(p[i*4+2])<<8)|p[i*4+3];for(int i=16;i<64;i++){uint32_t s0=rotr(w[i-15],7)^rotr(w[i-15],18)^(w[i-15]>>3),s1=rotr(w[i-2],17)^rotr(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+s0+w[i-7]+s1;}uint32_t a=h_[0],bb=h_[1],c=h_[2],d=h_[3],e=h_[4],f=h_[5],g=h_[6],hh=h_[7];for(int i=0;i<64;i++){uint32_t S1=rotr(e,6)^rotr(e,11)^rotr(e,25),ch=(e&f)^((~e)&g),t1=hh+S1+ch+k[i]+w[i],S0=rotr(a,2)^rotr(a,13)^rotr(a,22),maj=(a&bb)^(a&c)^(bb&c),t2=S0+maj;hh=g;g=f;f=e;e=d+t1;d=c;c=bb;bb=a;a=t1+t2;}h_[0]+=a;h_[1]+=bb;h_[2]+=c;h_[3]+=d;h_[4]+=e;h_[5]+=f;h_[6]+=g;h_[7]+=hh;}public:void update(const uint8_t*p,size_t n){bits_+=uint64_t(n)*8;while(n){size_t take=n<(64-n_)?n:(64-n_);std::copy(p,p+take,b_.data()+n_);p+=take;n-=take;n_+=take;if(n_==64){block(b_.data());n_=0;}}}std::string finish(){b_[n_++]=0x80;if(n_>56){while(n_<64)b_[n_++]=0;block(b_.data());n_=0;}while(n_<56)b_[n_++]=0;for(int i=7;i>=0;--i)b_[n_++]=uint8_t(bits_>>(i*8));block(b_.data());std::ostringstream o;for(auto x:h_)o<<std::hex<<std::setw(8)<<std::setfill('0')<<x;return o.str();}};
std::string sha256File(const std::string&path){std::ifstream f(path,std::ios::binary);if(!f)return{};Sha256 s;std::array<uint8_t,65536>b{};while(f){f.read(reinterpret_cast<char*>(b.data()),b.size());auto n=f.gcount();if(n>0)s.update(b.data(),static_cast<size_t>(n));}return s.finish();}
}
bool LycanStore::loadCatalog(const std::string&json){apps_.clear();size_t pos=0;while((pos=json.find('{',pos))!=std::string::npos){size_t end=json.find('}',pos);if(end==std::string::npos)break;std::string o=json.substr(pos,end-pos+1);StoreApp a;a.id=field(o,"id");a.name=field(o,"name");a.version=field(o,"version");a.description=field(o,"description");a.author=field(o,"author");a.downloadUrl=field(o,"downloadUrl");a.sha256=field(o,"sha256");if(!a.id.empty()&&!a.name.empty())apps_.push_back(std::move(a));pos=end+1;}return!apps_.empty();}
std::optional<StoreApp> LycanStore::find(const std::string&id)const{auto it=std::find_if(apps_.begin(),apps_.end(),[&](const StoreApp&a){return a.id==id;});return it==apps_.end()?std::nullopt:std::optional<StoreApp>(*it);}
bool LycanStore::verifyPackage(const std::string&path,const std::string&expectedSha256)const{if(expectedSha256.empty())return false;auto actual=sha256File(path);if(actual.empty())return false;std::string expected=expectedSha256;std::transform(expected.begin(),expected.end(),expected.begin(),[](unsigned char c){return char(std::tolower(c));});return actual==expected;}
bool LycanStore::download(const StoreApp&app,const std::string&destination,std::string&error)const{
#ifdef _WIN32
if(app.downloadUrl.empty()){error="Package has no download URL";return false;}URL_COMPONENTSW uc{};uc.dwStructSize=sizeof(uc);std::wstring url(app.downloadUrl.begin(),app.downloadUrl.end());wchar_t host[256]{},path[2048]{};uc.lpszHostName=host;uc.dwHostNameLength=256;uc.lpszUrlPath=path;uc.dwUrlPathLength=2048;if(!WinHttpCrackUrl(url.c_str(),0,0,&uc)){error="Invalid download URL";return false;}if(uc.nScheme!=INTERNET_SCHEME_HTTPS){error="Package downloads require HTTPS";return false;}HINTERNET s=WinHttpOpen(L"LYCAN-OS/0.6",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,nullptr,nullptr,0);if(!s){error="WinHTTP initialization failed";return false;}HINTERNET c=WinHttpConnect(s,host,uc.nPort,0);HINTERNET r=c?WinHttpOpenRequest(c,L"GET",path,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr;bool ok=false;if(r&&WinHttpSendRequest(r,WINHTTP_NO_ADDITIONAL_HEADERS,0,nullptr,0,0,0)&&WinHttpReceiveResponse(r,nullptr)){DWORD status=0,len=sizeof(status);WinHttpQueryHeaders(r,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&status,&len,nullptr);if(status>=200&&status<300){std::ofstream out(destination,std::ios::binary);if(out){char buf[65536];DWORD got=0;ok=true;while(WinHttpReadData(r,buf,sizeof(buf),&got)&&got){out.write(buf,got);if(!out){ok=false;break;}}}}else error="HTTP request failed with status "+std::to_string(status);}if(!ok&&error.empty())error="Download failed";if(r)WinHttpCloseHandle(r);if(c)WinHttpCloseHandle(c);WinHttpCloseHandle(s);return ok;
#else
(void)app;(void)destination;error="Internet downloads are currently implemented for Windows.";return false;
#endif
}
bool GeckoRuntime::discover(const std::string&explicitPath){if(!explicitPath.empty()&&std::filesystem::exists(explicitPath)){runtimePath_=explicitPath;return true;}
#ifdef _WIN32
const char*candidates[]={"C:/Program Files/Mozilla Firefox/firefox.exe","C:/Program Files (x86)/Mozilla Firefox/firefox.exe"};for(const char*c:candidates)if(std::filesystem::exists(c)){runtimePath_=c;return true;}
#endif
runtimePath_.clear();return false;}
}
