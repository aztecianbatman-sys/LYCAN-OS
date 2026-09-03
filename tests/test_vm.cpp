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

    // SHA-256 known-answer test.
    const std::vector<unsigned char> abc{'a','b','c'};
    assert(lycan::PackageArchive::sha256(abc)=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    assert(lycan::PackageManager::verifySha256(
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD"));
    assert(!lycan::PackageManager::verifySha256("", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));

    // End-to-end rejection test: a modified downloaded package must fail before extraction.
    const auto root=std::filesystem::temp_directory_path()/"lycan-sha-test";
    std::error_code ec;
    std::filesystem::remove_all(root,ec);
    std::filesystem::create_directories(root,ec);
    const auto package=root/"test.lypkg";
    lycan::PackageManifest manifest{"sha-test","SHA Test","1.0.0","LYCAN","/apps/sha-test/app.bin",{}};
    assert(lycan::PackageArchive::create(package,manifest,{{"app.bin",{'O','K'}}},*new std::string));
    const auto expected=lycan::PackageManager::packageSha256(package);

    std::fstream tamper(package,std::ios::in|std::ios::out|std::ios::binary);
    tamper.seekp(0,std::ios::end);
    auto size=tamper.tellp();
    tamper.seekp(size-1);
    char byte=0;
    tamper.write(&byte,1);
    tamper.close();

    lycan::Lyfs fs(root/"guest");
    assert(fs.format());
    lycan::SecurityPolicy security;
    security.trustPublisher("LYCAN");
    lycan::PackageManager manager(fs,security);
    std::string error;
    assert(!manager.installArchive(package,expected,&error));
    assert(error.find("SHA-256 verification failed")!=std::string::npos);
    assert(error.find("The package was NOT installed.")!=std::string::npos);
    assert(!std::filesystem::exists(fs.hostPath("/apps/sha-test/manifest.json")));
    std::filesystem::remove_all(root,ec);

    std::cout<<"LYCAN 1.0 tests passed (including SHA-256 package verification)\n";
    return 0;
}
