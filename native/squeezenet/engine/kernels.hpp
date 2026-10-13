#pragma once
#include "squeezenet/engine/types.hpp"
#include "squeezenet/engine/decision.hpp"
#include <string>
namespace aqcf2 {
using Word=std::vector<Bits>;
inline Bits flatten(const Word&w){require(!w.empty(),"empty word");std::size_t n=w[0].v.size();Bits z(n*w.size());for(std::size_t j=0;j<w.size();++j){require(w[j].v.size()==n,"word shape");std::copy(w[j].v.begin(),w[j].v.end(),z.v.begin()+j*n);}return z;}
template<class Backend> inline Word add_word(Backend&s,Word a,Word b,unsigned width){
 require(!a.empty()&&!b.empty(),"adder words");std::size_t n=a[0].v.size();while(a.size()<width)a.push_back(s.constant(false,n));while(b.size()<width)b.push_back(s.constant(false,n));a.resize(width);b.resize(width);
 Word p;std::vector<std::pair<Bits,Bits>> pairs;for(unsigned j=0;j<width;++j){p.push_back(s.xor_(a[j],b[j]));pairs.emplace_back(a[j],b[j]);}Word g=s.and_batch(pairs),orig=p;
 for(unsigned h=1;h<width;h*=2){pairs.clear();for(unsigned j=h;j<width;++j){pairs.emplace_back(p[j],g[j-h]);pairs.emplace_back(p[j],p[j-h]);}auto ab=s.and_batch(pairs);Word ng=g,np=p;for(unsigned j=h;j<width;++j){ng[j]=s.xor_(g[j],ab[2*(j-h)]);np[j]=ab[2*(j-h)+1];}g=std::move(ng);p=std::move(np);}
 Word out{orig[0]};for(unsigned j=1;j<width;++j)out.push_back(s.xor_(orig[j],g[j-1]));return out;
}
template<class Backend> inline Bits reduce_or(Backend&s,Word x){require(!x.empty(),"empty reduction");while(x.size()>1){std::vector<std::pair<Bits,Bits>> p;for(std::size_t j=0;j+1<x.size();j+=2)p.emplace_back(x[j],x[j+1]);auto ab=s.and_batch(p);Word y;for(std::size_t j=0;j<ab.size();++j)y.push_back(s.xor_(s.xor_(x[2*j],x[2*j+1]),ab[j]));if(x.size()%2)y.push_back(x.back());x=std::move(y);}return x[0];}
template<class Backend> inline Word owned_word(Backend&s,const std::vector<U>&x,unsigned width,int owner){Word out;for(unsigned j=0;j<width;++j){auto b=s.local_bit(x,j);if(s.party!=owner)std::fill(b.v.begin(),b.v.end(),0);out.push_back(std::move(b));}return out;}
struct Biased {Ring base;std::vector<U>low;Bits parity;unsigned f;};
template<class Backend> inline Biased half_bias(Backend&s,const Ring&x,unsigned f,unsigned quotient_width=0){
 x.check();require(x.width==55&&(f==6||f==16),"R55 RTE6/16 only");unsigned k=quotient_width?quotient_width:55-f;require(k>=3&&k<=55-f,"certified quotient width");Biased a{Ring(k,x.v.size()),std::vector<U>(x.v.size()),Bits(x.v.size()),f};U d=U(1)<<f;
 for(std::size_t j=0;j<x.v.size();++j){U u=(x.v[j]+(s.party==1?C+d/2-1:0))&mask(55);a.low[j]=u&(d-1);a.base.v[j]=((u>>f)-(s.party==1?(C>>f):0))&mask(k);a.parity.v[j]=a.base.v[j]&1;}return a;
}
template<class Backend> inline Bits correction(Backend&s,const Biased&a,const std::string&method="native"){return s.decision_profile([&](){if(method=="tree"||method=="folded")return decision_tree(s,a.low,a.parity,a.f,method=="folded");auto ge=s.carry_eq(a.low,a.f);return s.xor_(ge.first,s.and_(ge.second,a.parity));});}
template<class Backend> inline Ring positive_lift(Backend&s,const Ring&x){
 // True shared y is in [0, 2^(width-1)); local shares need not be small.
 x.check();require(x.width<55,"positive lift width");unsigned f=55-x.width;std::vector<std::uint8_t>top(x.v.size());for(std::size_t j=0;j<x.v.size();++j)top[j]=(x.v[j]>>(x.width-1))&1;
 auto prod=s.bit_product(top,f);Ring y(55,x.v.size());for(std::size_t j=0;j<x.v.size();++j){U w=(U(top[j])-prod.v[j])&mask(f);y.v[j]=(x.v[j]-(w<<x.width))&mask(55);}return y;
}
template<class Backend> inline Ring signed_lift(Backend&s,const Ring&x){
 // Existing geometric extension. Bound |x| < floor(2^width/3).
 x.check();require(x.width<55,"signed lift width");unsigned f=55-x.width;U h=(U(1)<<x.width)/3;std::vector<std::uint8_t>own(2*x.v.size());std::size_t n=x.v.size();
 for(std::size_t j=0;j<n;++j){own[j]=x.v[j]<h;own[n+j]=x.v[j]>=(U(1)<<x.width)-h;}auto ab=s.bit_product(own,f);Ring y(55,n);
 for(std::size_t j=0;j<n;++j){U k=((s.party==1?1:0)-ab.v[j]+ab.v[n+j])&mask(f);y.v[j]=(x.v[j]-(k<<x.width))&mask(55);}return y;
}
template<class Backend> inline Ring ring_rte(Backend&s,const Ring&x,unsigned f,const std::string&method="native"){auto a=half_bias(s,x,f);auto d=s.b2a(correction(s,a,method),a.base.width);for(std::size_t j=0;j<d.v.size();++j)d.v[j]=(d.v[j]+a.base.v[j])&mask(d.width);return signed_lift(s,d);}
template<class Backend> inline Ring ring_relu(Backend&s,const Ring&x,unsigned f,const std::string&variant,unsigned quotient_width=0){
 auto a=half_bias(s,x,f,quotient_width);auto d=correction(s,a,variant=="m-tree"?"tree":variant=="m-folded"?"folded":"native");Bits g;Ring q=a.base;
 if(variant=="m-sequential"){
  auto da=s.b2a(d,q.width);for(std::size_t j=0;j<q.v.size();++j)q.v[j]=(q.v[j]+da.v[j])&mask(q.width);g=s.not_(s.msb(q));
 }else if((variant=="m-strong"||variant=="m-tree"||variant=="m-folded")){
  // A strong control may select by coarse sign: q=0 makes its sign irrelevant.
  g=s.not_(s.msb(q));auto da=s.b2a(d,q.width);for(std::size_t j=0;j<q.v.size();++j)q.v[j]=(q.v[j]+da.v[j])&mask(q.width);
 }else if(variant=="deferred-audit-only"){
  // Algebraic candidate retained for auditing, NOT enabled by the run plan.
  // It adds an AND versus m-strong and has no established independent gain.
  g=s.not_(s.msb(q));auto da=s.b2a(s.and_(g,d),q.width);auto y=s.mux(g,q);for(std::size_t j=0;j<q.v.size();++j)y.v[j]=(y.v[j]+da.v[j])&mask(q.width);return positive_lift(s,y);
 }else throw std::runtime_error("unrecognized ring algorithm");
 return positive_lift(s,s.mux(g,q));
}
template<class Backend> inline Field prime_rte(Backend&s,const Field&x,unsigned f,bool relu,unsigned quotient_width=0){
 x.check();require(f==6||f==16,"prime RTE shift");const std::size_t n=x.v.size();const U D=U(1)<<f,HP=(P+D-1)/D,delta=HP*D-P,h=U(1)<<58;const unsigned kw=quotient_width?quotient_width:55-f,gw=f==16?2:3;
 require(kw>=3&&kw<=55-f,"certified prime quotient width");Bits l0(n),l1(n),r0(n),r1(n);std::vector<U>lo(n),a(n);
 for(std::size_t j=0;j<n;++j){lo[j]=x.v[j]&(D-1);a[j]=x.v[j]>>f;if(s.party==1){l0.v[j]=x.v[j]<h;r0.v[j]=x.v[j]>=3*h;}else{l1.v[j]=x.v[j]<h;r1.v[j]=x.v[j]>=3*h;}}
 auto edge=s.and_batch({{l0,l1},{r0,r1}});Bits odd=s.xor_(s.not_(edge[0]),edge[1]),hi=edge[1];
 Word low=add_word(s,owned_word(s,lo,f+2,1),owned_word(s,lo,f+2,2),f+2),fix;
 for(unsigned b=0;b<f+2;++b){auto q=s.constant(false,n);if((delta>>b)&1)q=s.xor_(q,odd);if(((2*delta)>>b)&1)q=s.xor_(q,hi);fix.push_back(std::move(q));}
 auto t=add_word(s,low,fix,f+2);auto parity=s.xor_(s.local_bit(a,0),t[f]);if(HP&1)parity=s.xor_(parity,odd);
 Word sticky(t.begin(),t.begin()+f-1);auto eta=s.and_(t[f-1],s.or_(reduce_or(s,sticky),parity));Word gamma=add_word(s,Word{t[f],t[f+1]},Word{eta},gw);
 Word states{odd,hi};states.insert(states.end(),gamma.begin(),gamma.end());auto fp=s.b2p(flatten(states));Field q(n);
 for(std::size_t j=0;j<n;++j){W v=a[j]-W(HP)*(W(fp.v[j])+2*W(fp.v[n+j]));for(unsigned b=0;b<gw;++b)v+=(W(fp.v[(2+b)*n+j])<<b);q.v[j]=residue(v,P);}
 if(!relu)return q;
 // Ring view used only for sign. It is derived with PAID true B2A, not a cast.
 Word narrow_states=gamma;const U hp=HP&mask(kw);if(hp){narrow_states.push_back(odd);narrow_states.push_back(hi);}auto small=s.b2a(flatten(narrow_states),kw);Ring qr(kw,n);
 for(std::size_t j=0;j<n;++j){U v=a[j];for(unsigned b=0;b<gw;++b)v+=small.v[b*n+j]<<b;if(hp)v-=hp*(small.v[gw*n+j]+2*small.v[(gw+1)*n+j]);qr.v[j]=v&mask(kw);}
 return s.mux(s.not_(s.msb(qr)),q);
}
}
