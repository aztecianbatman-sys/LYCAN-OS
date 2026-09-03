#include "runtime/app_host.h"
#include <cassert>
#include <iostream>
using namespace lycan;
int main(){
 AppHost host("lycan-runtime-test"); host.boot();
 assert(host.fs().ready());
 assert(host.runtime().find("lycan-terminal"));
 assert(host.security().granted("lycan-terminal",Capability::ReadGuestFs));
 CapabilityToken token{}; assert(host.security().issueToken("lycan-terminal",Capability::ReadGuestFs,token)); assert(host.security().validateToken(token,"lycan-terminal",Capability::ReadGuestFs)); host.security().revokeToken(token.id); assert(!host.security().validateToken(token,"lycan-terminal",Capability::ReadGuestFs));
 auto pid=host.runtime().start("lycan-terminal"); assert(pid);
 assert(host.runtime().sendIpc("lycan-terminal","lycan-terminal","test","hello")); IpcMessage m{};assert(host.runtime().receiveIpc("lycan-terminal",m));assert(m.payload=="hello");
 assert(host.runtime().writeSandboxFile("lycan-terminal","hello.txt","sandbox"));std::string text;assert(host.runtime().readSandboxFile("lycan-terminal","hello.txt",text));assert(text=="sandbox");assert(!host.runtime().writeSandboxFile("lycan-terminal","../escape.txt","x"));
 AresHardware& hw=host.hardware();assert(hw.configure(4,256ull*1024*1024,1440,900));auto timer=host.hardware().createTimer(2);assert(timer);hw.tick(2);assert(!hw.drainInterrupts().empty());assert(hw.attachDevice("audio0","audio",1024));assert(hw.detachDevice("audio0"));
 auto win=host.windows().create(pid,"lycan-terminal","Terminal");assert(win);assert(host.windows().minimize(win));assert(host.windows().restore(win));assert(host.windows().maximize(win));assert(host.windows().close(win));auto n=host.windows().notify("lycan-terminal","Test","Notification");assert(n);assert(host.windows().markRead(n));
 assert(host.recovery().verifyFilesystem().ok); host.recovery().enterSafeMode(); assert(host.recovery().safeMode());
 assert(host.gecko().newTab("https://example.com")!=0); assert(host.gecko().navigate(1,"https://example.org")); assert(!host.gecko().navigate(1,"http://insecure.example"));
 host.runtime().crash("lycan-terminal","test crash","intentional test"); assert(host.runtime().find("lycan-terminal")->state==AppState::Crashed);
 std::cout<<"LYCAN runtime tests passed\n";
}
