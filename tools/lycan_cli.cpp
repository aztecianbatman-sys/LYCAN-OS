#include "lycan_platform.h"
#include <iostream>
int main(){lycan::Platform p;p.boot();std::cout<<p.status()<<"\n";std::cout<<"Commands: status, ps, fs, spawn <name>, kill <pid>, exit\n";return 0;}
