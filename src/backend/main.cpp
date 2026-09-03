#include "vm/vm.h"
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
int main(){lycan::VirtualMachine vm(rootPath());vm.boot();std::string line;while(std::getline(std::cin,line)){if(line=="__LYCAN_EXIT__")break;std::cout<<vm.execute(line)<<"\n<<<LYCAN_END>>>\n"<<std::flush;}return 0;}
