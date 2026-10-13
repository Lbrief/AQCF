// Pool-only fresh radix16 banks; the qualified k2/k3 providers stay unchanged.
extern "C" int aqcf_pool263_prepare(aqcf_silent_handle* h,int sender,int rows,int banks,uint64_t* ids,uint8_t* alpha){
 return invoke(h,[&]{
  check(rows>0&&rows<=100352&&(banks==5||banks==8)&&ids&&(sender==1||sender==2),"263 public prepare bound");
  check(h->pool263.empty()&&h->pool263_next<=std::numeric_limits<uint64_t>::max()-unsigned(banks),"263 fresh bank set");
  const bool sending=h->party==sender;check(sending?alpha!=nullptr:alpha==nullptr,"263 mask ownership");
  check(h->slice207&&h->slice208,"263 qualified full-key native provider required");
  auto& ot=h->direction(sender);auto& io=h->io[sender-1];
  using aqcf_control_emp::block;
  Q197CleanVector<uint8_t> wire(q197_bytes(size_t(rows)*banks,3));
  const int group_limit=int(((1u<<20)-1)/(5*unsigned(rows)));check(group_limit>0,"263 native call capacity");
  uint64_t calls=0;
  for(int first=0;first<banks;first+=group_limit){
   const int count=std::min(group_limit,banks-first),native_rows=count*rows*5;
   check(native_rows>0&&native_rows<(1<<20),"263 unchanged native row bound");
   Q197CleanVector<block> keys0(native_rows),keys1(sending?native_rows:0);
   Q197CleanBools choices(sending?0:native_rows);
   if(sending)ot.send_ot_rm_rc(keys0.data(),keys1.data(),native_rows);
   else ot.recv_ot_rm_rc(keys0.data(),choices.p.get(),native_rows);
   Q197Wipe hash_state(&ot.mitccrh,sizeof(ot.mitccrh));++calls;
   for(int z=0;z<count;++z){
    const size_t keyoff=size_t(z)*rows*5,offset=size_t(first+z)*rows;
    auto b=std::make_unique<Pool263Bank>();b->sender=sender;b->rows=rows;
    b->k0.resize(rows);
    if(sending){
     b->pads.resize(size_t(rows)*16);b->k1.resize(rows);b->alpha.resize(rows);
     aqcf_radix16_263::sender_pads(keys0.data()+keyoff,keys1.data()+keyoff,rows,b->pads.data());
     for(int j=0;j<rows;++j){
      b->k0[j]=uint64_t(_mm_extract_epi64(keys0[keyoff+size_t(j)*5+4],0))&15;
      b->k1[j]=uint64_t(_mm_extract_epi64(keys1[keyoff+size_t(j)*5+4],0))&15;
      alpha[offset+j]=b->alpha[j]=aqcf_radix16_263::alpha(b->k0[j],b->k1[j]);
      const unsigned D=aqcf_radix16_263::early(b->alpha[j],b->k0[j],b->k1[j]);
      check((D&1)==0,"263 derived even early word");q197_put(wire.data(),offset+j,3,D>>1);
     }
    }else{
     b->pads.resize(rows);b->R.resize(rows);b->r.resize(rows);b->D.resize(rows);
     for(int j=0;j<rows;++j){unsigned R=0;for(unsigned l=0;l<4;++l)R|=unsigned(choices.p[keyoff+size_t(j)*5+l])<<l;
      b->R[j]=R;b->r[j]=choices.p[keyoff+size_t(j)*5+4];
      b->k0[j]=uint64_t(_mm_extract_epi64(keys0[keyoff+size_t(j)*5+4],0))&15;
     }
     aqcf_radix16_263::receiver_pads(keys0.data()+keyoff,b->R.data(),rows,b->pads.data());
    }
    const auto id=++h->pool263_next;ids[first+z]=id;h->pool263.emplace(id,std::move(b));
   }
  }
  if(sending){io.send_data(wire.data(),int(wire.size()));io.flush();}
  else{
   io.recv_data(wire.data(),int(wire.size()));q197_check_padding(wire.data(),size_t(rows)*banks,3);
   for(int b=0;b<banks;++b)for(int j=0;j<rows;++j)h->pool263.at(ids[b])->D[j]=q197_get(wire.data(),size_t(b)*rows+j,3)<<1;
  }
  io.flush();auto& s=h->pool263_stats;s.prepared_banks+=banks;s.prepared_rows+=uint64_t(rows)*banks;
  s.native_rot_calls+=calls;s.native_rot_rows+=uint64_t(rows)*banks*5;s.early_bytes+=wire.size();
  s.live_banks=h->pool263.size();s.peak_live_banks=std::max(s.peak_live_banks,s.live_banks);
  h->rot_calls+=calls;h->rot_rows+=uint64_t(rows)*banks*5;
 });
}
extern "C" int aqcf_pool263_consume(aqcf_silent_handle* h,uint64_t id,const uint8_t* digit,const uint8_t* carry,const uint8_t* mask,uint8_t* result,int rows){
 return invoke(h,[&]{
  auto it=h->pool263.find(id);check(it!=h->pool263.end(),"263 reused or unknown bank");auto& b=*it->second;
  const bool sending=h->party==b.sender;
  check(rows==b.rows&&digit&&carry&&result&&(sending?mask!=nullptr:mask==nullptr),"263 consume ownership");
  for(int j=0;j<rows;++j){check(digit[j]<16&&carry[j]<2,"263 digit range");if(sending)check(mask[j]<2&&carry[j]==b.alpha[j],"263 unchanged predecessor mask");}
  auto& io=h->io[b.sender-1];Q197CleanVector<uint8_t> corrections(q197_bytes(rows,5)),wire(q197_bytes(rows,16)),quotients(sending?0:rows);
  if(sending){
   io.recv_data(corrections.data(),int(corrections.size()));q197_check_padding(corrections.data(),rows,5);
   for(int j=0;j<rows;++j)q197_put(wire.data(),j,16,aqcf_radix16_263::table(digit[j],carry[j],mask[j],b.k0[j],b.k1[j],q197_get(corrections.data(),j,5),b.pads.data()+size_t(j)*16));
   io.send_data(wire.data(),int(wire.size()));io.flush();std::copy_n(mask,rows,result);
  }else{
   for(int j=0;j<rows;++j){auto c=aqcf_radix16_263::correct(digit[j],carry[j],b.r[j],b.k0[j],b.D[j],b.R[j]);q197_put(corrections.data(),j,5,c.wire);quotients[j]=c.quotient;}
   io.send_data(corrections.data(),int(corrections.size()));io.flush();io.recv_data(wire.data(),int(wire.size()));q197_check_padding(wire.data(),rows,16);
   for(int j=0;j<rows;++j)result[j]=aqcf_radix16_263::finish(q197_get(wire.data(),j,16),b.R[j],b.pads[j],quotients[j]);
  }
  auto& s=h->pool263_stats;++s.consumed_banks;s.consumed_rows+=rows;s.correction_bytes+=corrections.size();s.table_bytes+=wire.size();
  h->pool263.erase(it);s.live_banks=h->pool263.size();
 });
}
extern "C" int aqcf_pool263_read_stats(const aqcf_silent_handle* h,aqcf_pool263_stats* s){
 if(!h||!s||h->poisoned)return -1;*s=h->pool263_stats;
 return h->pool263.empty()&&s->prepared_banks==s->consumed_banks&&s->prepared_rows==s->consumed_rows?0:-2;
}
