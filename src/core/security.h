#pragma once
#include <cstdint>
#include <set>
#include <string>
#include <vector>
namespace lycan {
enum class Capability { ReadGuestFs, WriteGuestFs, Network, LaunchProcess, HostBridge };
const char* capabilityName(Capability) noexcept;
struct CapabilityToken { uint64_t id{}; std::string appId; Capability capability{Capability::ReadGuestFs}; bool active{false}; };
struct SecurityAuditEntry { uint64_t sequence{}; std::string appId; std::string action; std::string result; };
class SecurityPolicy {
public:
 void trustPublisher(std::string publisher); bool publisherTrusted(const std::string&) const;
 bool allowed(Capability) const; void setAllowed(Capability,bool);
 bool grant(const std::string& appId,Capability); bool revoke(const std::string& appId,Capability); bool granted(const std::string&,Capability) const;
 bool issueToken(const std::string&,Capability,CapabilityToken&); bool validateToken(const CapabilityToken&,const std::string&,Capability) const; void revokeToken(uint64_t);
 void audit(const std::string& appId,const std::string& action,const std::string& result); const std::vector<SecurityAuditEntry>& auditLog() const noexcept;
 std::string describe() const;
private:
 std::set<std::string> trusted_; std::set<Capability> denied_; std::set<std::string> grants_; std::vector<CapabilityToken> tokens_; std::vector<SecurityAuditEntry> audit_; uint64_t nextToken_{1}; uint64_t auditSequence_{1};
 static std::string grantKey(const std::string&,Capability);
};
} // namespace lycan
