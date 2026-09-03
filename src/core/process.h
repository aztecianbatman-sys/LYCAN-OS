#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace lycan {

enum class ProcessState { Running, Suspended };
const char* processStateName(ProcessState state) noexcept;

struct Process {
    uint32_t pid{};
    std::string name;
    std::string state{"running"};
    uint32_t memoryKiB{4096};
    uint64_t cpuTicks{};
    std::string appId;
    std::string launchTarget;
};

class ProcessManager {
public:
    uint32_t spawn(std::string name, uint32_t memoryKiB = 4096);
    uint32_t launchApp(const std::string& appId, const std::string& launchTarget, uint32_t memoryKiB = 4096);
    bool stop(uint32_t pid);
    bool suspend(uint32_t pid);
    bool resume(uint32_t pid);
    bool closeApp(const std::string& appId);
    std::vector<Process> list() const;
    const Process* find(uint32_t pid) const noexcept;
    const Process* findApp(const std::string& appId) const noexcept;
    uint32_t activePid() const noexcept;
private:
    uint32_t nextPid_{100};
    uint32_t active_{0};
    std::vector<Process> processes_;
};

} // namespace lycan
