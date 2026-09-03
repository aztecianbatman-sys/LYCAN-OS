#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
namespace lycan {
struct Cpu { uint64_t r[8]{}; uint64_t pc=0,sp=0,flags=0; bool halted=false; };
enum class Op:uint8_t{NOP=0,MOV,ADD,SUB,MUL,DIV,CMP,JMP,JZ,JNZ,PUSH,POP,HALT};
class Memory { public: explicit Memory(size_t n); size_t size()const; bool read8(uint64_t,uint8_t&)const; bool write8(uint64_t,uint8_t); bool read64(uint64_t,uint64_t&)const; bool write64(uint64_t,uint64_t); bool map(uint64_t,uint64_t,bool,bool,bool); bool canRead(uint64_t)const; bool canWrite(uint64_t)const; bool canExecute(uint64_t)const; private: std::vector<uint8_t> b_; struct R{uint64_t a,n;bool r,w,x;}; std::vector<R> regions_;};
class CPU { public: explicit CPU(Memory&); void reset(uint64_t,uint64_t); bool step(); void run(uint64_t); const Cpu& state()const{return s_;} void setState(const Cpu& s){s_=s;} private: Memory&m_;Cpu s_{}; bool fetch8(uint8_t&);bool fetch64(uint64_t&);void z(uint64_t);};
struct Process {uint32_t pid;std::string name;enum class State{Ready,Running,Sleeping,Terminated};State state=State::Ready;uint8_t priority=8;uint64_t ticks=0;};
class Kernel { public: bool boot(); uint32_t spawn(const std::string&,uint8_t=8); bool kill(uint32_t); bool sleep(uint32_t); bool wake(uint32_t); uint32_t schedule(); const std::vector<Process>& processes()const{return p_;} std::string diagnostics()const; private: bool booted_=false;uint32_t next_=1,current_=0;std::vector<Process>p_;};
class Lyfs { public: bool format(); bool write(const std::string&,const std::string&);std::optional<std::string> read(const std::string&)const;std::vector<std::string> list(const std::string&)const; size_t fileCount()const{return f_.size();} private: std::unordered_map<std::string,std::string> f_;};
class Security { public: bool allow(const std::string&,const std::string&); bool check(const std::string&,const std::string&)const; private:std::unordered_map<std::string,std::string>a_;};
class Syscalls { public: explicit Syscalls(Kernel&,Lyfs&,Security&); uint32_t spawn(const std::string&,uint8_t); bool terminate(uint32_t); std::optional<std::string> readFile(const std::string&,const std::string&); bool writeFile(const std::string&,const std::string&,const std::string&); private:Kernel&k_;Lyfs&fs_;Security&sec_;};
struct Snapshot { Cpu cpu; std::vector<std::pair<std::string,std::string>> files; };
class Platform { public: Platform(size_t ram=256ull*1024*1024); bool boot(); std::string status()const; Memory& memory(){return mem_;} CPU& cpu(){return cpu_;} Kernel& kernel(){return k_;} Lyfs& fs(){return fs_;} Security& security(){return sec_;} Syscalls& syscalls(){return sc_;} Snapshot snapshot()const; bool restore(const Snapshot&); private:Memory mem_;CPU cpu_;Kernel k_;Lyfs fs_;Security sec_;Syscalls sc_;bool booted_=false;};
}
