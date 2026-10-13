#include "squeezenet/core/retained_squeezenet_phase.hpp"
#include "squeezenet/core/protected_argmax.hpp"
#include "squeezenet/core/range_tail.hpp"
#include "squeezenet/core/small_pool.hpp"
#include "squeezenet/core/quarter_consumer_fusion.hpp"
#include "squeezenet/core/pool_quotient_domain.hpp"
#include "squeezenet/core/wan_wave_lanes.hpp"
#include "squeezenet/core/nonnegative_lift.hpp"
#include "squeezenet/core/pool_range.hpp"
#include "squeezenet/core/pool_lanes.hpp"
#include "squeezenet/core/ring37_direct_floor.hpp"
#include "squeezenet/core/quotient_pool38.hpp"
#pragma once
#include "squeezenet/core/sq_executor.hpp"
class SqBackend {
public:
    using U=aqcf3::U;using V=std::vector<U>;
    aqcf3::BfvProgram& he;aqcf2::WorkerLanes& lanes;std::ofstream nodes;
    U widened_elements=0,completed_nodes=0,affines=0;
    U certified_lift_affines=0,certified_lift_elements=0;
    U pool38_lift_elements=0,pool38_max_elements=0;
    bool phase_pending=false;U pending_phase_size=0,fused_phase_calls=0,fused_phase_rows=0;
    int phase_tensor_id=-1;
    std::array<U,4> fused_phase_lane_rows{};std::array<int,4> fused_phase_cpus{{-1,-1,-1,-1}};

    // Conventional element-parallel helpers, not a new secure protocol.
    // Each callback touches disjoint outputs; the order WITHIN a reduction is
    // unchanged. No secrets, values or value-dependent timings are logged.
    template<class Work>void tensor_work(const char* phase,std::size_t n,Work work){
        aqcf3::require(n>=4,"four useful tensor helper ranges; no padding");
        he.chunks4(phase,-1,n,work);
    }
    V copy_tensor(const V& x,const char* phase){V out(x.size());
        tensor_work(phase,x.size(),[&](std::size_t a,std::size_t b){for(auto j=a;j<b;++j)out[j]=x[j];});return out;}
    V concat_tensor(const V& x,const V& y){V out(x.size()+y.size());
        tensor_work("EXEC_CONCAT",out.size(),[&](std::size_t a,std::size_t b){
            for(auto j=a;j<b;++j)out[j]=j<x.size()?x[j]:y[j-x.size()];});return out;}
    void mask37_tensor(V& x,const char* phase){
        tensor_work(phase,x.size(),[&](std::size_t a,std::size_t b){for(auto j=a;j<b;++j)x[j]&=aqcf2::mask(37);});}
    void prepare_phase_affine(){
#if AQCF_FUSED_QUARTER_ENTRY
        aqcf3::require(!phase_pending&&phase_tensor_id==-1,"previous raw phase must be consumed before next affine");
        aqcf3::require(he.quarter_enabled&&he.modulus==aqcf3::BfvProgram::plaintext_modulus,"fused entry requires original quarter Ring55 producer");
        he.defer_quarter_recovery=true;
#endif
    }
    V finish_phase_affine(V y){
#if AQCF_FUSED_QUARTER_ENTRY
        aqcf3::require(he.defer_quarter_recovery&&!phase_pending&&!y.empty(),"unique raw affine phase");
        tensor_work("BACKEND_RAW_PHASE_CHECK",y.size(),[&](std::size_t a,std::size_t b){for(auto j=a;j<b;++j)aqcf3::require(y[j]<(U(1)<<57),"do not project raw phase to37");});
        phase_pending=true;pending_phase_size=y.size();he.defer_quarter_recovery=false;
#else
        mask37_tensor(y,"BACKEND_AFFINE_MASK37");
#endif
        return y;
    }
    V consume_phase(const V& x,bool quotient){
        aqcf3::require(AQCF_FUSED_QUARTER_ENTRY&&phase_pending&&x.size()==pending_phase_size,"bound phase consumer");
        auto apply=[&](aqcf2::Sci& s,const V& part,std::size_t){
            unsigned lane=0;while(lane<lanes.size()&& &lanes.at(lane)!=&s)++lane;
            aqcf3::require(lane<4,"unique owning phase OT lane");
            const int cpu=sched_getcpu();
            aqcf3::require(cpu==aqcf_native4::cpu(lane),"actual fused recovery CPU");
            std::unique_ptr<phase317::Phase> phase;
            if(aqcf_tree280::enabled()&&aqcf_terminal302::enabled()&&part.size()>8192)
                phase=std::make_unique<phase317::Phase>(s.control().owned189_handle(),int(part.size()),quotient?1:0);
            auto result=s.boundary_profile(quotient?"quarter_to_quotient26_fused":"quarter_to_relu37_fused",[&]{
                return quotient?aqcf_small_pool20::input_relu25(s,part):aqcf_quarter_entry::floor_relu37(s,part);
            });
            fused_phase_lane_rows[lane]+=part.size();fused_phase_cpus[lane]=cpu;return result.v;
        };
        auto y=quotient?aqcf_pool_wave::run(lanes,x,apply):aqcf_wan_wave::relu_run(lanes,x,apply);
        ++fused_phase_calls;fused_phase_rows+=x.size();phase_pending=false;pending_phase_size=0;phase_tensor_id=-1;
        return y;
    }
    SqBackend(aqcf3::BfvProgram& h,aqcf2::WorkerLanes& l,const std::filesystem::path& p):he(h),lanes(l),nodes(p){aqcf3::require(bool(nodes),"node log");nodes<<std::setprecision(17);}
    struct Marker{aqcf3::Steady::time_point t;U bytes,inputs,outputs,pm;};
    Marker begin_node(const aqcf_sq::Node& n){std::cout<<"NODE_BEGIN "<<n.index<<" "<<n.kind<<"\n"<<std::flush;lanes.flush();return{aqcf3::Steady::now(),lanes.bytes(),he.metrics.input_ct,he.metrics.output_ct,he.metrics.pm};}
    void end_node(const aqcf_sq::Node& n,Marker m,const V& y){
        ++completed_nodes;if(n.affine>=0)++affines;lanes.flush();
        nodes<<"{\"node\":"<<n.index<<",\"kind\":\""<<n.kind<<"\",\"affine\":"<<n.affine<<",\"elements\":"<<y.size()
             <<",\"seconds\":"<<aqcf3::elapsed(m.t)<<",\"local_io_counter_delta\":"<<lanes.bytes()-m.bytes
             <<",\"input_CT\":"<<he.metrics.input_ct-m.inputs<<",\"output_CT\":"<<he.metrics.output_ct-m.outputs
             <<",\"PM\":"<<he.metrics.pm-m.pm<<"}\n"<<std::flush;
        aqcf3::require(bool(nodes),"node write");std::cout<<"NODE "<<n.index<<" "<<n.kind<<'\n'<<std::flush;
    }
    V affine_with_nonnegative_certificate(int id,const V& x,std::size_t n,unsigned bits){
        aqcf3::require(bits==24||bits==36,"fixed public producer certificate");
        ++certified_lift_affines;certified_lift_elements+=x.size();widened_elements+=x.size();
        std::cout<<"CERTIFIED_AFFINE_LIFT "<<id<<" "<<x.size()<<"\n"<<std::flush;
        // Canonical share lifts commute with the final37 quotient projection
        // of an integer affine over Ring55. Never apply to nonlinear or final55.
        V lifted(x.size());tensor_work("BACKEND_AFFINE_INGRESS",x.size(),[&](std::size_t a,std::size_t b){for(auto j=a;j<b;++j)lifted[j]=x[j]&aqcf2::mask(37);});
        std::cout<<"AFFINE_QUOTIENT_INGRESS "<<id<<" "<<x.size()<<"\n"<<std::flush;

        prepare_phase_affine();auto y=he.affine(id,lifted);aqcf3::require(y.size()==n,"complete certified affine");
        return finish_phase_affine(std::move(y));
    }
    V affine(int id,const V& x,std::size_t n){
        // Exact signed Ring37 to Ring55 before existing55-bit HE; projection
        // back to37 follows the full original finite-ring model semantics.
        // Canonical share lifts commute with the final37 quotient projection
        // of an integer affine over Ring55. Never apply to nonlinear or final55.
        V lifted(x.size());tensor_work("BACKEND_AFFINE_INGRESS",x.size(),[&](std::size_t a,std::size_t b){for(auto j=a;j<b;++j)lifted[j]=x[j]&aqcf2::mask(37);});
        std::cout<<"AFFINE_QUOTIENT_INGRESS "<<id<<" "<<x.size()<<"\n"<<std::flush;

        prepare_phase_affine();auto y=he.affine(id,lifted);
        aqcf3::require(y.size()==n,"complete SqNet affine");return finish_phase_affine(std::move(y));
    }
    V boundary(const V& x,unsigned f,bool relu,bool just_lift=false){
        if(phase_pending){aqcf3::require(relu&&!just_lift&&f==12,"only fixed phase floor12-ReLU consumer");return consume_phase(x,false);}
        if(relu&&!just_lift&&f==12){
            widened_elements+=x.size();
            return aqcf_wan_wave::relu_run(lanes,x,[&](aqcf2::Sci&s,const V&a,std::size_t){
                aqcf2::Ring z(55);z.v=a;
                return s.boundary_profile("joint_direct_ring37_floor_relu",[&]{return aqcf_cifar_comm::floor_relu37(s,z,f);}).v;
            });
        }
        widened_elements+=x.size();auto result=lanes.run(x,[&](aqcf2::Sci&s,const V&a,std::size_t){
            aqcf2::Ring z(55);z.v=a;
            if(relu&&!just_lift&&f==12)return s.boundary_profile("joint_direct_ring37_floor_relu",[&]{return aqcf_cifar_comm::floor_relu37(s,z,f);}).v;
            if(!relu&&!just_lift&&f==12)return s.boundary_profile("joint_direct_ring37_floor",[&]{return aqcf_cifar_comm::floor37_direct(s,z,f);}).v;
            auto full=s.boundary_profile("centered_ring37_to55",[&]{return aqcf_rhombus::centered_lift(s,z,37);});
            if(just_lift)return full.v;
            if(f==0&&relu)return aqcf2::backbone_relu(s,full,aqcf2::BackboneCarry::Linked).v;
            return aqcf2::backbone_floor(s,full,f,relu,aqcf2::BackboneCarry::Linked,true).v;
        });
        if(!just_lift)mask37_tensor(result,"BACKEND_BOUNDARY_MASK37");return result;
    }
    V quotient_pool_lift(const V& x){
        if(phase_pending)return consume_phase(x,true);
        return aqcf_pool_wave::run(lanes,x,[&](aqcf2::Sci&s,const V&a,std::size_t){
            aqcf2::Ring z(37);z.v=a;return s.boundary_profile("exact_quotient_lift37_floor12_to26",[&]{return aqcf_pool_quotient::quotient_lift(s,z);}).v;
        });
    }
    V quotient_pool_maximum(const V& a,const V& b){
        aqcf3::require(a.size()==b.size(),"quotient maximum shapes");
        return aqcf_pool_wave::run(lanes,a,[&](aqcf2::Sci&s,const V&v,std::size_t offset){
            aqcf2::Ring x(25),y(25,v.size());x.v=v;std::copy_n(b.begin()+offset,v.size(),y.v.begin());
            return s.boundary_profile("exact_relu_conjugated_pool_maximum25",[&]{return aqcf_small_pool20::maximum25(s,x,y);}).v;
        });
    }
    V quotient_pool_finish(const V& x){
        return aqcf_pool_wave::run(lanes,x,[&](aqcf2::Sci&s,const V&a,std::size_t){
            aqcf2::Ring z(25);z.v=a;return s.boundary_profile("exact_pool_terminal_lift25_to37",[&]{return aqcf_small_pool20::finish37(s,z);}).v;
        });
    }
    V pool_range_maximum(const V& a,const V& b,unsigned bits){
        aqcf3::require(a.size()==b.size(),"certified pool shapes");
        return aqcf_pool_wave::run(lanes,a,[&](aqcf2::Sci&s,const V&v,std::size_t offset){
            aqcf2::Ring x(37),y(37,v.size());x.v=v;std::copy_n(b.begin()+offset,v.size(),y.v.begin());
            return s.boundary_profile("public_range_maximum37",[&]{return aqcf_pool_range::maximum(s,x,y,bits);}).v;
        });
    }
    V pool_lift(const V& x){
        std::cout<<"POOL_STAGE lift "<<x.size()<<"\n"<<std::flush;
#if AQCF_POOL38
        pool38_lift_elements+=x.size();
        return aqcf_pool_wave::run(lanes,x,[&](aqcf2::Sci&s,const V&a,std::size_t){aqcf2::Ring z(55);z.v=a;return s.boundary_profile("pool_quotient_lift37_to38",[&]{return aqcf_pool_wave::lift(s,z);}).v;});
#else
        return boundary(x,0,false,true);
#endif
    }
    V pool_maximum(const V& a,const V& b){
        std::cout<<"POOL_STAGE max "<<a.size()<<"\n"<<std::flush;
#if AQCF_POOL38
        aqcf3::require(a.size()==b.size(),"same full pool shapes");pool38_max_elements+=a.size();
        return aqcf_pool_wave::run(lanes,a,[&](aqcf2::Sci&s,const V&v,std::size_t offset){aqcf2::Ring x(38),y(38,v.size());x.v=v;std::copy_n(b.begin()+offset,v.size(),y.v.begin());return s.boundary_profile("pool_exact_difference38",[&]{return aqcf_pool_wave::maximum(s,x,y);}).v;});
#else
        return maximum55(a,b);
#endif
    }
    // Both inputs are already exact signed37 values shared in Ring55.
    V maximum55(const V&a,const V&b){
        aqcf3::require(a.size()==b.size(),"max pair shape");V d(a.size());
        tensor_work("BACKEND_MAX_DIFFERENCE",d.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)d[j]=(b[j]-a[j])&aqcf2::mask(55);});
        auto pos=lanes.run(d,[&](aqcf2::Sci&s,const V&v,std::size_t){aqcf2::Ring z(55);z.v=v;return aqcf2::backbone_relu(s,z,aqcf2::BackboneCarry::Linked).v;});
        tensor_work("BACKEND_MAX_RESTORE",pos.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)pos[j]=(pos[j]+a[j])&aqcf2::mask(55);});return pos;
    }
    // Standard secure restoring division, NOT claimed as an original algorithm.
    // x in signed Ring37. Offset makes n=x+2^36 nonnegative and below 2^37.
    // Each public comparison uses signed Ring55, hence no signed overflow.
    V divide169(const V& input){
        auto lifted=boundary(input,0,false,true);
        auto result=lanes.run(lifted,[&](aqcf2::Sci&s,const V&a,std::size_t){
            aqcf2::Ring rem(55);rem.v=a;aqcf2::Ring quotient(55,a.size());
            if(s.party==1)for(auto&v:rem.v)v=(v+(U(1)<<36))&aqcf2::mask(55);
            for(int k=31;k>=0;--k){
                U d=U(169)<<k;aqcf2::Ring diff=rem;
                if(s.party==1)for(auto&v:diff.v)v=(v-d)&aqcf2::mask(55);
                auto ge=s.not_(s.msb(diff));auto bit=s.b2a(ge,55);
                for(std::size_t j=0;j<a.size();++j){rem.v[j]=(rem.v[j]-bit.v[j]*d)&aqcf2::mask(55);quotient.v[j]=(quotient.v[j]+(bit.v[j]<<k))&aqcf2::mask(55);}
            }
            constexpr U offset=U(1)<<36;auto diff=rem;
            if(s.party==1)for(auto&v:diff.v)v=(v-offset%169)&aqcf2::mask(55);
            auto correction=s.b2a(s.msb(diff),55);
            for(std::size_t j=0;j<a.size();++j)quotient.v[j]=(quotient.v[j]-correction.v[j]-(s.party==1?offset/169:0))&aqcf2::mask(37);
            return quotient.v;
        });return result;
    }
    V divide169_nonnegative24(const V& input){
        std::cout<<"CERTIFIED_AVERAGE_169 "<<input.size()<<"\n"<<std::flush;
        return lanes.run(input,[&](aqcf2::Sci&s,const V&a,std::size_t){aqcf2::Ring x(37);x.v=a;
            return s.boundary_profile("range_exact_divide169",[&]{return aqcf_range_tail::divide169(s,x);}).v;});
    }
    V final_scores_nonnegative55(const V& input){
        return lanes.run(input,[&](aqcf2::Sci&s,const V&a,std::size_t){aqcf2::Ring x(37);x.v=a;
            return aqcf_nonnegative_lift::lift37_to55(s,x).v;});
    }
    // Four distinct surviving candidates, not replicated/dummy work.
    // The existing tree preserves increasing original-index order on ties.
    V terminal_owned4(const V& value,const V& index,bool range24){
        aqcf3::require(value.size()==4&&index.size()==4,"four real terminal candidates");
        V owners{0,1,2,3};
        auto terms=lanes.run(owners,[&](aqcf2::Sci& s,const V& part,std::size_t offset){
            aqcf3::require(part.size()==1&&offset<4,"one useful owner per worker");
            const unsigned i=unsigned(offset),width=range24?37:55;
            aqcf2::Ring diff(width,3);unsigned at=0;
            for(unsigned j=0;j<4;++j)if(j!=i)
                diff.v[at++]=(value[i]-value[j]-(s.party==1&&i>j?1:0))&aqcf2::mask(width);
            auto loses=range24?s.xor_(aqcf_range_tail::carry(s,diff.v,24),s.local_bit(diff.v,24)):s.msb(diff);
            auto wins=s.not_(loses);aqcf2::Bits a(1),b(1),c(1);
            a.v[0]=wins.v[0];b.v[0]=wins.v[1];c.v[0]=wins.v[2];
            auto winner=s.and_(s.and_(a,b),c);aqcf2::Ring payload(55,1);payload.v[0]=index[i];
            return s.mux(winner,payload).v;
        });
        U result=0;for(U term:terms)result=(result+term)&aqcf2::mask(55);
        return V{result}; // charged scalar reduction of four protected shares
    }
    V argmax_nonnegative24(const V& input){
        if(aqcf_pool273::enabled())return aqcf_argmax274::run(lanes,input);
        std::cout<<"CERTIFIED_ARGMAX_24 "<<input.size()<<"\n"<<std::flush;
        auto value=copy_tensor(input,"ARGMAX_INPUT_COPY");V index(value.size(),0);
        if(he.s.party==1)tensor_work("ARGMAX_INDEX",index.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)index[j]=j;});
        // Stable left winner on equality, all 1000 classes participate.
        while(value.size()>4){std::size_t pairs=value.size()/2;V diff(pairs);
            tensor_work("ARGMAX_DIFFERENCE",pairs,[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)diff[j]=(value[2*j]-value[2*j+1])&aqcf2::mask(37);});
            auto bits=lanes.run(diff,[&](aqcf2::Sci&s,const V&a,std::size_t offset){
                aqcf2::Ring d(37);d.v=a;auto right=s.xor_(aqcf_range_tail::carry(s,a,24),s.local_bit(a,24));aqcf2::Ring payload(55,a.size());
                for(std::size_t j=0;j<a.size();++j)payload.v[j]=(index[2*(offset+j)+1]-index[2*(offset+j)])&aqcf2::mask(55);
                auto chosen=s.mux(right,payload);auto dv=s.mux(right,d);
                for(std::size_t j=0;j<a.size();++j){std::size_t k=offset+j;index[2*k]=(index[2*k]+chosen.v[j])&aqcf2::mask(55);value[2*k]=(value[2*k]-dv.v[j])&aqcf2::mask(37);}
                return chosen.v;
            });
            V nv((value.size()+1)/2),ni(nv.size());
            tensor_work("ARGMAX_COMPACT",nv.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j){nv[j]=value[2*j];ni[j]=index[2*j];}});value=std::move(nv);index=std::move(ni);
        }return terminal_owned4(value,index,true);
    }
    V argmax(const V& input){
        auto value=boundary(input,0,false,true);V index(value.size(),0);
        if(he.s.party==1)tensor_work("ARGMAX_INDEX",index.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)index[j]=j;});
        // Stable left winner on equality, all 1000 classes participate.
        while(value.size()>4){std::size_t pairs=value.size()/2;V diff(pairs);
            tensor_work("ARGMAX_DIFFERENCE",pairs,[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)diff[j]=(value[2*j]-value[2*j+1])&aqcf2::mask(55);});
            auto bits=lanes.run(diff,[&](aqcf2::Sci&s,const V&a,std::size_t offset){
                aqcf2::Ring d(55);d.v=a;auto right=s.msb(d);aqcf2::Ring payload(55,a.size());
                for(std::size_t j=0;j<a.size();++j)payload.v[j]=(index[2*(offset+j)+1]-index[2*(offset+j)])&aqcf2::mask(55);
                auto chosen=s.mux(right,payload);auto dv=s.mux(right,d);
                for(std::size_t j=0;j<a.size();++j){std::size_t k=offset+j;index[2*k]=(index[2*k]+chosen.v[j])&aqcf2::mask(55);value[2*k]=(value[2*k]-dv.v[j])&aqcf2::mask(55);}
                return chosen.v;
            });
            V nv((value.size()+1)/2),ni(nv.size());
            tensor_work("ARGMAX_COMPACT",nv.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j){nv[j]=value[2*j];ni[j]=index[2*j];}});value=std::move(nv);index=std::move(ni);
        }return terminal_owned4(value,index,false);
    }
    void public_selftest(){
        // Existing unchanged HE core is qualified by the complete public graph.
        using I=aqcf3::I;const U m55=aqcf2::mask(55),m37=aqcf2::mask(37);
        auto check=[&](const V& own,const std::vector<I>& expected,unsigned width){
            V peer(own.size());lanes.flush();
            if(he.s.party==1)he.s.io.io->send_data(own.data(),int(8*own.size()));
            else {he.s.io.io->recv_data(peer.data(),int(8*peer.size()));
                for(std::size_t j=0;j<peer.size();++j){U x=(peer[j]+own[j])&aqcf2::mask(width);I v=x&(U(1)<<(width-1))?I(x)-I(U(1)<<width):I(x);aqcf3::require(v==expected[j],"public R50 operator reference mismatch");}}
            lanes.flush();
        };
        // Exhaust all packed one-bit tables, both choices, and 1/2-bit outputs.
        for(unsigned width:{1U,2U}){V table,choices;std::vector<I> expect;
            for(U word=0;word<(U(1)<<(2*width));++word)for(U c=0;c<2;++c){
                if(he.s.party==1)table.push_back(word);choices.push_back(c);expect.push_back((word>>(width*c))&aqcf2::mask(width));}
            auto got=he.s.lookup_words(table,choices,choices.size(),1,width,true);
            if(he.s.party==1)std::fill(got.begin(),got.end(),0);check(got,expect,55);
        }
        std::cout<<"PUBLIC_BINARY_COFACTOR_COMPAT_PASS\n"<<std::flush;
        std::vector<I> values={-(I(1)<<36),-100,-99,-98,-50,-49,-48,-1,0,1,48,49,50,98,99,100,(I(1)<<36)-1};
        V x(values.size(),0);std::vector<I> expected;
        for(std::size_t j=0;j<x.size();++j){if(he.s.party==2)x[j]=U(values[j])&m37;I z=values[j];expected.push_back(z>=0?z/169:-((-z+168)/169));}
        check(divide169(x),expected,37);
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
        pool38_lift_elements=pool38_max_elements=0;
        std::cout<<"PUBLIC_POOL38_FULL_DOMAIN_PASS\n"<<std::flush;
        V scores(1000,0);if(he.s.party==2){for(std::size_t j=0;j<scores.size();++j)scores[j]=U(I(j)-900)&m37;scores[731]=scores[732]=1001;}
        check(argmax(scores),{731},55);
        const U upper=169*((U(1)<<24)-1);V average_inputs{0,1,168,169,170,337,338,upper-1,upper};std::vector<I> average_expected;
        for(auto x:average_inputs)average_expected.push_back(x/169);
        if(he.s.party==1)std::fill(average_inputs.begin(),average_inputs.end(),0);
        check(divide169_nonnegative24(average_inputs),average_expected,37);
        for(unsigned fixture=0;fixture<3;++fixture){V x(1000,0);I label=0;
            if(fixture==1){label=731;if(he.s.party==2){x[731]=x[732]=1001;}}
            if(fixture==2){label=999;if(he.s.party==2)x[999]=(U(1)<<24)-1;}
            check(argmax_nonnegative24(x),{label},55);
        }
        std::cout<<"PUBLIC_RANGE_TAIL_PASS\n"<<std::flush;
        std::cout<<"SQ_PUBLIC_OPERATORS_PASS\n"<<std::flush;
    }
};
