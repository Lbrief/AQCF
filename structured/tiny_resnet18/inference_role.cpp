// Complete owner-bound HE-MPC ResNet18, conditional research only.
// PUBLIC and one-use REAL modes use identical arithmetic and cryptography.
#include "bfv_program.hpp"
#include "worker_lanes.hpp"
#include "ring41_boundary.hpp"
#include "ring41_direct.hpp"
#include "aqcf_nonlinear_ablation.hpp"
#include "resnet_schedule.hpp"
#include "role_admission.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <csignal>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <omp.h>
extern "C" int __real_bind(int,const struct sockaddr*,socklen_t);
extern "C" int __wrap_bind(int fd,const struct sockaddr*p,socklen_t n){
    if(!p||p->sa_family!=AF_INET||n<sizeof(sockaddr_in)){errno=EACCES;return -1;}
    auto a=*reinterpret_cast<const sockaddr_in*>(p);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    return __real_bind(fd,reinterpret_cast<const sockaddr*>(&a),sizeof(a));
}
using namespace aqcf3;
class ProtectedBackend {
public:
    using Value=U;BfvProgram&he;aqcf2::WorkerLanes&lanes;std::ofstream nodes;
    aqcf_rhombus::Coverage coverage;U widened_elements=0;
    ProtectedBackend(BfvProgram&h,aqcf2::WorkerLanes&l,const std::filesystem::path&p):he(h),lanes(l),nodes(p){require(bool(nodes),"node log");nodes<<std::setprecision(17);}
    struct Marker{Steady::time_point t;U bytes,inputs,outputs,pm;};
    Marker begin_node(const aqcf_rhombus::Node&){lanes.flush();return{Steady::now(),lanes.bytes(),he.metrics.input_ct,he.metrics.output_ct,he.metrics.pm};}
    void end_node(const aqcf_rhombus::Node&n,Marker m,const std::vector<U>&){
        lanes.flush();nodes<<"{\"node\":"<<n.id<<",\"affine\":"<<n.affine<<",\"elements\":"<<n.elements
          <<",\"seconds\":"<<elapsed(m.t)<<",\"local_io_counter_delta\":"<<lanes.bytes()-m.bytes
          <<",\"input_CT\":"<<he.metrics.input_ct-m.inputs<<",\"output_CT\":"<<he.metrics.output_ct-m.outputs
          <<",\"PM\":"<<he.metrics.pm-m.pm<<"}\n"<<std::flush;
        require(bool(nodes),"node write");std::cout<<"NODE "<<n.id<<'\n'<<std::flush;
    }
    U zero(){return 0;}
    U add(U a,U b){return(a+b)&aqcf2::mask(41);}
    U lift_scale(U a,unsigned f){require(f==12,"identity residual scale");return(a<<f)&aqcf2::mask(41);}
    std::vector<U> affine(int id,const std::vector<U>&x,std::size_t n){
        auto projected=x;for(auto&v:projected)v&=aqcf2::mask(41);
        auto y=he.affine(id,projected);require(y.size()==n,"all trained affine outputs");
        for(auto&v:y)v&=aqcf2::mask(41);return y;
    }
    std::vector<U> boundary(const std::vector<U>&x,unsigned f,bool relu,bool just_lift=false){
        widened_elements+=x.size();
        return lanes.run(x,[&](aqcf2::Sci&s,const std::vector<U>&a,std::size_t){
            aqcf2::Ring z(55);z.v=a;
            if(relu&&!just_lift)return s.boundary_profile("joint_direct_ring41_floor_relu",[&]{return aqcf_nl_control::floor_relu41(s,z,f);}).v;
            auto full=s.boundary_profile("centered_ring41_to55",[&]{return aqcf_nl_control::centered_lift(s,z);});
            return just_lift?full.v:aqcf_nl_control::floor_after_lift(s,full,f,relu).v;
        });
    }
    std::vector<U> floor(const std::vector<U>&x,unsigned f){return boundary(x,f,false);}
    std::vector<U> floor_relu(const std::vector<U>&x,unsigned f){return boundary(x,f,true);}
    void accept_coverage(const aqcf_rhombus::Coverage&c){coverage=c;}
};
int main(int argc,char**argv){try{
    require(argc==9,"explicit role-owned admission required");
    int party=std::stoi(argv[1]),port=std::stoi(argv[2]);unsigned workers=std::stoul(argv[3]);
    require((party==1||party==2)&&(workers==1||workers==4)&&port>=45000&&port<49000,"role allocation");
    require(workers==1 || (!std::getenv("AQCF_CPU_SERVER")&&!std::getenv("AQCF_CPU_CLIENT")),"mapped CPU selection requires one worker per party");
    auto admitted=aqcf_rhombus::admit(argc,argv,party,workers);
    // User-selected v27: fresh source-bound PUBLIC/REAL n1 admission.
    const auto process_start=Steady::now();std::signal(SIGALRM,[](int){::_exit(124);});alarm(420);
    std::signal(SIGPIPE,SIG_IGN);omp_set_dynamic(0);omp_set_num_threads(1);
    aqcf2::pin_lane(party==1?0:(workers==1?2:4));rlimit lim{0,0};require(setrlimit(RLIMIT_CORE,&lim)==0,"no dumps");
    const auto out=std::filesystem::path(argv[6]);
    require(out.is_absolute()&&!std::filesystem::exists(out),"fresh explicit paths");std::filesystem::create_directory(out);
    auto layout=parse_layout(bound_bytes(argv[4],admitted.layout_sha,1ULL<<30));
    std::unique_ptr<Program>program;
    std::vector<I> input;
    if(party==1){program=std::make_unique<Program>(parse_program(bound_bytes(argv[5],admitted.asset_sha,1ULL<<30)));match_program(*program,layout);}
    else input=read_i64_asset(argv[5],admitted.asset_sha,12288);
    U returns=0,inputs=0;for(auto&l:layout){returns+=l.queries.size();inputs+=l.maps.size();}
    require(returns==478&&inputs==610&&layout.size()==21,"all21 trained affine layouts");
    std::cout<<"ASSETS_READY\n"<<std::flush;char command;require(::read(0,&command,1)==1&&command=='C',"controller connect");
    sci::IOPack io(party,port,"127.0.0.1");sci::OTPack ot(&io,party,true);aqcf2::Sci s(party,io,ot);s.configure_quotient(aqcf_nl_control::extension_mode());
    aqcf2::WorkerLanes lanes(s,port,workers);for(unsigned i=1;i<workers;++i)lanes.at(i).configure_quotient(aqcf_nl_control::extension_mode());
    BfvProgram he(s,U(1)<<41,layout,program.get(),false,true,"broadcast","packed-query","legacy","prescaled","symmetric-seeded","native");
    he.configure_linear_workers(workers,!admitted.real);
    ProtectedBackend b(he,lanes,out/"nodes.jsonl");std::vector<U>x(12288,0);
    if(party==2)for(std::size_t j=0;j<x.size();++j){require(input[j]>=-12288&&input[j]<=12288,"normalization envelope");x[j]=aqcf2::residue(input[j],aqcf2::M);}
    input.clear();
    lanes.flush();const U setup_bytes=lanes.bytes();const double setup=elapsed(process_start);
    std::cout<<"READY\n"<<std::flush;require(::read(0,&command,1)==1&&command=='G',"controller start");
    auto y=aqcf_rhombus::execute(b,aqcf_rhombus::graph,x,aqcf_rhombus::final_id);
    auto finalshare=b.boundary(y,0,false,true);auto final=he.final200(finalshare);lanes.flush();
    auto c=b.coverage;require(c.nodes==47&&c.affines==21&&c.relus==17&&c.adds==8&&c.pools==1,"full graph coverage");
    require(c.floor_elements==557568&&c.relu_elements==557056&&b.widened_elements==557768,"all nonlinear conversion elements");
    require(he.reserved_returns==478&&he.metrics.input_ct==610&&he.metrics.output_ct==478,"full bidirectional HE counts");
    require(he.metrics.pm==(party==1?13841ULL:0ULL),"role-specific server HE multiplication count");
    if(party==2){require(final.size()==200,"complete scores");std::ofstream f(out/"FINAL200.i64",std::ios::binary);f.write(reinterpret_cast<const char*>(final.data()),1600);require(bool(f),"full final result");}
    rusage usage{};require(getrusage(RUSAGE_SELF,&usage)==0,"resource receipt");
    std::ofstream r(out/"RESULT.json");r<<std::setprecision(17)<<"{\"status\":\"COMPLETE_ADMITTED_TRAINED_GRAPH_CHECK_SCOPE\",\"private_model\":"<<(admitted.real?"true":"false")<<",\"real_image\":"<<(admitted.real?"true":"false")<<",\"conditional_security\":true,\"party\":"<<party
     <<",\"workers\":"<<workers<<",\"nodes\":47,\"affines\":21,\"relu_nodes\":17,\"input_CT\":"<<he.metrics.input_ct<<",\"output_CT\":"<<he.metrics.output_ct<<",\"PM\":"<<he.metrics.pm
     <<",\"core_execution\":{"<<"\"quotient_eligible_layers\":"<<he.metrics.quotient_eligible_layers<<",\"short_action_layers\":"<<he.metrics.action_layers<<",\"short_weight_ntts\":"<<he.metrics.short_weight_ntts<<",\"full_weight_ntts\":"<<he.metrics.full_weight_ntts<<",\"fresh_zero_inverse_ntts\":"<<he.metrics.lifted.inverse_ntts<<",\"avoided_inverse_ntts\":"<<he.metrics.lifted.avoided_inverse_ntts<<"}"
     <<",\"core_flags\":{"<<"\"SHORT\":"<<AQCF_ABLATE_SHORT<<",\"FUSION\":"<<AQCF_ABLATE_FUSION<<",\"PREFIX\":"<<AQCF_ABLATE_PREFIX<<",\"SHARED_PREFIX\":"<<AQCF_ABLATE_SHARED_PREFIX<<",\"EMITTER\":"<<AQCF_ABLATE_EMITTER<<",\"QUOTIENT\":"<<AQCF_ABLATE_QUOTIENT<<",\"LIFT\":"<<AQCF_ABLATE_LIFT<<",\"QUARTER\":"<<AQCF_ABLATE_QUARTER<<"}"
     <<",\"ring_bits\":41,\"class_count\":200"
     <<",\"widened_elements\":"<<b.widened_elements<<",\"setup_seconds\":"<<setup<<",\"full_process_seconds\":"<<elapsed(process_start)
     <<",\"service_raw_seconds\":"<<(party==2?he.service_delta.raw:0)<<",\"service_monotonic_seconds\":"<<(party==2?he.service_delta.monotonic:0)<<",\"service_realtime_seconds\":"<<(party==2?he.service_delta.realtime:0)
     <<",\"online_local_io_counter_delta\":"<<lanes.bytes()-setup_bytes<<",\"total_local_io_counter\":"<<lanes.bytes()<<",\"max_rss_kib\":"<<usage.ru_maxrss
     <<",\"maximum_nonlinear_workers\":"<<lanes.maximum_workers<<",\"fresh_zero_returns\":"<<he.metrics.fresh_zero
     <<",\"client_encryptions\":"<<he.metrics.encryptions<<",\"client_masked_decryptions\":"<<he.metrics.decryptions
     <<",\"full_mask_coefficients\":"<<he.metrics.full_mask<<",\"ct_adds\":"<<he.metrics.ct_add<<",\"quarter_recovery_elements\":"<<he.quarter_rows
     <<",\"HE_phases_seconds\":{";
    bool first=true;for(auto&[k,v]:he.metrics.seconds){if(!first)r<<',';first=false;r<<'"'<<k<<"\":"<<v;}
    r<<"},\"nonlinear_phases_seconds\":{";first=true;for(auto&[k,v]:s.boundary_seconds){if(!first)r<<',';first=false;r<<'"'<<k<<"\":"<<v;}
    r<<"},\"linear_parallel\":{\"maximum_workers\":"<<he.maximum_linear_workers
     <<",\"layers\":"<<he.parallel_layers<<",\"outputs\":"<<he.parallel_outputs
     <<",\"audited_outputs\":"<<he.parallel_audited_outputs<<",\"worker_cpus\":[";
    for(unsigned j=0;j<4;++j){if(j)r<<',';r<<he.linear_worker_cpus[j];}
    r<<"],\"worker_outputs\":[";for(unsigned j=0;j<4;++j){if(j)r<<',';r<<he.linear_worker_outputs[j];}
    r<<"]}}\n";require(bool(r),"result write");std::cout<<"DONE\n"<<std::flush;alarm(0);return 0;
}catch(const std::exception&e){std::cerr<<"ADMITTED_TRAINED_HE_FAIL "<<e.what()<<'\n';return 1;}}
