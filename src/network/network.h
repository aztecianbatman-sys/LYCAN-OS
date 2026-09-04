#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace lycan {
struct NetworkInterface {
    std::string name;
    std::string state;
    std::string address;
    std::string gateway;
    std::string dns;
};
class NetworkStack {
public:
    explicit NetworkStack(std::filesystem::path root);
    void boot();
    std::string execute(const std::string& command);
    bool online() const noexcept;
private:
    void persist() const;
    void load();
    std::string list() const;
    std::string routes() const;
    std::string dns() const;
    std::string configure(const std::string& spec);
    std::string setInterfaceState(const std::string& name, bool up);
    bool validName(const std::string& name) const;
    bool validIpv4(const std::string& ip) const;
    NetworkInterface* find(const std::string& name);
    const NetworkInterface* find(const std::string& name) const;
    std::filesystem::path root_;
    std::vector<NetworkInterface> interfaces_;
    bool internet_{true};
};
}