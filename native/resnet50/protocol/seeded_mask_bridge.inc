// Isolated frontend over the unchanged production012 native supplier.
// No native/model experiment is admitted by this source file.
extern "C" int aqcf_seed221_prepare2(aqcf_silent_handle* h,int sender,
                                      uint8_t* alpha,int rows,uint64_t* id) {
  return invoke(h,[&]{
    check(rows>0&&rows<=200704&&id&&(sender==1||sender==2),"quotient169 prepare shape");
    check(h->quotient169.size()<16,"quotient169 live bank limit");
    check(h->quotient169_next<std::numeric_limits<uint64_t>::max(),"quotient169 id exhaustion");
    const bool sending=h->party==sender;
    check(sending?alpha!=nullptr:alpha==nullptr,"quotient169 owned prepare input");
    check(h->slice207&&h->slice208,"221 requires qualified207/208 provider");
    auto b=std::make_unique<Quotient169Bank>();b->sender=sender;b->rows=rows;
    auto& ot=h->direction(sender);auto& io=h->io[sender-1];
    using aqcf_control_emp::block;using aqcf_control_emp::makeBlock;
    Q169CleanVector<block> keys0(size_t(rows)*3),keys1(sending?size_t(rows)*3:0);
    Q169CleanBools choice(sending?0:size_t(rows)*3);
    // Native RM hashing and fresh native seed are INCLUDED. Only then truncate.
    if(sending)ot.send_ot_rm_rc(keys0.data(),keys1.data(),rows*3);
    else ot.recv_ot_rm_rc(keys0.data(),choice.p.get(),rows*3);
    Q169Wipe hash_state(&ot.mitccrh,sizeof(ot.mitccrh));
    Q169CleanVector<uint8_t> wire(q169_bytes(rows,1));
    if(sending) {
      block hi0[3]={makeBlock(0,0),makeBlock(0,0),makeBlock(1,0)};
      block hi1[3]={makeBlock(1,0),makeBlock(2,0),makeBlock(3,0)};
      block ho[6];Q169Wipe hash_values(ho,sizeof(ho));
      b->pads.resize(size_t(rows)*4);b->k0.resize(rows);b->k1.resize(rows);
      b->alpha.resize(rows);
      if(h->slice207)aqcf_slice207::sender(2,keys0.data(),keys1.data(),rows,b->pads.data());
      for(int j=0;j<rows;++j) {
        if(!h->slice207){
        ot.mitccrh.renew_ks(keys0.data()+j*3,2);ot.mitccrh.hash_exp(ho,hi0,2);
        ot.mitccrh.renew_ks(keys1.data()+j*3,2);ot.mitccrh.hash_exp(ho+3,hi1,2);
        for(unsigned k=0;k<4;++k){
          block p=ho[(k&1)?3:0]^ho[1+(k&1)+((k&2)?3:0)];
          b->pads[size_t(j)*4+k]=uint64_t(_mm_extract_epi64(p,0))&1;
        }
        }
        b->k0[j]=uint64_t(_mm_extract_epi64(keys0[j*3+2],0))&3;
        b->k1[j]=uint64_t(_mm_extract_epi64(keys1[j*3+2],0))&3;
        alpha[j]=b->alpha[j]=(b->k0[j]+b->k1[j])&1;
        const unsigned D=quotient169::early(alpha[j],b->k0[j],b->k1[j]);
        check((D&1)==0,"221 derived even early2");q169_put(wire.data(),j,1,D>>1);
      }
      io.send_data(wire.data(),int(wire.size()));io.flush();
    } else {
      b->pads.resize(rows);b->k0.resize(rows);b->R.resize(rows);b->r.resize(rows);b->D.resize(rows);
      block hi[2],ho[2];Q169Wipe hash_values(ho,sizeof(ho));
      for(int j=0;j<rows;++j) {
        b->R[j]=unsigned(choice.p[j*3])|(unsigned(choice.p[j*3+1])<<1);
        b->r[j]=choice.p[j*3+2];
        b->k0[j]=uint64_t(_mm_extract_epi64(keys0[j*3+2],0))&3;
        if(!h->slice207){
        hi[0]=makeBlock(b->R[j]&1,0);hi[1]=makeBlock(b->R[j],0);
        ot.mitccrh.renew_ks(keys0.data()+j*3,2);ot.mitccrh.hash_single(ho,hi,2);
        const block p=ho[0]^ho[1];b->pads[j]=uint64_t(_mm_extract_epi64(p,0))&1;
        }
      }
      if(h->slice207)aqcf_slice207::receiver(2,keys0.data(),b->R.data(),rows,b->pads.data());
      io.recv_data(wire.data(),int(wire.size()));q169_check_padding(wire.data(),rows,1);
      for(int j=0;j<rows;++j)b->D[j]=q169_get(wire.data(),j,1)<<1;
    }
    io.flush();
    const uint64_t next=h->quotient169_next+1;
    h->quotient169.emplace(next,std::move(b));h->quotient169_next=next;*id=next;
    auto& s=h->quotient169_stats;++s.prepared_calls;s.prepared_rows+=rows;
    if(h->slice207)h->sliced207_radix4_rows+=rows;
    s.native_rot_rows+=uint64_t(rows)*3;s.live_banks=h->quotient169.size();
    s.peak_live_banks=std::max(s.peak_live_banks,s.live_banks);
    s.explicit_early_bytes+=wire.size();h->seed221_core_rows+=rows;
    ++h->rot_calls;h->rot_rows+=uint64_t(rows)*3;
  });
}

extern "C" int aqcf_seed221_prepare3(aqcf_silent_handle* h,int sender,uint8_t* alpha,int rows,uint64_t* id){
 return invoke(h,[&]{const unsigned bits=3;
  check(rows>0&&rows<=200704&&id&&(sender==1||sender==2)&&(bits==2||bits==3),"197 prepare shape");
  check(h->quotient197.size()<11&&h->quotient197_next<std::numeric_limits<uint64_t>::max(),"197 bank limit/id");
  const bool sending=h->party==sender;const unsigned B=1u<<bits,tree=B-1,stride=bits+1;
  check(sending?alpha!=nullptr:alpha==nullptr,"197 prepare ownership");
  check(h->slice207&&h->slice208,"221 requires qualified207/208 provider");
  auto b=std::make_unique<Quotient197Bank>();b->sender=sender;b->rows=rows;b->bits=bits;
  auto& ot=h->direction(sender);auto& io=h->io[sender-1];
  using aqcf_control_emp::block;using aqcf_control_emp::makeBlock;
  Q197CleanVector<block> keys0(size_t(rows)*stride),keys1(sending?size_t(rows)*stride:0);
  Q197CleanBools choice(sending?0:size_t(rows)*stride);
  if(sending)ot.send_ot_rm_rc(keys0.data(),keys1.data(),rows*stride);
  else ot.recv_ot_rm_rc(keys0.data(),choice.p.get(),rows*stride);
  Q197Wipe hash_state(&ot.mitccrh,sizeof(ot.mitccrh));Q197CleanVector<uint8_t> wire(q197_bytes(rows,bits-1));
  if(sending){
   block hi0[7],hi1[7],ho[14];Q197Wipe hash_values(ho,sizeof(ho));
   for(unsigned l=0;l<bits;++l)for(unsigned j=0;j<(1u<<l);++j){hi0[(1u<<l)-1+j]=makeBlock(j,0);hi1[(1u<<l)-1+j]=makeBlock((1u<<l)+j,0);}
   b->pads.resize(size_t(rows)*B);b->k0.resize(rows);b->k1.resize(rows);b->alpha.resize(rows);
   if(h->slice207)aqcf_slice207::sender(bits,keys0.data(),keys1.data(),rows,b->pads.data());
   for(int j=0;j<rows;++j){
    if(!h->slice207){
    ot.mitccrh.renew_ks(keys0.data()+j*stride,bits);ot.mitccrh.hash_exp(ho,hi0,bits);
    ot.mitccrh.renew_ks(keys1.data()+j*stride,bits);ot.mitccrh.hash_exp(ho+tree,hi1,bits);
    for(unsigned v=0;v<B;++v){block p=makeBlock(0,0);for(unsigned l=0;l<bits;++l)p^=ho[((v>>l)&1)*tree+((1u<<l)-1)+(v&((1u<<l)-1))];b->pads[size_t(j)*B+v]=uint64_t(_mm_extract_epi64(p,0))&1;}
    }
    b->k0[j]=uint64_t(_mm_extract_epi64(keys0[j*stride+bits],0))&(B-1);b->k1[j]=uint64_t(_mm_extract_epi64(keys1[j*stride+bits],0))&(B-1);
    alpha[j]=b->alpha[j]=(b->k0[j]+b->k1[j])&1;
    const unsigned D=quotient197::early(bits,alpha[j],b->k0[j],b->k1[j]);
    check((D&1)==0,"221 derived even early3");q197_put(wire.data(),j,bits-1,D>>1);
   }
   io.send_data(wire.data(),int(wire.size()));io.flush();
  }else{
   b->pads.resize(rows);b->k0.resize(rows);b->R.resize(rows);b->r.resize(rows);b->D.resize(rows);
   block hi[3],ho[3];Q197Wipe hash_values(ho,sizeof(ho));
   for(int j=0;j<rows;++j){
    unsigned R=0;for(unsigned l=0;l<bits;++l)R|=unsigned(choice.p[j*stride+l])<<l;b->R[j]=R;
    b->r[j]=choice.p[j*stride+bits];b->k0[j]=uint64_t(_mm_extract_epi64(keys0[j*stride+bits],0))&(B-1);
    if(!h->slice207){
    for(unsigned l=0;l<bits;++l)hi[l]=makeBlock(R&((1u<<(l+1))-1),0);
    ot.mitccrh.renew_ks(keys0.data()+j*stride,bits);ot.mitccrh.hash_single(ho,hi,bits);
    block p=makeBlock(0,0);for(unsigned l=0;l<bits;++l)p^=ho[l];b->pads[j]=uint64_t(_mm_extract_epi64(p,0))&1;
    }
   }
   if(h->slice207)aqcf_slice207::receiver(bits,keys0.data(),b->R.data(),rows,b->pads.data());
   io.recv_data(wire.data(),int(wire.size()));q197_check_padding(wire.data(),rows,bits-1);
   for(int j=0;j<rows;++j)b->D[j]=q197_get(wire.data(),j,bits-1)<<1;
  }
  io.flush();const uint64_t next=h->quotient197_next+1;h->quotient197.emplace(next,std::move(b));h->quotient197_next=next;*id=next;
  auto& s=h->quotient197_stats;++s.prepared_calls;s.prepared_rows+=rows;if(h->slice207){if(bits==2)h->sliced207_radix4_rows+=rows;else h->sliced207_radix8_rows+=rows;}
  s.native_rot_rows+=uint64_t(rows)*stride;s.live_banks=h->quotient197.size();
  s.peak_live_banks=std::max(s.peak_live_banks,s.live_banks);s.explicit_early_bytes+=wire.size();h->seed221_leaf_rows+=rows;++h->rot_calls;h->rot_rows+=uint64_t(rows)*stride;
 });
}

extern "C" int aqcf_seed221_read_stats(const aqcf_silent_handle* h,uint64_t* core,uint64_t* leaf){
 if(!h||!core||!leaf||h->poisoned)return -1;*core=h->seed221_core_rows;*leaf=h->seed221_leaf_rows;
 return h->quotient169.empty()&&h->quotient197.empty()?0:-2;
}
