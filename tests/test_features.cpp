#include "core/lyfs.h"
#include "core/settings.h"
#include "runtime/app_host.h"
#include <cassert>
#include <filesystem>
#include <iostream>
int main(){
 auto root=std::filesystem::temp_directory_path()/"lycan-feature-test";std::error_code ec;std::filesystem::remove_all(root,ec);
 lycan::Lyfs fs(root/"lyfs");assert(fs.format());assert(fs.createDirectory("/home/projects"));assert(fs.writeText("/home/projects/a.txt","hello"));assert(fs.rename("/home/projects/a.txt","/home/projects/b.txt"));std::string text;assert(fs.readText("/home/projects/b.txt",text)&&text=="hello");assert(!fs.rename("/home/projects/b.txt","/home/projects/missing/../b.txt"));assert(fs.remove("/home/projects/b.txt"));
 lycan::SettingsManager settings(root/"system"/"settings.conf");assert(settings.load());assert(settings.set("cpus","4"));assert(settings.set("ram","268435456"));assert(settings.set("width","1920"));assert(settings.set("height","1080"));assert(settings.set("network","false"));assert(settings.save());lycan::SettingsManager loaded(root/"system"/"settings.conf");assert(loaded.load());assert(loaded.get().virtualCpus==4&&loaded.get().ramBytes==268435456&&loaded.get().width==1920&&loaded.get().height==1080&&!loaded.get().networkEnabled);
 lycan::AppHost host(root/"host");host.boot();assert(host.execute("settings").find("CPU: 2")!=std::string::npos);assert(host.execute("set cpus 6")=="setting saved");assert(host.execute("set network false")=="setting saved");assert(host.execute("settings").find("CPU: 6")!=std::string::npos);assert(host.execute("mkdir /home/testdir")=="directory created");assert(host.execute("write /home/testdir/a hello")=="written");assert(host.execute("rename /home/testdir/a /home/testdir/b")=="renamed");assert(host.execute("cat /home/testdir/b")=="hello");assert(host.execute("rm /home/testdir/b")=="removed");
 assert(host.execute("snapshot feature")=="snapshot saved: feature");assert(host.execute("snapshots").find("feature")!=std::string::npos);assert(host.execute("delete-snapshot feature")=="snapshot deleted");assert(host.execute("snapshots").find("feature")==std::string::npos);
 std::cout<<"LYCAN feature tests passed (LYFS rename + persistent settings + command surface + snapshots)\n";std::filesystem::remove_all(root,ec);}
