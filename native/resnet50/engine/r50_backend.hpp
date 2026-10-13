#include "resnet50/engine/floor_before_pool.hpp"
#include "resnet50/engine/directed_carry_consumer.hpp"
#include "resnet50/engine/joint_quotient_adapter.hpp"
#include "resnet50/engine/prefix_lut_adapter.hpp"
#include "resnet50/engine/native_radix_adapter.hpp"
#include "resnet50/engine/native_seed_adapter.hpp"
#include "resnet50/engine/quotient_phase_relu.hpp"
#include "resnet50/engine/quotient_residual_phase.hpp"
#include "resnet50/engine/protected_residual.hpp"
#include "resnet50/engine/ring37_direct_floor.hpp"
#include "resnet50/engine/quotient_pool38.hpp"
#include "resnet50/engine/pool_frontier.hpp"
#include "resnet50/engine/pool_comparison_policy.hpp"
#include "resnet50/engine/pool_floor_policy.hpp"
#include "resnet50/engine/phase_relu.hpp"
#include "resnet50/engine/phase_plan.hpp"
#include "resnet50/engine/residual_phase.hpp"
#include "resnet50/engine/residual_plan.hpp"
#pragma once
#include "resnet50/engine/r50_executor.hpp"
#include "resnet50/engine/parallel_tensor.hpp"
#include "resnet50/engine/average_pool_reciprocal.hpp"
class R50Backend {
public:
    using U=aqcf3::U;using V=std::vector<U>;
    bool enabled444=false,pool_phase_pending=false,pool_quotient_pending=false;
    std::array<U,4> pool444_reverse{};
    U pool_phase_calls=0,pool_phase_rows=0,pool26_lift_elements=0,pool26_max_elements=0,pool_restore_calls=0;
    bool quotient169=false;int pending191=-1;
    std::array<std::map<std::size_t,std::unique_ptr<compose189::Bank>>,4> banks191;
    std::array<U,4> joint_attempts191{},joint_rejects191{};
    U calls191=0,rows191=0;

    aqcf3::BfvProgram& he;aqcf2::WorkerLanes& lanes;std::ofstream nodes;
    U widened_elements=0,completed_nodes=0,affines=0;
    U avg29_calls=0,avg29_rows=0;
    U pool38_lift_elements=0,pool38_max_elements=0;
    bool phase_pending=false;int phase_producer=-1,phase_tensor=-1,phase_consumer=-1;
    std::size_t phase_size=0;U phase_calls=0,phase_rows=0;
    const aqcf_r50_residual_phase::BoundChain* residual_pending=nullptr;
    bool residual_added=false;V residual_other;
    U residual_calls=0,residual_rows=0,residual_add_calls=0;
    bool enabled216=false;U calls216=0,rows216=0;
    bool enabled221=false,enabled233=false;
    bool enabled255=aqcf_pool258::enabled();U pool255_maximum_calls=0;
    U layer233_calls=0,layer233_rows=0,pool230_calls=0;
    template<class F> V frontier233(const V& input,F fn){
      if(!enabled233)return lanes.run_frontier(input,fn);
      ++layer233_calls;layer233_rows+=input.size();
      return lanes.run_layer229(input,fn);
    }
    R50Backend(aqcf3::BfvProgram& h,aqcf2::WorkerLanes& l,const std::filesystem::path& p):he(h),lanes(l),nodes(p){aqcf3::require(bool(nodes),"node log");nodes<<std::setprecision(17);const char* flag=std::getenv("AQCF_QUOTIENT169");
      aqcf3::require(flag&&(std::string(flag)=="0"||std::string(flag)=="1"),"trusted quotient169 flag required");
      quotient169=std::string(flag)=="1";
      const char* pf=std::getenv("AQCF_POOLFLOOR444");aqcf3::require(pf&&(std::string(pf)=="0"||std::string(pf)=="1"),"444 explicit public bound schedule");enabled444=pf[0]=='1';
      const auto& g=aqcf_r50::graph;
      aqcf3::require(g.at(1).index==1&&g.at(1).affine==0&&g.at(1).out==256&&g.at(1).kind=="Conv2DWrapper"&&g.at(1).shape==std::vector<int>({1,112,112,64}),"444 original full affine0 producer");
      aqcf3::require(g.at(2).index==2&&g.at(2).kind=="MaxPool"&&g.at(2).out==259&&g.at(2).args==std::vector<int>({1,56,56,64,3,3,0,1,0,1,2,2,1,112,112,64,256,259}),"444 exact maxpool padding and value-only consumer");
      aqcf3::require(g.at(3).kind=="ScaleDown4"&&g.at(3).args==std::vector<int>({1,56,56,64,259,12})&&g.at(3).out==259&&g.at(4).kind=="FusedBatchNorm4411"&&g.at(4).args.at(4)==259,"444 unique immediately following floor before BN");
      std::cout<<"POOL444_CONFIGURATION "<<he.s.party<<" "<<enabled444<<"\n"<<std::flush;
      const char* floor_flag=std::getenv("AQCF_FLOOR216");aqcf3::require(floor_flag&&(std::string(floor_flag)=="0"||std::string(floor_flag)=="1"),"216 explicit trusted terminal flag");
      const char* seed_flag=std::getenv("AQCF_SEED221");aqcf3::require(seed_flag&&(std::string(seed_flag)=="0"||std::string(seed_flag)=="1"),"221 explicit trusted prefix flag");enabled221=seed_flag[0]=='1';
      aqcf3::require(floor_flag[0]=='1',"222 retains qualified216 in both arms");
      std::cout<<"SEED221_CONFIGURATION "<<he.s.party<<" "<<enabled221<<"\n"<<std::flush;
      const char* layer_flag=std::getenv("AQCF_FRONTIER233");aqcf3::require(layer_flag&&(std::string(layer_flag)=="0"||std::string(layer_flag)=="1"),"233 explicit public schedule flag");enabled233=layer_flag[0]=='1';
      aqcf3::require(enabled221,"233 strongest222 prefix in both arms");
      std::cout<<"FRONTIER233_CONFIGURATION "<<he.s.party<<" "<<enabled233<<"\n"<<std::flush;
      enabled216=floor_flag[0]=='1';aqcf3::require(!enabled216||(quotient169&&!aqcf2::radix197_enabled()),"216 qualified original169 caller only");
      std::cout<<"FLOOR216_CONFIGURATION "<<he.s.party<<" "<<enabled216<<"\n"<<std::flush;
      const char* compose=std::getenv("AQCF_BN191");aqcf3::require(compose&&(std::string(compose)=="0"||std::string(compose)=="1"),"trusted BN191 port flag");
      he.enabled191=std::string(compose)=="1";aqcf3::require(!he.enabled191||quotient169,"191 same qualified prefix");
      aqcf3::require(!aqcf2::radix197_enabled()||(he.enabled191&&quotient169),"197 qualified191 caller only");
      std::cout<<"RADIX197_CONFIGURATION "<<he.s.party<<" "<<aqcf2::radix197_enabled()<<"\n"<<std::flush;
      std::cout<<"BN191_CONFIGURATION "<<he.s.party<<" "<<he.enabled191<<"\n"<<std::flush;
      he.complete191=[this](unsigned id,const V& correction){return finish191(id,correction);};

      std::cout<<"QUOTIENT169_CONFIGURATION "<<he.s.party<<" "<<quotient169<<"\n"<<std::flush;}
    V finish191(unsigned id,const V& correction){
      namespace b=aqcf_bn19;namespace q=compose197;namespace l=late188;
      aqcf3::require(he.selected191(id)&&he.state191[id]&&he.phase191&&pending191<0&&!phase_pending,"191 fresh sole bound caller");
      auto& state=*he.state191[id];const auto& g=state.g;const auto& phase=*he.phase191;const U n=state.rows();
      aqcf3::require(state.early&&!state.late&&correction.size()==n,"191 actual delta precedes programming");
      for(auto& bank:banks191)aqcf3::require(bank.empty(),"191 no surviving previous bank");
      std::array<U,4> payload{},wire{};
      auto actual=frontier233(correction,[&](aqcf2::Sci& sc,const V& corr,std::size_t offset){
        const int lane=aqcf_owner4::current_lane;aqcf3::require(lane>=0&&lane<4,"191 actual native owner identity");
        trace037::event("OWNER_CRYPTO_ENTER",-1,lane);
        if(retained307::enabled()){
          V part(corr.size());
          if(sc.party==1){aqcf3::require(state.phase306.size()==n,"307 full retained server phases");for(U j=0;j<corr.size();++j)part[j]=(state.phase306[offset+j]-4*corr[j])&l::phase_mask;}
          else for(U j=0;j<corr.size();++j)part[j]=b::local_phase(phase.scalar(state.product.data(),offset+j,n),g);
          trace037::event("OWNER_CRYPTO_EXIT",-1,lane,nullptr,corr.size());return part;
        }
        auto bank=std::make_unique<q::Bank>(sc.party,sc.control().owned189_handle(),corr.size());
        {pcn_parallel::OuterGuard original_owner_scope;bank->prepare(*sc.io.io,phase,corr,b::log_channels(g));}
        V part(corr.size());b::Bytes frame;const U expected=(49*corr.size()+7)/8;
        if(sc.party==1){b::Bits::Writer writer;for(U j=0;j<corr.size();++j){b::W value=state.c0[offset+j]+bank->rows[j].R;if(value>=phase.Q)value-=phase.Q;writer.put(U((value+(b::W(1)<<64))>>65),49);part[j]=(b::local_phase(bank->rows[j].y,g)-4*corr[j])&l::phase_mask;}frame=writer.finish();
          aqcf3::require(frame.size()==expected,"191 fixed late c0 frame");sc.io.io->send_data(&expected,8);sc.io.io->send_data(frame.data(),sc.dim(frame.size()));sc.io.io->flush();
        }else{U length=0;sc.io.io->recv_data(&length,8);aqcf3::require(length==expected,"191 fixed late c0 receive");frame.resize(length);sc.io.io->recv_data(frame.data(),sc.dim(length));b::Bits::Reader reader{frame};std::array<U,3> residues{};
          for(U j=0;j<corr.size();++j){b::W value=(b::W(reader.get(49))<<65)%phase.Q;for(U z=0;z<3;++z){U r=state.product[z*n+offset+j]+U(value%phase.qs[z]);residues[z]=r>=phase.qs[z]?r-phase.qs[z]:r;}part[j]=b::local_phase(phase.scalar(residues.data(),0,1),g);}reader.end();}
        payload[lane]+=expected;wire[lane]+=expected+8;joint_attempts191[lane]+=bank->attempted;joint_rejects191[lane]+=bank->rejected;
        aqcf3::require(banks191[lane].emplace(offset,std::move(bank)).second,"191 unique lane and row bank");
        trace037::event("OWNER_CRYPTO_EXIT",-1,lane,nullptr,corr.size());return part;
      });
      U bytes=0,framed=0;for(unsigned k=0;k<4;++k){bytes+=payload[k];framed+=wire[k];}
      he.metrics.output_payload_bytes+=bytes;he.return_bytes19+=framed;he.late_payload191+=bytes;
      if(he.s.party==1)he.sent_bytes+=framed;else he.received_bytes+=framed;
      state.late=true;++he.late_layers191;he.late_rows191+=n;pending191=int(id);
      bn191::discard(state.c0);bn191::discard(state.product);bn191::discard(state.phase306);return actual;
    }
    V consume191(const V& x){
      aqcf3::require(pending191>=0&&he.state191[pending191]&&he.state191[pending191]->late&&he.state191[pending191]->rows()==x.size(),"191 exact immediate nonlinear consumer");
      auto output=frontier233(x,[&](aqcf2::Sci& sc,const V& part,std::size_t offset){
        const int lane=aqcf_owner4::current_lane;aqcf3::require(lane>=0&&lane<4,"191 actual consumer owner");
        if(retained307::enabled()){
          trace037::event("OWNER_CRYPTO_ENTER",-1,lane);
          auto output=sc.boundary_profile("bn307_retained_joint304_floor_relu",[&]{return direction359::consume(sc,part,aqcf_owner4::current_lane,nullptr,true);}).v;
          trace037::event("OWNER_CRYPTO_EXIT",-1,lane,nullptr,part.size());return output;
        }
        auto found=banks191[lane].find(offset);aqcf3::require(found!=banks191[lane].end()&&found->second->n==part.size(),"191 exact bank interval");
        trace037::event("OWNER_CRYPTO_ENTER",-1,lane);
        auto output=sc.boundary_profile("bn191_complete_programmed_floor_relu",[&]{return enabled221?compose221::nonlinear(sc,part,*found->second):compose197::nonlinear(sc,part,*found->second);}).v;
        aqcf3::require(found->second->spent,"191 bank fully consumed");banks191[lane].erase(found);
        trace037::event("OWNER_CRYPTO_EXIT",-1,lane,nullptr,part.size());return output;
      });
      for(auto& bank:banks191)aqcf3::require(bank.empty(),"191 all bound banks erased");
      he.state191[pending191].reset();pending191=-1;++calls191;rows191+=x.size();return output;
    }
    // Local disjoint-element execution only; no HE/MPC/OT changes.
    template<class Work>void tensor_work(const char* phase,std::size_t n,Work work){
        aqcf_tensor4::run(he.s.party,phase,n,work);
    }
    V copy_tensor(const V& x,const char* phase){V out(x.size());
        tensor_work(phase,x.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)out[j]=x[j];});return out;
    }
    void mask37_tensor(V& x,const char* phase){
        tensor_work(phase,x.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)x[j]&=aqcf2::mask(37);});
    }
    struct Marker{aqcf3::Steady::time_point t;U bytes,inputs,outputs,pm;};
    Marker begin_node(const aqcf_r50::Node& n){
        aqcf3::require(!pool_phase_pending||(n.index==2&&n.kind=="MaxPool"),"444 raw phase only at node2");
        aqcf3::require(!pool_quotient_pending||(n.index==3&&n.kind=="ScaleDown4"),"444 quotient only at node3");
        if(residual_pending){const auto&q=*residual_pending;
            const bool local=std::find(q.intervening.begin(),q.intervening.end(),n.index)!=q.intervening.end();
            aqcf3::require(!phase_pending&&((local&&!residual_added&&n.kind=="ScaleUp4")||
              (n.index==q.add&&!residual_added&&n.kind=="MatAdd4")||
              (n.index==q.consumer&&residual_added&&n.kind=="ScaleDown4")),"residual raw phase lifetime/order");
        }
        aqcf3::require(!phase_pending||(n.index==phase_consumer&&n.kind=="Relu4"),"raw phase only reaches bound next consumer");lanes.flush();return{aqcf3::Steady::now(),lanes.bytes(),he.metrics.input_ct,he.metrics.output_ct,he.metrics.pm};}
    void end_node(const aqcf_r50::Node& n,Marker m,const V& y){
        ++completed_nodes;if(n.affine>=0)++affines;lanes.flush();
        nodes<<"{\"node\":"<<n.index<<",\"kind\":\""<<n.kind<<"\",\"affine\":"<<n.affine<<",\"elements\":"<<y.size()
             <<",\"seconds\":"<<aqcf3::elapsed(m.t)<<",\"local_io_counter_delta\":"<<lanes.bytes()-m.bytes
             <<",\"input_CT\":"<<he.metrics.input_ct-m.inputs<<",\"output_CT\":"<<he.metrics.output_ct-m.outputs
             <<",\"PM\":"<<he.metrics.pm-m.pm<<"}\n"<<std::flush;
        aqcf3::require(bool(nodes),"node write");std::cout<<"NODE "<<n.index<<" "<<n.kind<<'\n'<<std::flush;
    }
    V affine(int id,const V& x,std::size_t n,int producer,int tensor,int input_tensor){
        aqcf3::require(!phase_pending&&!residual_pending&&!he.defer_raw_phase&&!pool_phase_pending&&!pool_quotient_pending,"previous phase consumed exactly once");
        const aqcf_r50_phase_relu::BoundConsumer* selected=nullptr;
        for(const auto& q:aqcf_r50_phase_relu::plan)if(q.producer==producer){aqcf3::require(!selected&&q.affine==id&&q.tensor==tensor&&q.elements==n,"source-bound unique producer");selected=&q;}
        const aqcf_r50_residual_phase::BoundChain* chain=nullptr;
        for(const auto& q:aqcf_r50_residual_phase::plan)if(q.producer==producer){
            aqcf3::require(!chain&&!selected&&q.affine==id&&q.phase_tensor==tensor&&q.elements==n,"unique residual producer");chain=&q;}
        V projected(x.size());tensor_work("R50_AFFINE_INPUT_PROJECT",x.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)projected[j]=x[j]&aqcf2::mask(37);});
        const bool selected444=enabled444&&id==0;
        if(selected444)aqcf3::require(producer==1&&tensor==256&&input_tensor==253&&n==802816&&!selected&&!chain,"444 unique original first producer");
        he.defer_raw_phase=selected!=nullptr||chain!=nullptr||selected444;auto y=he.affine(id,projected,producer,input_tensor,tensor);he.defer_raw_phase=false;
        aqcf3::require(y.size()==n,"all affine outputs");
        if(selected444){
            tensor_work("R50_POOL_PHASE39",n,[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)aqcf3::require(y[j]<(U(1)<<39),"444 raw full phase domain");});
            pool_phase_pending=true;
        }else if(selected){
            tensor_work("R50_PHASE39_CANONICAL",n,[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)aqcf3::require(y[j]<(U(1)<<39),"raw phase no37projection");});
            phase_pending=true;phase_producer=producer;phase_tensor=tensor;phase_consumer=selected->consumer;phase_size=n;
        }else if(chain){
            tensor_work("R50_RESIDUAL_PHASE39",n,[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)aqcf3::require(y[j]<(U(1)<<39),"raw residual phase39");});
            residual_pending=chain;residual_added=false;aqcf3::require(residual_other.empty(),"no stale residual addend");
        }else mask37_tensor(y,"R50_AFFINE_OUTPUT_PROJECT");
        return y;
    }
    V residual_add(const V& a,const V& b,int node,int aid,int bid,int output){
        aqcf3::require(a.size()==b.size(),"complete residual shapes");
        if(residual_pending){const auto&q=*residual_pending;
            aqcf3::require(!residual_added&&q.add==node&&q.sum_tensor==output&&a.size()==q.elements,"bound residual add");
            const bool left=aid==q.phase_tensor;
            aqcf3::require((left?bid:aid)==q.other_tensor&&(left?aid:bid)==q.phase_tensor,"exact ordinary versus raw input identity");
            const V& phase=left?a:b;const V& other=left?b:a;
            residual_other=copy_tensor(other,"R50_RESIDUAL_ORDINARY_CAPTURE");
            tensor_work("R50_RESIDUAL_ORDINARY_CHECK",other.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)aqcf3::require(other[j]<=aqcf2::mask(37),"no second raw phase");});
            auto out=copy_tensor(phase,"R50_RESIDUAL_PHASE_TRANSFER");residual_added=true;++residual_add_calls;
            return out; // opaque bound raw phase; exact addition charged at its next floor consumer
        }
        V out(a.size());tensor_work("R50_RESIDUAL_ADD",out.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)out[j]=(a[j]+b[j])&aqcf2::mask(37);});return out;
    }
    V boundary(const V& x,unsigned f,bool relu,bool just_lift=false,int consumer=-1,int tensor=-1){
        if(pool_quotient_pending){
            aqcf3::require(enabled444&&!pool_phase_pending&&!phase_pending&&!residual_pending&&f==12&&!relu&&!just_lift&&consumer==3&&tensor==259&&x.size()==200704,"444 sole guarded floor-output restoration");
            auto y=lanes.run_frontier(x,[&](aqcf2::Sci&s,const V&v,std::size_t){
                trace037::event("OWNER_CRYPTO_ENTER",-1,aqcf_owner4::current_lane);
                auto z=s.boundary_profile("pool431_guarded26_to37",[&]{return poolfloor431::guarded_lift37(s,v);}).v;
                trace037::event("OWNER_CRYPTO_EXIT",-1,aqcf_owner4::current_lane,nullptr,v.size());return z;});
            pool_quotient_pending=false;++pool_restore_calls;return y;
        }
        if(residual_pending){const auto&q=*residual_pending;
            aqcf3::require(!phase_pending&&residual_added&&f==12&&!relu&&!just_lift&&consumer==q.consumer&&tensor==q.sum_tensor&&x.size()==q.elements&&residual_other.size()==q.elements,"sole bound residual floor");
            auto y=frontier233(x,[&](aqcf2::Sci&s,const V&v,std::size_t offset){
                V add(residual_other.begin()+offset,residual_other.begin()+offset+v.size());
                trace037::event("OWNER_CRYPTO_ENTER",-1,aqcf_owner4::current_lane);auto result=s.boundary_profile("raw39_residual_floor37_fused",[&]{if(!quotient169)return aqcf_r50_residual_phase::consume(s,v,add);if(enabled221)return aqcf_terminal304::enabled()?direction359::consume(s,v,aqcf_owner4::current_lane,&add):aqcf_mixed238::enabled()?aqcf_mixed237::consume(s,v,&add):aqcf_seed221::consume(s,v,&add);if(enabled216)return aqcf_r50_residual_phase216::consume(s,v,add);return aqcf_r50_residual_phase169::consume(s,v,add);}).v;trace037::event("OWNER_CRYPTO_EXIT",-1,aqcf_owner4::current_lane,nullptr,v.size());return result;});
            ++residual_calls;residual_rows+=x.size();if(enabled216){++calls216;rows216+=x.size();}
            std::cout<<"R50_RESIDUAL_PHASE_CONSUMER "<<q.producer<<" "<<q.add<<" "<<q.consumer<<" "<<q.phase_tensor<<" "<<q.other_tensor<<" "<<q.sum_tensor<<" "<<x.size()<<"\n"<<std::flush;
            tensor_work("R50_RESIDUAL_CAPTURE_CLEAR",residual_other.size(),[&](std::size_t lo,std::size_t hi){OPENSSL_cleanse(residual_other.data()+lo,(hi-lo)*sizeof(U));});
            residual_other.clear();residual_added=false;residual_pending=nullptr;return y;
        }
        if(phase_pending){
            aqcf3::require(f==12&&relu&&!just_lift&&consumer==phase_consumer&&tensor==phase_tensor&&x.size()==phase_size,"sole exact consumer identity/lifetime");
            auto y=pending191>=0?consume191(x):frontier233(x,[&](aqcf2::Sci&s,const V&v,std::size_t){trace037::event("OWNER_CRYPTO_ENTER",-1,aqcf_owner4::current_lane);auto result=s.boundary_profile("raw39_to_floor12_relu37_fused",[&]{if(!quotient169)return aqcf_r50_phase_relu::consume(s,v);if(enabled221)return aqcf_terminal304::enabled()?direction359::consume(s,v,aqcf_owner4::current_lane):aqcf_elide250::enabled()?aqcf_mixed249::consume(s,v):(aqcf_mixed238::enabled()?aqcf_mixed237::consume(s,v):aqcf_seed221::consume(s,v));return aqcf_r50_phase_relu169::consume(s,v);}).v;trace037::event("OWNER_CRYPTO_EXIT",-1,aqcf_owner4::current_lane,nullptr,v.size());return result;});
            ++phase_calls;phase_rows+=x.size();
            std::cout<<"R50_PHASE_CONSUMER "<<phase_producer<<" "<<phase_tensor<<" "<<phase_consumer<<" "<<x.size()<<"\n"<<std::flush;
            phase_pending=false;phase_producer=phase_tensor=phase_consumer=-1;phase_size=0;return y;
        }
        widened_elements+=x.size();auto execute=[&](aqcf2::Sci&s,const V&a,std::size_t){
            aqcf2::Ring z(55);z.v=a;
            if(relu&&!just_lift)return s.boundary_profile("joint_direct_ring37_floor_relu",[&]{return aqcf_cifar_comm::floor_relu37(s,z,f);}).v;
            if(!relu&&!just_lift&&f==12)return s.boundary_profile("joint_direct_ring37_floor",[&]{return aqcf_cifar_comm::floor37_direct(s,z,f);}).v;
            auto full=s.boundary_profile("centered_ring37_to55",[&]{return aqcf_rhombus::centered_lift(s,z,37);});
            return just_lift?full.v:aqcf2::backbone_floor(s,full,f,relu,aqcf2::BackboneCarry::Linked,true).v;
        };
        auto result=(f==12&&!just_lift)?frontier233(x,execute):lanes.run(x,execute);
        if(!just_lift)mask37_tensor(result,"R50_BOUNDARY_PROJECT");return result;
    }
    V pool_lift(const V& x){
        if(pool_phase_pending){
            aqcf3::require(enabled444&&enabled233&&enabled255&&!pool_quotient_pending&&!phase_pending&&!residual_pending&&x.size()==802816,"444 exact original full raw pool frontier");
            auto y=frontier233(x,[&](aqcf2::Sci&s,const V&v,std::size_t){
                trace037::event("OWNER_CRYPTO_ENTER",-1,aqcf_owner4::current_lane);
                auto z=s.boundary_profile("pool431_raw39_floor26",[&]{const int lane=aqcf_owner4::current_lane;aqcf3::require(lane>=0&&lane<4,"444 actual original owner identity");
                const bool reverse=direction359::enabled()&&(lane%2);direction359::View view(s,reverse);pool444_reverse[lane]=reverse;
                return poolfloor431::raw_floor26(view,v);}).v;
                trace037::event("OWNER_CRYPTO_EXIT",-1,aqcf_owner4::current_lane,nullptr,v.size());return z;});
            pool_phase_pending=false;pool_quotient_pending=true;++pool_phase_calls;pool_phase_rows+=x.size();pool26_lift_elements+=x.size();++pool230_calls;return y;
        }
#if AQCF_POOL38
        pool38_lift_elements+=x.size();
        auto fn=[&](aqcf2::Sci&s,const V&a,std::size_t){aqcf2::Ring z(55);z.v=a;return s.boundary_profile("pool_quotient_lift37_to38",[&]{return aqcf_pool38::lift(s,z);}).v;};
        ++pool230_calls;return enabled233?lanes.run_frontier(x,fn):lanes.run(x,fn);
#else
        return boundary(x,0,false,true);
#endif
    }
    V pool_maximum(const V& a,const V& b){
        if(pool_quotient_pending){
            aqcf3::require(enabled444&&enabled255&&enabled233&&a.size()==b.size(),"444 exact pool comparison frontier");
            ++pool255_maximum_calls;pool26_max_elements+=a.size();++layer233_calls;layer233_rows+=a.size();
            return lanes.run_impl(a,[&](aqcf2::Sci&s,const V&v,std::size_t offset){aqcf2::Ring x(26),y(26,v.size());x.v=v;std::copy_n(b.begin()+offset,v.size(),y.v.begin());return s.boundary_profile("pool431_exact25_in26",[&]{return poolfloor431::maximum(s,x,y);}).v;},100352);
        }
#if AQCF_POOL38
        aqcf3::require(a.size()==b.size(),"same full pool shapes");pool38_max_elements+=a.size();++pool255_maximum_calls;
        auto fn=[&](aqcf2::Sci&s,const V&v,std::size_t offset){aqcf2::Ring x(38),y(38,v.size());x.v=v;std::copy_n(b.begin()+offset,v.size(),y.v.begin());return s.boundary_profile("pool_exact_difference38_frontier",[&]{return pool295::enabled()?aqcf_pool263::maximum(s,x,y):aqcf_pool_frontier::maximum(s,x,y);}).v;};
        if(enabled255){aqcf3::require(enabled233,"258 qualified layer scheduler");++layer233_calls;layer233_rows+=a.size();return lanes.run_impl(a,fn,100352);}
        return frontier233(a,fn);
#else
        return maximum55(a,b);
#endif
    }
    // Both inputs are already exact signed37 values shared in Ring55.
    V maximum55(const V&a,const V&b){
        aqcf3::require(a.size()==b.size(),"max pair shape");V d(a.size());
        tensor_work("R50_MAX_DIFFERENCE",d.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)d[j]=(b[j]-a[j])&aqcf2::mask(55);});
        auto pos=lanes.run(d,[&](aqcf2::Sci&s,const V&v,std::size_t){aqcf2::Ring z(55);z.v=v;return aqcf2::backbone_relu(s,z,aqcf2::BackboneCarry::Linked).v;});
        tensor_work("R50_MAX_ADD",pos.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)pos[j]=(pos[j]+a[j])&aqcf2::mask(55);});return pos;
    }
    // Standard secure restoring division, NOT claimed as an original algorithm.
    // x in signed Ring37. Offset makes n=x+2^40 nonnegative and below 2^37.
    // Each public comparison uses signed Ring55, hence no signed overflow.
    V divide49(const V& input){
        auto lifted=boundary(input,0,false,true);
        auto result=lanes.run(lifted,[&](aqcf2::Sci&s,const V&a,std::size_t){
            aqcf2::Ring rem(55);rem.v=a;aqcf2::Ring quotient(55,a.size());
            if(s.party==1)for(auto&v:rem.v)v=(v+(U(1)<<36))&aqcf2::mask(55);
            for(int k=31;k>=0;--k){
                U d=U(49)<<k;aqcf2::Ring diff=rem;
                if(s.party==1)for(auto&v:diff.v)v=(v-d)&aqcf2::mask(55);
                auto ge=s.not_(s.msb(diff));auto bit=s.b2a(ge,55);
                for(std::size_t j=0;j<a.size();++j){rem.v[j]=(rem.v[j]-bit.v[j]*d)&aqcf2::mask(55);quotient.v[j]=(quotient.v[j]+(bit.v[j]<<k))&aqcf2::mask(55);}
            }
            constexpr U offset=U(1)<<36;auto diff=rem;
            if(s.party==1)for(auto&v:diff.v)v=(v-offset%49)&aqcf2::mask(55);
            auto correction=s.b2a(s.msb(diff),55);
            for(std::size_t j=0;j<a.size();++j)quotient.v[j]=(quotient.v[j]-correction.v[j]-(s.party==1?offset/49:0))&aqcf2::mask(37);
            return quotient.v;
        });return result;
    }
    V divide49_certified173(const V& input){
        aqcf3::require(input.size()==2048&&avg29_calls==0,"one certified node174 only");
        auto result=lanes.run(input,[&](aqcf2::Sci& s,const V& v,std::size_t){
            aqcf2::Ring x(37);x.v=v;return aqcf_avg29::divide49_bounded(s,x).v;
        });
        ++avg29_calls;avg29_rows+=input.size();
        std::cout<<"AVG29_CENSUS "<<he.s.party<<" "<<avg29_calls<<" "<<avg29_rows<<" 173 174 765 767 49 21913098 30"<<std::endl;
        return result;
    }
    V argmax(const V& input){
        auto value=boundary(input,0,false,true);V index(value.size(),0);
        if(he.s.party==1)tensor_work("R50_CLASS_INDEX",index.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)index[j]=j;});
        // Stable left winner on equality, all 1001 classes participate.
        while(value.size()>4){std::size_t pairs=value.size()/2;V diff(pairs);
            tensor_work("R50_CLASS_DIFFERENCE",pairs,[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)diff[j]=(value[2*j]-value[2*j+1])&aqcf2::mask(55);});
            auto bits=lanes.run(diff,[&](aqcf2::Sci&s,const V&a,std::size_t offset){
                aqcf2::Ring d(55);d.v=a;auto right=s.msb(d);aqcf2::Ring payload(55,a.size());
                for(std::size_t j=0;j<a.size();++j)payload.v[j]=(index[2*(offset+j)+1]-index[2*(offset+j)])&aqcf2::mask(55);
                auto chosen=s.mux(right,payload);auto dv=s.mux(right,d);
                for(std::size_t j=0;j<a.size();++j){std::size_t k=offset+j;index[2*k]=(index[2*k]+chosen.v[j])&aqcf2::mask(55);value[2*k]=(value[2*k]-dv.v[j])&aqcf2::mask(55);}
                return chosen.v;
            });
            V nv((value.size()+1)/2),ni(nv.size());
            tensor_work("R50_CLASS_GATHER",nv.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j){nv[j]=value[2*j];ni[j]=index[2*j];}});value=std::move(nv);index=std::move(ni);
        }
        aqcf3::require(value.size()==4,"terminal exactly four real surviving candidates");
        V ids{0,1,2,3};auto contribution=lanes.run(ids,[&](aqcf2::Sci& s,const V& part,std::size_t offset){
          aqcf3::require(part.size()==1,"one distinct useful candidate per worker");
          const auto i=offset;aqcf2::Ring d(55,3);unsigned k=0;
          for(unsigned j=0;j<4;++j)if(j!=i)d.v[k++]=(value[i]-value[j]-(s.party==1&&i>j?1:0))&aqcf2::mask(55);
          auto wins=s.not_(s.msb(d));aqcf2::Bits a(1),b(1),c(1);a.v[0]=wins.v[0];b.v[0]=wins.v[1];c.v[0]=wins.v[2];
          auto win=s.and_(s.and_(a,b),c);aqcf2::Ring idx(55,1);idx.v[0]=index[i];
          return s.mux(win,idx).v;
        });
        U result=0;for(U part:contribution)result=(result+part)&aqcf2::mask(55);
        return V{result}; // four-term share reduction, scalar control explicitly charged
    }
    void public_selftest(){
        he.public_client_parallel_selftest();
        he.public_server_ingress_selftest();
        using I=aqcf3::I;const U m55=aqcf2::mask(55),m37=aqcf2::mask(37);
        auto check=[&](const V& own,const std::vector<I>& expected,unsigned width){
            V peer(own.size());lanes.flush();
            if(he.s.party==1)he.s.io.io->send_data(own.data(),int(8*own.size()));
            else {he.s.io.io->recv_data(peer.data(),int(8*peer.size()));
                for(std::size_t j=0;j<peer.size();++j){U x=(peer[j]+own[j])&aqcf2::mask(width);I v=x&(U(1)<<(width-1))?I(x)-I(U(1)<<width):I(x);aqcf3::require(v==expected[j],"public R50 operator reference mismatch");}}
            lanes.flush();
        };
        std::vector<I> values={-(I(1)<<36),-100,-99,-98,-50,-49,-48,-1,0,1,48,49,50,98,99,100,(I(1)<<36)-1};
        V x(values.size(),0);std::vector<I> expected;
        for(std::size_t j=0;j<x.size();++j){if(he.s.party==2)x[j]=U(values[j])&m37;I z=values[j];expected.push_back(z>=0?z/49:-((-z+48)/49));}
        check(divide49(x),expected,37);
        // Public synthetic additive shares exercise carries, wrap and both signs.
        V floorx(4096);std::vector<I> floorexpected(4096);U prng=17370012;
        for(std::size_t j=0;j<floorx.size();++j){
            prng^=prng<<13;prng^=prng>>7;prng^=prng<<17;U raw=prng&m37;
            if(j<values.size())raw=U(values[j])&m37;
            I signed_value=raw&(U(1)<<36)?I(raw)-I(U(1)<<37):I(raw);
            U share=(prng*6364136223846793005ULL+1442695040888963407ULL)&m37;
            floorx[j]=he.s.party==1?share:(raw-share)&m37;
            floorexpected[j]=signed_value>=0?signed_value/4096:-((-signed_value+4095)/4096);
        }
        check(boundary(floorx,12,false),floorexpected,37);
        auto reluexpected=floorexpected;for(auto&v:reluexpected)v=std::max<I>(0,v);check(boundary(floorx,12,true),reluexpected,37);

        auto full=boundary(x,0,false,true);V zeros(x.size(),0);for(std::size_t j=0;j<expected.size();++j)expected[j]=std::max<I>(0,values[j]);check(maximum55(full,zeros),expected,55);
        V other(x.size(),0);std::vector<I> maxexpected(x.size());
        for(std::size_t j=0;j<x.size();++j){if(he.s.party==2)other[j]=U(values[values.size()-1-j])&m37;maxexpected[j]=std::max(values[j],values[values.size()-1-j]);}
        auto pa=pool_lift(x),pb=pool_lift(other);auto pm=pool_maximum(pa,pb);check(pm,maxexpected,37);
        pool38_lift_elements=pool38_max_elements=pool255_maximum_calls=0;
        layer233_calls=layer233_rows=pool230_calls=0; // diagnostic epoch only; no protocol state or cost reset
        std::cout<<"PUBLIC_POOL38_FULL_DOMAIN_PASS\n"<<std::flush;
        V scores(1001,0);if(he.s.party==2){for(std::size_t j=0;j<scores.size();++j)scores[j]=U(I(j)-900)&m37;scores[731]=scores[732]=1001;}
        check(argmax(scores),{731},55);std::cout<<"R50_PUBLIC_OPERATORS_PASS\n"<<std::flush;
    }
};
