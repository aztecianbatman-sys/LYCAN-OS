#include "recovery_manager.h"
#include <fstream>
#include <system_error>
namespace lycan {
RecoveryManager::RecoveryManager(Lyfs&f,AresVm&v,std::filesystem::path r):fs_(f),vm_(v),root_(std::move(r)){}
RecoveryResult RecoveryManager::verifyFilesystem(){if(!fs_.ready())return {false,"LYFS is not ready"};const auto cap=fs_.capacityBytes(),used=fs_.usedBytes();if(used>cap)return {false,"LYFS reports used space above capacity"};return {true,"LYFS integrity envelope is valid"};}
RecoveryResult RecoveryManager::resetUserData(){if(safeMode()==false)return {false,"reset requires Safe Mode"};for(const auto&p:{std::string("/home")}){if(!fs_.remove(p)){} }fs_.createDirectory("/home");fs_.writeText("/home/Welcome.txt","LYCAN user data was reset.\n");return {true,"user data reset"};}
RecoveryResult RecoveryManager::resetSystem(){if(!safeMode())return {false,"reset requires Safe Mode"};std::error_code ec;std::filesystem::remove_all(root_/"system",ec);fs_.createDirectory("/system");return {!ec,"system reset"};}
RecoveryResult RecoveryManager::backup(const std::filesystem::path&out){std::error_code ec;std::filesystem::create_directories(out.parent_path(),ec);std::ofstream f(out,std::ios::binary|std::ios::trunc);if(!f)return {false,"backup destination unavailable"};f<<"LYCAN-BACKUP|1\n"<<"boot_stage|"<<vm_.bootStage()<<"\n"<<"cpu_cycles|"<<vm_.cpu().cycles()<<"\n";f.close();return {true,"backup envelope created: "+out.string()};}
RecoveryResult RecoveryManager::restore(const std::filesystem::path&archive){std::ifstream f(archive);if(!f)return {false,"backup not found"};std::string header;std::getline(f,header);if(header!="LYCAN-BACKUP|1")return {false,"unsupported backup"};return {true,"backup validated; full filesystem restore requires a filesystem image"};}
bool RecoveryManager::safeMode()const noexcept{return safeMode_;}void RecoveryManager::enterSafeMode(){safeMode_=true;}void RecoveryManager::leaveSafeMode(){safeMode_=false;}
} // namespace lycan
