// Request-owned state; no cached masks, keys, rows or handles survive this object.
bool enabled191=false;
std::array<std::unique_ptr<bn191::State>,AFFINES> state191;
std::unique_ptr<late188::Phase> phase191;
std::function<std::vector<U>(unsigned,const std::vector<U>&)> complete191;
std::array<U,4> input_attempts191{};
U late_layers191=0,late_rows191=0,late_payload191=0;
bool selected191(unsigned id)const{return enabled191&&aqcf_bn19::find(id)!=nullptr;}
std::vector<U> direct_bn191(const aqcf_bn19::Geometry& g,const std::vector<U>& own){
 using namespace aqcf_bn19;
 require(ready19&&preparing18&&defer_raw_phase&&!requested[g.id]&&!state191[g.id]&&own.size()==layouts[g.id].inputs,"191 one bound prepared BN");
 requested[g.id]=true;const U n=groups(g);require(n>=4&&n<=cap-reserved_returns,"191 original full return budget");
 reserved_returns+=n;expected_inputs+=n;expected_outputs+=n;original_ranks.push_back(layouts[g.id].maps.size());ranks.push_back(n);
 if(!phase191){auto& lane=*lanes19[0];phase191=std::make_unique<late188::Phase>(lane.context->first_context_data()->parms().coeff_modulus(),4*T,lane.quarter->first_context_data()->rns_tool()->gamma().value());}
 state191[g.id]=std::make_unique<bn191::State>(g,s.party);auto& state=*state191[g.id];
 std::vector<U> out(layouts[g.id].outputs);std::vector<Bytes> frames(n);auto begin=Steady::now();
 constexpr U early_size=62*N/8;U input_payload=0;for(U group=0;group<n;++group)input_payload+=aqcf_input371::wire_size(g,group);
 if(s.party==2){
  auto& mask=masks18[g.id];require(mask.empty(),"191 fresh programmed mask");mask.resize(own.size());
  produce467(n,"BN191_CLIENT_PROGRAMMED_INPUT",[&](U group,unsigned k){auto packet=aqcf_input371::generate(*lanes19[k],g,group);input_attempts191[k]+=packet.attempts;
   auto queries=query(g,group);U begin=group*g.C*g.L;for(U j=0;j<queries.size();++j)mask[begin+j]=packet.plaintext[queries[j]]&(M-1);
   frames[group]=std::move(packet.wire);bn191::discard(packet.plaintext);++lanes19[k]->encrypted;client_input_cpus[k]=sched_getcpu();},
   [&]{number(g.id);number(n);},[&](U group){send19(frames[group],aqcf_input371::wire_size(g,group));aqcf_bn19::Bytes().swap(frames[group]);});
  metrics.encryptions+=n;client_input_max_workers=4;++client_input_parallel_layers;client_input_parallel_frames+=n;
 }else{
  require(number()==g.id&&number()==n,"191 original affine/group input binding");
  transform467(n,"BN191_SERVER_ORIGINAL_TRACE_EARLY_C1",[&](U group){frames[group]=receive19(aqcf_input371::wire_size(g,group));},[&](U group,unsigned k){frames[group]=bn191::early_server(*lanes19[k],state,group,frames[group],owners19[g.id]);linear_worker_cpus[k]=sched_getcpu();++linear_worker_outputs[k];},
   [&]{number(g.id);number(n);},[&](U group){send19(frames[group],early_size);aqcf_bn19::Bytes().swap(frames[group]);});state.early=true;
  metrics.output_payload_bytes+=n*early_size;return_bytes19+=n*(early_size+8)+16;
  metrics.input_restore+=n;metrics.fresh_zero+=n*(horner293::enabled()&&g.C>2?2:1);if(!retained307::enabled()){metrics.full_mask+=n*DEGREE;metrics.c0_add+=3*n*DEGREE;}metrics.bias_add+=n;maximum_linear_workers=4;++parallel_layers;parallel_outputs+=n;
 }
 metrics.input_payload_bytes+=input_payload;input_bytes19+=input_payload+8*n+16;metrics.input_ct+=n;metrics.output_ct+=n;++layers19;
 metrics.seconds["BN191_prepare_input_early_return_all_cost"]+=elapsed(begin);
 std::cout<<"BN191_PREPARE "<<s.party<<" "<<g.id<<" "<<n<<" "<<elapsed(begin)<<"\n"<<std::flush;return out;
}
void decode_bn191(const aqcf_bn19::Geometry& g,std::vector<U>& out,std::vector<aqcf_bn19::Bytes>& frames){
 require(ready19&&s.party==2&&state191[g.id]&&!state191[g.id]->early&&frames.size()==aqcf_bn19::groups(g)&&out.size()==layouts[g.id].outputs,"191 full original queued early return");
 auto begin=Steady::now();auto& state=*state191[g.id];
 work19(frames.size(),"BN191_CLIENT_C1_SECRET_PRODUCT",[&](U group,unsigned k){bn191::early_client(*lanes19[k],state,group,frames[group]);client_return_cpus[k]=sched_getcpu();++client_return_counts[k];aqcf_bn19::Bytes().swap(frames[group]);});
 state.early=true;metrics.decryptions+=frames.size();U payload=frames.size()*62*aqcf_bn19::N/8;metrics.output_payload_bytes+=payload;return_bytes19+=payload+8*frames.size()+16;
 client_return_max_workers=4;++client_return_layers;client_return_frames+=frames.size();metrics.seconds["BN191_client_early_product_all_cost"]+=elapsed(begin);
}
