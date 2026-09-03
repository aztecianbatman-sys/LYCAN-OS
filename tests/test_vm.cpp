#include "core/ares_vm.h"
#include "core/lyfs.h"
#include "core/process.h"
#include "core/security.h"
#include "core/snapshot.h"
#include <cassert>
#include <iostream>
int main(){lycan::Memory m(8192);assert(!m.write32(0,1));assert(m.write32(4096,42));uint32_t v=0;assert(m.read32(4096,v)&&v==42);lycan::AresCpu c(m);c.load({{lycan::Op::MOVI,0,0,5},{lycan::Op::MOVI,1,0,7},{lycan::Op::ADD,0,1,0},{lycan::Op::HALT,0,0,0}});c.run();assert(c.regs()[0]==12&&c.halted());lycan::ProcessManager pm;auto p=pm.spawn("test");assert(p>=100&&!pm.list().empty());assert(pm.stop(p));std::cout<<"LYCAN 1.0 tests passed\n";return 0;}
