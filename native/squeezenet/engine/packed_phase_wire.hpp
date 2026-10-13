#pragma once
#include <array>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
namespace aqcf_wire288 {
using U=uint64_t;constexpr size_t N=8192,PREFIX=113,SUFFIX=81,RAW=PREFIX+2*N*8+SUFFIX;
constexpr unsigned WIDTH[2]={53,41};constexpr U PRIMES[2]={9007199254429697ULL,2199023190017ULL};
constexpr size_t PACKED=8+PREFIX+N*(53+41)/8+SUFFIX;
inline void need(bool x,const char*m){if(!x)throw std::runtime_error(m);}
inline bool enabled(){const char*p=std::getenv("AQCF_SQ_WIRE288");need(p&&(std::string(p)=="0"||std::string(p)=="1"),"288 explicit fixed arm");return p[0]=='1';}
inline U word(const char*p){U v;std::memcpy(&v,p,8);return v;}
inline void putword(char*p,U v){std::memcpy(p,&v,8);}
inline void header(const std::string&s,size_t at,U len){need(at+16<=s.size(),"288 short header");const auto*p=reinterpret_cast<const unsigned char*>(s.data()+at);need(p[0]==0x5e&&p[1]==0xa1&&p[2]==16&&p[3]==4&&p[4]==1&&p[5]==0,"288 original SEAL4.1 none header");need(word(s.data()+at+8)==len,"288 original serialized length");}
inline void validate(const std::string&s){need(s.size()==RAW,"288 fixed raw family");header(s,0,RAW);need(s[48]==0&&word(s.data()+49)==2&&word(s.data()+57)==N&&word(s.data()+65)==2,"288 seeded input shape");header(s,89,24+2*N*8);need(word(s.data()+105)==2*N,"288 all input c0 coefficients");header(s,PREFIX+2*N*8,SUFFIX);}
template<unsigned W>inline void encode(const char* src,char* dst,U q){
 for(size_t g=0;g<N/8;++g){std::array<U,(8*W+63)/64> b{};
  for(unsigned j=0;j<8;++j){U v=word(src+8*(8*g+j));need(v<q,"288 canonical source");unsigned p=W*j,k=p/64,s=p%64;b[k]|=v<<s;if(s>64-W)b[k+1]|=v>>(64-s);}
  std::memcpy(dst+W*g,b.data(),W);
 }}
template<unsigned W>inline void decode(const char* src,char* dst,U q){
 for(size_t g=0;g<N/8;++g){std::array<U,(8*W+63)/64> b{};std::memcpy(b.data(),src+W*g,W);
  for(unsigned j=0;j<8;++j){unsigned p=W*j,k=p/64,s=p%64;U v=b[k]>>s;if(s>64-W)v|=b[k+1]<<(64-s);v&=(U(1)<<W)-1;need(v<q,"288 noncanonical coefficient");putword(dst+8*(8*g+j),v);}
 }}
inline std::string pack(const std::string&s){validate(s);std::string o(PACKED,'\0');std::memcpy(o.data(),"AQSQ5341",8);std::memcpy(o.data()+8,s.data(),PREFIX);encode<53>(s.data()+PREFIX,o.data()+8+PREFIX,PRIMES[0]);encode<41>(s.data()+PREFIX+N*8,o.data()+8+PREFIX+N*53/8,PRIMES[1]);std::memcpy(o.data()+PACKED-SUFFIX,s.data()+RAW-SUFFIX,SUFFIX);return o;}
inline std::string unpack(const std::string&s){need(s.size()==PACKED&&s.compare(0,8,"AQSQ5341")==0,"288 fixed packed family");std::string o(RAW,'\0');std::memcpy(o.data(),s.data()+8,PREFIX);decode<53>(s.data()+8+PREFIX,o.data()+PREFIX,PRIMES[0]);decode<41>(s.data()+8+PREFIX+N*53/8,o.data()+PREFIX+N*8,PRIMES[1]);std::memcpy(o.data()+RAW-SUFFIX,s.data()+PACKED-SUFFIX,SUFFIX);validate(o);return o;}
}
