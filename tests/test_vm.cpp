#include "vm/vm.h"
#include <cassert>
#include <filesystem>
#include <string>

int main(){
    auto root=std::filesystem::temp_directory_path()/"lycan-test-vm";
    std::error_code ec; std::filesystem::remove_all(root,ec);
    lycan::VirtualMachine vm(root); vm.boot();

    assert(vm.execute("ping")=="LYCAN VM ONLINE");
    assert(vm.execute("version").find("LYCAN OS 1.0.0")!=std::string::npos);
    assert(vm.execute("diagnostics").find("LYCAN DIAGNOSTIC CORE")!=std::string::npos);
    assert(vm.execute("diagnostics").find("HOST ACCESS         DENIED")!=std::string::npos);

    assert(vm.execute("ls /home").find("Welcome.txt")!=std::string::npos);
    assert(vm.execute("mkdir /home/Test")=="DIRECTORY CREATED");
    assert(vm.execute("write /home/Test/hello.txt hello-world").find("WROTE")!=std::string::npos);
    assert(vm.execute("cat /home/Test/hello.txt")=="hello-world");
    assert(vm.execute("tree /home/Test").find("hello.txt")!=std::string::npos);

    assert(vm.execute("ls /../../").find("ACCESS DENIED")!=std::string::npos);
    assert(vm.execute("cat /../../Windows/System32").find("ACCESS DENIED")!=std::string::npos);
    assert(vm.execute("write /../../host.txt forbidden").find("ACCESS DENIED")!=std::string::npos);
    assert(vm.execute("mkdir /..").find("ACCESS DENIED")!=std::string::npos);
    assert(vm.execute("rm /").find("REFUSED: LYFS ROOT IS IMMUTABLE")!=std::string::npos);
    assert(vm.execute("rm /../LYCAN").find("ACCESS DENIED")!=std::string::npos);

    assert(vm.execute("network off")=="NETWORK OFFLINE");
    assert(vm.execute("snapshot safe-point").find("SNAPSHOT SAVED")!=std::string::npos);
    assert(vm.execute("snapshot-info safe-point").find("LYCAN-SNAPSHOT 1")!=std::string::npos);
    assert(vm.execute("network on")=="NETWORK ONLINE");
    assert(vm.execute("restore safe-point")=="SNAPSHOT RESTORED safe-point");
    assert(vm.execute("network")=="NETWORK OFFLINE");
    assert(vm.execute("delete-snapshot safe-point")=="SNAPSHOT DELETED safe-point");
    assert(vm.execute("snapshots").find("(none)")!=std::string::npos);

    assert(vm.execute("network on")=="NETWORK ONLINE");
    assert(vm.execute("web start").find("GECKO RUNTIME READY")!=std::string::npos);
    assert(vm.execute("web tab https://example.com").find("GECKO TAB OPENED")!=std::string::npos);
    assert(vm.execute("open lycan-terminal").find("OPENED")!=std::string::npos);
    assert(vm.execute("open lycan-terminal")=="APP ALREADY RUNNING");
    assert(vm.execute("ps").find("lycan-terminal")!=std::string::npos);
    assert(vm.execute("close lycan-terminal").find("CLOSED")!=std::string::npos);

    std::filesystem::remove_all(root,ec); return 0;
}
