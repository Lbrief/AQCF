// Owner-bound conditional research role; PUBLIC and REAL scope explicit.
// Same numeric/cryptographic executable is publicly qualified before real use.
#include "bfv_program.hpp"
#include "worker_lanes.hpp"
#include "backbone_boundary.hpp"
#include "model_graph.hpp"
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
extern "C" int __wrap_bind(int fd,const struct sockaddr* p,socklen_t n){
    if(!p || p->sa_family!=AF_INET || n<sizeof(sockaddr_in)){errno=EACCES;return -1;}
    auto a=*reinterpret_cast<const sockaddr_in*>(p);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    return __real_bind(fd,reinterpret_cast<const sockaddr*>(&a),sizeof(a));
}
using namespace aqcf3;
static constexpr auto mobile_carry = AQCF_ABLATE_PREFIX ? aqcf2::BackboneCarry::PreparedControl : aqcf2::BackboneCarry::Linked;
static constexpr bool mobile_direct = AQCF_ABLATE_EMITTER==0;
static constexpr const char* mobile_extension = AQCF_ABLATE_QUOTIENT ? "stock" : "quotient";
class PortBackend {
public:
    using Value=U;BfvProgram& he;aqcf2::WorkerLanes& lanes;std::ofstream nodes;aqcf_backbone::Coverage coverage;
    PortBackend(BfvProgram& h,aqcf2::WorkerLanes& l,const std::filesystem::path& p):he(h),lanes(l),nodes(p){require(bool(nodes),"node log");nodes<<std::setprecision(17);}
    U floor_emission_elements=0;
    struct Marker {Steady::time_point t;U bytes,in,out,pm;};
    Marker begin_node(const aqcf_backbone::Node&){lanes.flush();return {Steady::now(),lanes.bytes(),he.metrics.input_ct,he.metrics.output_ct,he.metrics.pm};}
    void end_node(const aqcf_backbone::Node& n,Marker m){lanes.flush();
        nodes<<"{\"node\":"<<n.id<<",\"affine\":"<<n.affine<<",\"elements\":"<<n.elements
             <<",\"seconds\":"<<elapsed(m.t)<<",\"local_io_counter_delta\":"<<lanes.bytes()-m.bytes
             <<",\"input_CT\":"<<he.metrics.input_ct-m.in<<",\"output_CT\":"<<he.metrics.output_ct-m.out
             <<",\"PM\":"<<he.metrics.pm-m.pm<<"}\n"<<std::flush;
        require(bool(nodes),"node result write");std::cout<<"NODE "<<n.id<<'\n'<<std::flush;
    }
    U zero(){return 0;}
    U add(U a,U b){return(a+b)&aqcf2::mask(55);}
    U lift_scale(U a,unsigned n){require(n<=24,"public scale lift");return(a<<n)&aqcf2::mask(55);}
    std::vector<U> affine(int id,const std::vector<U>&x){return he.affine(id,x);}
    std::vector<U> floor(const std::vector<U>&x,unsigned f){floor_emission_elements+=x.size();return lanes.run(x,[&](aqcf2::Sci&s,const std::vector<U>&a,std::size_t){
        aqcf2::Ring z(55);z.v=a;return aqcf2::backbone_floor(s,z,f,false,mobile_carry,mobile_direct).v;});}
    std::vector<U> floor_clip6(const std::vector<U>&x,unsigned f){floor_emission_elements+=x.size();return lanes.run(x,[&](aqcf2::Sci&s,const std::vector<U>&a,std::size_t){
        aqcf2::Ring z(55);z.v=a;return aqcf2::backbone_floor_relu6(s,z,f,mobile_carry,mobile_direct).v;});}
    std::vector<U> clip6(const std::vector<U>&x){return lanes.run(x,[&](aqcf2::Sci&s,const std::vector<U>&a,std::size_t){
        aqcf2::Ring z(55);z.v=a;auto y=aqcf2::backbone_relu(s,z,mobile_carry);auto cut=y;
        if(s.party==1)for(auto&v:cut.v)v=(v-24576)&aqcf2::mask(55);
        auto excess=aqcf2::backbone_relu(s,cut,mobile_carry);
        for(std::size_t j=0;j<a.size();++j)y.v[j]=(y.v[j]-excess.v[j])&aqcf2::mask(55);return y.v;});}
    void accept_coverage(const aqcf_backbone::Coverage&c){coverage=c;}
};
int main(int argc,char**argv){try{
    require(argc==9,"explicit owner admission required");
    int party=std::stoi(argv[1]),port=std::stoi(argv[2]);unsigned workers=std::stoul(argv[3]);
    require((party==1||party==2)&&(workers==1||workers==4)&&port>=45000&&port<49000,"public role allocation");
    require(workers==1 || (!std::getenv("AQCF_CPU_SERVER")&&!std::getenv("AQCF_CPU_CLIENT")),"mapped CPU selection requires one worker per party");
    auto admitted=aqcf_backbone::admit(argc,argv,party,workers);
    const auto process_start=Steady::now();std::signal(SIGALRM,[](int){::_exit(124);});alarm(240);
    std::signal(SIGPIPE,SIG_IGN);omp_set_dynamic(0);omp_set_num_threads(1);
    aqcf2::pin_lane(party==1?0:(workers==1?2:4));rlimit no_core{0,0};require(setrlimit(RLIMIT_CORE,&no_core)==0,"no core dumps");
    auto out=std::filesystem::path(argv[6]);require(out.is_absolute()&&!std::filesystem::exists(out),"fresh public role output");std::filesystem::create_directory(out);
    auto layout=parse_layout(bound_bytes(argv[4],admitted.layout_sha,1ULL<<30));
    std::unique_ptr<Program> program;std::vector<I> input;
    if(party==1){program=std::make_unique<Program>(parse_program(bound_bytes(argv[5],admitted.asset_sha,1ULL<<30)));match_program(*program,layout);}
    else input=read_i64_asset(argv[5],admitted.asset_sha,3072);
    U returns=0;for(auto&l:layout)returns+=l.queries.size();require(returns==1442,"complete public port returns");
    for(auto&n:aqcf_backbone::graph)if(n.affine>=0){require(layout[n.affine].outputs==n.elements,"graph output geometry");
        U ni=3072;if(n.parents[0]){auto it=std::find_if(aqcf_backbone::graph.begin(),aqcf_backbone::graph.end(),[&](auto&v){return v.id==n.parents[0];});require(it!=aqcf_backbone::graph.end(),"graph parent");ni=it->elements;}
        require(layout[n.affine].inputs==ni,"graph input geometry");}
    std::cout<<"ASSETS_READY\n"<<std::flush;char command;require(::read(0,&command,1)==1&&command=='C',"controller connect");
    sci::IOPack io(party,port,"127.0.0.1");sci::OTPack ot(&io,party,true);aqcf2::Sci s(party,io,ot);s.configure_quotient(mobile_extension);
    aqcf2::WorkerLanes lanes(s,port,workers);for(unsigned i=1;i<workers;++i)lanes.at(i).configure_quotient(mobile_extension);
    BfvProgram he(s,aqcf2::M,layout,program.get(),false,true,"broadcast","packed-query","legacy","prescaled","symmetric-seeded","native");
    PortBackend backend(he,lanes,out/"nodes.jsonl");std::vector<U> x(3072);
    if(party==2)for(std::size_t j=0;j<x.size();++j){require(input[j]>=-12288&&input[j]<=12288,"input range");x[j]=aqcf2::residue(input[j],aqcf2::M);}
    input.clear();lanes.flush();const U setup_bytes=lanes.bytes();const double setup_seconds=elapsed(process_start);
    std::cout<<"READY\n"<<std::flush;require(::read(0,&command,1)==1&&command=='G',"controller start");
    const auto started=Steady::now();auto y=aqcf_backbone::execute(backend,aqcf_backbone::graph,x,aqcf_backbone::final_id);
    auto final=he.final100(y);lanes.flush();double total=elapsed(started);auto c=backend.coverage;
    require(c.nodes==99&&c.affines==53&&c.clips==35&&c.adds==10&&c.pools==1&&c.floor_elements==1612644&&c.clip_elements==1502208,"all native graph operations executed");
    require(he.reserved_returns==1442&&he.metrics.input_ct==he.expected_inputs&&he.metrics.output_ct==1442,"all transport/execution counts");
    if(party==2){require(final.size()==100,"full final scores");std::ofstream f(out/"FINAL100.i64",std::ios::binary);f.write(reinterpret_cast<const char*>(final.data()),800);require(bool(f),"public final result");}
    rusage ru{};require(getrusage(RUSAGE_SELF,&ru)==0,"resource receipt");
    std::ofstream r(out/"RESULT.json");r<<std::setprecision(17)<<"{\"status\":\"COMPLETE_ADMITTED_GRAPH_CHECK_SCOPE_FIELDS\",\"party\":"<<party<<",\"workers\":"<<workers
      <<",\"nodes\":99,\"affines\":53,\"clips\":35,\"full_process_seconds\":"<<elapsed(process_start)
      <<",\"setup_seconds\":"<<setup_seconds<<",\"graph_and_reconstruction_seconds\":"<<total
      <<",\"online_local_io_counter_delta\":"<<lanes.bytes()-setup_bytes<<",\"total_local_io_counter\":"<<lanes.bytes()
      <<",\"input_CT\":"<<he.metrics.input_ct<<",\"output_CT\":"<<he.metrics.output_ct<<",\"PM\":"<<he.metrics.pm
      <<",\"action_layers\":"<<he.metrics.action_layers<<",\"max_rss_kib\":"<<ru.ru_maxrss
      <<",\"maximum_nonlinear_workers\":"<<lanes.maximum_workers
      <<",\"service_raw_seconds\":"<<(party==2?he.service_delta.raw:0)
      <<",\"service_monotonic_seconds\":"<<(party==2?he.service_delta.monotonic:0)
      <<",\"service_realtime_seconds\":"<<(party==2?he.service_delta.realtime:0)
      <<",\"client_encryptions\":"<<he.metrics.encryptions<<",\"client_masked_decryptions\":"<<he.metrics.decryptions
      <<",\"fresh_zero_returns\":"<<he.metrics.fresh_zero<<",\"full_mask_coefficients\":"<<he.metrics.full_mask
      <<",\"ct_adds\":"<<he.metrics.ct_add<<",\"bias_adds\":"<<he.metrics.bias_add
      <<",\"core_flags\":{"<<"\"PREFIX\":"<<AQCF_ABLATE_PREFIX<<",\"EMITTER\":"<<AQCF_ABLATE_EMITTER<<",\"QUOTIENT\":"<<AQCF_ABLATE_QUOTIENT<<",\"SHORT\":"<<AQCF_ABLATE_SHORT<<",\"FUSION\":"<<AQCF_ABLATE_FUSION<<"}"
      <<",\"core_execution\":{"<<"\"quotient_eligible_layers\":"<<he.metrics.quotient_eligible_layers<<",\"short_action_layers\":"<<he.metrics.action_layers<<",\"short_weight_ntts\":"<<he.metrics.short_weight_ntts<<",\"full_weight_ntts\":"<<he.metrics.full_weight_ntts<<",\"fresh_zero_inverse_ntts\":"<<he.metrics.lifted.inverse_ntts<<",\"avoided_inverse_ntts\":"<<he.metrics.lifted.avoided_inverse_ntts<<",\"linked_low_elements\":"<<s.linked_low_elements<<",\"linked_high_elements\":"<<s.linked_high_elements<<",\"prepared_control_elements\":"<<s.prepared_control_elements<<",\"materialized_and_elements\":"<<s.materialized_and_elements<<",\"quotient_rows\":"<<s.quotient_rows<<",\"quotient_elided_bytes\":"<<s.quotient_elided_bytes<<",\"quotient_seed_bytes\":"<<s.quotient_seed_bytes<<",\"direct_emission_elements\":"<<(mobile_direct?backend.floor_emission_elements:0)<<",\"materialized_emission_elements\":"<<(mobile_direct?0:backend.floor_emission_elements)<<",\"b2a_elements\":"<<s.b2a_elements<<",\"mux_elements\":"<<s.mux_elements<<"}"
      <<",\"quarter_recovery_elements\":"<<he.quarter_rows<<",\"HE_phases_seconds\":{";
    bool first=true;for(auto&[name,value]:he.metrics.seconds){if(!first)r<<',';first=false;r<<'"'<<name<<"\":"<<value;}
    r<<"},\"nonlinear_phases_seconds\":{";first=true;
    for(auto&[name,value]:s.boundary_seconds){if(!first)r<<',';first=false;r<<'"'<<name<<"\":"<<value;}
    r<<"},\"private_model\":"<< (admitted.real?"true":"false") <<",\"real_image\":"<<(admitted.real?"true":"false")<<",\"conditional_security\":true}"<<'\n';require(bool(r),"complete receipt");
    std::cout<<"DONE\n"<<std::flush;alarm(0);return 0;
}catch(const std::exception&e){std::cerr<<"ADMITTED_FULL_PORT_FAIL "<<e.what()<<'\n';return 1;}}
