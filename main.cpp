#include "vm/vm.h"
#include "gui/host.h"
int main(int argc,char** argv){(void)argc;(void)argv;lycan::VMConfig config;config.ramBytes=256ull*1024ull*1024ull;config.diskBytes=64ull*1024ull*1024ull;config.displayWidth=1280;config.displayHeight=720;lycan::VirtualMachine vm(config);if(!vm.initialize())return 1;lycan::Host host(vm);return host.run();}
