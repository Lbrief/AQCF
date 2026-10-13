#include <utility>
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "426 public block codec is defined for little endian x86"
#endif
namespace poolmux426 {
using U=uint64_t;
template<unsigned W,unsigned J> inline void store_word(U* dst,U x){
 static_assert((W==26||W==38)&&J<8,"426 fixed public block geometry");
 constexpr unsigned off=J*W/64,shift=J*W%64;
 dst[off]|=x<<shift;
 if constexpr(shift+W>64)dst[off+1]|=x>>(64-shift);
}
template<unsigned W,unsigned J> inline U load_word(const U* src){
 constexpr unsigned off=J*W/64,shift=J*W%64;
 U x=src[off]>>shift;
 if constexpr(shift+W>64)x|=src[off+1]<<(64-shift);
 return x&((U(1)<<W)-1);
}
template<unsigned W,size_t...J> inline void pack_impl(uint8_t*dst,const U*x,std::index_sequence<J...>){
 U words[(8*W+63)/64]{};(store_word<W,J>(words,x[J]),...);std::memcpy(dst,words,W);
}
template<unsigned W,size_t...J> inline void unpack_impl(const uint8_t*src,U*x,std::index_sequence<J...>){
 U words[(8*W+63)/64]{};std::memcpy(words,src,W);((x[J]=load_word<W,J>(words)),...);
}
template<unsigned W> inline void pack8(uint8_t*dst,const U*x){pack_impl<W>(dst,x,std::make_index_sequence<8>{});}
template<unsigned W> inline void unpack8(const uint8_t*src,U*x){unpack_impl<W>(src,x,std::make_index_sequence<8>{});}
template<unsigned W> void execute(aqcf_silent_handle*h,const U*a,const uint8_t*g,U*out,int rows){
 using aqcf_control_emp::block;constexpr int B=aqcf_control_emp::ot_bsize;
 static_assert(B==8&&(W==25||W==37),"426 original eight-row hash batches");
 constexpr U M=(U(1)<<W)-1;
 for(int i=0;i<rows;++i)check(a[i]<=M&&g[i]<2,"426 share domain");
 Q197CleanVector<block> own_raw(rows),peer_raw(rows);
 // Identical native directions and per-call row demand; no generated rows omitted.
 for(int sender=1;sender<=2;++sender){auto*f=h->direction(sender).ferret;
  block*dst=h->party==sender?own_raw.data():peer_raw.data();
  commonA317::raw(f,rows,2,h->party==sender?1:2,[&]{f->rcot(dst,rows);});}
 auto*send=h->direction(h->party).ferret;auto*recv=h->direction(3-h->party).ferret;
 const size_t size=16+size_t(rows/B)*(W+1);
 Q197CleanVector<uint8_t> own(size),peer(size);
 block seed;Q197Wipe wipe_seed(&seed,sizeof(seed));send->prg.random_block(&seed,1);send->mitccrh.setS(seed);std::memcpy(own.data(),&seed,16);
 block pads[2*B];Q197Wipe wipe_pads(pads,sizeof(pads));U words[B];Q197Wipe wipe_words(words,sizeof(words));
 for(int base=0;base<rows;base+=B){
  for(int j=0;j<B;++j){pads[2*j]=own_raw[base+j];pads[2*j+1]=own_raw[base+j]^send->Delta;}
  send->mitccrh.template hash<B,2>(pads);
  for(int j=0;j<B;++j){int i=base+j;U k0=U(_mm_extract_epi64(pads[2*j],0))&M,k1=U(_mm_extract_epi64(pads[2*j+1],0))&M;
   // Raw sender blocks are dead now; keep both needed keys in that same slot.
   own_raw[i]=aqcf_control_emp::makeBlock(k1,k0);
   U corr=(a[i]*(U(1)-2*U(g[i])))&M,e=U(g[i])^U(aqcf_control_emp::getLSB(peer_raw[i]));
   words[j]=((corr+k0+k1)&M)|(e<<W);
  }
  pack8<W+1>(own.data()+16+size_t(base/B)*(W+1),words);
 }
 if(h->party==1){h->io[0].send_data(own.data(),int(size));h->io[0].flush();h->io[1].recv_data(peer.data(),int(size));}
 else{h->io[0].recv_data(peer.data(),int(size));h->io[1].send_data(own.data(),int(size));h->io[1].flush();}
 std::memcpy(&seed,peer.data(),16);recv->mitccrh.setS(seed);
 for(int base=0;base<rows;base+=B){
  std::memcpy(pads,peer_raw.data()+base,B*sizeof(block));recv->mitccrh.template hash<B,1>(pads);
  unpack8<W+1>(peer.data()+16+size_t(base/B)*(W+1),words);
  for(int j=0;j<B;++j){int i=base+j;U D=words[j]&M,e=words[j]>>W,K=U(_mm_extract_epi64(pads[j],0))&M;
   U k0=U(_mm_extract_epi64(own_raw[i],0)),k1=U(_mm_extract_epi64(own_raw[i],1));
   U sender_share=k0^((k0^k1)&(U(0)-e)),receiver_share=K+((D-2*K)&(U(0)-U(g[i])));
   out[i]=(a[i]*U(g[i])+receiver_share-sender_share)&M;
  }
 }
 h->io[0].flush();h->io[1].flush();h->stats.cot_calls+=2;h->stats.cot_rows+=uint64_t(rows)*2;
 ++h->pool426[0];h->pool426[1]+=rows;h->pool426[2]+=uint64_t(rows)*2;
 h->pool426[W==25?3:4]+=rows;h->pool426[5]+=size;h->pool426[6]+=16;
}
}
extern "C" int aqcf_poolmux426(aqcf_silent_handle*h,const uint64_t*a,const uint8_t*g,uint64_t*out,int rows,unsigned width){
 return invoke(h,[&]{check(rows>0&&rows<=262144&&rows%8==0&&(width==25||width==37)&&a&&g&&out,"426 fixed aligned pool domain");
  if(width==25)poolmux426::execute<25>(h,a,g,out,rows);else poolmux426::execute<37>(h,a,g,out,rows);});
}
extern "C" int aqcf_poolmux426_stats(const aqcf_silent_handle*h,uint64_t*out){
 if(!h||!out||h->poisoned)return -1;std::copy(h->pool426,h->pool426+7,out);return 0;
}
