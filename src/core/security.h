#pragma once
#include <set>
#include <string>
namespace lycan {
enum class Capability { ReadGuestFs, WriteGuestFs, Network, LaunchProcess, HostBridge };
class SecurityPolicy {
public:
    void trustPublisher(std::string publisher);
    bool publisherTrusted(const std::string& publisher) const;
    bool allowed(Capability cap) const;
    std::string describe() const;
private:
    std::set<std::string> trusted_;
};
}
