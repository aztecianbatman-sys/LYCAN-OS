#include "lycan_platform.h"
#include "lycan_shell.h"
#include "lycan_store.h"
#include <cassert>
#include <fstream>
#include <iostream>
int main(){
 lycan::Platform p(1024*1024); assert(p.boot());
 assert(p.fs().read("/config/version").value()=="0.5.0");
 auto a=p.kernel().spawn("high",10), b=p.kernel().spawn("low",5); assert(a==1&&b==2);
 assert(p.kernel().schedule()==a); assert(p.kernel().sleep(a)); assert(p.kernel().schedule()==b); assert(p.kernel().wake(a)); assert(p.kernel().kill(b));
 assert(p.security().check("system","process.spawn")); assert(p.syscalls().spawn("svc",3)==3);
 assert(p.syscalls().writeFile("system","/home/test","hello")); assert(p.syscalls().readFile("system","/home/test").value()=="hello");
 auto&m=p.memory(); assert(m.map(0,128,true,true,true)); uint8_t x=42,r=0; assert(m.write8(8,x)); assert(m.read8(8,r)&&r==42);
 auto catalog=R"([{"id":"demo","name":"Demo","version":"1.0","description":"test","author":"test","downloadUrl":"","sha256":""}])";
 lycan::LycanStore store; assert(store.loadCatalog(catalog)); assert(store.find("demo").has_value());
 lycan::ShellRegistry shell;shell.add({"demo","Demo","test","demo.exe"});assert(shell.find("demo")!=nullptr);
 lycan::Snapshot s=p.snapshot(); assert(p.fs().write("/tmp/change","x")); assert(p.restore(s)); assert(!p.fs().read("/tmp/change"));
 std::cout<<"LYCAN 0.6.0 core tests passed\n";
}
