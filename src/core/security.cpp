#include "security.h"
#include <sstream>
namespace lycan {
const char* capabilityName(Capability c) noexcept{switch(c){case Capability::ReadGuestFs:return "lyfs.read";case Capability::WriteGuestFs:return "lyfs.write";case Capability::Network:return "network";case Capability::LaunchProcess:return "process.launch";case Capability::HostBridge:return "host.bridge";}return "unknown";}
void SecurityPolicy::trustPublisher(std::string p){if(!p.empty())trusted_.insert(std::move(p));}
bool SecurityPolicy::publisherTrusted(const std::string&p)const{return trusted_.contains(p);}
bool SecurityPolicy::allowed(Capability c)const{return !denied_.contains(c);}
void SecurityPolicy::setAllowed(Capability c,bool on){if(on)denied_.erase(c);else denied_.insert(c);}
std::string SecurityPolicy::grantKey(const std::string&id,Capability c){return id+":"+capabilityName(c);}
bool SecurityPolicy::grant(const std::string&id,Capability c){if(id.empty()||!allowed(c))return false;grants_.insert(grantKey(id,c));audit(id,"grant "+std::string(capabilityName(c)),"allow");return true;}
bool SecurityPolicy::revoke(const std::string&id,Capability c){auto n=grants_.erase(grantKey(id,c));audit(id,"revoke "+std::string(capabilityName(c)),n?"allow":"missing");return n!=0;}
bool SecurityPolicy::granted(const std::string&id,Capability c)const{return allowed(c)&&grants_.contains(grantKey(id,c));}
bool SecurityPolicy::issueToken(const std::string&id,Capability c,CapabilityToken&out){if(!granted(id,c))return false;out={nextToken_++,id,c,true};audit(id,"issue "+std::string(capabilityName(c)),"allow");tokens_.push_back(out);return true;}
bool SecurityPolicy::validateToken(const CapabilityToken&t,const std::string&id,Capability c)const{return t.active&&t.id!=0&&t.appId==id&&t.capability==c&&granted(id,c);}
void SecurityPolicy::revokeToken(uint64_t id){for(auto&t:tokens_)if(t.id==id)t.active=false;}
void SecurityPolicy::audit(const std::string&id,const std::string&action,const std::string&result){audit_.push_back({auditSequence_++,id,action,result});if(audit_.size()>2048)audit_.erase(audit_.begin());}
const std::vector<SecurityAuditEntry>& SecurityPolicy::auditLog()const noexcept{return audit_;}
std::string SecurityPolicy::describe()const{std::ostringstream s;s<<"capabilities:";for(auto c:{Capability::ReadGuestFs,Capability::WriteGuestFs,Capability::Network,Capability::LaunchProcess,Capability::HostBridge})s<<" "<<capabilityName(c)<<"="<<(allowed(c)?"allowed":"denied");s<<"; grants="<<grants_.size()<<"; trusted_publishers="<<trusted_.size()<<"; audit_entries="<<audit_.size();return s.str();}
} // namespace lycan
