#include "runtime/app_host.h"
#include <iostream>
#include <string>
int main(){
 const char* cmds[]={"help","ls","cat /home/Welcome.txt","ps","vm"};
 lycan::AppHost h("./lycan-data");h.boot();
 for(auto c:cmds)std::cout<<"lycan$ "<<c<<"\n"<<h.execute(c)<<"\n";
 return 0;
}
