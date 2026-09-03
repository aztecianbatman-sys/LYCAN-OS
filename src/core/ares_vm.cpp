#include "ares_vm.h"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace lycan {

Memory::Memory(size_t bytes) : data_(bytes, 0) {}
size_t Memory::size() const noexcept { return data_.size(); }
bool Memory::protectedRange(uint32_t addr, size_t len) const {
    const uint64_t end = uint64_t(addr) + len;
    return end > data_.size() || addr < 4096; // page zero is unmapped in the guest
}
bool Memory::read32(uint32_t addr, uint32_t& out) const {
    if (protectedRange(addr, 4)) return false;
    std::memcpy(&out, data_.data() + addr, 4);
    return true;
}
bool Memory::write32(uint32_t addr, uint32_t value) {
    if (protectedRange(addr, 4)) return false;
    std::memcpy(data_.data() + addr, &value, 4);
    return true;
}

AresCpu::AresCpu(Memory& memory) : memory_(memory) { reset(); }
void AresCpu::reset() { r_.fill(0); pc_ = 0; cycles_ = 0; halted_ = false; }
bool AresCpu::load(const std::vector<Instruction>& program) { program_ = program; reset(); return true; }
bool AresCpu::step() {
    if (halted_ || pc_ >= program_.size()) { halted_ = true; return false; }
    const auto ins = program_[pc_++];
    const auto reg = [](uint8_t x) { return static_cast<size_t>(x & 7u); };
    switch (ins.op) {
        case Op::NOP: break;
        case Op::MOVI: r_[reg(ins.a)] = static_cast<uint32_t>(ins.imm); break;
        case Op::ADD: r_[reg(ins.a)] += r_[reg(ins.b)]; break;
        case Op::SUB: r_[reg(ins.a)] -= r_[reg(ins.b)]; break;
        case Op::XOR: r_[reg(ins.a)] ^= r_[reg(ins.b)]; break;
        case Op::LOAD: { uint32_t v; if (!memory_.read32(static_cast<uint32_t>(ins.imm), v)) { halted_ = true; return false; } r_[reg(ins.a)] = v; break; }
        case Op::STORE: if (!memory_.write32(static_cast<uint32_t>(ins.imm), r_[reg(ins.a)])) { halted_ = true; return false; } break;
        case Op::JMP: pc_ = static_cast<uint32_t>(ins.imm); break;
        case Op::JZ: if (r_[reg(ins.a)] == 0) pc_ = static_cast<uint32_t>(ins.imm); break;
        case Op::HALT: halted_ = true; break;
    }
    ++cycles_;
    return !halted_;
}
void AresCpu::run(size_t budget) { while (!halted_ && budget--) step(); }
bool AresCpu::halted() const noexcept { return halted_; }
uint64_t AresCpu::cycles() const noexcept { return cycles_; }
const std::array<uint32_t,8>& AresCpu::regs() const noexcept { return r_; }
uint32_t AresCpu::pc() const noexcept { return pc_; }
std::string AresCpu::state() const {
    std::ostringstream s; s << "PC=" << pc_ << "  CYC=" << cycles_ << (halted_ ? "  HALTED" : "  RUNNING"); return s.str();
}

AresVm::AresVm() : memory_(256 * 1024), cpu_(memory_) {}
void AresVm::boot() {
    log_.clear(); booted_ = false; stage_ = "Firmware"; addLog("ARES firmware: POST");
    stage_ = "MMU"; addLog("Virtual memory: 256 MiB address space policy / 256 KiB initial RAM");
    stage_ = "Kernel"; addLog("LYCAN kernel: scheduler, IPC, capability gate online");
    stage_ = "Services"; addLog("Core services: init, storage, package, desktop");
    stage_ = "Desktop";
    cpu_.load({{Op::MOVI,0,0,42},{Op::MOVI,1,0,8},{Op::ADD,0,1,0},{Op::STORE,0,0,8192},{Op::HALT,0,0,0}});
    booted_ = true; addLog("Boot complete: LYCAN desktop ready");
}
void AresVm::tick(size_t budget) { if (booted_ && !cpu_.halted()) cpu_.run(budget); }
bool AresVm::booted() const noexcept { return booted_; }
const std::string& AresVm::bootStage() const noexcept { return stage_; }
AresCpu& AresVm::cpu() noexcept { return cpu_; }
Memory& AresVm::memory() noexcept { return memory_; }
const std::vector<std::string>& AresVm::log() const noexcept { return log_; }
void AresVm::addLog(std::string line) { log_.push_back(std::move(line)); if (log_.size() > 80) log_.erase(log_.begin()); }

} // namespace lycan
