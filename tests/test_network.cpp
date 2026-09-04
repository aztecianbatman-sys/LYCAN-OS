#include "network/network.h"
#include <cassert>
#include <filesystem>
#include <string>

int main(){
    const auto root=std::filesystem::temp_directory_path()/"lycan-test-network";
    std::error_code ec; std::filesystem::remove_all(root,ec);
    lycan::NetworkStack net(root); net.boot();

    assert(net.online());
    assert(net.execute("vnet status").find("VIRTUAL NETWORK ONLINE")!=std::string::npos);
    assert(net.execute("vnet interfaces").find("VNET0")!=std::string::npos);
    assert(net.execute("vnet routes").find("0.0.0.0/0")!=std::string::npos);
    assert(net.execute("vnet dns").find("10.42.0.1")!=std::string::npos);
    assert(net.execute("vnet down VNET0")=="INTERFACE VNET0 DOWN");
    assert(!net.online());
    assert(net.execute("vnet routes").find("no active VNET route")!=std::string::npos);
    assert(net.execute("vnet up VNET0")=="INTERFACE VNET0 UP");
    assert(net.execute("vnet config VNET0 10.42.0.9 10.42.0.1 1.1.1.1")=="INTERFACE CONFIGURED VNET0");
    assert(net.execute("vnet status").find("10.42.0.9")!=std::string::npos);
    assert(net.execute("vnet down LOOP0").find("MANDATORY")!=std::string::npos);
    assert(net.execute("vnet off")=="VNET OFFLINE");
    assert(!net.online());

    lycan::NetworkStack net2(root); net2.boot();
    assert(net2.execute("vnet status").find("VNET0")!=std::string::npos);
    assert(net2.execute("vnet status").find("BLOCKED")!=std::string::npos);
    assert(net2.execute("vnet on")=="VNET ONLINE");
    assert(net2.online());

    std::filesystem::remove_all(root,ec); return 0;
}
