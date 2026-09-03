#include "lycan_platform.h"
#include <cassert>
#include <iostream>
int main(){lycan::Platform p(1024*1024);assert(p.boot());assert(p.fs().read("/config/version").value()=="0.4.1");auto id=p.kernel().spawn("test",5);assert(id==1);assert(p.kernel().kill(id));assert(p.security().check("system","boot"));auto&m=p.memory();assert(m.map(0,128,true,true,true));uint8_t b=42;assert(m.write8(8,b));uint8_t r=0;assert(m.read8(8,r)&&r==42);std::cout<<"LYCAN tests passed\n";}
