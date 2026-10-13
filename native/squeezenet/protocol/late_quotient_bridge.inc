// Extracts the exact native prefix-hash pads; no dummy table is sent.
extern "C" int aqcf_late37_prepare(aqcf_silent_handle* h,int sender,const uint8_t* digit,const uint8_t* carry,int rows,uint64_t* id){
 return invoke(h,[&]{
  check(rows>0&&rows<=65536&&id&&(sender==1||sender==2),"late37 shape");
  check(h->late37.size()<8,"late37 maximum live banks");
  const bool sending=h->party==sender;check(sending||(digit&&carry),"late37 choices");
  auto b=std::make_unique<Late37Bank>();b->sender=sender;b->rows=rows;
  auto& ot=h->direction(sender);using aqcf_control_emp::block;using aqcf_control_emp::makeBlock;
  std::vector<block> keys0(size_t(rows)*5),keys1(sending?size_t(rows)*5:0);
  if(sending)ot.send_ot_rm_cc(keys0.data(),keys1.data(),size_t(rows)*5);
  else{
   std::unique_ptr<bool[]> choices(new bool[size_t(rows)*5]);b->choice.assign(digit,digit+rows);b->carry.assign(carry,carry+rows);
   for(int j=0;j<rows;++j){check(digit[j]<16&&carry[j]<2,"late37 canonical choice");for(int s=0;s<4;++s)choices[j*5+s]=(digit[j]>>s)&1;choices[j*5+4]=carry[j];}
   ot.recv_ot_rm_cc(keys0.data(),choices.get(),size_t(rows)*5);OPENSSL_cleanse(choices.get(),size_t(rows)*5*sizeof(bool));
  }
  if(sending){
   block hi0[15],hi1[15],ho[30];int idx=0;
   for(int x=0;x<4;++x)for(int y=0;y<(1<<x);++y){hi0[idx]=makeBlock(y,0);hi1[idx]=makeBlock((1<<x)+y,0);++idx;}
   b->pads.resize(size_t(rows)*16);b->k0.resize(rows);b->k1.resize(rows);
   for(int j=0;j<rows;++j){
    ot.mitccrh.renew_ks(keys0.data()+j*5,4);ot.mitccrh.hash_exp(ho,hi0,4);
    ot.mitccrh.renew_ks(keys1.data()+j*5,4);ot.mitccrh.hash_exp(ho+15,hi1,4);
    for(int k=0;k<16;++k){block p=_mm_setzero_si128();int at=0;for(int s=0;s<4;++s){int pref=k&((1<<s)-1);p^=ho[at+pref+((k&(1<<s))?15:0)];at+=1<<s;}b->pads[size_t(j)*16+k]=uint64_t(_mm_extract_epi64(p,0))&3;}
    b->k0[j]=uint64_t(_mm_extract_epi64(keys0[j*5+4],0))&1;b->k1[j]=uint64_t(_mm_extract_epi64(keys1[j*5+4],0))&1;
   }OPENSSL_cleanse(ho,sizeof(ho));
  }else{
   block hi[4],ho[4];b->pads.resize(rows);b->k0.resize(rows);
   for(int j=0;j<rows;++j){for(int s=0;s<4;++s)hi[s]=makeBlock(digit[j]&((1<<(s+1))-1),0);
    ot.mitccrh.renew_ks(keys0.data()+j*5,4);ot.mitccrh.hash_single(ho,hi,4);block p=_mm_setzero_si128();for(int s=0;s<4;++s)p^=ho[s];
    b->pads[j]=uint64_t(_mm_extract_epi64(p,0))&3;b->k0[j]=uint64_t(_mm_extract_epi64(keys0[j*5+4],0))&1;
   }OPENSSL_cleanse(ho,sizeof(ho));
  }
  OPENSSL_cleanse(keys0.data(),keys0.size()*sizeof(block));if(!keys1.empty())OPENSSL_cleanse(keys1.data(),keys1.size()*sizeof(block));
  // Native hash key schedules are refreshed by the next call; erase the last
  // expansion state here as well, without changing the pinned construction.
  OPENSSL_cleanse(&ot.mitccrh,sizeof(ot.mitccrh));
  h->io[sender-1].flush();*id=++h->late37_next;h->late37.emplace(*id,std::move(b));++h->late37_calls;h->late37_rows+=rows;
 });
}
extern "C" int aqcf_late37_consume(aqcf_silent_handle* h,uint64_t id,const uint8_t* digit,const uint8_t* carry,const uint8_t* mask,uint8_t* result,int rows){
 return invoke(h,[&]{
  auto it=h->late37.find(id);check(it!=h->late37.end(),"late37 reused/unknown bank");auto& b=*it->second;
  check(rows==b.rows&&result,"late37 consume shape");const bool sending=h->party==b.sender;
  std::vector<uint32_t> wire(rows);
  if(sending){check(digit&&carry&&mask,"late37 sender inputs");
   for(int j=0;j<rows;++j){check(digit[j]<16&&carry[j]<2&&mask[j]<2,"late37 sender canonical");uint32_t w=0;
    for(unsigned x=0;x<16;++x){unsigned sum=digit[j]+x;
     unsigned u0=((sum+carry[j])>>4)^mask[j]^b.k0[j],u1=((sum+(carry[j]^1))>>4)^mask[j]^b.k1[j];
     w|=uint32_t((u0|(u1<<1))^b.pads[size_t(j)*16+x])<<(2*x);
    }wire[j]=w;result[j]=mask[j];}
   h->io[b.sender-1].send_data(wire.data(),rows*4);h->io[b.sender-1].flush();
  }else{
   h->io[b.sender-1].recv_data(wire.data(),rows*4);
   for(int j=0;j<rows;++j){unsigned pair=((wire[j]>>(2*b.choice[j]))&3)^b.pads[j];result[j]=((pair>>b.carry[j])&1)^b.k0[j];}
  }
  OPENSSL_cleanse(wire.data(),wire.size()*sizeof(uint32_t));h->late37.erase(it);++h->late37_consumed;
 });
}
extern "C" int aqcf_late37_stats(const aqcf_silent_handle* h,uint64_t* calls,uint64_t* rows,uint64_t* live){
 if(!h||!calls||!rows||!live)return -1;*calls=h->late37_calls;*rows=h->late37_rows;*live=h->late37.size();return h->late37_consumed==h->late37_calls?0:-2;
}
