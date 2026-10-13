// Preserve original chosen-choice native RM generation and 8-row wire framing.
// Call only on eligible short words, never on raw correlated keys.
namespace {
void lookup208_send(NativeOT& ot,const uint64_t* table,int rows,unsigned bits,unsigned width){
 using aqcf_control_emp::block;using aqcf_control_emp::makeBlock;
 check(aqcf_word208::eligible(bits,width),"208 eligible sender shape");const unsigned B=1u<<bits;
 Q169CleanVector<block> a(size_t(rows)*bits),b(size_t(rows)*bits);
 ot.send_ot_rm_cc(a.data(),b.data(),size_t(rows)*bits);
 Q169CleanVector<block> pad(8*B);Q169CleanVector<uint64_t> wire((8*B*width+63)/64);
 uint64_t* pointers[8];
 for(int begin=0;begin<rows;begin+=8){const unsigned count=std::min(8,rows-begin),words=(count*B*width+63)/64;
  std::fill(pad.begin(),pad.end(),makeBlock(0,0));
  for(unsigned j=0;j<count;++j){pointers[j]=const_cast<uint64_t*>(table+size_t(begin+j)*B);
   for(unsigned v=0;v<B;++v)pad[j*B+v]=makeBlock(0,aqcf_word208::sender_pad(a.data()+size_t(begin+j)*bits,b.data()+size_t(begin+j)*bits,bits,width,v));}
  aqcf_control_sci::pack_ot_messages<uint64_t>(wire.data(),pointers,pad.data(),words,count,width,B);
  ot.ferret->io->send_data(wire.data(),words*sizeof(uint64_t));
 }
}
void lookup208_recv(NativeOT& ot,uint64_t* result,const uint8_t* choice,int rows,unsigned bits,unsigned width){
 using aqcf_control_emp::block;using aqcf_control_emp::makeBlock;
 check(aqcf_word208::eligible(bits,width),"208 eligible receiver shape");const unsigned B=1u<<bits;
 Q169CleanVector<block> keys(size_t(rows)*bits);Q169CleanBools selections(size_t(rows)*bits);
 for(int j=0;j<rows;++j)for(unsigned l=0;l<bits;++l)selections.p[size_t(j)*bits+l]=(choice[j]>>l)&1;
 ot.recv_ot_rm_cc(keys.data(),selections.p.get(),size_t(rows)*bits);
 Q169CleanVector<block> pad(8);Q169CleanVector<uint64_t> wire((8*B*width+63)/64);
 for(int begin=0;begin<rows;begin+=8){const unsigned count=std::min(8,rows-begin),words=(count*B*width+63)/64;
  ot.ferret->io->recv_data(wire.data(),words*sizeof(uint64_t));std::fill(pad.begin(),pad.end(),makeBlock(0,0));
  for(unsigned j=0;j<count;++j)pad[j]=makeBlock(0,aqcf_word208::receiver_pad(keys.data()+size_t(begin+j)*bits,bits,width,choice[begin+j]));
  aqcf_control_sci::unpack_ot_messages<uint64_t>(result+begin,choice+begin,wire.data(),pad.data(),count,width,B);
 }
}
}
