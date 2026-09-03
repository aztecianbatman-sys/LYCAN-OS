#pragma once
#include "core/lyfs.h"
#include "core/process.h"
#include "core/security.h"
#include "package/package_manager.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace lycan {
enum class AppState { Created, Starting, Running, Suspended, Crashed, Stopped };
const char* appStateName(AppState) noexcept;
struct AppResourceLimits { uint32_t memoryKiB{64 * 1024}; uint64_t cpuTicksPerTick{1000}; uint32_t maxProcesses{8}; uint32_t maxIpcMessages{256}; };
struct AppEnvironment { std::map<std::string,std::string> variables; };
struct AppCrashInfo { uint64_t sequence{}; std::string reason; std::string detail; };
struct AppRuntimeInfo { std::string id; std::string version; std::string sandboxRoot; AppState state{AppState::Created}; AppResourceLimits limits{}; AppEnvironment environment{}; std::vector<std::string> permissions; std::vector<AppCrashInfo> crashes; uint32_t pid{}; };
struct IpcMessage { std::string sender; std::string receiver; std::string topic; std::string payload; };
class IpcBus { public: bool send(const IpcMessage&); bool receive(const std::string&, IpcMessage&); std::vector<IpcMessage> pending(const std::string&) const; size_t size() const noexcept; private: std::vector<IpcMessage> queue_; };
class AppRuntime {
public:
 AppRuntime(Lyfs&,ProcessManager&,SecurityPolicy&,PackageManager&);
 bool installEnvironment(const Package&,AppRuntimeInfo* out=nullptr,std::string* error=nullptr);
 bool start(const std::string&,std::string* error=nullptr); bool suspend(const std::string&); bool resume(const std::string&); bool stop(const std::string&); bool crash(const std::string&,std::string,std::string detail={});
 bool sendIpc(const std::string&,const std::string&,const std::string&,const std::string&,std::string* error=nullptr); bool receiveIpc(const std::string&,IpcMessage&);
 bool requestCapability(const std::string&,Capability,std::string* error=nullptr);
 bool writeSandboxFile(const std::string&,const std::string&,const std::string&,std::string* error=nullptr); bool readSandboxFile(const std::string&,const std::string&,std::string&,std::string* error=nullptr) const;
 const AppRuntimeInfo* find(const std::string&) const noexcept; std::vector<AppRuntimeInfo> list() const; const IpcBus& ipc() const noexcept;
private:
 Lyfs& fs_; ProcessManager& processes_; SecurityPolicy& security_; PackageManager& packages_; IpcBus ipc_; std::map<std::string,AppRuntimeInfo> apps_; uint64_t crashSequence_{1};
 static bool safeRelativePath(const std::string&); bool hasPermission(const AppRuntimeInfo&,Capability) const; std::string sandboxPath(const std::string&,const std::string&) const;
};
} // namespace lycan
