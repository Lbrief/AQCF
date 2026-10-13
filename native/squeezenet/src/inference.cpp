#include "squeezenet/core/squeezenet_thread_trace.hpp"
// Owner-bound conditional research role; PUBLIC and REAL scope explicit.
// Same numeric/cryptographic executable is publicly qualified before real use.
#include "squeezenet/engine/native_worker_runtime.hpp"
#include "squeezenet/engine/bfv_program.hpp"
#include "squeezenet/engine/worker_lanes.hpp"
#include "squeezenet/engine/backbone_boundary.hpp"
#include "squeezenet/core/backbone_admission.hpp"
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
#include "squeezenet/core/ring37_boundary.hpp"
#include "squeezenet/core/ring37_direct.hpp"
#include "squeezenet/core/sq_backend.hpp"
int main(int argc,char**argv){try{
    const char* allow=std::getenv("AQCF_CALLBACK_ALLOW");require(allow,"267 bound callback admission");std::ifstream af(allow);std::string address;
    while(af>>address)trace267::allowed.push_back(reinterpret_cast<trace267::Fn>(std::stoull(address,nullptr,16)));require(!trace267::allowed.empty(),"267 nonempty callback admission");
    trace267::event("PROCESS_ENTER");trace267::Finalizer trace_lifetime;

    require(argc==9,"explicit owner admission required");
    require(std::string(argv[7])=="PUBLIC"||std::string(argv[7])=="REAL","fresh pool range qualification modes");
    int party=std::stoi(argv[1]),port=std::stoi(argv[2]);unsigned workers=std::stoul(argv[3]);
    require((party==1||party==2)&&workers==4&&port>=45000&&port<49000,"public role allocation");
    const bool candidate281=aqcf_tree280::enabled();require(aqcf_sq266::enabled()&&aqcf_pool273::enabled(),"281 retains all qualified275 consumers in both arms");
    const char* k207=std::getenv("AQCF_KEY_SLICE207"),*k208=std::getenv("AQCF_WORD_SLICE208");require(k207&&k208&&std::string(k207)==std::string(k208)&&std::string(k207)=="1","267 bound supplier flags");
    const bool packed303=aqcf_wire288::enabled(),retained303=retain296::enabled(),joint303=aqcf_terminal302::enabled();require(packed303&&retained303,"303 keeps accepted wire288 and retained297 in both arms");std::cout<<"SQ303_CONFIGURATION "<<party<<" "<<joint303<<"\n";require(candidate281&&joint303,"317 strongest303 construction fixed in both arms");if(std::getenv("AQCF_COMPUTE4_LOCAL_PUBLIC"))require(std::string(argv[7])=="PUBLIC"&&!std::getenv("AQCF_COMPUTE4_CLOUD_PUBLIC"),"303 local resource switch only for public graph");
    const bool input379=aqcf_input379::enabled(),support402=aqcf_support402::enabled();const bool packing408=aqcf_packing408::enabled();const bool pipeline416=aqcf_pipeline416::enabled();const bool mux426=aqcf_pool426::enabled();require(pipeline416,"426 keeps qualified416 pipeline in both arms");require(packing408,"416 accepted408 packing in both arms");require(support402,"408 keeps qualified402 support projection in both arms");require(input379,"408 qualified rounded codec fixed in both arms");
    auto admitted=aqcf_backbone::admit(argc,argv,party,workers);if(admitted.real)require(input379&&support402&&packing408&&pipeline416&&mux426,"408 real scope uses qualified rounded input");if(admitted.real){const char* common=std::getenv("AQCF_COMMON317");require(candidate281&&packed303&&retained303&&joint303&&common&&std::string(common)=="1"&&!std::getenv("AQCF_COMPUTE4_LOCAL_PUBLIC"),"319 bound qualified317 construction only");}
    if(admitted.real){const char* gate=std::getenv("AQCF_SOURCE_BOUND_PRIVATE_GATE");require(gate && std::string(gate)=="QUALIFIED_SQ426_COMPLETE_MODEL","source-bound real qualification admission");}
    const auto process_start=Steady::now();std::signal(SIGALRM,[](int){::_exit(124);});alarm(1550);
    std::signal(SIGPIPE,SIG_IGN);omp_set_dynamic(0);omp_set_num_threads(1);
    aqcf_native4::initialize(party);rlimit no_core{0,0};require(setrlimit(RLIMIT_CORE,&no_core)==0,"no core dumps");
    auto out=std::filesystem::path(argv[6]);require(out.is_absolute()&&!std::filesystem::exists(out),"fresh public role output");std::filesystem::create_directory(out);
    auto layout=parse_layout(bound_bytes(argv[4],admitted.layout_sha,1ULL<<30));
    std::unique_ptr<Program> program;std::vector<I> input;
    if(party==1){program=std::make_unique<Program>(parse_program(bound_bytes(argv[5],admitted.asset_sha,1ULL<<30)));match_program(*program,layout);}
    else input=read_i64_asset(argv[5],admitted.asset_sha,154587);
    const U input_count408=packing408?510:641,return_count408=packing408?1277:1592,nominal_inputs408=packing408?630:761;
    U returns=0,inputs=0;for(auto&l:layout){returns+=l.queries.size();inputs+=l.maps.size();}
    require(BfvProgram::uniform_input_share&&returns==return_count408&&inputs==nominal_inputs408&&layout.size()==26,"complete bound SqNet layout geometry");
    if(program&&support402)aqcf_support402::certify(*program);
    std::cout<<"ASSETS_READY\n"<<std::flush;char command;require(::read(0,&command,1)==1&&command=='C',"controller connect");
    std::array<std::unique_ptr<aqcf2::ExtraLane>,4> owned_channels;
    pcn_data089::each("SQ_FRESH_INDEPENDENT_OT_SETUP",4,[&](std::size_t j){owned_channels[j]=std::make_unique<aqcf2::ExtraLane>(party,port+1000*j);owned_channels[j]->protocol.configure_quotient("quotient");});
    auto& s=owned_channels[0]->protocol;aqcf2::WorkerLanes lanes(s,owned_channels);
    for(unsigned i=0;i<workers;++i)lanes.at(i).compact_cofactor=true;
    BfvProgram he(s,BfvProgram::plaintext_modulus,layout,program.get(),false,true,"broadcast","packed-query","legacy","prescaled","symmetric-seeded","native");
    he.configure_linear_workers(workers,false);
    std::array<int,AFFINES> input_tensor_ids;input_tensor_ids.fill(-1);
    for(const auto& node:aqcf_sq::graph)if(node.affine>=0)input_tensor_ids[node.affine]=node.args[13];
    he.bind_uniform_request(argv[8],input_tensor_ids);
    std::array<U,4> recovery_lane_rows{};std::array<int,4> recovery_cpus{{-1,-1,-1,-1}};U recovery_calls=0;
    he.recovery_workers=[&](std::vector<U>& own){
      own=lanes.run(own,[&](aqcf2::Sci& sc,const std::vector<U>& part,std::size_t){
        int cpu=sched_getcpu();unsigned lane=0;while(lane<4&&aqcf_native4::cpu(lane)!=cpu)++lane;require(lane<4,"recovery physical CPU");
        std::vector<U> out=part;quarter_recover_all(sc,out);
        recovery_cpus[lane]=cpu;recovery_lane_rows[lane]+=part.size();return out;});++recovery_calls;};

    SqBackend backend(he,lanes,out/"nodes.jsonl");std::vector<U>x(154587,0);
    if(party==2)he.chunks4("CLIENT_INPUT_RESIDUE",-1,x.size(),[&](std::size_t a,std::size_t b){for(auto j=a;j<b;++j){
      require(input[j]>=-(1<<20)&&input[j]<=(1<<20),"original BGR input envelope");x[j]=aqcf2::residue(input[j],U(1)<<37);}});
    input.clear();backend.public_selftest();lanes.flush();
    #if AQCF_SILENTOT_CONTROL
    // Never turn the public selftest into free private-inference preprocessing.
    // Discard every pretest provider. Fresh independent state is instantiated
    // lazily during the measured graph, on its first actual protocol call.
    for(unsigned i=0;i<workers;++i){auto& p=lanes.at(i).silent_control;
      if(p)aqcf_silent_control::report(std::cout,"PUBLIC_SELFTEST_DISCARDED",i,*p);
      p.reset();}
    #endif
    require(aqcf_common317_reset()==0,"317 fresh measured providers after public selftests");
    he.wire_counter_snapshot=[&](){
      BfvProgram::WireCounters snap{};U sum=0;
      for(unsigned lane=0;lane<4;++lane){auto& io=lanes.at(lane).io;
        snap[3*lane]=io.io->counter;snap[3*lane+1]=io.io_rev->counter;snap[3*lane+2]=io.io_GC->counter;
        sum+=snap[3*lane]+snap[3*lane+1]+snap[3*lane+2];}
      require(sum==lanes.bytes(),"all four lanes and three original channels counted once");return snap;};
    lanes.flush();const U setup_bytes=lanes.bytes();const double setup_seconds=elapsed(process_start);
    std::cout<<"READY\n"<<std::flush;require(::read(0,&command,1)==1&&command=='G',"controller start");
    const auto started=Steady::now();auto shares=aqcf_sq::execute(backend,x);auto final=he.final1001(shares);lanes.flush();double total=elapsed(started);
    require(he.cipher_wire_finished,"first ciphertext through final protected1001 send-counter snapshots");
    std::cout<<"CIPHER_WIRE_COUNTERS "<<party<<" "<<he.cipher_wire_send_bytes();
    for(unsigned k=0;k<12;++k)std::cout<<" "<<he.cipher_wire_begin[k]<<":"<<he.cipher_wire_end[k];
    std::cout<<"\n"<<std::flush;
    require(backend.completed_nodes==117&&backend.affines==26,"complete original graph");
    require(he.metrics.action_layers==17,"all seventeen quotient pointwise kernels executed");
    const U expected_pm=packing408?30480:25528;
    he.verify_uniform_complete();
    for(unsigned lane=0;lane<4;++lane){uint64_t z[7]{};require(aqcf_poolmux426_stats(lanes.at(lane).control().owned189_handle(),z)==0,"426 counter receipt");
      const uint64_t expected[7]={12,512832,1025664,302848,209984,1981872,192};
      for(unsigned j=0;j<7;++j)require(z[j]==(mux426?expected[j]:0),"426 all public pool rows exactly once");
      std::cout<<"POOLMUX426_RECEIPT "<<party<<" "<<lane<<" "<<mux426;for(auto q:z)std::cout<<" "<<q;std::cout<<"\n";
    }

    require(he.pipeline416_layers==(pipeline416&&party==1?26:0)&&he.pipeline416_outputs==(pipeline416&&party==1?1277:0),"416 all fresh outputs pipelined exactly once");
    std::cout<<"PIPE416_RECEIPT "<<pipeline416<<" "<<he.pipeline416_layers<<" "<<he.pipeline416_outputs<<"\n";

    require(he.reserved_returns==return_count408&&he.metrics.input_ct==input_count408&&he.metrics.output_ct==return_count408&&he.metrics.pm==(party==1?expected_pm:0),"all transport and multiplication counts");
    if(party==2){require(final.size()==1001,"1000scores plus secure class");std::ofstream f(out/"FINAL1001.i64",std::ios::binary);f.write(reinterpret_cast<const char*>(final.data()),8008);require(bool(f),"full classification payload");}
    require(he.maximum_linear_workers==4,"actual HE worker count");
#if AQCF_FUSED_QUARTER_ENTRY
    require(recovery_calls==0&&he.quarter_rows==0,"no legacy recovery hidden inside fused path");
    require(backend.fused_phase_calls==26&&!backend.phase_pending,"all26 raw producers consumed");
    require(he.deferred_quarter_rows==2649992&&backend.fused_phase_rows==2649992,"all original recovery elements consumed");
    U fused_sum=0;
    for(unsigned i=0;i<4;++i){
      require(backend.fused_phase_lane_rows[i]>0&&backend.fused_phase_cpus[i]==aqcf_native4::cpu(i),"four actual fused recovery lanes");
      fused_sum+=backend.fused_phase_lane_rows[i];
    }
    require(fused_sum==2649992,"no dropped fused recovery tail");
    std::cout<<"FUSED_RECOVERY4_RECEIPT {\"calls\":"<<backend.fused_phase_calls<<",\"rows\":"<<fused_sum<<",\"cpus\":[";
    for(unsigned i=0;i<4;++i){if(i)std::cout<<",";std::cout<<backend.fused_phase_cpus[i];}
    std::cout<<"],\"lane_rows\":[";
    for(unsigned i=0;i<4;++i){if(i)std::cout<<",";std::cout<<backend.fused_phase_lane_rows[i];}
    std::cout<<"]}\n"<<std::flush;
#else
    require(recovery_calls==26,"every affine recovery on worker lanes");
    U recovery_sum=0;for(unsigned i=0;i<4;++i){require(recovery_lane_rows[i]>0&&recovery_cpus[i]==aqcf_native4::cpu(i),"all four secure recovery lanes executed");recovery_sum+=recovery_lane_rows[i];}
    require(recovery_sum==he.quarter_rows&&recovery_sum==2649992,"all recovery elements, no skipped tail");
    std::cout<<"RECOVERY4_RECEIPT {\"calls\":"<<recovery_calls<<",\"rows\":"<<recovery_sum<<",\"cpus\":[";
    for(unsigned i=0;i<4;++i){if(i)std::cout<<",";std::cout<<recovery_cpus[i];}std::cout<<"],\"lane_rows\":[";
    for(unsigned i=0;i<4;++i){if(i)std::cout<<",";std::cout<<recovery_lane_rows[i];}std::cout<<"]}\n"<<std::flush;
#endif
    require(he.narrowed_returns.load()==(BfvProgram::projected_output&&party==1?return_count408:0),"all projected returns accounted");
    require(he.wire288_encoded.load()==(packed303&&party==2?input_count408:0)&&he.wire288_decoded.load()==(packed303&&party==1?input_count408:0),"408 all actual input frames coded once");
    std::cout<<"PACKING408_RECEIPT "<<packing408<<" "<<input_count408<<" "<<return_count408<<" "<<expected_pm<<"\n";
    std::cout<<"INPUT379_RECEIPT "<<input379<<" "<<he.wire288_encoded.load()<<" "<<he.wire288_decoded.load()<<" "<<aqcf_input379::frame_size()<<"\n";
    require(he.support402_frames==(packing408?368:145)&&he.support402_coefficients==(packing408?921216:453760)&&he.support402_saved_bytes==(packing408?8175792:4027120),"408 complete public omission census");
    std::cout<<"SUPPORT402_RECEIPT "<<support402<<" "<<he.support402_frames.load()<<" "<<he.support402_coefficients.load()<<" "<<he.support402_saved_bytes.load()<<"\n";
    const U retained_count=retained303&&party==1?return_count408:0;
    require(he.retained303_returns==retained_count&&he.retained303_coefficients==8192*retained_count&&he.retained303_queries==(retained_count?2649992:0),"303 complete retained component census");
    std::cout<<"RETAINED303_RECEIPT "<<retained303<<" "<<he.retained303_returns<<" "<<he.retained303_coefficients<<" "<<he.retained303_queries<<" "<<he.retained303_saved_bytes<<"\n";
    std::cout<<"PROJECTED_OUTPUT_RECEIPT "<<BfvProgram::projected_output<<" "<<he.narrowed_returns.load()<<"\n"<<std::flush;
    std::cout<<"UNIFORM_INPUT_RECEIPT "<<BfvProgram::uniform_input_share<<" "<<he.uniform_shared_pairs<<" "<<he.uniform_saved_inputs<<" "<<he.uniform_metadata_bytes<<"\n"<<std::flush;
    std::cout<<"HE_WORKER_RECEIPT ";for(auto x:he.linear_worker_cpus)std::cout<<x<<",";std::cout<<"\n";
    std::cout<<"NL_WORKER_RECEIPT ";for(auto x:lanes.observed_cpu)std::cout<<x<<",";std::cout<<"\n"<<std::flush;
    U cc=0,cr=0;for(unsigned i=0;i<workers;++i){cc+=lanes.at(i).compact_response_calls;cr+=lanes.at(i).compact_response_rows;}
    require(cc>0&&cr>0,"frozen compact mechanism executed");
    std::cout<<"COMPACT_RECEIPT "<<cc<<" "<<cr<<"\n"<<std::flush;
    rusage ru{};require(getrusage(RUSAGE_SELF,&ru)==0,"resource receipt");
    std::ofstream r(out/"RESULT.json");r<<std::setprecision(17)<<"{\"status\":\"COMPLETE_ADMITTED_GRAPH_CHECK_SCOPE_FIELDS\",\"party\":"<<party<<",\"workers\":"<<workers
      <<",\"nodes\":117,\"affines\":26,\"clips\":0,\"relus\":26,\"full_process_seconds\":"<<elapsed(process_start)
      <<",\"setup_seconds\":"<<setup_seconds<<",\"graph_and_reconstruction_seconds\":"<<total
      <<",\"online_local_io_counter_delta\":"<<lanes.bytes()-setup_bytes<<",\"total_local_io_counter\":"<<lanes.bytes()
      <<",\"first_cipher_to_final_payload_local_send_bytes\":"<<he.cipher_wire_send_bytes()
      <<",\"cipher_counter_scope\":\"12_send_counters_first_input_frame_through_final_protected_payload_no_IP_SSH_overhead\""
      <<",\"input_CT\":"<<he.metrics.input_ct<<",\"output_CT\":"<<he.metrics.output_ct<<",\"PM\":"<<he.metrics.pm
      <<",\"action_layers\":"<<he.metrics.action_layers<<",\"max_rss_kib\":"<<ru.ru_maxrss
      <<",\"maximum_linear_workers\":"<<he.maximum_linear_workers<<",\"linear_parallel_outputs\":"<<he.parallel_outputs
      <<",\"compact_response_calls\":"<<cc<<",\"compact_response_rows\":"<<cr
      <<",\"maximum_nonlinear_workers\":"<<lanes.maximum_workers
      <<",\"service_raw_seconds\":"<<(party==2?he.service_delta.raw:0)
      <<",\"service_monotonic_seconds\":"<<(party==2?he.service_delta.monotonic:0)
      <<",\"service_realtime_seconds\":"<<(party==2?he.service_delta.realtime:0)
      <<",\"client_encryptions\":"<<he.metrics.encryptions<<",\"client_masked_decryptions\":"<<he.metrics.decryptions
      <<",\"fresh_zero_returns\":"<<he.metrics.fresh_zero<<",\"full_mask_coefficients\":"<<he.metrics.full_mask
      <<",\"ct_adds\":"<<he.metrics.ct_add<<",\"bias_adds\":"<<he.metrics.bias_add
      <<",\"quarter_recovery_elements\":"<<he.quarter_rows
      <<",\"deferred_quarter_elements\":"<<he.deferred_quarter_rows<<",\"fused_recovery_elements\":"<<backend.fused_phase_rows
      <<",\"fused_recovery_calls\":"<<backend.fused_phase_calls<<",\"HE_phases_seconds\":{";
    bool first=true;for(auto&[name,value]:he.metrics.seconds){if(!first)r<<',';first=false;r<<'"'<<name<<"\":"<<value;}
    r<<"},\"nonlinear_phases_seconds\":{";first=true;
    for(auto&[name,value]:s.boundary_seconds){if(!first)r<<',';first=false;r<<'"'<<name<<"\":"<<value;}
    r<<"},\"private_model\":"<< (admitted.real?"true":"false") <<",\"real_image\":"<<(admitted.real?"true":"false")<<",\"conditional_security\":true}"<<'\n';require(bool(r),"complete receipt");
    #if AQCF_SILENTOT_CONTROL
    for(unsigned i=0;i<workers;++i){auto& p=lanes.at(i).silent_control;
      require(bool(p)&&p->stats().lookup_rows>0,"SilentOT used by every owning lane");
      auto q2=p->quotient169_stats();auto q3=p->quotient197_stats();uint64_t seed2=0,seed3=0,ec=0,er=0,en=0,eb=0,eo=0;
      require(aqcf_seed221_read_stats(p->owned189_handle(),&seed2,&seed3)==0&&aqcf_elide249_stats(p->owned189_handle(),&ec,&er,&en,&eb,&eo)==0,"267 all native banks consumed once");
      U rows=backend.fused_phase_lane_rows[i];require(q2.prepared_rows==(rows-(candidate281?53576:0))&&q3.prepared_rows==(11*(rows-(candidate281?53576:0))+7*512832)&&q2.consumed_rows==q2.prepared_rows&&q3.consumed_rows==q3.prepared_rows,"267 complete SQ prefix census");
      require(q2.native_rot_rows==3*q2.prepared_rows&&q3.native_rot_rows==4*q3.prepared_rows&&seed2==q2.prepared_rows&&seed3==q3.prepared_rows,"267 no unpaid supplier");
      uint64_t jpc=0,jpr=0,jcc=0,jcr=0;
      require(aqcf_joint302_stats(p->owned189_handle(),&jpc,&jpr,&jcc,&jcr)==0,"303 complete joint bank closure");
      require(jpc==(joint303?16:0)&&jcc==jpc&&jpr==(joint303?rows-53576:0)&&jcr==jpr,"303 fixed16 serial shape tap accounting");
      require(ec==51&&en==3*rows-204304-(joint303?rows-53576:0),"303 terminal chosen OT rows are fully paid");
      std::cout<<"SQ303_JOINT "<<i<<" "<<jpc<<" "<<jpr<<" "<<jcc<<" "<<jcr<<"\n";
      std::cout<<"SQ281_NATIVE "<<i<<" "<<rows<<" "<<seed2<<" "<<seed3<<" "<<ec<<" "<<er<<" "<<en<<" "<<eb<<" "<<eo<<"\n";
      aqcf_silent_control::report(std::cout,"MEASURED_FRESH_GRAPH",i,*p);}
    std::cout<<"LEGACY_AUX_RECEIPT "<<s.and_elements<<" "<<s.b2a_elements<<" "
      <<s.compare_elements<<" "<<s.late_nco_rows<<"\n";
    #endif
    for(unsigned lane=0;lane<4;++lane){uint64_t values[28]{};require(aqcf_common317_read(lane,values)==0,"317 complete model phase demand and FIFO closure");
      std::cout<<"COMMON317 "<<party<<" "<<lane;for(auto v:values)std::cout<<" "<<v;std::cout<<"\n";}
    std::cout<<"DONE\n"<<std::flush;alarm(0);return 0;
}catch(const std::exception&e){std::cerr<<"ADMITTED_FULL_PORT_FAIL "<<e.what()<<'\n';return 1;}}
