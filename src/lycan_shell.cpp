#include "lycan_shell.h"
#include <algorithm>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

namespace lycan {
void ShellRegistry::add(ShellApp app) {
    auto it=std::find_if(apps_.begin(),apps_.end(),[&](const ShellApp& x){return x.id==app.id;});
    if(it==apps_.end()) apps_.push_back(std::move(app)); else *it=std::move(app);
}
const ShellApp* ShellRegistry::find(const std::string& id) const {
    auto it=std::find_if(apps_.begin(),apps_.end(),[&](const ShellApp& x){return x.id==id;});
    return it==apps_.end()?nullptr:&*it;
}
std::string hostDiagnostics() {
    std::ostringstream s;
    s << "LYCAN host diagnostics\n";
#ifdef _WIN32
    SYSTEM_INFO si{}; GetSystemInfo(&si);
    MEMORYSTATUSEX ms{}; ms.dwLength=sizeof(ms); GlobalMemoryStatusEx(&ms);
    s << "Host: Windows native\n";
    s << "CPU logical processors: " << si.dwNumberOfProcessors << "\n";
    s << "Physical memory: " << (ms.ullTotalPhys/(1024ull*1024ull)) << " MB\n";
    s << "Memory available: " << (ms.ullAvailPhys/(1024ull*1024ull)) << " MB\n";
#else
    s << "Host: non-Windows development environment\n";
#endif
    s << "Guest RAM: 256 MB default\n";
    s << "Isolation boundary: active\n";
    return s.str();
}
}
