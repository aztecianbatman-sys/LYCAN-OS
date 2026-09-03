#pragma once
#include "ares_vm.h"
#include <cstdint>
#include <string>
#include <vector>
namespace lycan {
struct VirtualDevice { std::string id; std::string type; uint64_t capacity{}; bool online{true}; };
struct AresTimer { uint32_t id{}; uint64_t intervalTicks{}; uint64_t nextFire{}; bool periodic{true}; };
struct AresInterrupt { uint32_t vector{}; std::string source; uint64_t sequence{}; };
struct AresHardwareInfo { uint32_t cpuCount{2}; uint64_t ramBytes{512ull*1024ull*1024ull}; uint32_t displayWidth{1280}; uint32_t displayHeight{720}; uint64_t netRxBytes{}; uint64_t netTxBytes{}; };
class AresHardware {
public:
 explicit AresHardware(AresVm&);
 bool configure(uint32_t cpus,uint64_t ramBytes,uint32_t width,uint32_t height);
 bool attachDevice(std::string id,std::string type,uint64_t capacity=0); bool detachDevice(const std::string& id);
 uint32_t createTimer(uint64_t intervalTicks,bool periodic=true); bool cancelTimer(uint32_t id); void tick(uint64_t cycles=1);
 void raiseInterrupt(uint32_t vector,std::string source); std::vector<AresInterrupt> drainInterrupts();
 void networkRx(uint64_t bytes); void networkTx(uint64_t bytes);
 const AresHardwareInfo& info() const noexcept; const std::vector<VirtualDevice>& devices() const noexcept; std::vector<AresTimer> timers() const; std::string diagnostics() const;
private:
 AresVm& vm_; AresHardwareInfo info_{}; std::vector<VirtualDevice> devices_; std::vector<AresTimer> timers_; std::vector<AresInterrupt> interrupts_; uint32_t nextTimer_{1}; uint64_t interruptSeq_{1}; uint64_t ticks_{};
};
} // namespace lycan
