#pragma once
#include "program.hpp"
namespace aqcf_rhombus {
struct OwnAdmission { std::string layout_sha,asset_sha;bool real; };
inline OwnAdmission admit(int argc,char**argv,int party,unsigned workers){
 using namespace aqcf3;require(argc==9,"complete role args");
 require(std::filesystem::path(argv[4]).is_absolute() && std::filesystem::path(argv[5]).is_absolute() && std::filesystem::path(argv[6]).is_absolute(),"absolute role-owned paths");
 std::string mode=argv[7];require(mode=="PUBLIC"||mode=="REAL","fixed mode");
 auto raw=bound_bytes(std::string(argv[5])+".proof",argv[8],4096);std::istringstream in(std::string(raw.begin(),raw.end()));
 std::string l,a,s,e;require(bool(std::getline(in,l))&&bool(std::getline(in,a))&&bool(std::getline(in,s))&&!std::getline(in,e),"own proof only");
 require(l.size()==64&&a.size()==64&&s.size()==64,"own proof fields");return{l,a,mode=="REAL"};
}
}
