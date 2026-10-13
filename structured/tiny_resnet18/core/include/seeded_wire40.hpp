#pragma once
// Fixed-width lossless serialization, not a cryptographic primitive.
#include "seeded_wire.hpp"
namespace aqcf_wire40 {
using namespace aqcf_seeded_wire;
constexpr std::size_t SIZE=8+PREFIX+COUNT*40/8+SUFFIX;
inline std::string pack(const std::string& raw,const std::array<U,2>& qs){
 validate(raw);std::string out(SIZE,'\0');std::memcpy(out.data(),"AQSW0040",8);std::memcpy(out.data()+8,raw.data(),PREFIX);
 for(std::size_t g=0;g<COUNT/8;++g){std::array<U,5>b{};
  for(unsigned j=0;j<8;++j){U v=word(raw.data()+PREFIX+8*(8*g+j));require(v<qs[(8*g+j)/DEGREE]&&v<(U(1)<<40),"canonical40 coefficient");unsigned pos=40*j,k=pos/64,s=pos%64;b[k]|=v<<s;if(s>24)b[k+1]|=v>>(64-s);}
  std::memcpy(out.data()+8+PREFIX+40*g,b.data(),40);
 }
 std::memcpy(out.data()+SIZE-SUFFIX,raw.data()+RAW_SIZE-SUFFIX,SUFFIX);return out;
}
inline std::string unpack(const std::string& packed,const std::array<U,2>& qs){
 require(packed.size()==SIZE&&packed.compare(0,8,"AQSW0040")==0,"fixed40 frame");std::string raw(RAW_SIZE,'\0');std::memcpy(raw.data(),packed.data()+8,PREFIX);
 for(std::size_t g=0;g<COUNT/8;++g){std::array<U,5>b{};std::memcpy(b.data(),packed.data()+8+PREFIX+40*g,40);
  for(unsigned j=0;j<8;++j){unsigned pos=40*j,k=pos/64,s=pos%64;U v=b[k]>>s;if(s>24)v|=b[k+1]<<(64-s);v&=(U(1)<<40)-1;require(v<qs[(8*g+j)/DEGREE],"noncanonical40 frame");put(raw.data()+PREFIX+8*(8*g+j),v);}
 }
 std::memcpy(raw.data()+RAW_SIZE-SUFFIX,packed.data()+SIZE-SUFFIX,SUFFIX);validate(raw);return raw;
}
}
