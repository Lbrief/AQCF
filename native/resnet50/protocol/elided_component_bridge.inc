// One-use ideal-word-OT row reduction over Z_(2^a) x (Z_2)^b.
// The original full hashed native chosen-choice OT supplier is unchanged.
namespace {
uint64_t e249mask(unsigned w){return (uint64_t(1)<<w)-1;}
uint64_t e249sub(uint64_t x,uint64_t y,unsigned w,unsigned low){
 const uint64_t lm=low?e249mask(low):0;
 return ((((x>>low)-(y>>low))&e249mask(w-low))<<low)|((x^y)&lm);
}
void e249put(uint8_t* p,size_t item,unsigned w,uint64_t v){
 const size_t bit=item*w,offset=bit/8;const unsigned shift=bit%8;
 uint64_t z=0;std::memcpy(&z,p+offset,8);z|=v<<shift;std::memcpy(p+offset,&z,8);
}
uint64_t e249get(const uint8_t* p,size_t item,unsigned w){
 const size_t bit=item*w;uint64_t z=0;std::memcpy(&z,p+bit/8,8);return (z>>(bit%8))&e249mask(w);
}
}
extern "C" int aqcf_elide359_lookup(aqcf_silent_handle* h,int sender,const uint64_t* table,
 const uint8_t* choice,uint64_t* result,int rows,unsigned ib,unsigned width,unsigned low){
 return invoke(h,[&]{
  check(rows>0&&rows<=262144&&result,"249 bounded rows and output");
  check((ib==2&&width==26&&low==1)||(ib==1&&width==26&&low==1)||(ib==1&&width==12&&low==0),"249 fixed qualified terminal group");
  check(h->slice207&&h->slice208&&aqcf_word208::eligible(ib,width),"249 full-word supplier and coordinate bound");
  check(sender==1||sender==2,"359 public sender permutation");const bool sending=h->party==sender;const unsigned B=1u<<ib;const uint64_t mm=e249mask(width);
  check(sending?(table&&!choice):(!table&&choice),"249 role-owned input");
  if(sending)for(size_t j=0;j<size_t(rows)*B;++j)check(table[j]<=mm,"249 canonical function table");
  else for(int j=0;j<rows;++j)check(choice[j]<B,"249 canonical selection");
  auto& ot=h->direction(sender);auto& io=h->io[sender-1];using aqcf_control_emp::block;
  Q197CleanVector<block> a(size_t(rows)*ib),b(sending?size_t(rows)*ib:0);
  Q197CleanBools selections(sending?0:size_t(rows)*ib);
  if(sending)ot.send_ot_rm_cc(a.data(),b.data(),size_t(rows)*ib);
  else{
   for(int j=0;j<rows;++j)for(unsigned l=0;l<ib;++l)selections.p[size_t(j)*ib+l]=(choice[j]>>l)&1;
   ot.recv_ot_rm_cc(a.data(),selections.p.get(),size_t(rows)*ib);
  }
  const size_t bits=size_t(rows)*(B-1)*width,bytes=(bits+7)/8;
  // Eight local zero guard bytes make unaligned fixed-width loads safe.
  // They are never transmitted; all actual padding bits are checked below.
  Q197CleanVector<uint8_t> wire(bytes+8);
  if(sending){
   for(int j=0;j<rows;++j){
    const auto* k0=a.data()+size_t(j)*ib;const auto* k1=b.data()+size_t(j)*ib;
    const uint64_t p0=aqcf_word208::sender_pad(k0,k1,ib,width,0);
    const uint64_t r=e249sub(table[size_t(j)*B],p0,width,low);result[j]=r;
    check((e249sub(table[size_t(j)*B],r,width,low)^p0)==0,"249 omitted row is identically zero");
    for(unsigned v=1;v<B;++v){
     const uint64_t pad=aqcf_word208::sender_pad(k0,k1,ib,width,v);
     e249put(wire.data(),size_t(j)*(B-1)+v-1,width,e249sub(table[size_t(j)*B+v],r,width,low)^pad);
    }
   }
   io.send_data(wire.data(),int(bytes));io.flush();
  }else{
   io.recv_data(wire.data(),int(bytes));
   if(bits%8)check((wire[bytes-1]>>(bits%8))==0,"249 nonzero padding");
   for(int j=0;j<rows;++j){const unsigned v=choice[j];
    const uint64_t ciphertext=v?e249get(wire.data(),size_t(j)*(B-1)+v-1,width):0;
    result[j]=ciphertext^aqcf_word208::receiver_pad(a.data()+size_t(j)*ib,ib,width,v);
   }
  }
  io.flush();++h->stats.lookup_calls;h->stats.lookup_rows+=rows;
  ++h->word208_calls;h->word208_rows+=rows;h->word208_native_rows+=uint64_t(rows)*ib;
  ++h->elide249_calls;h->elide249_rows+=rows;h->elide249_rm_rows+=uint64_t(rows)*ib;
  h->elide249_payload_bytes+=bytes;h->elide249_omitted_bits+=uint64_t(rows)*width;
 });
}
extern "C" int aqcf_elide249_stats(const aqcf_silent_handle* h,uint64_t* calls,
 uint64_t* rows,uint64_t* rm,uint64_t* bytes,uint64_t* omitted){
 if(!h||h->poisoned||!calls||!rows||!rm||!bytes||!omitted)return -1;
 *calls=h->elide249_calls;*rows=h->elide249_rows;*rm=h->elide249_rm_rows;
 *bytes=h->elide249_payload_bytes;*omitted=h->elide249_omitted_bits;return 0;
}

extern "C" int aqcf_elide249_lookup(aqcf_silent_handle*h,const uint64_t*t,const uint8_t*c,uint64_t*r,int n,unsigned i,unsigned w,unsigned l){return aqcf_elide359_lookup(h,1,t,c,r,n,i,w,l);}
