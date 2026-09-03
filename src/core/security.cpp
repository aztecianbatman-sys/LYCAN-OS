#include "security.h"
namespace lycan {
void SecurityPolicy::trustPublisher(std::string p){if(!p.empty())trusted_.insert(std::move(p));}
bool SecurityPolicy::publisherTrusted(const std::string&p)const{return trusted_.contains(p);}
bool SecurityPolicy::allowed(Capability cap)const{return cap!=Capability::HostBridge;}
std::string SecurityPolicy::describe()const{return "Capability gate: guest FS=allowed, process launch=allowed, network=declared, host bridge=denied";}
}
