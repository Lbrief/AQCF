// Fresh passive hashed-ROT composition. No cached bank and no cross-call reuse.
extern "C" int aqcf_duplex334_product(aqcf_silent_handle* h,const uint8_t* a,
 const uint8_t* b,uint8_t* out,int rows,unsigned width){
 return invoke(h,[&]{
  check(rows>0&&rows<=(1<<20)&&(width==1||width==2)&&a&&b&&out,"334 public shape");
  const unsigned M=(1u<<width)-1;
  for(int i=0;i<rows;++i)check(a[i]<=M&&b[i]<2,"334 XOR input domain");
  using aqcf_control_emp::block;
  Q197CleanVector<block> keys0(rows),keys1(rows),selected(rows);
  Q197CleanBools choices(rows);
  // Each direction gets its own fresh native hashed random-OT rows, in fixed order.
  for(int sender=1;sender<=2;++sender){
   auto& ot=h->direction(sender);
   if(h->party==sender)ot.send_ot_rm_rc(keys0.data(),keys1.data(),rows);
   else ot.recv_ot_rm_rc(selected.data(),choices.p.get(),rows);
   Q197Wipe hash_state(&ot.mitccrh,sizeof(ot.mitccrh));
   ++h->rot_calls;h->rot_rows+=rows;
  }
  Q197CleanVector<uint8_t> own_wire(q197_bytes(rows,width+1));
  Q197CleanVector<uint8_t> peer_wire(q197_bytes(rows,width+1));
  for(int i=0;i<rows;++i){
   unsigned k0=uint64_t(_mm_extract_epi64(keys0[i],0))&M;
   unsigned k1=uint64_t(_mm_extract_epi64(keys1[i],0))&M;
   unsigned d=a[i]^k0^k1, e=b[i]^unsigned(choices.p[i]);
   q197_put(own_wire.data(),i,width+1,d|(e<<width));
  }
  // One role-ordered duplex exchange. No simultaneous large blocking sends.
  if(h->party==1){
   h->io[0].send_data(own_wire.data(),int(own_wire.size()));h->io[0].flush();
   h->io[1].recv_data(peer_wire.data(),int(peer_wire.size()));
  }else{
   h->io[0].recv_data(peer_wire.data(),int(peer_wire.size()));
   h->io[1].send_data(own_wire.data(),int(own_wire.size()));h->io[1].flush();
  }
  q197_check_padding(peer_wire.data(),rows,width+1);
  for(int i=0;i<rows;++i){
   unsigned peer=q197_get(peer_wire.data(),i,width+1),e=peer>>width,d=peer&M;
   unsigned sender_share=uint64_t(_mm_extract_epi64(e?keys1[i]:keys0[i],0))&M;
   unsigned receiver_share=(uint64_t(_mm_extract_epi64(selected[i],0))&M)^(b[i]?d:0);
   out[i]=(b[i]?a[i]:0)^sender_share^receiver_share;
  }
  h->io[0].flush();h->io[1].flush();
 });
}

