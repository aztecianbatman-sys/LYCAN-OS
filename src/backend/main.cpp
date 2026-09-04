#include "vm/vm.h"
#include "network/network.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

static std::filesystem::path rootPath(){
#ifdef _WIN32
    if(const char* p=std::getenv("LOCALAPPDATA"))return std::filesystem::path(p)/"LYCAN";
#endif
    return std::filesystem::current_path()/"lycan-data";
}

int main(){
    const auto root=rootPath();
    lycan::VirtualMachine vm(root); vm.boot();
    lycan::NetworkStack vnet(root/"network"); vnet.boot();
    std::string line;
    while(std::getline(std::cin,line)){
        if(line=="__LYCAN_EXIT__") break;
        std::string result;
        if(line.rfind("vnet",0)==0) result=vnet.execute(line);
        else result=vm.execute(line);
        std::cout<<result<<"\n<<<LYCAN_END>>>\n"<<std::flush;
    }
    return 0;
}
