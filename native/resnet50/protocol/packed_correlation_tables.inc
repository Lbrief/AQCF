#include <utility>
static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"352 original native little endian wire");
// Same native chosen-COT generation and MITCCRH hash. Only encode true bits.
// Extra eight bytes are local bounds guards, never transmitted.
namespace packed353 {
inline bool enabled(){const char* p=std::getenv("AQCF_PACKED353");check(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"352 explicit arm");return p[0]=='1';}
inline bool width_ok(unsigned w){return w==12||w==25||w==37;}
inline void put(uint8_t* p,size_t row,unsigned w,uint64_t value){
 const size_t bit=row*w;uint64_t word=0;std::memcpy(&word,p+bit/8,8);word|=value<<(bit%8);std::memcpy(p+bit/8,&word,8);
}
inline uint64_t get(const uint8_t* p,size_t row,unsigned w){
 const size_t bit=row*w;uint64_t word=0;std::memcpy(&word,p+bit/8,8);return (word>>(bit%8))&((uint64_t(1)<<w)-1);
}

template<unsigned W,size_t J>inline void encode_word(uint64_t* words,const uint64_t* values){
 constexpr size_t word=(J*W)/64,shift=(J*W)%64;
 words[word]|=values[J]<<shift;
 if constexpr(shift+W>64)words[word+1]|=values[J]>>(64-shift);
}
template<unsigned W,size_t... J>inline void pack8_impl(uint8_t* p,const uint64_t* v,std::index_sequence<J...>){
 static_assert(W==12||W==25||W==37,"352 fixed original widths");
 uint64_t words[(8*W+63)/64]{};
 (encode_word<W,J>(words,v),...);
 std::memcpy(p,words,W);
}
template<unsigned W,size_t J>inline void decode_word(const uint64_t* words,uint64_t* values){
 constexpr size_t word=(J*W)/64,shift=(J*W)%64;
 uint64_t v=words[word]>>shift;
 if constexpr(shift+W>64)v|=words[word+1]<<(64-shift);
 values[J]=v&((uint64_t(1)<<W)-1);
}
template<unsigned W,size_t... J>inline void unpack8_impl(const uint8_t* p,uint64_t* v,std::index_sequence<J...>){
 uint64_t words[(8*W+63)/64]{};std::memcpy(words,p,W);
 (decode_word<W,J>(words,v),...);
}
template<unsigned W>inline void pack8(uint8_t* p,const uint64_t* v){pack8_impl<W>(p,v,std::make_index_sequence<8>{});}
template<unsigned W>inline void unpack8(const uint8_t* p,uint64_t* v){unpack8_impl<W>(p,v,std::make_index_sequence<8>{});}
inline void pack8_dispatch(uint8_t* p,const uint64_t* v,unsigned w){
 switch(w){case 12:pack8<12>(p,v);break;case 25:pack8<25>(p,v);break;case 37:pack8<37>(p,v);break;default:check(false,"352 width");}
}
inline void unpack8_dispatch(const uint8_t* p,uint64_t* v,unsigned w){
 switch(w){case 12:unpack8<12>(p,v);break;case 25:unpack8<25>(p,v);break;case 37:unpack8<37>(p,v);break;default:check(false,"352 width");}
}
template<unsigned W>inline void cot_width(NativeOT& ot,bool sending,uint64_t* output,const uint64_t* corr,const bool* choice,int n){
 using aqcf_control_emp::block;constexpr int64_t batch=aqcf_control_emp::ot_bsize;
 static_assert(batch==8,"353 original hash batch");constexpr int chunk_rows=4096;
 const uint64_t mask=(uint64_t(1)<<W)-1;
 check(width_ok(W)&&n>0&&n<=(1<<20),"353 original public dimensions");
 // Every element is fully initialized by the original native OT call before any read.
 // Wipe remains mandatory on normal and exceptional exits; allocation need not zero first.
 std::unique_ptr<block[]> raw(new block[n]);Q197Wipe raw_wipe(raw.get(),size_t(n)*sizeof(block));
 Q197CleanVector<uint8_t> wire(size_t(chunk_rows)*W/8+8);
 block seed;Q197Wipe seed_wipe(&seed,sizeof(seed));
 if(sending){
  ot.send_ot_rcm_cc(raw.get(),n);
  ot.ferret->prg.random_block(&seed,1);ot.ferret->io->send_block(&seed,1);ot.ferret->mitccrh.setS(seed);ot.ferret->io->flush();
  block pad[2*batch]{};Q197Wipe pad_wipe(pad,sizeof(pad));
  uint64_t encoded[8]{};Q197Wipe encoded_wipe(encoded,sizeof(encoded));
  for(int begin=0;begin<n;begin+=chunk_rows){
   const int m=std::min(chunk_rows,n-begin);const size_t bytes=(size_t(m)*W+7)/8;
   for(int i=0;i<m;i+=batch){
    const int count=std::min<int>(batch,m-i);const int pos=begin+i;
    for(int j=0;j<count;++j){pad[2*j]=raw[pos+j];pad[2*j+1]=raw[pos+j]^ot.ferret->Delta;}
    ot.ferret->mitccrh.template hash<batch,2>(pad);
    for(int j=0;j<count;++j){
     output[pos+j]=uint64_t(_mm_extract_epi64(pad[2*j],0))&mask;
     encoded[j]=(corr[pos+j]+output[pos+j]+uint64_t(_mm_extract_epi64(pad[2*j+1],0)))&mask;
    }
    if(count==8)pack8<W>(wire.data()+size_t(i)*W/8,encoded);
    else{
     auto* tail=wire.data()+size_t(i)*W/8;std::memset(tail,0,(size_t(count)*W+7)/8+8);
     for(int j=0;j<count;++j)put(wire.data(),i+j,W,encoded[j]);
    }
   }
   ot.ferret->io->send_data(wire.data(),int(bytes));ot.ferret->io->flush();
  }
 }else{
  ot.recv_ot_rcm_cc(raw.get(),choice,n);
  ot.ferret->io->recv_block(&seed,1);ot.ferret->mitccrh.setS(seed);
  block pad[batch]{};Q197Wipe pad_wipe(pad,sizeof(pad));
  uint64_t decoded[8]{};Q197Wipe decoded_wipe(decoded,sizeof(decoded));
  for(int begin=0;begin<n;begin+=chunk_rows){
   const int m=std::min(chunk_rows,n-begin);const size_t bytes=(size_t(m)*W+7)/8;
   ot.ferret->io->recv_data(wire.data(),int(bytes));q197_check_padding(wire.data(),m,W);
   for(int i=0;i<m;i+=batch){
    const int count=std::min<int>(batch,m-i);const int pos=begin+i;
    std::memcpy(pad,raw.get()+pos,count*sizeof(block));ot.ferret->mitccrh.template hash<batch,1>(pad);
    if(count==8)unpack8<W>(wire.data()+size_t(i)*W/8,decoded);
    else for(int j=0;j<count;++j)decoded[j]=get(wire.data(),i+j,W);
    for(int j=0;j<count;++j){const uint64_t key=uint64_t(_mm_extract_epi64(pad[j],0));output[pos+j]=(choice[pos+j]?decoded[j]-key:key)&mask;}
   }
  }
 }
}
inline void cot(NativeOT& ot,bool sending,uint64_t* output,const uint64_t* corr,const bool* choice,int n,unsigned w){
 switch(w){
 case 12:cot_width<12>(ot,sending,output,corr,choice,n);break;
 case 25:cot_width<25>(ot,sending,output,corr,choice,n);break;
 case 37:cot_width<37>(ot,sending,output,corr,choice,n);break;
 default:check(false,"352 fixed original widths");}
}
}

extern "C" int aqcf_packed353_codec_selftest(){
 uint64_t checked=0,state=0x3525eeda7b0192ULL;
 for(unsigned w:{12,25,37})for(size_t n:{1,2,7,8,9,15,16,17,33,257,65537}){
  const size_t bytes=(n*w+7)/8;Q197CleanVector<uint8_t> wire(bytes+8),reference(bytes);
  const uint64_t mask=(uint64_t(1)<<w)-1;Q197CleanVector<uint64_t> value(n);
  for(size_t j=0;j<n;++j){state^=state<<13;state^=state>>7;state^=state<<17;value[j]=(j==0?0:j==1?mask:state&mask);
   for(unsigned k=0;k<w;++k)reference[(j*w+k)/8]|=((value[j]>>k)&1)<<((j*w+k)%8);
  }
  for(size_t j=0;j<n;j+=8){if(n-j>=8)packed353::pack8_dispatch(wire.data()+j*w/8,value.data()+j,w);else for(size_t k=j;k<n;++k)packed353::put(wire.data(),k,w,value[k]);}
  check(!std::memcmp(wire.data(),reference.data(),bytes),"352 independent scalar wire oracle");
  Q197CleanVector<uint64_t> decoded(n);
  for(size_t j=0;j<n;j+=8){if(n-j>=8)packed353::unpack8_dispatch(wire.data()+j*w/8,decoded.data()+j,w);else for(size_t k=j;k<n;++k)decoded[k]=packed353::get(wire.data(),k,w);}
  for(size_t j=0;j<n;++j){check(decoded[j]==value[j],"352 codec inverse");++checked;}
  q197_check_padding(wire.data(),n,w);
  for(size_t j=bytes;j<wire.size();++j)check(wire[j]==0,"352 zero unsent bounds guard");
 }

 check(checked==197706,"352 fixed boundary QA count");
 for(unsigned w:{12,25,37})for(size_t bit=0;bit<8*w;++bit){
  uint64_t values[8]{},got[8]{};values[bit/w]=uint64_t(1)<<(bit%w);
  uint8_t buf[80]{},reference[80]{};reference[11+bit/8]=uint8_t(1)<<(bit%8);
  packed353::pack8_dispatch(buf+11,values,w);
  check(std::memcmp(buf,reference,sizeof buf)==0,"352 bit basis exact and no out-of-bounds write");
  packed353::unpack8_dispatch(buf+11,got,w);
  check(std::memcmp(values,got,sizeof values)==0,"352 full inverse basis");
 }
 return 0;
}
