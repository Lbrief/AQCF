#pragma once
#include "resnet50/engine/late_program.hpp"
#include "resnet50/protocol/joint_mask_core.hpp"
#include "resnet50/engine/prescribed_correlation.hpp"
#include "resnet50/engine/quotient_phase_relu.hpp"
#include "resnet50/engine/phase_relu.hpp"
#include <numeric>
namespace compose189 {
namespace b=aqcf_bn19;namespace l=late188;namespace jt=aqcf_joint_mask;
using U=std::uint64_t;using W=__uint128_t;
// Same CSPRNG family as the original full-Q mask producer. Fresh per bank;
// sampling and all consumed random words stay inside the paid call.
struct Random {
 std::shared_ptr<seal::UniformRandomGenerator> rng=seal::Blake2xbPRNGFactory().create();
 template<class T>T word(){T x;rng->generate(sizeof(T),reinterpret_cast<seal::seal_byte*>(&x));return x;}
 W below(W bound){W cut=(W(0)-bound)%bound,x;do{x=word<W>();}while(x<cut);return x%bound;}
};
struct Row {jt::Candidate terminal;W R=0;U y=0;};
inline Row propose(const l::Phase& phase,U correction,unsigned shift,U k0,U k1,Random& random){
 b::require(k0<(U(1)<<26)&&k1<(U(1)<<26)&&correction<b::M&&shift<=7,"189 fresh native26 row and late offset");
 Row row;auto& z=row.terminal;
 U bits=random.word<U>(),diff=(k1-k0)&l::word_mask;
 z.low=std::uint8_t((k0^k1)>>25);z.gate=bits&1;
 z.coarse=((z.gate?U(0)-diff:diff)-z.low)&l::word_mask;
 z.high=std::uint8_t(1^(z.coarse>>24)^z.gate);
 U final=(z.coarse<<12)|((bits>>1)&4095),raw=(final-correction)&(b::M-1);
 U u=(U(0)-4*raw-((bits>>13)&3))&l::phase_mask,extra=(bits>>15)&127;
 row.y=(u<<shift)|(extra&((U(1)<<shift)-1))|((extra>>shift)<<(39+shift));
 row.R=phase.point(row.y,random.below(phase.L));
 z.accepted=phase.scalar(row.R)==row.y;return row;
}
template<class IO>void bits(IO& io,bool sending,std::vector<std::uint8_t>& v){
 std::vector<std::uint8_t> wire((v.size()+7)/8,0);
 if(sending){for(std::size_t j=0;j<v.size();++j){b::require(v[j]<2,"189 control bit");wire[j/8]|=v[j]<<(j%8);}io.send_data(wire.data(),int(wire.size()));io.flush();}
 else{io.recv_data(wire.data(),int(wire.size()));for(std::size_t j=0;j<v.size();++j)v[j]=(wire[j/8]>>(j%8))&1;}
}
struct Bank {
 int party;aqcf_silent_handle* native;std::size_t n;bool prepared=false,spent=false;
 std::vector<U> k0,k1,key;std::vector<std::uint8_t> choice;std::vector<Row> rows;
 U attempted=0,rejected=0;
 Bank(int p,aqcf_silent_handle* h,std::size_t count):party(p),native(h),n(count){
  b::require((p==1||p==2)&&h&&n&&n<=200704,"189 fixed original frontier");
  if(p==1){k0.resize(n);k1.resize(n);rows.resize(n);}else{key.resize(n);choice.resize(n);}
 }
 Bank(const Bank&)=delete;Bank& operator=(const Bank&)=delete;
 ~Bank(){for(auto* v:{&k0,&k1,&key})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));if(!choice.empty())OPENSSL_cleanse(choice.data(),choice.size());if(!rows.empty())OPENSSL_cleanse(rows.data(),rows.size()*sizeof(Row));}
 void check(int rc){b::require(rc==0,aqcf_silent_error(native));}
 template<class IO>void prepare(IO& io,const l::Phase& phase,const std::vector<U>& corr,unsigned shift){
  b::require(!prepared&&!spent&&corr.size()==n,"189 one fresh bank after actual delta");Random random;
  std::vector<std::size_t> pending(n);std::iota(pending.begin(),pending.end(),0);
  for(unsigned a=0;!pending.empty()&&a<128;++a){auto m=pending.size();attempted+=m;
   std::vector<U> x(party==1?m:0),y(party==1?m:0),z(party==2?m:0);std::unique_ptr<bool[]> c(party==2?new bool[m]:nullptr);
   check(aqcf_silent_joint26(native,x.empty()?nullptr:x.data(),y.empty()?nullptr:y.data(),c.get(),z.empty()?nullptr:z.data(),int(m)));
   std::vector<Row> proposed(party==1?m:0);std::vector<std::uint8_t> accept(m);
   if(party==1)for(std::size_t j=0;j<m;++j){proposed[j]=propose(phase,corr[pending[j]],shift,x[j],y[j],random);accept[j]=proposed[j].terminal.accepted;}
   bits(io,party==1,accept);std::vector<std::size_t> next;
   for(std::size_t j=0;j<m;++j){auto at=pending[j];if(!accept[j]){next.push_back(at);++rejected;continue;}
    if(party==1){k0[at]=x[j];k1[at]=y[j];rows[at]=proposed[j];}else{key[at]=z[j];choice[at]=c[j];}}
   pending.swap(next);
  }
  b::require(pending.empty(),"189 bounded native sampler exhaustion; no replay");prepared=true;
 }
 template<class B>std::vector<U> consume(B& s,const std::vector<U>& coarse,const aqcf2::Bits& low,const aqcf2::Bits& high){
  b::require(prepared&&!spent&&coarse.size()==n&&low.v.size()==n&&high.v.size()==n,"189 ready one-use terminal");spent=true;
  std::vector<std::uint8_t> e(n);std::vector<jt::SenderStep> ss(party==1?n:0);std::vector<jt::ReceiverStep> cs(party==2?n:0);
  for(std::size_t j=0;j<n;++j){if(party==1){auto& z=rows[j].terminal;b::require(coarse[j]==z.coarse&&low.v[j]==z.low&&high.v[j]==z.high,"189 actual HE and prefix coupling");}
   else{cs[j]=jt::receiver_step(key[j],choice[j],coarse[j],low.v[j],std::uint8_t((coarse[j]>>24)^high.v[j]));e[j]=cs[j].e;}}
  bits(*s.io.io,party==2,e);std::vector<U> corr(party==2?2*n:0),out(2*n);std::unique_ptr<bool[]> selectors(party==1?new bool[2*n]:nullptr);
  for(std::size_t j=0;j<n;++j)if(party==1){ss[j]=jt::sender_step(k0[j],k1[j],rows[j].terminal,e[j]);selectors[j]=ss[j].gate;selectors[n+j]=ss[j].h;}
   else{corr[j]=cs[j].c1;corr[n+j]=cs[j].c2;}
  s.control().cot(2,out.data(),corr.empty()?nullptr:corr.data(),selectors.get(),int(2*n),25);
  std::vector<U> result(n);for(std::size_t j=0;j<n;++j)result[j]=party==1?jt::sender_output(ss[j],out[j],out[n+j]):jt::receiver_output(cs[j],out[j],out[n+j]);return result;
 }
};
template<class B>aqcf2::Ring nonlinear(B& s,const std::vector<U>& phase,Bank& bank){
 using namespace aqcf2;const auto n=phase.size();std::vector<U> own(n),shifted(n),choice(n),table(s.party==1?64*n:0),coarse(n);
 Bits seed(n),low;std::vector<std::uint8_t> pad,pl(n),ph(n);if(s.party==1)pad=s.random_small(n);
 for(std::size_t j=0;j<n;++j){b::require(phase[j]<(U(1)<<39),"189 native Ring39 phase");U lo=phase[j]&3,hi=phase[j]>>2;
  own[j]=(s.party==1?U(0)-hi-U(lo!=0):hi+U(lo==3))&mask(37);U digit=own[j]&15;choice[j]=(digit<<2)|lo;shifted[j]=own[j]>>4;coarse[j]=own[j]>>12;
  if(s.party==1){pl[j]=bank.rows[j].terminal.low;ph[j]=bank.rows[j].terminal.high;
   for(U c=0;c<64;++c)table[64*j+c]=((digit+(c>>2)+aqcf_r50_phase_relu169::correction(c&3,unsigned(lo)))>>4)^(pad[j]&1);}}
 auto got=s.lookup_words(table,choice,n,6,1,false);s.flush();for(std::size_t j=0;j<n;++j)seed.v[j]=s.party==1?(pad[j]&1):got[j];seed.check();
 std::vector<CarryTask> tasks{{&shifted,32,seed,false}};auto rest=prescribed189_execute(s,tasks,8,low,pl,ph);
 auto small=bank.consume(s,coarse,low,rest.value[0]);std::vector<U> lift_choice(n),lift_table,high_pad;
 if(s.party==1){high_pad=s.random_ring_words(n,12);lift_table.resize(2*n);}
 for(std::size_t j=0;j<n;++j){U top=small[j]>>24;lift_choice[j]=top;if(s.party==1)for(U c=0;c<2;++c)lift_table[2*j+c]=(U(0)-(top|c)-high_pad[j])&mask(12);}
 auto high=s.lookup_words(lift_table,lift_choice,n,1,12);Ring result(37,n);
 for(std::size_t j=0;j<n;++j)result.v[j]=(small[j]+((s.party==1?high_pad[j]:high[j])<<25))&mask(37);
 result.check();return result;
}
}
