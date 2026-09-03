#include "runtime/app_host.h"
#ifdef _WIN32
#include "platform/win32_desktop.h"
#endif
#include <cstdlib>
#include <filesystem>
#include <iostream>
int main(){
    const char* local=std::getenv("LOCALAPPDATA");
    std::filesystem::path root=(local?local:".");
    root/="LycanOS"; root/="data";
    lycan::AppHost host(root); host.boot();
#ifdef _WIN32
    return lycan::runDesktop(host);
#else
    std::cout<<"LYCAN OS 1.0 booted\n";
    for(std::string s;;){std::cout<<"lycan$ ";if(!std::getline(std::cin,s)||s=="exit")break;std::cout<<host.execute(s)<<"\n";}
    return 0;
#endif
}
