#include "process.h"
#include <algorithm>
namespace lycan {
uint32_t ProcessManager::spawn(std::string name,uint32_t memoryKiB){ uint32_t pid=nextPid_++; processes_.push_back({pid,std::move(name),"running",memoryKiB,0}); if(!active_) active_=pid; return pid; }
bool ProcessManager::stop(uint32_t pid){ auto it=std::find_if(processes_.begin(),processes_.end(),[&](auto&p){return p.pid==pid;}); if(it==processes_.end()) return false; processes_.erase(it); if(active_==pid) active_=processes_.empty()?0:processes_.front().pid; return true; }
std::vector<Process> ProcessManager::list() const{return processes_;}
uint32_t ProcessManager::activePid() const noexcept{return active_;}
} // namespace lycan
