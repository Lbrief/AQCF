#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <stdexcept>
namespace joint_crt_wire {
using U=std::uint64_t;using W=__uint128_t;
constexpr U q0=274877153281ULL,q1=274877202433ULL;
constexpr W Q=W(q0)*q1;
constexpr unsigned d0=35,d1=22,N=8192;
constexpr W E=W(22)*(U(1)<<30)+4098;
constexpr W added=(W(1)<<(d0-1))+W(N)*(W(1)<<(d1-1));
static_assert(W(8)*(U(1)<<37)*(E+added)<Q,"exact inherited quarter margin plus wire error");
inline void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
inline U powmod(U a,U e,U p){U r=1;for(;e;e>>=1,a=U(W(a)*a%p))if(e&1)r=U(W(r)*a%p);return r;}
inline W crt(U a,U b){need(a<q0&&b<q1,"canonical joint residues");
 static const U inv=powmod(q0,q1-2,q1);U v=b>=a?b-a:q1-(a-b);
 return W(q0)*U(W(v)*inv%q1)+a;}
inline U limit(unsigned d){return U((Q-1+(W(1)<<(d-1)))>>d)+1;}
inline U quantize(U a,U b,unsigned d){return U((crt(a,b)+(W(1)<<(d-1)))>>d);}
inline std::array<U,2> restore(U k,unsigned d){need(k<limit(d),"canonical joint quotient");W x=(W(k)<<d)%Q;return {U(x%q0),U(x%q1)};}
struct Writer {std::vector<unsigned char> out;W buf=0;unsigned bits=0;
 void put(U x,unsigned width){need(width<64&&(x>>width)==0,"joint width");buf|=W(x)<<bits;bits+=width;
 while(bits>=8){out.push_back(unsigned(buf&255));buf>>=8;bits-=8;}}
 std::vector<unsigned char> finish(){if(bits)out.push_back(unsigned(buf&255));return std::move(out);}};
struct Reader {const std::vector<unsigned char>& in;std::size_t at=0;W buf=0;unsigned bits=0;
 U take(unsigned width){while(bits<width){need(at<in.size(),"truncated joint frame");buf|=W(in[at++])<<bits;bits+=8;}
 U x=U(buf&((W(1)<<width)-1));buf>>=width;bits-=width;return x;}
 void end(){need(at==in.size()&&buf==0,"noncanonical joint frame tail");}};
inline std::size_t size(std::size_t queries,bool retained=false){return ((retained?0:queries)*(76-d0)+N*(76-d1)+7)/8;}
// Caller has performed the original full mask and fresh zero. Public query
// order is unchanged, including duplicate exponents at distinct destinations.
inline std::vector<unsigned char> encode(const U* c0,const U* c1,const std::vector<U>& query,bool retained=false){
 Writer w;w.out.reserve(size(query.size(),retained));
 if(!retained)for(U j:query){need(j<N,"query exponent");w.put(quantize(c0[j],c0[N+j],d0),76-d0);}
 for(U j=0;j<N;++j)w.put(quantize(c1[j],c1[N+j],d1),76-d1);
 auto out=w.finish();need(out.size()==size(query.size(),retained),"fixed public joint length");return out;}
inline void decode(const std::vector<unsigned char>& in,U* c0,U* c1,const std::vector<U>& query,bool retained=false){
 need(in.size()==size(query.size(),retained),"fixed public joint frame length");
 std::fill(c0,c0+2*N,U(0));std::fill(c1,c1+2*N,U(0));Reader r{in};
 if(!retained)for(U j:query){need(j<N,"query exponent");auto z=restore(r.take(76-d0),d0);c0[j]=z[0];c0[N+j]=z[1];}
 for(U j=0;j<N;++j){auto z=restore(r.take(76-d1),d1);c1[j]=z[0];c1[N+j]=z[1];}r.end();}
}
