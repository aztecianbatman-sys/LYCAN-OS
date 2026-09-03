#include "ares_hardware.h"
#include <algorithm>
#include <sstream>
namespace lycan {
AresHardware::AresHardware(AresVm&v):vm_(v){devices_={{"display","display",0,true},{"input","input",0,true},{"net0","network",0,true},{"clock","timer",0,true}};}
bool AresHardware::configure(uint32_t c,uint64_t r,uint32_t w,uint32_t h){if(c==0||c>64||r<64ull*1024*1024||w<640||h<480)return false;info_.cpuCount=c;info_.ramBytes=r;info_.displayWidth=w;info_.displayHeight=h;return true;}
bool AresHardware::attachDevice(std::string id,std::string type,uint64_t cap){if(id.empty()||type.empty())return false;for(const auto&d:devices_)if(d.id==id)return false;devices_.push_back({std::move(id),std::move(type),cap,true});return true;}
bool AresHardware::detachDevice(const std::string&id){auto it=std::remove_if(devices_.begin(),devices_.end(),[&](const auto&d){return d.id==id&&d.id!="display"&&d.id!="input";});if(it==devices_.end())return false;devices_.erase(it,devices_.end());return true;}
uint32_t AresHardware::createTimer(uint64_t interval,bool periodic){if(!interval)return 0;AresTimer t{nextTimer_++,interval,ticks_+interval,periodic};timers_.push_back(t);return t.id;}
bool AresHardware::cancelTimer(uint32_t id){auto it=std::remove_if(timers_.begin(),timers_.end(),[&](const auto&t){return t.id==id;});if(it==timers_.end())return false;timers_.erase(it,timers_.end());return true;}
void AresHardware::tick(uint64_t cycles){ticks_+=cycles;for(auto it=timers_.begin();it!=timers_.end();){if(ticks_>=it->nextFire){raiseInterrupt(32+it->id,"timer:"+std::to_string(it->id));if(it->periodic){it->nextFire+=it->intervalTicks;}else{it=timers_.erase(it);continue;}}++it;}vm_.tick(size_t(std::min<uint64_t>(cycles,4096)));}
void AresHardware::raiseInterrupt(uint32_t v,std::string s){interrupts_.push_back({v,std::move(s),interruptSeq_++});if(interrupts_.size()>1024)interrupts_.erase(interrupts_.begin());}
std::vector<AresInterrupt> AresHardware::drainInterrupts(){auto r=interrupts_;interrupts_.clear();return r;}
void AresHardware::networkRx(uint64_t b){info_.netRxBytes+=b;}void AresHardware::networkTx(uint64_t b){info_.netTxBytes+=b;}
const AresHardwareInfo&AresHardware::info()const noexcept{return info_;}const std::vector<VirtualDevice>&AresHardware::devices()const noexcept{return devices_;}std::vector<AresTimer>AresHardware::timers()const{return timers_;}
std::string AresHardware::diagnostics()const{std::ostringstream s;s<<"ARES HARDWARE\nCPU: "<<info_.cpuCount<<"\nRAM: "<<info_.ramBytes<<"\nDISPLAY: "<<info_.displayWidth<<"x"<<info_.displayHeight<<"\nNET RX/TX: "<<info_.netRxBytes<<"/"<<info_.netTxBytes<<"\nDEVICES: "<<devices_.size()<<"\nTIMERS: "<<timers_.size()<<"\nINTERRUPTS: "<<interrupts_.size();return s.str();}
} // namespace lycan
