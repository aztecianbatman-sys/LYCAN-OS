#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace lycan {

enum class Op : uint8_t { NOP, MOVI, ADD, SUB, XOR, LOAD, STORE, JMP, JZ, HALT };
struct Instruction { Op op{Op::NOP}; uint8_t a{0}, b{0}; int32_t imm{0}; };

class Memory {
public:
    explicit Memory(size_t bytes = 256 * 1024);
    bool read32(uint32_t addr, uint32_t& out) const;
    bool write32(uint32_t addr, uint32_t value);
    size_t size() const noexcept;
    bool protectedRange(uint32_t addr, size_t len) const;
private:
    std::vector<uint8_t> data_;
};

class AresCpu {
public:
    explicit AresCpu(Memory& memory);
    void reset();
    bool load(const std::vector<Instruction>& program);
    bool step();
    void run(size_t budget = 10000);
    bool halted() const noexcept;
    uint64_t cycles() const noexcept;
    const std::array<uint32_t, 8>& regs() const noexcept;
    uint32_t pc() const noexcept;
    std::string state() const;
private:
    Memory& memory_;
    std::array<uint32_t, 8> r_{};
    std::vector<Instruction> program_;
    uint32_t pc_{0};
    uint64_t cycles_{0};
    bool halted_{false};
};

class AresVm {
public:
    AresVm();
    void boot();
    void tick(size_t budget = 256);
    bool booted() const noexcept;
    const std::string& bootStage() const noexcept;
    AresCpu& cpu() noexcept;
    Memory& memory() noexcept;
    const std::vector<std::string>& log() const noexcept;
    void addLog(std::string line);
private:
    Memory memory_;
    AresCpu cpu_;
    bool booted_{false};
    std::string stage_{"Power On"};
    std::vector<std::string> log_;
};

} // namespace lycan
