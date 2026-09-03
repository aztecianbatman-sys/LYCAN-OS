#include "snapshot.h"
#include <fstream>
namespace lycan {
SnapshotManager::SnapshotManager(std::filesystem::path dir):dir_(std::move(dir)){std::filesystem::create_directories(dir_);}
bool SnapshotManager::save(const std::string& name,const std::string& stage,uint64_t cycles){if(name.empty()||name.find("..")!=std::string::npos||name.find('/')!=std::string::npos||name.find('\\')!=std::string::npos)return false;std::ofstream f(dir_/(name+".snap"));if(!f)return false;f<<"name="<<name<<"\nstage="<<stage<<"\ncycles="<<cycles<<"\n";return true;}
bool SnapshotManager::load(const std::string& name,SnapshotInfo& out)const{if(name.empty()||name.find("..")!=std::string::npos)return false;std::ifstream f(dir_/(name+".snap"));if(!f)return false;std::string line;while(std::getline(f,line)){auto p=line.find('=');if(p==std::string::npos)continue;auto k=line.substr(0,p),v=line.substr(p+1);if(k=="name")out.name=v;else if(k=="stage")out.stage=v;else if(k=="cycles")try{out.cycles=std::stoull(v);}catch(...){return false;}}return !out.name.empty();}
bool SnapshotManager::remove(const std::string& name){if(name.empty()||name.find("..")!=std::string::npos)return false;std::error_code ec;return std::filesystem::remove(dir_/(name+".snap"),ec)&&!ec;}
std::vector<SnapshotInfo> SnapshotManager::list()const{std::vector<SnapshotInfo> v;std::error_code ec;for(auto&e:std::filesystem::directory_iterator(dir_,ec))if(!ec&&e.path().extension()==".snap"){SnapshotInfo i{};if(load(e.path().stem().string(),i))v.push_back(i);}return v;}
}
