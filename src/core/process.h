#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace lycan {
struct Process { uint32_t pid; std::string name; std::string state; uint32_t memoryKiB; uint64_t cpuTicks; };
class ProcessManager {
public:
    uint32_t spawn(std::string name, uint32_t memoryKiB = 4096);
    bool stop(uint32_t pid);
    std::vector<Process> list() const;
    uint32_t activePid() const noexcept;
private:
    uint32_t nextPid_{100};
    uint32_t active_{0};
    std::vector<Process> processes_;
};
} // namespace lycan
