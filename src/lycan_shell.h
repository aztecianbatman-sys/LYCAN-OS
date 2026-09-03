#pragma once
#include <string>
#include <vector>

namespace lycan {

struct ShellApp {
    std::string id;
    std::string name;
    std::string description;
    std::string command;
};

class ShellRegistry {
public:
    void add(ShellApp app);
    const std::vector<ShellApp>& apps() const { return apps_; }
    const ShellApp* find(const std::string& id) const;
private:
    std::vector<ShellApp> apps_;
};

std::string hostDiagnostics();

}
