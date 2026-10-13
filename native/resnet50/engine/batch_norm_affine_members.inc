// Included inside BfvProgram. All state belongs to this fresh request only.
std::array<std::unique_ptr<aqcf_bn19::Lane>,4> lanes19;
std::array<aqcf_bn19::PrivateOwner,AFFINES> owners19;
bool ready19=false;U layers19=0,key_bytes19=0,input_bytes19=0,return_bytes19=0;
template<class F> void work19(U n,const char* label,F f){
 require(n>=4,"BN19 four nonempty workers");std::array<std::exception_ptr,4> errors;std::vector<aqcf_owner4::Task> workers;
 for(unsigned k=0;k<4;++k)workers.emplace_back(k,[&,k]{try{cpu_set_t mask;CPU_ZERO(&mask);CPU_SET(aqcf_native4::cpu(k),&mask);require(!sched_setaffinity(0,sizeof(mask),&mask),"BN19 pinned worker");aqcf_native4::Outer audit(k,label);for(U j=k;j<n;j+=4){f(j,k);++audit.jobs;}}catch(...){errors[k]=std::current_exception();}});
 for(auto& t:workers)t.join();for(auto& e:errors)if(e)std::rethrow_exception(e);
}

void abort467() noexcept {::shutdown(s.io.io->consocket,SHUT_RDWR);::shutdown(s.io.io_rev->consocket,SHUT_RDWR);}
template<class Work,class Begin,class Send>void produce467(U n,const char* label,Work work,Begin begin,Send send){
 if(!pipeline467::enabled()){work19(n,label,work);begin();for(U j=0;j<n;++j)send(j);return;}
 begin();pipeline467::execute(n,label,0,[](U){},work,send,[&]{abort467();});
}
template<class Receive,class Work>void ingest467(U n,const char* label,Receive receive,Work work){
 if(!pipeline467::enabled()){for(U j=0;j<n;++j)receive(j);work19(n,label,work);return;}
 pipeline467::execute(n,label,1,receive,work,[](U){},[&]{abort467();});
}
template<class Receive,class Work,class Begin,class Send>void transform467(U n,const char* label,Receive receive,Work work,Begin begin,Send send){
 if(!pipeline467::enabled()){for(U j=0;j<n;++j)receive(j);work19(n,label,work);begin();for(U j=0;j<n;++j)send(j);return;}
 begin();pipeline467::execute(n,label,2,receive,work,send,[&]{abort467();});
}

void send19(const aqcf_bn19::Bytes& b,U expected){require(b.size()==expected,"BN19 fixed outgoing frame");number(b.size());output18()->send_data(b.data(),s.dim(b.size()));flush18();sent_bytes+=b.size();}
aqcf_bn19::Bytes receive19(U expected){U n=number();require(n==expected,"BN19 fixed incoming frame");aqcf_bn19::Bytes b(n);s.io.io->recv_data(b.data(),s.dim(n));received_bytes+=n;return b;}
void initialize19(){
 require(!ready19&&!preparing18&&frame_counter_started4,"BN19 new key setup strictly after first genuine input frame");
 auto t=Steady::now();work19(aqcf_bn19::geometry.size(),"BN19_ORIGINAL_OWNER_BIND",[&](U i,unsigned){const auto& g=aqcf_bn19::geometry[i];aqcf_bn19::bind_layout(layouts[g.id],g);if(s.party==1)owners19[g.id]=aqcf_bn19::bind_owner(program->at(g.id),g);});
 std::array<aqcf_bn19::Bytes,4> pk,gk;
 work19(4,"BN19_FRESH_CONTEXT_KEY_INIT",[&](U k,unsigned){lanes19[k]=std::make_unique<aqcf_bn19::Lane>(s.party==2);if(s.party==2){if(policy343::enabled()){pk[k]=std::move(lanes19[k]->key_wire343[0]);gk[k]=std::move(lanes19[k]->key_wire343[1]);require(lanes19[k]->key_wire343[0].empty()&&lanes19[k]->key_wire343[1].empty(),"343 one-use public key frames consumed");}else{pk[k]=aqcf_bn19::Lane::serialize(lanes19[k]->pk);gk[k]=aqcf_bn19::Lane::serialize(lanes19[k]->gk);}}});
 // Distinct bounded key channel: the old general SEAL 8MiB bound is unchanged.
 constexpr U max_pk=1ULL<<20,max_gk=16ULL<<20;
 for(unsigned k=0;k<4;++k){if(s.party==2){require(pk[k].size()>0&&pk[k].size()<=max_pk&&gk[k].size()>0&&gk[k].size()<=max_gk,"BN19 key size caps");number(k);send19(pk[k],pk[k].size());send19(gk[k],gk[k].size());}
  else{require(number()==k,"BN19 key lane identity");auto key_read=[&](U cap){U n=number();require(n>0&&n<=cap,"BN19 bounded key receive");aqcf_bn19::Bytes b(n);s.io.io->recv_data(b.data(),s.dim(n));received_bytes+=n;return b;};pk[k]=key_read(max_pk);gk[k]=key_read(max_gk);}
  key_bytes19+=24+pk[k].size()+gk[k].size();
 }
 if(s.party==1)work19(4,"BN19_PUBLIC_KEY_LOAD",[&](U k,unsigned){lanes19[k]->load_keys(pk[k],gk[k]);});
 if(horner293::enabled())for(auto& lane:lanes19)horner292::validate_noise(*lane);
 require(policy343::enabled()?key_bytes19<=46409752ULL-(20ULL<<20):key_bytes19==46409752ULL,"343 prospective key-wire saving");
 std::cout<<"SEED343_KEY_CENSUS "<<s.party<<" "<<policy343::enabled()<<" "<<key_bytes19<<" 4 7\n"<<std::flush;
 ready19=true;metrics.seconds["BN19_additional_online_owner_context_key_all_cost"]+=elapsed(t);
 std::cout<<"BN19_INIT "<<s.party<<" "<<key_bytes19<<" "<<elapsed(t)<<"\n"<<std::flush;
}
U return_count19(unsigned id)const{if(selected28&&selected28(id))return count28(id);auto g=aqcf_bn19::find(id);return g?aqcf_bn19::groups(*g):layouts[id].queries.size();}
U return_size19(unsigned id,U group)const{if(selected28&&selected28(id))return size28(id,group);auto g=aqcf_bn19::find(id);return g?(selected191(id)?62*aqcf_bn19::N/8:aqcf_bn19::return_size(*g,group)):joint_crt_wire::size(layouts[id].queries.at(group).size(),retained299::enabled());}
std::vector<U> direct_bn19(const aqcf_bn19::Geometry& g,const std::vector<U>& own){
 if(selected191(g.id))return direct_bn191(g,own);
 require(ready19&&preparing18&&defer_raw_phase&&!requested[g.id]&&own.size()==layouts[g.id].inputs,"BN19 exact scheduled one-use producer");requested[g.id]=true;
 const U n=aqcf_bn19::groups(g);require(n>=4&&n<=cap-reserved_returns,"BN19 full return budget");reserved_returns+=n;expected_inputs+=n;expected_outputs+=n;original_ranks.push_back(layouts[g.id].maps.size());ranks.push_back(n);
 std::vector<U> phase(layouts[g.id].outputs);std::vector<aqcf_bn19::Bytes> frames(n);auto begin=Steady::now();
 if(s.party==2){
  work19(n,"BN19_CLIENT_GATHER_ENCRYPT_SERIALIZE",[&](U group,unsigned k){frames[group]=lanes19[k]->input(g,group,own);client_input_cpus[k]=sched_getcpu();});
  number(g.id);number(n);for(auto& b:frames)send19(b,aqcf_bn19::Codec::input_size);
  metrics.encryptions+=n;client_input_max_workers=4;++client_input_parallel_layers;client_input_parallel_frames+=n;
 }else{
  require(number()==g.id&&number()==n,"BN19 input affine/group identity");for(auto& b:frames)b=receive19(aqcf_bn19::Codec::input_size);
  work19(n,"BN19_SERVER_RESTORE_TRACE_ZERO_MASK_PACK",[&](U group,unsigned k){frames[group]=lanes19[k]->produce(g,group,frames[group],own,owners19[g.id],phase);linear_worker_cpus[k]=sched_getcpu();++linear_worker_outputs[k];});
  number(g.id);number(n);for(U group=0;group<n;++group)send19(frames[group],aqcf_bn19::return_size(g,group));
  U payload=0;for(U group=0;group<n;++group)payload+=aqcf_bn19::return_size(g,group);metrics.output_payload_bytes+=payload;return_bytes19+=payload+8*n+16;
  metrics.input_restore+=n;metrics.fresh_zero+=n;metrics.full_mask+=n*DEGREE;metrics.c0_add+=3*n*DEGREE;metrics.bias_add+=n;maximum_linear_workers=4;++parallel_layers;parallel_outputs+=n;
 }
 metrics.input_payload_bytes+=n*aqcf_bn19::Codec::input_size;input_bytes19+=n*(aqcf_bn19::Codec::input_size+8)+16;
 metrics.input_ct+=n;metrics.output_ct+=n;++layers19;metrics.seconds["BN19_produce_all_cost_wall"]+=elapsed(begin);
 std::cout<<"BN19_PREPARE "<<s.party<<" "<<g.id<<" "<<n<<" "<<elapsed(begin)<<"\n"<<std::flush;return phase;
}
void decode_bn19(const aqcf_bn19::Geometry& g,std::vector<U>& out,std::vector<aqcf_bn19::Bytes>& frames){
 if(selected191(g.id)){decode_bn191(g,out,frames);return;}
 require(ready19&&s.party==2&&frames.size()==aqcf_bn19::groups(g)&&out.size()==layouts[g.id].outputs,"BN19 complete queued response");auto begin=Steady::now();U payload=0;for(U group=0;group<frames.size();++group){require(frames[group].size()==aqcf_bn19::return_size(g,group),"BN19 fixed queued response");payload+=frames[group].size();}
 work19(frames.size(),"BN19_CLIENT_DECODE_DECRYPT_LOCAL_PHASE",[&](U group,unsigned k){lanes19[k]->consume(g,group,frames[group],out);client_return_cpus[k]=sched_getcpu();++client_return_counts[k];aqcf_bn19::Bytes().swap(frames[group]);});
 metrics.decryptions+=frames.size();metrics.output_payload_bytes+=payload;return_bytes19+=payload+8*frames.size()+16;client_return_max_workers=4;++client_return_layers;client_return_frames+=frames.size();metrics.seconds["BN19_client_complete_decode_wall"]+=elapsed(begin);
}
void census19(){
 require(ready19&&layers19==18,"BN19 all original18 consumers");U enc=0,ret=0,dec=0,sw=0,zero=0,mask=0;
 for(unsigned k=0;k<4;++k){auto& l=*lanes19[k];enc+=l.encrypted;ret+=l.returned;dec+=l.decrypted;sw+=l.switches;zero+=l.zeros;mask+=l.masked_coefficients;require(s.party==1?(l.returned&&l.switches&&l.zeros):(l.encrypted&&l.decrypted),"BN19 all four useful crypto workers");}
 require(s.party==1?(enc==0&&ret==928&&dec==0&&sw==(horner293::enabled()?4480:14368)&&zero==(horner293::enabled()?1440:928)&&mask==(retained307::enabled()?0:928*DEGREE)):(enc==928&&ret==0&&dec==928&&sw==0&&zero==0&&mask==0),"BN19 exact new crypto operation census");
 std::cout<<"BN19_CENSUS {\"party\":"<<s.party<<",\"layers\":"<<layers19<<",\"encrypted\":"<<enc<<",\"returns\":"<<ret<<",\"decrypted\":"<<dec<<",\"switches\":"<<sw<<",\"fresh_zeros\":"<<zero<<",\"full_mask_coefficients\":"<<mask<<",\"additional_key_wire_bytes\":"<<key_bytes19<<",\"input_wire_bytes\":"<<input_bytes19<<",\"return_wire_bytes\":"<<return_bytes19<<"}\n"<<std::flush;
}
