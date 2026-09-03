#include "process.h"
#include <algorithm>

namespace lycan {
const char* processStateName(ProcessState state) noexcept { return state == ProcessState::Suspended ? "suspended" : "running"; }
uint32_t ProcessManager::spawn(std::string name, uint32_t memoryKiB) {
    uint32_t pid=nextPid_++; processes_.push_back({pid,std::move(name),"running",memoryKiB,0,{},{}}); if(!active_) active_=pid; return pid;
}
uint32_t ProcessManager::launchApp(const std::string& appId,const std::string& launchTarget,uint32_t memoryKiB) {
    if(appId.empty()) return 0; if(const auto* existing=findApp(appId)){active_=existing->pid;return existing->pid;}
    uint32_t pid=nextPid_++; processes_.push_back({pid,appId,"running",memoryKiB,0,appId,launchTarget}); active_=pid; return pid;
}
bool ProcessManager::stop(uint32_t pid){auto it=std::find_if(processes_.begin(),processes_.end(),[&](const auto&p){return p.pid==pid;});if(it==processes_.end())return false;processes_.erase(it);if(active_==pid)active_=processes_.empty()?0:processes_.front().pid;return true;}
bool ProcessManager::suspend(uint32_t pid){auto it=std::find_if(processes_.begin(),processes_.end(),[&](const auto&p){return p.pid==pid;});if(it==processes_.end())return false;it->state="suspended";return true;}
bool ProcessManager::resume(uint32_t pid){auto it=std::find_if(processes_.begin(),processes_.end(),[&](const auto&p){return p.pid==pid;});if(it==processes_.end())return false;it->state="running";active_=pid;return true;}
bool ProcessManager::closeApp(const std::string& appId){const auto*p=findApp(appId);return p?stop(p->pid):false;}
std::vector<Process> ProcessManager::list()const{return processes_;}
const Process* ProcessManager::find(uint32_t pid)const noexcept{auto it=std::find_if(processes_.begin(),processes_.end(),[&](const auto&p){return p.pid==pid;});return it==processes_.end()?nullptr:&*it;}
const Process* ProcessManager::findApp(const std::string& appId)const noexcept{auto it=std::find_if(processes_.begin(),processes_.end(),[&](const auto&p){return p.appId==appId;});return it==processes_.end()?nullptr:&*it;}
uint32_t ProcessManager::activePid()const noexcept{return active_;}
} // namespace lycan
