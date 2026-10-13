#include "squeezenet/protocol/quotient_recovery.h"
#include "squeezenet/protocol/quotient_recovery_math.hpp"
#include <limits>
struct Quotient169Bank {
  int sender=0, rows=0;
  std::vector<uint8_t> pads,k0,k1,alpha,R,r,D;
  ~Quotient169Bank() {
    for(auto* v:{&pads,&k0,&k1,&alpha,&R,&r,&D})
      if(!v->empty())OPENSSL_cleanse(v->data(),v->size());
  }
};
namespace {
template<class T> struct Q169CleanVector : std::vector<T> {
  explicit Q169CleanVector(size_t n):std::vector<T>(n){}
  Q169CleanVector(const Q169CleanVector&)=delete;
  Q169CleanVector& operator=(const Q169CleanVector&)=delete;
  ~Q169CleanVector(){if(!this->empty())OPENSSL_cleanse(this->data(),this->size()*sizeof(T));}
};
struct Q169CleanBools {
  size_t count; std::unique_ptr<bool[]> p;
  explicit Q169CleanBools(size_t n):count(n),p(n?new bool[n]:nullptr){}
  ~Q169CleanBools(){if(p)OPENSSL_cleanse(p.get(),count*sizeof(bool));}
};
struct Q169Wipe {
  void* p; size_t n;
  Q169Wipe(void* value,size_t bytes):p(value),n(bytes){}
  Q169Wipe(const Q169Wipe&)=delete;
  ~Q169Wipe(){OPENSSL_cleanse(p,n);}
};
inline size_t q169_bytes(size_t rows,unsigned bits){return (rows*bits+7)/8;}
inline unsigned q169_get(const uint8_t* p,size_t row,unsigned bits) {
  unsigned v=0;
  for(unsigned k=0;k<bits;++k){size_t at=row*bits+k;v|=((p[at/8]>>(at%8))&1)<<k;}
  return v;
}
inline void q169_put(uint8_t* p,size_t row,unsigned bits,unsigned v) {
  check(v<(1u<<bits),"quotient169 packed range");
  for(unsigned k=0;k<bits;++k){size_t at=row*bits+k;p[at/8]|=((v>>k)&1)<<(at%8);}
}
inline void q169_check_padding(const uint8_t* p,size_t rows,unsigned bits) {
  const unsigned tail=(rows*bits)%8;
  if(tail)check((p[q169_bytes(rows,bits)-1]>>tail)==0,"quotient169 nonzero wire padding");
}
}
