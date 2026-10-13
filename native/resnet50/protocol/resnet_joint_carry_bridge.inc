namespace {
void j304put(uint8_t* p,size_t row,unsigned bits,uint64_t v){
 check(bits==23||bits==24||bits==35,"304 early width");check(v<(uint64_t(1)<<bits),"304 early canonical");size_t at=row*bits;uint64_t z=0;std::memcpy(&z,p+at/8,8);z|=v<<(at%8);std::memcpy(p+at/8,&z,8);
}
uint64_t j304get(const uint8_t* p,size_t row,unsigned bits){size_t at=row*bits;uint64_t z=0;std::memcpy(&z,p+at/8,8);return (z>>(at%8))&((uint64_t(1)<<bits)-1);}
}
// One joint prefix/arithmetic bank. The four native hashed RM rows are paid once, then erased once.
extern "C" int aqcf_joint304_prepare3(aqcf_silent_handle* h,int sender,uint8_t* alpha,int rows,unsigned w,uint64_t* id){
 return invoke(h,[&]{const unsigned bits=3;check(w==25||w==26||w==37,"304 tap width");const uint64_t wm=(uint64_t(1)<<(w-1))-1;const unsigned early_bits=w-2;const size_t early_bytes=q197_bytes(rows,early_bits);
  check(rows>0&&rows<=262144&&id&&(sender==1||sender==2)&&(bits==2||bits==3),"197 prepare shape");
  check(h->quotient197.size()<11&&h->quotient197_next<std::numeric_limits<uint64_t>::max(),"197 bank limit/id");
  const bool sending=h->party==sender;const unsigned B=1u<<bits,tree=B-1,stride=bits+1;
  check(sending?alpha!=nullptr:alpha==nullptr,"197 prepare ownership");
  check(h->slice207&&h->slice208,"221 requires qualified207/208 provider");
  auto b=std::make_unique<Quotient197Bank>();b->sender=sender;b->rows=rows;b->bits=bits;b->joint304=true;b->tap_width304=w;
  auto& ot=h->direction(sender);auto& io=h->io[sender-1];
  using aqcf_control_emp::block;using aqcf_control_emp::makeBlock;
  Q197CleanVector<block> keys0(size_t(rows)*stride),keys1(sending?size_t(rows)*stride:0);
  Q197CleanBools choice(sending?0:size_t(rows)*stride);
  if(sending)ot.send_ot_rm_rc(keys0.data(),keys1.data(),rows*stride);
  else ot.recv_ot_rm_rc(keys0.data(),choice.p.get(),rows*stride);
  Q197Wipe hash_state(&ot.mitccrh,sizeof(ot.mitccrh));Q197CleanVector<uint8_t> wire(early_bytes+8);
  if(sending){
   block hi0[7],hi1[7],ho[14];Q197Wipe hash_values(ho,sizeof(ho));
   for(unsigned l=0;l<bits;++l)for(unsigned j=0;j<(1u<<l);++j){hi0[(1u<<l)-1+j]=makeBlock(j,0);hi1[(1u<<l)-1+j]=makeBlock((1u<<l)+j,0);}
   b->pads.resize(size_t(rows)*B);b->k0.resize(rows);b->k1.resize(rows);b->alpha.resize(rows);b->wide0.resize(rows);b->wide1.resize(rows);
   if(h->slice207)aqcf_slice207::sender(bits,keys0.data(),keys1.data(),rows,b->pads.data());
   for(int j=0;j<rows;++j){
    if(!h->slice207){
    ot.mitccrh.renew_ks(keys0.data()+j*stride,bits);ot.mitccrh.hash_exp(ho,hi0,bits);
    ot.mitccrh.renew_ks(keys1.data()+j*stride,bits);ot.mitccrh.hash_exp(ho+tree,hi1,bits);
    for(unsigned v=0;v<B;++v){block p=makeBlock(0,0);for(unsigned l=0;l<bits;++l)p^=ho[((v>>l)&1)*tree+((1u<<l)-1)+(v&((1u<<l)-1))];b->pads[size_t(j)*B+v]=uint64_t(_mm_extract_epi64(p,0))&1;}
    }
    b->wide0[j]=uint64_t(_mm_extract_epi64(keys0[j*stride+bits],0))&wm;b->wide1[j]=uint64_t(_mm_extract_epi64(keys1[j*stride+bits],0))&wm;
    b->k0[j]=b->wide0[j]&(B-1);b->k1[j]=b->wide1[j]&(B-1);
    alpha[j]=b->alpha[j]=(b->k0[j]+b->k1[j])&1;
    const uint64_t D=(alpha[j]+b->wide0[j]+b->wide1[j])&wm;
    check((D&1)==0,"221 derived even early3");j304put(wire.data(),j,early_bits,D>>1);
   }
   io.send_data(wire.data(),int(early_bytes));io.flush();
  }else{
   b->pads.resize(rows);b->k0.resize(rows);b->R.resize(rows);b->r.resize(rows);b->D.resize(rows);b->wide0.resize(rows);b->wideD.resize(rows);
   block hi[3],ho[3];Q197Wipe hash_values(ho,sizeof(ho));
   for(int j=0;j<rows;++j){
    unsigned R=0;for(unsigned l=0;l<bits;++l)R|=unsigned(choice.p[j*stride+l])<<l;b->R[j]=R;
    b->r[j]=choice.p[j*stride+bits];b->wide0[j]=uint64_t(_mm_extract_epi64(keys0[j*stride+bits],0))&wm;b->k0[j]=b->wide0[j]&(B-1);
    if(!h->slice207){
    for(unsigned l=0;l<bits;++l)hi[l]=makeBlock(R&((1u<<(l+1))-1),0);
    ot.mitccrh.renew_ks(keys0.data()+j*stride,bits);ot.mitccrh.hash_single(ho,hi,bits);
    block p=makeBlock(0,0);for(unsigned l=0;l<bits;++l)p^=ho[l];b->pads[j]=uint64_t(_mm_extract_epi64(p,0))&1;
    }
   }
   if(h->slice207)aqcf_slice207::receiver(bits,keys0.data(),b->R.data(),rows,b->pads.data());
   io.recv_data(wire.data(),int(early_bytes));q197_check_padding(wire.data(),rows,early_bits);
   for(int j=0;j<rows;++j){b->wideD[j]=j304get(wire.data(),j,early_bits)<<1;b->D[j]=b->wideD[j]&(B-1);}
  }
  io.flush();const uint64_t next=h->quotient197_next+1;h->quotient197.emplace(next,std::move(b));h->quotient197_next=next;*id=next;
  auto& s=h->quotient197_stats;++s.prepared_calls;s.prepared_rows+=rows;if(h->slice207){if(bits==2)h->sliced207_radix4_rows+=rows;else h->sliced207_radix8_rows+=rows;}
  s.native_rot_rows+=uint64_t(rows)*stride;s.live_banks=h->quotient197.size();
  s.peak_live_banks=std::max(s.peak_live_banks,s.live_banks);s.explicit_early_bytes+=early_bytes;h->seed221_leaf_rows+=rows;++h->joint304_prepared_calls;h->joint304_prepared_rows+=rows;++h->rot_calls;h->rot_rows+=uint64_t(rows)*stride;
 });
}

extern "C" int aqcf_joint304_consume(aqcf_silent_handle* h,uint64_t id,const uint8_t* digit,const uint8_t* carry,const uint8_t* mask,uint8_t* result,uint64_t* arithmetic,int rows){
 return invoke(h,[&]{
  auto it=h->quotient197.find(id);check(it!=h->quotient197.end(),"197 reused/unknown bank");auto& b=*it->second;
  check(b.joint304&&arithmetic,"304 wrong bank or missing arithmetic output");
  const bool sending=h->party==b.sender;const unsigned bits=b.bits,B=1u<<bits;const uint64_t wm=(uint64_t(1)<<(b.tap_width304-1))-1,om=(uint64_t(1)<<b.tap_width304)-1;
  check(rows==b.rows&&digit&&carry&&result&&(sending?mask!=nullptr:mask==nullptr),"197 consume ownership");
  for(int j=0;j<rows;++j){check(digit[j]<B&&carry[j]<2,"197 digit/carry range");if(sending)check(mask[j]<2&&carry[j]==b.alpha[j],"197 changed prepared alpha");}
  auto& io=h->io[b.sender-1];Q197CleanVector<uint8_t> corrections(q197_bytes(rows,bits+1)),wire(q197_bytes(rows,B)),quotients(sending?0:rows);
  if(sending){
   io.recv_data(corrections.data(),int(corrections.size()));q197_check_padding(corrections.data(),rows,bits+1);
   for(int j=0;j<rows;++j)q197_put(wire.data(),j,B,quotient197::table(bits,digit[j],carry[j],mask[j],b.k0[j],b.k1[j],q197_get(corrections.data(),j,bits+1),b.pads.data()+size_t(j)*B));
   for(int j=0;j<rows;++j){unsigned e=q197_get(corrections.data(),j,bits+1)&1;uint64_t K=e?b.wide1[j]:b.wide0[j];arithmetic[j]=(uint64_t(carry[j])+2*K)&om;}
   io.send_data(wire.data(),int(wire.size()));io.flush();for(int j=0;j<rows;++j)result[j]=mask[j];
  }else{
   for(int j=0;j<rows;++j){auto c=quotient197::correct(bits,digit[j],carry[j],b.r[j],b.k0[j],b.D[j],b.R[j]);q197_put(corrections.data(),j,bits+1,c.wire);quotients[j]=c.quotient;}
   for(int j=0;j<rows;++j){uint64_t Q=carry[j]?((b.wideD[j]-b.wide0[j])&wm):b.wide0[j];arithmetic[j]=(uint64_t(carry[j])-2*Q)&om;}
   io.send_data(corrections.data(),int(corrections.size()));io.flush();io.recv_data(wire.data(),int(wire.size()));q197_check_padding(wire.data(),rows,B);
   for(int j=0;j<rows;++j)result[j]=quotient197::finish(bits,q197_get(wire.data(),j,B),b.R[j],b.pads[j],quotients[j]);
  }
  auto& s=h->quotient197_stats;s.explicit_correction_bytes+=corrections.size();s.explicit_table_bytes+=wire.size();++s.consumed_calls;s.consumed_rows+=rows;
  ++h->joint304_consumed_calls;h->joint304_consumed_rows+=rows;
  h->quotient197.erase(it);s.live_banks=h->quotient197.size();
 });
}
extern "C" int aqcf_joint304_stats(const aqcf_silent_handle* h,uint64_t* pc,uint64_t* pr,uint64_t* cc,uint64_t* cr){
 if(!h||!pc||!pr||!cc||!cr||h->poisoned)return -1;
 *pc=h->joint304_prepared_calls;*pr=h->joint304_prepared_rows;*cc=h->joint304_consumed_calls;*cr=h->joint304_consumed_rows;
 return (*pc==*cc&&*pr==*cr&&h->quotient197.empty())?0:-2;
}
