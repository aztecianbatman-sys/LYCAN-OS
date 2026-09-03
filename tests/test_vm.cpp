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
    lycan::Memory m(8192);
    assert(!m.write32(0,1));
    assert(m.write32(4096,42));
    uint32_t v=0;
    assert(m.read32(4096,v)&&v==42);

    lycan::AresCpu c(m);
    c.load({{lycan::Op::MOVI,0,0,5},{lycan::Op::MOVI,1,0,7},{lycan::Op::ADD,0,1,0},{lycan::Op::HALT,0,0,0}});
    c.run();
    assert(c.regs()[0]==12&&c.halted());

    lycan::ProcessManager pm;
    auto p=pm.spawn("test");
    assert(p>=100&&!pm.list().empty());
    assert(pm.stop(p));

    const std::vector<unsigned char> abc{'a','b','c'};
    assert(lycan::PackageArchive::sha256(abc)=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    assert(lycan::PackageManager::verifySha256("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD"));
    assert(!lycan::PackageManager::verifySha256("","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));

    const auto root=std::filesystem::temp_directory_path()/"lycan-package-transaction-test";
    std::error_code ec;
    std::filesystem::remove_all(root,ec);
    std::filesystem::create_directories(root,ec);
    const auto package=root/"test.lypkg";
    lycan::PackageManifest manifest{"sha-test","SHA Test","1.0.0","LYCAN","/apps/sha-test/app.bin",{}};
    std::string createError;
    assert(lycan::PackageArchive::create(package,manifest,{{"app.bin",{'O','K'}}},createError));
    const auto expected=lycan::PackageManager::packageSha256(package);

    const std::string catalog="{\"name\":\"LYCAN OS Store\",\"apps\":[{\"id\":\"sha-test\",\"version\":\"1.0.0\",\"sha256\":\""+expected+"\"}]}";
    assert(lycan::PackageManager::catalogSha256(catalog,"sha-test","1.0.0")==expected);
    assert(lycan::PackageManager::catalogSha256(catalog,"sha-test","9.9.9").empty());

    lycan::Lyfs fs(root/"guest");
    assert(fs.format());
    lycan::SecurityPolicy security;
    security.trustPublisher("LYCAN");
    lycan::PackageManager manager(fs,security);

    // First installation is fully staged before /apps is touched.
    std::string error;
    assert(manager.installArchiveFromCatalog(package,catalog,&error));
    assert(std::filesystem::exists(fs.hostPath("/apps/sha-test/app.bin")));
    assert(std::filesystem::exists(fs.hostPath("/apps/sha-test/manifest.json")));

    // Upgrade: the old version is moved aside, the new staged version is committed,
    // and only then is the package database updated.
    const auto upgrade=root/"upgrade.lypkg";
    lycan::PackageManifest upgradeManifest{"sha-test","SHA Test","2.0.0","LYCAN","/apps/sha-test/app.bin",{}};
    assert(lycan::PackageArchive::create(upgrade,upgradeManifest,{{"app.bin",{'N','E','W'}}},createError));
    const auto upgradeExpected=lycan::PackageManager::packageSha256(upgrade);
    const std::string upgradeCatalog="{\"apps\":[{\"id\":\"sha-test\",\"version\":\"2.0.0\",\"sha256\":\""+upgradeExpected+"\"}]}";
    assert(manager.installArchiveFromCatalog(upgrade,upgradeCatalog,&error));
    std::ifstream installed(fs.hostPath("/apps/sha-test/app.bin"),std::ios::binary);
    std::string installedData((std::istreambuf_iterator<char>(installed)),{});
    assert(installedData=="NEW");

    // A package rejected before the transaction begins cannot modify the installed
    // application, proving failed verification does not leave partial files.
    const auto before=fs.usedBytes();
    const auto tampered=root/"tampered.lypkg";
    assert(lycan::PackageArchive::create(tampered,manifest,{{"app.bin",{'B','A','D'}}},createError));
    const auto recorded=lycan::PackageManager::packageSha256(tampered);
    std::fstream tamper(tampered,std::ios::in|std::ios::out|std::ios::binary);
    tamper.seekp(0,std::ios::end);auto size=tamper.tellp();tamper.seekp(size-1);char byte=0;tamper.write(&byte,1);tamper.close();
    const std::string tamperedCatalog="{\"apps\":[{\"id\":\"sha-test\",\"version\":\"1.0.0\",\"sha256\":\""+recorded+"\"}]}";
    assert(!manager.installArchiveFromCatalog(tampered,tamperedCatalog,&error));
    assert(error.find("SHA-256 verification failed")!=std::string::npos);
    assert(error.find("The package was NOT installed.")!=std::string::npos);
    assert(fs.usedBytes()==before);
    std::ifstream stillInstalled(fs.hostPath("/apps/sha-test/app.bin"),std::ios::binary);
    std::string stillData((std::istreambuf_iterator<char>(stillInstalled)),{});
    assert(stillData=="NEW");

    // Missing catalog digests are hard failures and never enter staging.
    const auto missing=root/"missing.lypkg";
    assert(lycan::PackageArchive::create(missing,manifest,{{"app.bin",{'O','K'}}},createError));
    assert(!manager.installArchiveFromCatalog(missing,"{\"apps\":[{\"id\":\"sha-test\",\"version\":\"1.0.0\",\"sha256\":\"\"}]}",&error));
    assert(error.find("SHA-256 verification failed")!=std::string::npos);
    assert(fs.usedBytes()==before);

    std::filesystem::remove_all(root,ec);
    std::cout<<"LYCAN package tests passed (SHA-256 + transactional installation)\n";
    return 0;
}
