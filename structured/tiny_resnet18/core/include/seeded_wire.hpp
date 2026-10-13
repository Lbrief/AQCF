#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
namespace aqcf_seeded_wire {
using U=std::uint64_t;
constexpr std::size_t DEGREE=8192,COUNT=2*DEGREE,PREFIX=113,SUFFIX=81;
constexpr std::size_t RAW_SIZE=PREFIX+COUNT*8+SUFFIX,PACKED_SIZE=8+PREFIX+COUNT*47/8+SUFFIX;
constexpr std::array<U,2> PRIMES{140737488125953ULL,140737488273409ULL};
inline void require(bool ok,const char*m){if(!ok)throw std::runtime_error(m);}
inline U word(const char*p){U v;std::memcpy(&v,p,8);return v;}
inline void put(char*p,U v){std::memcpy(p,&v,8);}
inline void header(const std::string&s,std::size_t at,U expected){
 require(at+16<=s.size(),"SEAL frame truncated header");const auto*p=reinterpret_cast<const unsigned char*>(s.data()+at);
 require(p[0]==0x5e&&p[1]==0xa1&&p[2]==16&&p[3]==4&&p[4]==1&&p[5]==0,"pinned SEAL4.1 none header");
 require(word(s.data()+at+8)==expected,"fixed SEAL object size");
}
inline void validate(const std::string&s){
 require(s.size()==RAW_SIZE,"fixed seeded ciphertext family");header(s,0,RAW_SIZE);
 require(s[48]==0&&word(s.data()+49)==2&&word(s.data()+57)==DEGREE&&word(s.data()+65)==2,"seeded BFV shape and coefficient form");
 header(s,89,24+COUNT*8);require(word(s.data()+105)==COUNT,"complete seeded c0 array");header(s,PREFIX+COUNT*8,SUFFIX);
}
// Fixed8-coefficient blocks:8*47=376 bits=47 bytes. memcpy avoids unaligned
// accesses; no content-adaptive length, dropped coefficient or new modulus.
inline std::string pack(const std::string&raw){
 validate(raw);std::string out(PACKED_SIZE,'\0');std::memcpy(out.data(),"AQSW0047",8);std::memcpy(out.data()+8,raw.data(),PREFIX);
 for(std::size_t g=0;g<COUNT/8;++g){std::array<U,6> bits{};
  for(unsigned j=0;j<8;++j){U v=word(raw.data()+PREFIX+8*(8*g+j));require(v<PRIMES[(8*g+j)/DEGREE],"canonical client c0 residue");
   unsigned pos=47*j,k=pos/64,shift=pos%64;bits[k]|=v<<shift;if(shift>17)bits[k+1]|=v>>(64-shift);}
  std::memcpy(out.data()+8+PREFIX+47*g,bits.data(),47);
 }
 std::memcpy(out.data()+PACKED_SIZE-SUFFIX,raw.data()+RAW_SIZE-SUFFIX,SUFFIX);return out;
}
inline std::string unpack(const std::string&packed){
 require(packed.size()==PACKED_SIZE&&packed.compare(0,8,"AQSW0047")==0,"fixed packed seeded frame");
 std::string raw(RAW_SIZE,'\0');std::memcpy(raw.data(),packed.data()+8,PREFIX);
 for(std::size_t g=0;g<COUNT/8;++g){std::array<U,6> bits{};std::memcpy(bits.data(),packed.data()+8+PREFIX+47*g,47);
  for(unsigned j=0;j<8;++j){unsigned pos=47*j,k=pos/64,shift=pos%64;U v=bits[k]>>shift;if(shift>17)v|=bits[k+1]<<(64-shift);v&=(U(1)<<47)-1;
   require(v<PRIMES[(8*g+j)/DEGREE],"noncanonical packed client c0 residue");put(raw.data()+PREFIX+8*(8*g+j),v);}
 }
 std::memcpy(raw.data()+RAW_SIZE-SUFFIX,packed.data()+PACKED_SIZE-SUFFIX,SUFFIX);validate(raw);return raw;
}
}
