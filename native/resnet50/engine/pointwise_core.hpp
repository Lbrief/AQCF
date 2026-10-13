#pragma once
#include "resnet50/engine/pointwise_owner.hpp"
#include "resnet50/engine/bfv_program.hpp"
#include "resnet50/engine/short_ntt.hpp"
#include "resnet50/engine/exact_fma_dot.hpp"
#include "resnet50/engine/output_trace_merge.hpp"
namespace pw25 {
namespace b=aqcf_bn19;
inline std::vector<seal::Ciphertext> split(const seal::Ciphertext& in,U C,b::Lane& server){auto qs=server.context->first_context_data()->parms().coeff_modulus();std::vector<seal::Ciphertext> nodes{in};for(U step=1;step<C;step*=2){std::vector<seal::Ciphertext> next(2*step);for(U r=0;r<step;++r){auto rotated=nodes[r];server.ev->apply_galois_inplace(rotated,1+b::N/step,server.gk);++server.switches;b::shift(rotated,-I(b::N/step*r),qs);next[r]=nodes[r];next[r+step]=nodes[r];server.ev->add_inplace(next[r],rotated);server.ev->sub_inplace(next[r+step],rotated);}nodes=std::move(next);}for(U r=0;r<C;++r)b::shift(nodes[r],-I(r),qs);return nodes;}
struct Dense {
 aqcf3::BfvProgram& he;
 std::array<std::array<std::unique_ptr<b::Lane>,4>,4> keys;
 std::array<Owner,36> owners;
 std::array<U,4> pm{},key_uses{};
 U input_ct=0,output_ct=0;
 U programmed_layers213=0,programmed_ct213=0,programmed_words213=0,unused_words213=0;std::array<U,4> attempts213{};
 explicit Dense(aqcf3::BfvProgram& h):he(h){aqcf3::require(he.ready19&&!he.preparing18,"reuse live actual original BN keys after input");
  he.work19(shapes.size(),"DOT28_ORIGINAL_OWNER_BIND",[&](U j,unsigned){bind_layout(he.layouts[shapes[j].id],shapes[j]);if(he.s.party==1)owners[j]=bind_owner(he.program->at(shapes[j].id),shapes[j]);});
  he.work19(4,"DOT28_WORKER_KEY_HANDLES",[&](U worker,unsigned){for(U key=0;key<4;++key){auto& base=*he.lanes19[key];auto lane=std::make_unique<b::Lane>(false);lane->pk=base.pk;lane->gk=base.gk;if(he.s.party==2){aqcf3::require(bool(base.sk),"client owns original key");lane->sk=std::make_unique<seal::SecretKey>(*base.sk);}else aqcf3::require(!base.sk,"no server secret key");lane->finish_keys(he.s.party==2);keys[key][worker]=std::move(lane);}});
 }
 std::vector<U> prepare(U index,const std::vector<U>& own){const auto&s=shapes.at(index);U key=index%4;++key_uses[key];U ni=(s.Ci+s.C-1)/s.C,no=(s.Co+s.C-1)/s.C;aqcf3::require(ni>=4&&no>=4&&own.size()==he.layouts[s.id].inputs&&!he.requested[s.id]&&he.defer_raw_phase&&he.preparing18,"one-use bound dense layer");he.requested[s.id]=true;he.reserved_returns+=no;he.expected_inputs+=ni;he.expected_outputs+=no;he.original_ranks.push_back(s.old_inputs);he.ranks.push_back(ni);aqcf3::require(he.reserved_returns<=he.cap,"unchanged output budget");
  b::Geometry gi{unsigned(s.id),unsigned(s.L),unsigned(s.Ci),1,unsigned(s.C)},go{unsigned(s.id),unsigned(s.L),unsigned(s.Co),1,unsigned(s.C)};
  std::vector<U>sample(s.Ci*s.L),phase(s.Co*s.L);he.work18(sample.size(),"DOT28_ORIGINAL_SAMPLING",[&](U lo,U hi,unsigned){for(U at=lo;at<hi;++at){U ch=at/s.L,j=at%s.L;sample[at]=own[source_index(s,ch,j/s.W,j%s.W)];}});
  const bool merge515=output_merge515::enabled()&&s.Co<s.Ci;
  const bool programmed=he.selected213(s.id);auto input_size=[&](U group){return programmed?aqcf_input371::wire_size(gi,group):b::Codec::input_size;};
  std::vector<b::Bytes> input(ni),output(no);
  if(he.s.party==2){
   if(programmed){
    auto& mask=he.masks18[s.id];aqcf3::require(mask.empty(),"213 one fresh tensor mask");mask.resize(own.size());
    // Full ordinary masks on every unused tensor coordinate are mandatory.
    // Filling the entire vector first is intentional and fully charged.
    he.work18(mask.size(),"DOT213_FULL_TENSOR_MASK",[&](U lo,U hi,unsigned){auto rng=seal::Blake2xbPRNGFactory().create();rng->generate((hi-lo)*sizeof(U),reinterpret_cast<seal::seal_byte*>(mask.data()+lo));for(U j=lo;j<hi;++j)mask[j]&=b::M-1;});
    he.produce467(ni,"DOT213_PROGRAMMED_INPUT",[&](U ig,unsigned worker){auto& lane=*keys[key][worker];auto packet=aqcf_input371::generate(lane,gi,ig);attempts213[worker]+=packet.attempts;
     for(U r=0;r<b::count(gi,ig);++r)for(U j=0;j<s.L;++j){U at=source_index(s,ig*s.C+r,j/s.W,j%s.W);aqcf3::require(at<mask.size(),"213 original tensor coordinate");mask[at]=packet.plaintext[j*s.C+r]&(b::M-1);}
     input[ig]=std::move(packet.wire);bn191::discard(packet.plaintext);++lane.encrypted;
    },[&]{he.number(s.id);he.number(ni);},[&](U ig){he.send19(input[ig],input_size(ig));b::Bytes().swap(input[ig]);});
   }else {aqcf3::require(!pipeline467::enabled(),"467 qualified programmed-input path only");he.work19(ni,"DOT28_CLIENT_INPUT_HE",[&](U ig,unsigned worker){input[ig]=keys[key][worker]->input(gi,ig,sample);});
   he.number(s.id);he.number(ni);for(U ig=0;ig<ni;++ig)he.send19(input[ig],input_size(ig));}
   he.metrics.encryptions+=ni;he.client_input_max_workers=4;++he.client_input_parallel_layers;he.client_input_parallel_frames+=ni;
   for(unsigned k=0;k<4;++k)he.client_input_cpus[k]=aqcf_native4::cpu(k);
  }else{aqcf3::require(he.number()==s.id&&he.number()==ni,"dense request identity");std::vector<seal::Ciphertext> separated(merge515?ni:s.Ci);
   he.ingest467(ni,"DOT28_SERVER_RESTORE_TRACE_NTT",[&](U ig){input[ig]=he.receive19(input_size(ig));},[&](U ig,unsigned worker){auto&lane=*keys[key][worker];auto raw=programmed?aqcf_input371::restore(lane,input[ig],gi,ig):lane.codec->unpack_seeded(input[ig]);std::istringstream wire(raw,std::ios::binary);seal::Ciphertext ct;ct.load(*lane.context,wire);aqcf3::require(wire.peek()==std::char_traits<char>::eof(),"complete native input");auto p=lane.gather(gi,ig,sample);lane.ev->add_plain_inplace(ct,p);if(merge515){auto qs=lane.context->first_context_data()->parms().coeff_modulus();b::shift(ct,-I(s.C-1),qs);lane.ev->transform_to_ntt_inplace(ct);separated[ig]=std::move(ct);}else{auto nodes=split(ct,s.C,lane);for(U r=0;r<b::count(gi,ig);++r){lane.ev->transform_to_ntt_inplace(nodes[r]);separated[ig*s.C+r]=std::move(nodes[r]);}}if(pipeline467::enabled())b::Bytes().swap(input[ig]);});
   he.produce467(no,"DOT28_SERVER_PM_ZERO_MASK_PACK",[&](U og,unsigned worker){auto&lane=*keys[key][worker];auto&owner=owners[index];seal::Ciphertext ct;he.linear_worker_cpus[worker]=sched_getcpu();++he.linear_worker_outputs[worker];if(merge515){ct=output_merge515::produce(s,og,owner,separated,lane,pm[worker]);}else{lane.enc->encrypt_zero(ct);++lane.zeros;lane.ev->transform_to_ntt_inplace(ct);dot500::Accumulator accumulator(*lane.context,ct);
    for(U i=0;i<s.Ci;++i){seal::Plaintext p;std::vector<I> short_owner(s.C,0);for(U r=0;r<b::count(go,og);++r){I w=owner.w[(og*s.C+r)*s.Ci+i];short_owner[r]=w;}{short_ntt27::transform(short_owner,s.C,*lane.context,p);accumulator.add(separated[i],p);++pm[worker];}}
    ct=accumulator.finish();lane.ev->transform_from_ntt_inplace(ct);}aqcf_internal514::require_closed();seal::Plaintext bias;bias.resize(b::N);std::fill_n(bias.data(),b::N,U(0));for(U r=0;r<b::count(go,og);++r)for(U j=0;j<s.L;++j)bias[j*s.C+r]=b::mod(b::S(s.C)*owner.bias[og*s.C+r],b::T);lane.ev->add_plain_inplace(ct,bias);
    if(retained299::enabled()){
     auto vphase=retain298::first(ct,*lane.quarter);
     for(U r=0;r<b::count(go,og);++r)for(U j=0;j<s.L;++j)phase[(og*s.C+r)*s.L+j]=b::local_phase(vphase[j*s.C+r],go);
     OPENSSL_cleanse(vphase.data(),vphase.size()*sizeof(U));++lane.returned;retained299::record(true,b::count(go,og)*s.L);output[og]=lane.codec->pack(ct,go,og,true);
    }else{
    auto qs=lane.context->first_context_data()->parms().coeff_modulus();auto rng=seal::Blake2xbPRNGFactory().create();std::vector<U>mask(3*b::N),vphase(b::N);for(U z=0;z<3;++z){U q=qs[z].value(),cut=(U(0)-q)%q;for(U j=0;j<b::N;++j){U v;do{rng->generate(sizeof(v),reinterpret_cast<seal::seal_byte*>(&v));}while(v<cut);v%=q;mask[z*b::N+j]=v;auto&a=ct.data(0)[z*b::N+j];a+=v;if(a>=q)a-=q;}}
    lane.quarter->first_context_data()->rns_tool()->decrypt_scale_and_round(seal::util::ConstRNSIter(mask.data(),b::N),vphase.data(),seal::MemoryManager::GetPool());for(U r=0;r<b::count(go,og);++r)for(U j=0;j<s.L;++j)phase[(og*s.C+r)*s.L+j]=b::local_phase(vphase[j*s.C+r],go);OPENSSL_cleanse(mask.data(),mask.size()*sizeof(U));OPENSSL_cleanse(vphase.data(),vphase.size()*sizeof(U));lane.masked_coefficients+=b::N;++lane.returned;output[og]=lane.codec->pack(ct,go,og);
    }
   },[&]{he.number(s.id);he.number(no);},[&](U og){he.send19(output[og],b::return_size(go,og,retained299::enabled()));b::Bytes().swap(output[og]);});he.metrics.fresh_zero+=no;if(!retained299::enabled())he.metrics.full_mask+=b::N*no;he.metrics.input_restore+=ni;if(!retained299::enabled())he.metrics.c0_add+=3*b::N*no;he.metrics.bias_add+=no;
   he.metrics.pm+=s.Ci*no;he.expected_pm+=s.Ci*no;he.metrics.ct_add+=s.Ci*no;he.expected_adds+=s.Ci*no;
   he.maximum_linear_workers=4;++he.parallel_layers;he.parallel_outputs+=no;
  }
  input_ct+=ni;output_ct+=no;he.metrics.input_ct+=ni;he.metrics.output_ct+=no;
  for(U ig=0;ig<ni;++ig)he.metrics.input_payload_bytes+=input_size(ig);
  if(programmed){++programmed_layers213;programmed_ct213+=ni;programmed_words213+=s.Ci*s.L;unused_words213+=own.size()-s.Ci*s.L;}
  if(he.s.party==1)for(U og=0;og<no;++og)he.metrics.output_payload_bytes+=b::return_size(go,og,retained299::enabled());
  return phase;
 }
 std::vector<U> correction(unsigned id,const std::vector<U>& v){
  aqcf3::require(he.s.party==1&&he.correction341_enabled,"341 server-only exact correction");
  auto index=index_of(id);auto work=[&](U n,const char* label,auto f){he.work18(n,label,f);};
  return correction341::pointwise(shapes[index],owners[index],v,work);
 }
 U index_of(unsigned id)const{for(U j=0;j<shapes.size();++j)if(shapes[j].id==id)return j;throw std::runtime_error("bound dense id");}
 void decode(U index,std::vector<U>& phase,std::vector<b::Bytes>& frames){
  const auto& s=shapes.at(index);const U key=index%4,no=(s.Co+s.C-1)/s.C;
  b::Geometry go{unsigned(s.id),unsigned(s.L),unsigned(s.Co),1,unsigned(s.C)};
  aqcf3::require(he.s.party==2&&he.preparing18&&frames.size()==no&&phase.size()==s.Co*s.L,"one queued dense decode");
  for(U og=0;og<no;++og)aqcf3::require(frames[og].size()==b::return_size(go,og,retained299::enabled()),"bound dense returned frame");
  he.work19(no,"DOT28_CLIENT_DECODE_DECRYPT",[&](U og,unsigned worker){
   keys[key][worker]->consume(go,og,frames[og],phase,retained299::enabled());he.client_return_cpus[worker]=sched_getcpu();++he.client_return_counts[worker];
   b::Bytes().swap(frames[og]);
  });
  he.metrics.decryptions+=no;he.client_return_max_workers=4;++he.client_return_layers;he.client_return_frames+=no;
  for(U og=0;og<no;++og)he.metrics.output_payload_bytes+=b::return_size(go,og,retained299::enabled());
 }
 void census(){
  U switches=0,products=0,enc=0,ret=0,dec=0,zeros=0;
  for(U k=0;k<4;++k){aqcf3::require(key_uses[k]==9,"all dense key identities consumed");for(U w=0;w<4;++w){
   auto&l=*keys[k][w];switches+=l.switches;enc+=l.encrypted;ret+=l.returned;dec+=l.decrypted;zeros+=l.zeros;
  }}for(U p:pm)products+=p;
  aqcf3::require(input_ct==1204&&output_ct==1424,"all36 new dense layers");
  aqcf3::require(he.s.party==1?(switches==(output_merge515::enabled()?8796u:17548u)&&products==337920&&zeros==1424&&ret==1424):(enc==1204&&dec==1424),"complete dense crypto census");
  const bool programmed=aqcf_input213::enabled();U attempts=0;for(U v:attempts213)attempts+=v;
  aqcf3::require(programmed?(programmed_layers213==36&&programmed_ct213==1204):(programmed_layers213==0&&programmed_ct213==0&&programmed_words213==0&&unused_words213==0&&attempts==0),"213 whole roster census");
  if(programmed)aqcf3::require(he.s.party==1?attempts==0:(attempts>=1204&&attempts<=4816),"213 fully charged bounded attempts");
  std::cout<<"DOT213_CENSUS "<<he.s.party<<" "<<programmed<<" "<<programmed_layers213<<" "<<programmed_ct213<<" "<<programmed_words213<<" "<<unused_words213<<" "<<attempts<<"\n"<<std::flush;
  std::cout<<"DOT28_CENSUS "<<he.s.party<<" "<<input_ct<<" "<<output_ct<<" "<<products<<" "<<switches<<" "<<zeros<<"\n"<<std::flush;
 }

};
}
