#pragma once
// Route19 request-local native BFV machinery. No asset readers or network IO.
#include "resnet50/core/batch_norm_owner_binding.hpp"
#include "resnet50/core/seeded_wire.hpp"
#include "resnet50/engine/seeded_key_policy.hpp"
#include <seal/seal.h>
#include <seal/util/iterator.h>
#include <seal/util/rns.h>
#include <openssl/crypto.h>
#include <memory>
#include <sstream>
namespace aqcf_bn19 {
using W=__uint128_t;using S=__int128_t;
constexpr U N=8192,T=U(1)<<44,M=U(1)<<37;
using Bytes=std::vector<unsigned char>;
inline U mod(S x,U m){x%=S(m);return U(x<0?x+m:x);}
inline U power(U a,U e,U q){U r=1;for(;e;e>>=1,a=U(W(a)*a%q))if(e&1)r=U(W(r)*a%q);return r;}
inline U groups(const Geometry& g){return (g.channels+g.C-1)/g.C;}
inline U count(const Geometry& g,U group){require(group<groups(g),"BN19 group range");return std::min<U>(g.C,g.channels-group*g.C);}
inline std::vector<U> query(const Geometry& g,U group){std::vector<U> q;for(U r=0;r<count(g,group);++r)for(U j=0;j<g.L;++j)q.push_back(j*g.C+r);return q;}
inline U return_size(const Geometry& g,U group,bool retained=false){return ((retained?0:49*count(g,group)*g.L)+62*N+7)/8;}
struct Bits {
 struct Writer{Bytes b;W hold=0;unsigned n=0;void put(U x,unsigned width){require(width>0&&width<64&&(x>>width)==0,"BN19 bounded wire word");hold|=W(x)<<n;n+=width;while(n>=8){b.push_back(unsigned(hold&255));hold>>=8;n-=8;}}Bytes finish(){if(n)b.push_back(unsigned(hold&255));return std::move(b);}};
 struct Reader{const Bytes& b;W hold=0;unsigned n=0;U at=0;U get(unsigned width){require(width>0&&width<64,"BN19 reader width");while(n<width){require(at<b.size(),"BN19 short wire");hold|=W(b[at++])<<n;n+=8;}U x=U(hold&((W(1)<<width)-1));hold>>=width;n-=width;return x;}void end(){require(at==b.size()&&hold==0,"BN19 canonical tail");}};
};
struct Codec {
 std::array<U,3> q;W Q;U inv01,inv012;
 explicit Codec(const std::vector<seal::Modulus>& qs):q{qs.at(0).value(),qs.at(1).value(),qs.at(2).value()},Q(W(q[0])*q[1]*q[2]),inv01(power(q[0]%q[1],q[1]-2,q[1])),inv012(power(U(W(q[0])*q[1]%q[2]),q[2]-2,q[2])){require(qs.size()==3,"BN19 three data primes");for(auto v:q)require(v>=(U(1)<<37)&&v<(U(1)<<38),"BN19 fixed38 primes");}
 W crt(const U* a,U j)const{W x=a[j];U d=mod(S(a[N+j])-S(x%q[1]),q[1]);x+=W(q[0])*U(W(d)*inv01%q[1]);d=mod(S(a[2*N+j])-S(x%q[2]),q[2]);return x+W(q[0])*q[1]*U(W(d)*inv012%q[2]);}
 Bytes pack(const seal::Ciphertext& ct,const Geometry& g,U group,bool retained=false)const{require(ct.size()==2&&ct.poly_modulus_degree()==N&&ct.coeff_modulus_size()==3&&!ct.is_ntt_form(),"BN19 return metadata");Bits::Writer w;if(!retained)for(U j:query(g,group))w.put(U((crt(ct.data(0),j)+(W(1)<<64))>>65),49);for(U j=0;j<N;++j)w.put(U((crt(ct.data(1),j)+(W(1)<<51))>>52),62);auto b=w.finish();require(b.size()==return_size(g,group,retained),"BN19 projected exact length");return b;}
 void unpack(const Bytes& frame,seal::Ciphertext& ct,const Geometry& g,U group,bool retained=false)const{require(frame.size()==return_size(g,group,retained)&&ct.size()==2&&ct.coeff_modulus_size()==3&&ct.poly_modulus_degree()==N,"BN19 return frame shape");std::fill_n(ct.data(),6*N,U(0));Bits::Reader r{frame};auto get=[&](unsigned c,U j,unsigned d){U k=r.get(114-d);require(W(k)<((Q-1+(W(1)<<(d-1)))>>d)+1,"BN19 bounded CRT word");W v=(W(k)<<d)%Q;for(unsigned z=0;z<3;++z)ct.data(c)[z*N+j]=U(v%q[z]);};if(!retained)for(U j:query(g,group))get(0,j,65);for(U j=0;j<N;++j)get(1,j,52);r.end();}
 static constexpr U prefix=113,suffix=81,raw_size=prefix+3*N*8+suffix,input_size=8+prefix+3*N*38/8+suffix;
 void validate_seeded(const std::string& s)const{using namespace aqcf_seeded_wire;require(s.size()==raw_size,"BN19 seeded raw size");header(s,0,raw_size);require(s[48]==0&&word(s.data()+49)==2&&word(s.data()+57)==N&&word(s.data()+65)==3,"BN19 seeded shape");header(s,89,24+3*N*8);require(word(s.data()+105)==3*N,"BN19 full c0");header(s,prefix+3*N*8,suffix);}
 Bytes pack_seeded(const std::string& raw)const{validate_seeded(raw);Bits::Writer w;for(U j=0;j<3*N;++j){U v=aqcf_seeded_wire::word(raw.data()+prefix+8*j);require(v<q[j/N],"BN19 input residue");w.put(v,38);}auto packed=w.finish();Bytes b(input_size);std::memcpy(b.data(),"AQBN1938",8);std::memcpy(b.data()+8,raw.data(),prefix);std::copy(packed.begin(),packed.end(),b.begin()+8+prefix);std::memcpy(b.data()+input_size-suffix,raw.data()+raw_size-suffix,suffix);return b;}
 std::string unpack_seeded(const Bytes& b)const{require(b.size()==input_size&&!std::memcmp(b.data(),"AQBN1938",8),"BN19 fixed seeded frame");std::string raw(raw_size,'\0');std::memcpy(raw.data(),b.data()+8,prefix);Bytes packed(b.begin()+8+prefix,b.end()-suffix);Bits::Reader r{packed};for(U j=0;j<3*N;++j){U v=r.get(38);require(v<q[j/N],"BN19 canonical seeded residue");aqcf_seeded_wire::put(raw.data()+prefix+8*j,v);}r.end();std::memcpy(raw.data()+raw_size-suffix,b.data()+input_size-suffix,suffix);validate_seeded(raw);return raw;}
};
inline void shift(seal::Ciphertext& ct,I exponent,const std::vector<seal::Modulus>& qs){const U e=mod(exponent,2*N);std::vector<U> tmp(3*N);for(unsigned c=0;c<2;++c){for(unsigned r=0;r<3;++r){U q=qs[r].value();for(U i=0;i<N;++i){U j=(i+e)%(2*N),v=ct.data(c)[r*N+i];tmp[r*N+j%N]=(j<N||v==0)?v:q-v;}}std::copy(tmp.begin(),tmp.end(),ct.data(c));}}
inline seal::Ciphertext trace(const seal::Ciphertext& in,const std::vector<I>& weights,seal::Evaluator& ev,const seal::GaloisKeys& keys,const std::vector<seal::Modulus>& qs,U& switches){
 const unsigned C=weights.size();require(C>=2&&C<=128&&(C&(C-1))==0,"BN19 trace C");std::vector<seal::Ciphertext> nodes{in};
 for(unsigned step=1;step<C;step*=2){std::vector<seal::Ciphertext> next(2*step);for(unsigned r=0;r<step;++r){auto rotated=nodes[r];ev.apply_galois_inplace(rotated,1+N/step,keys);++switches;shift(rotated,-I(N/step*r),qs);next[r]=nodes[r];next[r+step]=nodes[r];ev.add_inplace(next[r],rotated);ev.sub_inplace(next[r+step],rotated);}nodes=std::move(next);}
 auto out=in;std::fill_n(out.data(),6*N,U(0));for(unsigned r=0;r<C;++r)for(unsigned c=0;c<2;++c)for(unsigned z=0;z<3;++z){U q=qs[z].value(),w=mod(weights[r],q);for(U j=0;j<N;++j){U v=U(W(nodes[r].data(c)[z*N+j])*w%q);auto& x=out.data(c)[z*N+j];x+=v;if(x>=q)x-=q;}}return out;
}
struct Lane {
 std::unique_ptr<seal::SEALContext> context,quarter;std::unique_ptr<Codec> codec;
 seal::PublicKey pk;seal::GaloisKeys gk;std::unique_ptr<seal::SecretKey> sk;
 std::array<Bytes,2> key_wire343;
 std::unique_ptr<seal::Encryptor> enc;std::unique_ptr<seal::Evaluator> ev;std::unique_ptr<seal::Decryptor> dec;
 U encrypted=0,returned=0,decrypted=0,switches=0,zeros=0,masked_coefficients=0;
 explicit Lane(bool client){seal::EncryptionParameters p(seal::scheme_type::bfv);p.set_poly_modulus_degree(N);p.set_plain_modulus(T);p.set_coeff_modulus(seal::CoeffModulus::Create(N,{38,38,38,38}));context=std::make_unique<seal::SEALContext>(p,true,seal::sec_level_type::tc128);auto pp=p;pp.set_plain_modulus(4*T);quarter=std::make_unique<seal::SEALContext>(pp,true,seal::sec_level_type::tc128);require(context->parameters_set()&&quarter->parameters_set(),"BN19 tc128 contexts");auto qs=context->first_context_data()->parms().coeff_modulus();codec=std::make_unique<Codec>(qs);W digits=0;for(auto& q:qs)digits+=q.value()-1;U special=p.coeff_modulus().back().value();W K=(W(N)*21*digits+special-1)/special+(N+1);for(const auto& g:geometry){W E=W(g.C)*g.C*(22+K)*(U(1)<<30)+8194,wire=(W(1)<<64)+W(N)*(W(1)<<51);require(W(8)*T*(E+wire)<codec->Q,"BN19 source-bound noise and codec margin");}
  if(client){seal::KeyGenerator kg(*context);sk=std::make_unique<seal::SecretKey>(kg.secret_key());std::vector<std::uint32_t> elements;for(unsigned step=1;step<128;step*=2)elements.push_back(1+N/step);if(policy343::enabled())key_wire343=key343::generate(kg,*context,pk,gk,elements);else{kg.create_public_key(pk);kg.create_galois_keys(elements,gk);}finish_keys(true);}
 }
 void finish_keys(bool client){require(seal::is_valid_for(pk,*context)&&seal::is_valid_for(gk,*context)&&gk.size()==7,"BN19 valid seven evaluation keys");for(unsigned step=1;step<128;step*=2)require(gk.has_key(1+N/step),"BN19 fixed Galois roster");ev=std::make_unique<seal::Evaluator>(*context);if(client){require(bool(sk),"BN19 client key");enc=std::make_unique<seal::Encryptor>(*context,pk,*sk);auto qsk=*sk;qsk.data().parms_id()=quarter->key_parms_id();dec=std::make_unique<seal::Decryptor>(*quarter,qsk);}else{require(!sk,"BN19 server never owns SK");enc=std::make_unique<seal::Encryptor>(*context,pk);}}
 template<class Obj> static Bytes serialize(const Obj& obj){std::ostringstream s(std::ios::binary);obj.save(s,seal::compr_mode_type::none);auto t=s.str();return Bytes(t.begin(),t.end());}
 void load_keys(const Bytes& p,const Bytes& g){require(!sk,"BN19 public-key receiver only");if(policy343::enabled()){key343::load(*context,p,pk,1ULL<<20);key343::load(*context,g,gk,16ULL<<20);finish_keys(false);return;}std::istringstream a(std::string(p.begin(),p.end()),std::ios::binary),b(std::string(g.begin(),g.end()),std::ios::binary);pk.load(*context,a);gk.load(*context,b);require(a.peek()==std::char_traits<char>::eof()&&b.peek()==std::char_traits<char>::eof(),"BN19 keys no trailing data");require(pk.save_size(seal::compr_mode_type::none)==p.size()&&gk.save_size(seal::compr_mode_type::none)==g.size(),"BN19 exact none key lengths");finish_keys(false);}
 seal::Plaintext gather(const Geometry& g,U group,const std::vector<U>& own){require(own.size()==U(g.L)*g.channels,"BN19 share length");seal::Plaintext a;a.resize(N);std::fill_n(a.data(),N,U(0));for(U r=0;r<count(g,group);++r)for(U j=0;j<g.L;++j){U x=own[(group*g.C+r)*g.L+j];require(x<M,"BN19 canonical original share");a[j*g.C+r]=x;}return a;}
 Bytes input(const Geometry& g,U group,const std::vector<U>& own){require(bool(sk),"BN19 input owner");auto a=gather(g,group,own);std::ostringstream wire(std::ios::binary);enc->encrypt_symmetric(a).save(wire,seal::compr_mode_type::none);++encrypted;return codec->pack_seeded(wire.str());}
 Bytes produce(const Geometry& g,U group,const Bytes& frame,const std::vector<U>& own,const PrivateOwner& owner,std::vector<U>& phase){
  require(!sk&&phase.size()==own.size()&&owner.weight.size()==g.channels&&owner.bias.size()==g.channels,"BN19 server-only complete owner");auto raw=codec->unpack_seeded(frame);std::istringstream wire(raw,std::ios::binary);seal::Ciphertext ct;ct.load(*context,wire);require(wire.peek()==std::char_traits<char>::eof()&&ct.size()==2&&!ct.is_ntt_form()&&ct.parms_id()==context->first_parms_id(),"BN19 original input metadata");auto add=gather(g,group,own);ev->add_plain_inplace(ct,add);
  std::vector<I> weights(g.C,0);for(U r=0;r<count(g,group);++r)weights[r]=owner.weight[group*g.C+r];auto qs=context->first_context_data()->parms().coeff_modulus();auto out=trace(ct,weights,*ev,gk,qs,switches);
  // Transparent all-zero linear maps must receive the mandatory fresh zero
  // BEFORE calling add_plain; no zero-map bypass or second zero.
  seal::Ciphertext zero;enc->encrypt_zero(zero);ev->add_inplace(out,zero);++zeros;
  seal::Plaintext bias;bias.resize(N);std::fill_n(bias.data(),N,U(0));for(U r=0;r<count(g,group);++r)for(U j=0;j<g.L;++j)bias[j*g.C+r]=mod(S(g.C)*owner.bias[group*g.C+r],T);ev->add_plain_inplace(out,bias);
  auto rng=seal::Blake2xbPRNGFactory().create();std::vector<U> mask(3*N),server(N);for(unsigned z=0;z<3;++z){U q=qs[z].value(),cut=(U(0)-q)%q;for(U j=0;j<N;++j){U v;do{rng->generate(sizeof(v),reinterpret_cast<seal::seal_byte*>(&v));}while(v<cut);v%=q;mask[z*N+j]=v;auto& x=out.data(0)[z*N+j];x+=v;if(x>=q)x-=q;}}
  quarter->first_context_data()->rns_tool()->decrypt_scale_and_round(seal::util::ConstRNSIter(mask.data(),N),server.data(),seal::MemoryManager::GetPool());for(U r=0;r<count(g,group);++r)for(U j=0;j<g.L;++j)phase[(group*g.C+r)*g.L+j]=local_phase(server[j*g.C+r],g);OPENSSL_cleanse(mask.data(),mask.size()*sizeof(U));OPENSSL_cleanse(server.data(),server.size()*sizeof(U));masked_coefficients+=N;++returned;return codec->pack(out,g,group);
 }
 void consume(const Geometry& g,U group,const Bytes& frame,std::vector<U>& phase,bool retained=false){require(sk&&phase.size()==U(g.L)*g.channels,"BN19 client-only finalizer");seal::Ciphertext ct;ct.resize(*context,context->first_parms_id(),2);codec->unpack(frame,ct,g,group,retained);ct.parms_id()=quarter->first_parms_id();seal::Plaintext pt;dec->decrypt(ct,pt);for(U r=0;r<count(g,group);++r)for(U j=0;j<g.L;++j){U e=j*g.C+r;phase[(group*g.C+r)*g.L+j]=local_phase(e<pt.coeff_count()?pt[e]:0,g);}++decrypted;}
};
}
