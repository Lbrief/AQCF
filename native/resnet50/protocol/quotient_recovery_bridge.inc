// Isolated frontend over the unchanged production012 native supplier.
// No native/model experiment is admitted by this source file.
extern "C" int aqcf_quotient169_prepare(aqcf_silent_handle* h,int sender,
                                      const uint8_t* alpha,int rows,uint64_t* id) {
  return invoke(h,[&]{
    check(rows>0&&rows<=200704&&id&&(sender==1||sender==2),"quotient169 prepare shape");
    check(h->quotient169.size()<16,"quotient169 live bank limit");
    check(h->quotient169_next<std::numeric_limits<uint64_t>::max(),"quotient169 id exhaustion");
    const bool sending=h->party==sender;
    check(sending?alpha!=nullptr:alpha==nullptr,"quotient169 owned prepare input");
    if(sending)for(int j=0;j<rows;++j)check(alpha[j]<2,"quotient169 alpha range");
    auto b=std::make_unique<Quotient169Bank>();b->sender=sender;b->rows=rows;
    auto& ot=h->direction(sender);auto& io=h->io[sender-1];
    using aqcf_control_emp::block;using aqcf_control_emp::makeBlock;
    Q169CleanVector<block> keys0(size_t(rows)*3),keys1(sending?size_t(rows)*3:0);
    Q169CleanBools choice(sending?0:size_t(rows)*3);
    // Native RM hashing and fresh native seed are INCLUDED. Only then truncate.
    if(sending)ot.send_ot_rm_rc(keys0.data(),keys1.data(),rows*3);
    else ot.recv_ot_rm_rc(keys0.data(),choice.p.get(),rows*3);
    Q169Wipe hash_state(&ot.mitccrh,sizeof(ot.mitccrh));
    Q169CleanVector<uint8_t> wire(q169_bytes(rows,2));
    if(sending) {
      block hi0[3]={makeBlock(0,0),makeBlock(0,0),makeBlock(1,0)};
      block hi1[3]={makeBlock(1,0),makeBlock(2,0),makeBlock(3,0)};
      block ho[6];Q169Wipe hash_values(ho,sizeof(ho));
      b->pads.resize(size_t(rows)*4);b->k0.resize(rows);b->k1.resize(rows);
      b->alpha.assign(alpha,alpha+rows);
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
        q169_put(wire.data(),j,2,quotient169::early(alpha[j],b->k0[j],b->k1[j]));
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
      io.recv_data(wire.data(),int(wire.size()));q169_check_padding(wire.data(),rows,2);
      for(int j=0;j<rows;++j)b->D[j]=q169_get(wire.data(),j,2);
    }
    io.flush();
    const uint64_t next=h->quotient169_next+1;
    h->quotient169.emplace(next,std::move(b));h->quotient169_next=next;*id=next;
    auto& s=h->quotient169_stats;++s.prepared_calls;s.prepared_rows+=rows;
    if(h->slice207)h->sliced207_radix4_rows+=rows;
    s.native_rot_rows+=uint64_t(rows)*3;s.live_banks=h->quotient169.size();
    s.peak_live_banks=std::max(s.peak_live_banks,s.live_banks);
    s.explicit_early_bytes+=wire.size();
    ++h->rot_calls;h->rot_rows+=uint64_t(rows)*3;
  });
}
extern "C" int aqcf_quotient169_consume(aqcf_silent_handle* h,uint64_t id,
 const uint8_t* digit,const uint8_t* carry,const uint8_t* mask,uint8_t* result,int rows) {
  return invoke(h,[&]{
    auto it=h->quotient169.find(id);check(it!=h->quotient169.end(),"quotient169 reused/unknown bank");
    auto& b=*it->second;const bool sending=h->party==b.sender;
    check(rows==b.rows&&digit&&carry&&result&&(sending?mask!=nullptr:mask==nullptr),"quotient169 consume shape/ownership");
    for(int j=0;j<rows;++j) {
      check(digit[j]<4&&carry[j]<2,"quotient169 consume range");
      if(sending)check(mask[j]<2&&carry[j]==b.alpha[j],"quotient169 changed prepared alpha");
    }
    auto& io=h->io[b.sender-1];
    Q169CleanVector<uint8_t> corrections(q169_bytes(rows,3)),wire(q169_bytes(rows,4));
    Q169CleanVector<uint8_t> quotients(sending?0:rows);
    if(sending) {
      io.recv_data(corrections.data(),int(corrections.size()));q169_check_padding(corrections.data(),rows,3);
      for(int j=0;j<rows;++j) {
        const auto msg=quotient169::table(digit[j],carry[j],mask[j],b.k0[j],b.k1[j],
          q169_get(corrections.data(),j,3),b.pads.data()+size_t(j)*4);
        q169_put(wire.data(),j,4,msg);
      }
      io.send_data(wire.data(),int(wire.size()));io.flush();
      for(int j=0;j<rows;++j)result[j]=mask[j];
    } else {
      for(int j=0;j<rows;++j) {
        auto c=quotient169::correct(digit[j],carry[j],b.r[j],b.k0[j],b.D[j],b.R[j]);
        q169_put(corrections.data(),j,3,c.wire);quotients[j]=c.quotient;
      }
      io.send_data(corrections.data(),int(corrections.size()));io.flush();
      io.recv_data(wire.data(),int(wire.size()));q169_check_padding(wire.data(),rows,4);
      for(int j=0;j<rows;++j)result[j]=quotient169::finish(q169_get(wire.data(),j,4),b.R[j],b.pads[j],quotients[j]);
    }
    auto& s=h->quotient169_stats;s.explicit_correction_bytes+=corrections.size();
    s.explicit_table_bytes+=wire.size();++s.consumed_calls;s.consumed_rows+=rows;
    h->quotient169.erase(it);s.live_banks=h->quotient169.size();
  });
}
extern "C" int aqcf_quotient169_read_stats(const aqcf_silent_handle* h,aqcf_quotient169_stats* s) {
  if(!h||!s)return -1;*s=h->quotient169_stats;
  if(h->poisoned)return -1;
  return s->prepared_calls==s->consumed_calls&&h->quotient169.empty()?0:-2;
}
