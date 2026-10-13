#pragma once
#include "squeezenet/engine/program.hpp"
namespace aqcf_backbone {
struct OwnAdmission { std::string layout_sha,asset_sha;bool real; };
inline OwnAdmission admit(int argc,char**argv,int party,unsigned workers){
 using namespace aqcf3;require(argc==9,"complete role args");
 require((party==1||party==2)&&workers==4,"B four-worker owner role only");
 require(std::string(argv[4])=="/program/CLIENT_LAYOUT.bin"&&std::string(argv[5])=="/assets/OWN.bin"&&std::string(argv[6])=="/work/RESULTS","isolated fixed role paths");
 std::string mode=argv[7];require(mode=="PUBLIC"||mode=="REAL","fixed mode");
 auto raw=bound_bytes("/assets/OWN.txt",argv[8],4096);std::istringstream in(std::string(raw.begin(),raw.end()));
 std::string l,a,s,domain,seq,returns,extra;
 require(bool(std::getline(in,l))&&bool(std::getline(in,a))&&bool(std::getline(in,s))&&bool(std::getline(in,domain))&&bool(std::getline(in,seq))&&bool(std::getline(in,returns))&&!std::getline(in,extra),"owner-only fixed100 reservation proof");
 auto hex=[](const std::string&v){return v.size()==64&&v.find_first_not_of("0123456789abcdef")==std::string::npos;};
 require(hex(l)&&hex(a)&&hex(s),"own hashes and fresh session fields");
 require(domain==(mode=="REAL"?"SQNET_WAN_WAVE_REAL_20260919":"SQNET_WAN_WAVE_PUBLIC_20260919"),"model-specific fresh wave domain");
 bool valid_sequence=false;for(unsigned i=0;i<1;++i)valid_sequence|=(seq==std::to_string(i));
 require(mode=="REAL"?valid_sequence:seq=="PUBLIC","fixed100 slot or public qualification only");
 require(returns=="4288","complete per-image ciphertext return reservation");
 return{l,a,mode=="REAL"};
}
}
