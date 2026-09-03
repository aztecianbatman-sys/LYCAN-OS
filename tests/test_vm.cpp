#include "vm/vm.h"
#include <cassert>
#include <filesystem>
#include <string>
int main(){auto root=std::filesystem::temp_directory_path()/"lycan-test-vm";std::error_code ec;std::filesystem::remove_all(root,ec);lycan::VirtualMachine vm(root);vm.boot();assert(vm.execute("diagnostics").find("ARES CPU")!=std::string::npos);assert(vm.execute("ls /home").find("Welcome.txt")!=std::string::npos);assert(vm.execute("open lycan-terminal").find("OPENED")!=std::string::npos);assert(vm.execute("ps").find("lycan-terminal")!=std::string::npos);std::filesystem::remove_all(root,ec);return 0;}
