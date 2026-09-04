#include "vm/vm.h"
#include <cassert>
#include <filesystem>
#include <string>

int main(){
    auto root=std::filesystem::temp_directory_path()/"lycan-test-vm";
    std::error_code ec; std::filesystem::remove_all(root,ec);
    lycan::VirtualMachine vm(root); vm.boot();
    assert(vm.execute("diagnostics").find("ARES CPU")!=std::string::npos);
    assert(vm.execute("ls /home").find("Welcome.txt")!=std::string::npos);
    assert(vm.execute("mkdir /home/Test")=="DIRECTORY CREATED");
    assert(vm.execute("write /home/Test/hello.txt hello-world").find("WROTE")!=std::string::npos);
    assert(vm.execute("cat /home/Test/hello.txt")=="hello-world");
    assert(vm.execute("ls /home/Test").find("hello.txt")!=std::string::npos);
    assert(vm.execute("ls /../../")=="ACCESS DENIED");
    assert(vm.execute("network off")=="NETWORK OFFLINE");
    assert(vm.execute("web start")=="NETWORK OFFLINE");
    assert(vm.execute("network on")=="NETWORK ONLINE");
    assert(vm.execute("web tab https://example.com").find("GECKO TAB OPENED")!=std::string::npos);
    assert(vm.execute("snapshot test").find("SNAPSHOT SAVED")!=std::string::npos);
    assert(vm.execute("snapshots").find("test")!=std::string::npos);
    assert(vm.execute("open lycan-terminal").find("OPENED")!=std::string::npos);
    assert(vm.execute("open lycan-terminal")=="APP ALREADY RUNNING");
    assert(vm.execute("ps").find("lycan-terminal")!=std::string::npos);
    assert(vm.execute("close lycan-terminal").find("CLOSED")!=std::string::npos);
    std::filesystem::remove_all(root,ec); return 0;
}
