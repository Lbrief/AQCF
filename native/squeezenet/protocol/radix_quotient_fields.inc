#include "squeezenet/protocol/radix_quotient.h"
#include "squeezenet/protocol/radix_quotient_math.hpp"
#include <limits>
struct Quotient197Bank {
  int sender=0, rows=0;unsigned bits=0;
  std::vector<uint8_t> pads,k0,k1,alpha,R,r,D;
  bool joint302=false;
  std::vector<uint32_t> wide0,wide1,wideD;
  ~Quotient197Bank() {
    for(auto* v:{&wide0,&wide1,&wideD})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(uint32_t));
    for(auto* v:{&pads,&k0,&k1,&alpha,&R,&r,&D})
      if(!v->empty())OPENSSL_cleanse(v->data(),v->size());
  }
};
namespace {
template<class T> struct Q197CleanVector : std::vector<T> {
  explicit Q197CleanVector(size_t n):std::vector<T>(n){}
  Q197CleanVector(const Q197CleanVector&)=delete;
  Q197CleanVector& operator=(const Q197CleanVector&)=delete;
  ~Q197CleanVector(){if(!this->empty())OPENSSL_cleanse(this->data(),this->size()*sizeof(T));}
};
struct Q197CleanBools {
  size_t count; std::unique_ptr<bool[]> p;
  explicit Q197CleanBools(size_t n):count(n),p(n?new bool[n]:nullptr){}
  ~Q197CleanBools(){if(p)OPENSSL_cleanse(p.get(),count*sizeof(bool));}
};
struct Q197Wipe {
  void* p; size_t n;
  Q197Wipe(void* value,size_t bytes):p(value),n(bytes){}
  Q197Wipe(const Q197Wipe&)=delete;
  ~Q197Wipe(){OPENSSL_cleanse(p,n);}
};
inline size_t q197_bytes(size_t rows,unsigned bits){return (rows*bits+7)/8;}
inline unsigned q197_get(const uint8_t* p,size_t row,unsigned bits) {
  unsigned v=0;
  for(unsigned k=0;k<bits;++k){size_t at=row*bits+k;v|=((p[at/8]>>(at%8))&1)<<k;}
  return v;
}
inline void q197_put(uint8_t* p,size_t row,unsigned bits,unsigned v) {
  check(v<(1u<<bits),"quotient197 packed range");
  for(unsigned k=0;k<bits;++k){size_t at=row*bits+k;p[at/8]|=((v>>k)&1)<<(at%8);}
}
inline void q197_check_padding(const uint8_t* p,size_t rows,unsigned bits) {
  const unsigned tail=(rows*bits)%8;
  if(tail)check((p[q197_bytes(rows,bits)-1]>>tail)==0,"quotient197 nonzero wire padding");
}
}
