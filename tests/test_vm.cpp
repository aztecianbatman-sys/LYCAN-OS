#include "core/ares_vm.h"
#include "core/lyfs.h"
#include "core/process.h"
#include "core/security.h"
#include "core/snapshot.h"
#include "package/package_archive.h"
#include "package/package_manager.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
int main(){
 lycan::Memory m(8192);assert(!m.write32(0,1));assert(m.write32(4096,42));uint32_t v=0;assert(m.read32(4096,v)&&v==42);
 lycan::AresCpu c(m);c.load({{lycan::Op::MOVI,0,0,5},{lycan::Op::MOVI,1,0,7},{lycan::Op::ADD,0,1,0},{lycan::Op::HALT,0,0,0}});c.run();assert(c.regs()[0]==12&&c.halted());
 lycan::ProcessManager pm;auto pid=pm.spawn("test");assert(pid>=100&&!pm.list().empty());assert(pm.stop(pid));
 const std::vector<unsigned char>abc{'a','b','c'};assert(lycan::PackageArchive::sha256(abc)=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");assert(lycan::PackageManager::verifySha256("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD"));
 const auto root=std::filesystem::temp_directory_path()/"lycan-package-db-test";std::error_code ec;std::filesystem::remove_all(root,ec);std::filesystem::create_directories(root,ec);const auto package=root/"test.lypkg";
 lycan::PackageManifest manifest{"sha-test","SHA Test","1.0.0","LYCAN","/apps/sha-test/app.bin",{"lyfs.read","lyfs.write"}};std::string ce;assert(lycan::PackageArchive::create(package,manifest,{{"app.bin",{'O','K'}}},ce));const auto expected=lycan::PackageManager::packageSha256(package);
 std::string catalog="{\"apps\":[{\"id\":\"sha-test\",\"version\":\"1.0.0\",\"sha256\":\""+expected+"\"}]}";
 lycan::Lyfs fs(root/"guest");assert(fs.format());lycan::SecurityPolicy security;security.trustPublisher("LYCAN");lycan::PackageManager manager(fs,security);std::string error;assert(manager.installArchiveFromCatalog(package,catalog,&error));
 auto installed=manager.installed();assert(installed.size()==1);assert(installed[0].id=="sha-test");assert(installed[0].version=="1.0.0");assert(installed[0].publisher=="LYCAN");assert(installed[0].installLocation=="/apps/sha-test");assert(!installed[0].installTime.empty());assert(installed[0].sha256==expected);assert(installed[0].previousVersion.empty());assert(installed[0].permissions.size()==2&&installed[0].permissions[0]=="lyfs.read"&&installed[0].permissions[1]=="lyfs.write");
 std::ifstream db(fs.hostPath("/system/packages.db"));std::string dbText((std::istreambuf_iterator<char>(db)),{});assert(dbText.find("LYCAN-PKGDB|1")!=std::string::npos);assert(dbText.find("sha-test")!=std::string::npos);assert(dbText.find("lyfs.read,lyfs.write")!=std::string::npos);
 const auto upgrade=root/"upgrade.lypkg";lycan::PackageManifest um{"sha-test","SHA Test","2.0.0","LYCAN","/apps/sha-test/app.bin",{"lyfs.read","lyfs.write"}};assert(lycan::PackageArchive::create(upgrade,um,{{"app.bin",{'N','E','W'}}},ce));const auto ue=lycan::PackageManager::packageSha256(upgrade);std::string uc="{\"apps\":[{\"id\":\"sha-test\",\"version\":\"2.0.0\",\"sha256\":\""+ue+"\"}]}";assert(manager.installArchiveFromCatalog(upgrade,uc,&error));installed=manager.installed();assert(installed.size()==1&&installed[0].version=="2.0.0"&&installed[0].previousVersion=="1.0.0"&&installed[0].sha256==ue);
 std::ifstream app(fs.hostPath("/apps/sha-test/app.bin"),std::ios::binary);std::string data((std::istreambuf_iterator<char>(app)),{});assert(data=="NEW");
 const auto before=fs.usedBytes();const auto tampered=root/"tampered.lypkg";assert(lycan::PackageArchive::create(tampered,manifest,{{"app.bin",{'B','A','D'}}},ce));const auto recorded=lycan::PackageManager::packageSha256(tampered);std::fstream tamper(tampered,std::ios::in|std::ios::out|std::ios::binary);tamper.seekp(0,std::ios::end);auto size=tamper.tellp();tamper.seekp(size-1);char b=0;tamper.write(&b,1);tamper.close();std::string tc="{\"apps\":[{\"id\":\"sha-test\",\"version\":\"1.0.0\",\"sha256\":\""+recorded+"\"}]}";assert(!manager.installArchiveFromCatalog(tampered,tc,&error));assert(error.find("SHA-256 verification failed")!=std::string::npos);assert(fs.usedBytes()==before);assert(manager.installed().size()==1&&manager.installed()[0].version=="2.0.0");
 assert(!manager.installArchiveFromCatalog(package,"{\"apps\":[{\"id\":\"sha-test\",\"version\":\"1.0.0\",\"sha256\":\"\"}]}",&error));assert(fs.usedBytes()==before);
 std::cout<<"LYCAN package tests passed (SHA-256 + transactional install + installed package database)\n";std::filesystem::remove_all(root,ec);return 0;
}
