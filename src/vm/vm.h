#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace lycan {

struct Process { uint32_t pid{}; std::string name; std::string state; };

class VirtualMachine {
public:
    explicit VirtualMachine(std::filesystem::path root);
    void boot();
    std::string execute(const std::string& command);
    const std::filesystem::path& root() const noexcept { return root_; }
private:
    std::string diagnostics() const;
    std::string ls(const std::string& path) const;
    std::string tree(const std::string& path) const;
    std::string apps() const;
    std::string ps() const;
    std::string snapshots() const;
    std::filesystem::path guestPath(const std::string& path) const;
    bool validGuestPath(const std::filesystem::path& path) const;
    bool isGuestRoot(const std::filesystem::path& path) const;
    std::filesystem::path root_;
    uint64_t cycles_{0};
    uint64_t ramBytes_{536870912ULL};
    bool network_{true};
    uint32_t nextPid_{3};
    std::vector<Process> processes_;
    std::map<std::string, std::string> installed_;
};

}
