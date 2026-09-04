#include "vm/vm.h"
#include <cassert>
#include <filesystem>
#include <string>

int main(){
    auto root=std::filesystem::temp_directory_path()/"lycan-test-vm";
    std::error_code ec; std::filesystem::remove_all(root,ec);
    lycan::VirtualMachine vm(root); vm.boot();

    assert(vm.execute("ping")=="LYCAN VM ONLINE");
    assert(vm.execute("version").find("LYCAN OS 1.2.0")!=std::string::npos);
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

    assert(vm.execute("app register demo-notes 1.0.0 4 storage").find("APP REGISTERED demo-notes")!=std::string::npos);
    assert(vm.execute("storage demo-notes data .").find("(empty)")!=std::string::npos);
    assert(vm.execute("storage-write demo-notes data prefs.txt hello-settings")=="STORAGE WROTE");
    assert(vm.execute("storage-read demo-notes data prefs.txt")=="hello-settings");
    assert(vm.execute("storage-usage demo-notes").find("USED")!=std::string::npos);
    assert(vm.execute("storage-quota demo-notes").find("QUOTA               4096 KB")!=std::string::npos);
    assert(vm.execute("app state demo-notes").find("STOPPED")!=std::string::npos);

    assert(vm.execute("memory").find("TOTAL       512 MB")!=std::string::npos);
    assert(vm.execute("vm pages").find("PAGE SIZE       4096 B")!=std::string::npos);
    assert(vm.execute("vm pages").find("USED PAGES")!=std::string::npos);
    assert(vm.execute("vm page 0").find("ALLOCATED")!=std::string::npos);
    assert(vm.execute("vm page 2048").find("FREE")!=std::string::npos);
    assert(vm.execute("vm page 999999999")=="PAGE OUT OF RANGE");

    assert(vm.execute("vm ram 768")=="RAM SET 768 MB");
    assert(vm.execute("memory").find("TOTAL       768 MB")!=std::string::npos);
    assert(vm.execute("vm page 0").find("ALLOCATED")!=std::string::npos);

    assert(vm.execute("open demo-notes").find("OPENED demo-notes")!=std::string::npos);
    assert(vm.execute("app state demo-notes").find("RUNNING")!=std::string::npos);
    assert(vm.execute("app state demo-notes").find("PAGE START")!=std::string::npos);
    assert(vm.execute("suspend demo-notes")=="SUSPENDED demo-notes");
    assert(vm.execute("app state demo-notes").find("SUSPENDED")!=std::string::npos);
    assert(vm.execute("resume demo-notes")=="RESUMED demo-notes");
    assert(vm.execute("crash demo-notes broken-storage")=="APP CRASHED demo-notes");
    assert(vm.execute("app state demo-notes").find("broken-storage")!=std::string::npos);
    assert(vm.execute("close demo-notes")=="CLOSED demo-notes");

    assert(vm.execute("network off")=="NETWORK OFFLINE");
    assert(vm.execute("network status").find("VNET0    DOWN")!=std::string::npos);
    assert(vm.execute("network app demo-notes")=="NETWORK PERMISSION DENIED\ndemo-notes");

    assert(vm.execute("snapshot safe-point").find("SNAPSHOT SAVED")!=std::string::npos);
    assert(vm.execute("write /home/Test/change.txt transient").find("WROTE")!=std::string::npos);
    assert(vm.execute("network on")=="NETWORK ONLINE");
    assert(vm.execute("restore safe-point")=="SNAPSHOT RESTORED safe-point");
    assert(vm.execute("network")=="NETWORK OFFLINE");
    assert(vm.execute("cat /home/Test/change.txt").find("FILE NOT FOUND")!=std::string::npos);
    assert(vm.execute("delete-snapshot safe-point")=="SNAPSHOT DELETED safe-point");
    assert(vm.execute("snapshots").find("(none)")!=std::string::npos);

    assert(vm.execute("app unregister demo-notes")=="APP UNREGISTERED demo-notes");
    lycan::VirtualMachine vm2(root); vm2.boot();
    assert(vm2.execute("version").find("LYCAN OS 1.2.0")!=std::string::npos);
    assert(vm2.execute("memory").find("TOTAL       768 MB")!=std::string::npos);
    assert(vm2.execute("network")=="NETWORK OFFLINE");
    assert(vm2.execute("vm page 0").find("ALLOCATED")!=std::string::npos);

    std::filesystem::remove_all(root,ec); return 0;
}
