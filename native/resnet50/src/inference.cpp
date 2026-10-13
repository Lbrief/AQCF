#include "resnet50/engine/resnet_thread_trace.hpp"
extern "C" std::uint64_t aqcf_pacer036_bytes();
extern "C" std::uint64_t aqcf_pacer036_calls();
extern "C" std::uint64_t aqcf_pacer036_chunks();
extern "C" std::uint64_t aqcf_pacer036_wait_ns();
extern "C" std::uint64_t aqcf_profile170_rate();
struct Trace37Finalizer { ~Trace37Finalizer(){aqcf_owner4::instance.reset();trace037::event("PROCESS_EXIT");} };
// Complete owner-bound HE-MPC ResNet50, conditional research only.
// PUBLIC and one-use REAL modes use identical arithmetic and cryptography.
#include "resnet50/engine/native_worker_runtime.hpp"
#include "resnet50/engine/bfv_program.hpp"
#include "resnet50/engine/polyphase_convolution.hpp"
#include "resnet50/core/worker_lanes.hpp"
#include "resnet50/engine/ring37_boundary.hpp"
#include "resnet50/engine/ring37_direct.hpp"
#include "resnet50/engine/r50_schedule.hpp"
#include "resnet50/engine/rhombus_admission.hpp"
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
extern "C" int aqcf_slice207_read_stats(const aqcf_silent_handle*,std::uint64_t*,std::uint64_t*,int*);
extern "C" int aqcf_word208_read_stats(const aqcf_silent_handle*,std::uint64_t*,std::uint64_t*,std::uint64_t*,int*);
#include "resnet50/engine/r50_backend.hpp"
#include "resnet50/engine/pointwise_core.hpp"
#include <openssl/aes.h>
U mix(U x){x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;return x^(x>>31);}
void hash_check(){
 struct alignas(32) K{sci::block256 v;};struct alignas(16) P{sci::block128 v;};
 U checked=0;for(std::size_t n:{1,2,3,7,8,9,15,16,17,255,256,257,4096}){
  std::vector<K>keys(n);std::vector<P>native(n),fused(n);
  for(std::size_t j=0;j<n;++j){U w[4];for(unsigned i=0;i<4;++i)w[i]=mix(j*4+i+97*n);std::memcpy(&keys[j].v,w,32);}
  sci::CCRF(reinterpret_cast<sci::block128*>(native.data()),reinterpret_cast<sci::block256*>(keys.data()),int(n));
  aqcf_tuple::streaming_ccrf(reinterpret_cast<sci::block128*>(fused.data()),reinterpret_cast<sci::block256*>(keys.data()),n);
  require(!std::memcmp(native.data(),fused.data(),n*16),"native CCRF exact byte equivalence");
  require(aqcf_tuple::have_parallel_aes(),"hardware instruction available");
  aqcf_tuple::parallel_ccrf(reinterpret_cast<sci::block128*>(fused.data()),reinterpret_cast<sci::block256*>(keys.data()),n);
  require(!std::memcmp(native.data(),fused.data(),n*16),"paired-state native CCRF exact byte equivalence");
  for(std::size_t j=0;j<n;++j){AES_KEY key;unsigned char answer[16];require(AES_set_encrypt_key(reinterpret_cast<unsigned char*>(&keys[j].v),256,&key)==0,"OpenSSL key");AES_encrypt(reinterpret_cast<const unsigned char*>(&sci::one),answer,&key);require(!std::memcmp(answer,&fused[j].v,16),"OpenSSL AES256 exact equivalence");}
  checked+=n;
 }
 std::cout<<"EXACT_RESPONSE_CCRF_CHECK "<<checked<<std::endl;
}
void compact_check(){
 U checks=0;
 for(unsigned d=1;d<=4;++d)for(unsigned a=0;a<(1U<<d);++a)for(unsigned r=0;r<2;++r)for(unsigned nx=0;nx<2;++nx)for(unsigned p0=0;p0<2;++p0)for(unsigned p1=0;p1<2;++p1){
  U w=aqcf_cofactor_word::pair(a,r,nx,p0,p1,d);
  for(unsigned c=0;c<(1U<<d);++c){U e=(((a+c+r)>>d)^nx^p0)|((((a+c+(r^1))>>d)^nx^p1)<<1);require(((w>>(2*c))&3)==e,"closed cofactor identity");++checks;}
 }
 for(unsigned N:{4,8,16,32})for(unsigned bits:{1,2,4})if(N*bits<=64)for(U n:{1,2,3,7,17,31,32,33,255,256,257}){
  struct alignas(16) P{sci::block128 v;};std::vector<P>pads(n*N);std::vector<U>dense(n*N),packed(n),a((n*N*bits+63)/64),z(a.size());std::vector<U*>ptr(n);
  for(U j=0;j<n;++j){ptr[j]=dense.data()+j*N;for(unsigned c=0;c<N;++c){U v=mix(j*N+c)&aqcf2::mask(bits);dense[j*N+c]=v;packed[j]|=v<<(c*bits);pads[j*N+c].v=_mm_set_epi64x(mix(j*N+c+17),mix(j*N+c+31));}}
  sci::pack_ot_messages<U>(a.data(),ptr.data(),reinterpret_cast<sci::block128*>(pads.data()),a.size(),n,bits,N);
  aqcf_cofactor_word::pack(z.data(),packed.data(),reinterpret_cast<sci::block128*>(pads.data()),n,N,bits);
  require(a==z,"identical canonical encrypted response bytes");checks+=n*N;
 }
 std::cout<<"COMPACT_ENCODING_CHECK "<<checks<<std::endl;
}


extern "C" int aqcf_packed354_stats(const aqcf_silent_handle*,uint64_t*);
int main(int argc,char**argv){try{
    const char* allow=std::getenv("AQCF_CALLBACK_ALLOW");aqcf3::require(allow,"bound production37 callback admission");
    std::ifstream af(allow);std::string address;while(af>>address)trace037::allowed.push_back(reinterpret_cast<trace037::Fn>(std::stoull(address,nullptr,16)));aqcf3::require(!trace037::allowed.empty(),"nonempty callback admission");
    trace037::event("PROCESS_ENTER");Trace37Finalizer trace_lifetime;
    require(argc==9,"explicit role-owned admission required");
    int party=std::stoi(argv[1]),port=std::stoi(argv[2]);unsigned workers=std::stoul(argv[3]);
    require((party==1||party==2)&&workers==4&&port>=45000&&port<49000,"role allocation");
    const char* a207=std::getenv("AQCF_KEY_SLICE207"),*a208=std::getenv("AQCF_WORD_SLICE208");
    require(a207&&a208&&(std::string(a207)=="0"||std::string(a207)=="1")&&std::string(a207)==a208&&!aqcf2::radix197_enabled(),"209 fixed paired flags and strongest169 schedule");
    const int slices209=a207[0]-'0';std::cout<<"SLICE209_CONFIGURATION "<<party<<" "<<slices209<<"\n";
    const bool retained307flag=retained307::enabled();std::cout<<"RETAINED307_CONFIGURATION "<<party<<" "<<retained307flag<<"\n";
    const bool joint305=aqcf_terminal304::enabled();require(joint305,"307 qualified304 in both arms");std::cout<<"JOINT305_CONFIGURATION "<<party<<" "<<joint305<<"\n";
    const bool retained=retained299::enabled();require(retained,"305 retains qualified299 in both arms");std::cout<<"RETAINED299_CONFIGURATION "<<party<<" "<<retained<<"\n";
    const bool carry295=pool295::enabled();require(carry295,"299 qualified295 pool in both arms");std::cout<<"POOL295_CONFIGURATION "<<party<<" "<<carry295<<"\n";
    const bool horner=horner293::enabled();require(horner,"295 accepted Horner293 in both arms");std::cout<<"HORNER293_CONFIGURATION "<<party<<" "<<horner<<"\n";
    const bool pool258=aqcf_pool258::enabled();require(pool258,"293 retains accepted258 pooling");std::cout<<"POOL258_CONFIGURATION "<<party<<" "<<pool258<<"\n";
    const bool elide250=aqcf_elide250::enabled();require(elide250,"258 retains qualified250 terminal in both arms");std::cout<<"ELIDE250_CONFIGURATION "<<party<<" "<<elide250<<"\n";
    const bool mixed238=aqcf_mixed238::enabled();require(mixed238,"250 mixed237 control retained in both arms");std::cout<<"MIXED238_CONFIGURATION "<<party<<" "<<mixed238<<"\n";
    const bool programmed213=aqcf_input213::enabled();require(slices209==1,"213 strongest accepted207/208 in both arms");
    std::cout<<"INPUT213_CONFIGURATION "<<party<<" "<<programmed213<<"\n";
    auto admitted=aqcf_rhombus::admit(argc,argv,party,workers);
    const char*cohort364=std::getenv("AQCF_COHORT364");require(cohort364&&(std::string(cohort364)=="0"||std::string(cohort364)=="1"),"364 explicit cohort");
    const bool direction359flag=direction359::enabled();
    require(direction359flag&&(cohort364[0]=='1'),"371 both arms fixed qualified364 direction and pairing");
    std::cout<<"INPUT371_CONFIGURATION "<<party<<" "<<aqcf_input371::enabled()<<"\n";
    require(direction359flag==(cohort364[0]=='1'),"364 fixed paired-role candidate, no retuning");
    std::cout<<"COHORT364_CONFIGURATION "<<party<<" "<<cohort364<<"\n";
    if(admitted.real)require(direction359flag,"359 real candidate only");
    std::cout<<"DIRECTION359_CONFIGURATION "<<party<<" "<<direction359flag<<"\n"<<std::flush;
    const char* packed_env=std::getenv("AQCF_PACKED353");
    require(packed_env&&(std::string(packed_env)=="0"||std::string(packed_env)=="1"),"354 explicit fixed codec");
    const int packed354=packed_env[0]=='1';
    require(packed354==1,"359 preserves strongest packed353 in both arms");
    std::cout<<"PACKED354_CONFIGURATION "<<party<<" "<<packed354<<"\n"<<std::flush;
    if(admitted.real)require(elide250&&pool258&&horner&&carry295,"295 private candidate only");
    if(admitted.real)require(programmed213,"213 candidate-only private boundary");
    if(admitted.real)require(slices209==1,"209 private candidate only");
    const char* correction342_flag=std::getenv("AQCF_CORRECTION341");
    require(correction342_flag&&(std::string(correction342_flag)=="0"||std::string(correction342_flag)=="1"),"342 explicit correction arm");
    const bool correction342=correction342_flag[0]=='1';
    const bool duplex336=candidate336::enabled();std::cout<<"DUPLEX336_CONFIGURATION "<<party<<" "<<duplex336<<"\n";
    if(admitted.real)require(policy343::enabled()&&correction342&&duplex336&&retained307flag&&joint305&&retained&&std::getenv("AQCF_COMMON315")&&std::string(std::getenv("AQCF_COMMON315"))=="1"&&!std::getenv("AQCF_COMPUTE4_LOCAL_PUBLIC"),"325 only qualified315 fixed candidate in real mode");
    if(admitted.real)require(std::getenv("AQCF_SOURCE_BOUND_PRIVATE_GATE")&&std::string(std::getenv("AQCF_SOURCE_BOUND_PRIVATE_GATE"))=="QUALIFIED_R50_515_COMPLETE_MODEL","fresh source-bound BN191 complete public qualification and separate one-use private admission required");
    if(admitted.real)require(std::getenv("AQCF_BN191")&&std::string(std::getenv("AQCF_BN191"))=="1","private191 candidate only");
    if(admitted.real)require(std::getenv("AQCF_QUOTIENT169")&&std::string(std::getenv("AQCF_QUOTIENT169"))=="1","REAL170 is reserved for the newly qualified candidate");
    // New R50 same-ELF PUBLIC/REAL experiment; the controller requires fresh source-bound budget and PUBLIC exactness.
    const bool poly458=aqcf_polyphase458::enabled();require(poly458,"467 retains qualified458 first affine in both arms");
    const bool pipe467=pipeline467::enabled();require(pipe467,"500 keeps accepted467 pipeline in both arms");
    const bool d500=dot500::enabled();require(d500,"515 retains accepted501 FMA arithmetic");
    const bool m515=output_merge515::enabled();if(admitted.real)require(m515,"515 real candidate only");
    std::cout<<"MERGE515_CONFIGURATION "<<party<<" "<<m515<<"\n";
    std::cout<<"DOT500_CONFIGURATION "<<party<<" "<<d500<<"\n";
    std::cout<<"PIPE467_CONFIGURATION "<<party<<" "<<pipe467<<"\n";
    const U ni458=poly458?3290:3350,no458=poly458?3132:3557;
    const U pm458=poly458?(party==1?387660ULL:387660ULL):387783ULL;
    require(std::string(std::getenv("AQCF_POOLFLOOR444"))=="1","458 keeps qualified444 pool/FIFO in BOTH arms");
    std::cout<<"POLYPHASE458_CONFIGURATION "<<party<<" "<<poly458<<" "<<ni458<<" "<<no458<<" "<<pm458<<"\n";
    const auto process_start=Steady::now();std::signal(SIGALRM,[](int){::_exit(124);});alarm(3600);
    std::signal(SIGPIPE,SIG_IGN);omp_set_dynamic(0);omp_set_num_threads(1);
    aqcf_native4::initialize(party);rlimit lim{0,0};require(setrlimit(RLIMIT_CORE,&lim)==0,"no dumps");
    const auto out=std::filesystem::path(argv[6]);
    require(out.is_absolute()&&!std::filesystem::exists(out),"fresh explicit paths");std::filesystem::create_directory(out);
    auto layout=parse_layout(bound_bytes(argv[4],admitted.layout_sha,4ULL<<30));
    std::unique_ptr<Program>program;
    std::vector<I> input;
    if(party==1){program=std::make_unique<Program>(parse_program(bound_bytes(argv[5],admitted.asset_sha,4ULL<<30)));match_program(*program,layout);}
    else input=read_i64_asset(argv[5],admitted.asset_sha,150528);
    // Same read-only preflight in BOTH binaries. Public plan fixes support;
    // a private mismatch rejects before traffic, never selects a fallback.
    if(program)pcn_data089::each("AQCF_OWNER_SUPPORT",72,[&](std::size_t id){for(unsigned oi=0;oi<program->at(id).output.size();++oi){
        const auto rule=r50_stride(id,oi);if(!rule.stride)continue;
        for(const auto& edge:program->at(id).output[oi].edges)for(const auto& c:edge.coefficients)
            require(c.exponent%rule.stride==0&&c.exponent/rule.stride<rule.support,"source-bound stride support");
    }});
    if(program)pcn_data089::each("AQCF_OWNER_SUPPORT",72,[&](std::size_t id){for(unsigned oi=0;oi<program->at(id).output.size();++oi){
        const auto z=r50_spatial(id,oi);if(!z.ell)continue;
        for(const auto& edge:program->at(id).output[oi].edges)for(const auto& c:edge.coefficients){
            U v=c.exponent%z.ell;
            require(c.exponent/z.ell<z.channels&&v/z.row<z.k&&v%z.row<z.k,"source-bound public spatial support");
        }
    }});
    U returns=0,inputs=0;for(auto&l:layout){returns+=l.queries.size();inputs+=l.maps.size();}
    require(returns==6672-425*poly458&&inputs==9226-60*poly458&&layout.size()==72,"all72 native trained affines");
    std::cout<<"ASSETS_READY\n"<<std::flush;char command;require(::read(0,&command,1)==1&&command=='C',"controller connect");
    std::array<std::unique_ptr<aqcf2::ExtraLane>,4> owned_channels;
    pcn_data089::each("AQCF_FRESH_INDEPENDENT_OT_SETUP",4,[&](std::size_t j){owned_channels[j]=std::make_unique<aqcf2::ExtraLane>(party,port+1000*j);owned_channels[j]->protocol.configure_quotient("quotient");});
    auto& s=owned_channels[0]->protocol;aqcf2::WorkerLanes lanes(s,owned_channels);
    for(unsigned i=0;i<workers;++i)lanes.at(i).compact_cofactor=true;
    BfvProgram he(s,U(1)<<37,layout,program.get(),false,true,"broadcast","packed-query","legacy","prescaled","symmetric-seeded","native");
    he.configure_linear_workers(workers,false);
    he.correction341_enabled=correction342;
    he.all_sent4=[&](){return lanes.bytes();};
    he.recover_four=[&](std::vector<U>& v){v=lanes.run(v,[&](aqcf2::Sci& lane,const std::vector<U>& part,std::size_t){auto r=part;quarter_recover_all(lane,r);return r;});}; // PUBLIC uses actual band/window producer; independent full-output oracle remains mandatory.
    std::unique_ptr<pw25::Dense> dense28;
    he.selected28=[](unsigned id){for(const auto&x:pw25::shapes)if(x.id==id)return true;return false;};
    he.initialize28=[&]{dense28=std::make_unique<pw25::Dense>(he);};
    he.count28=[&](unsigned id){const auto&x=pw25::shapes[dense28->index_of(id)];return (x.Co+x.C-1)/x.C;};
    he.size28=[&](unsigned id,U group){const auto&x=pw25::shapes[dense28->index_of(id)];aqcf_bn19::Geometry g{unsigned(id),unsigned(x.L),unsigned(x.Co),1,unsigned(x.C)};return aqcf_bn19::return_size(g,group,retained);};
    he.produce28=[&](unsigned id,const std::vector<U>&v){return dense28->prepare(dense28->index_of(id),v);};
    he.correction28=[&](unsigned id,const std::vector<U>&v){return dense28->correction(id,v);};
    he.decode28=[&](unsigned id,std::vector<U>&v,std::vector<std::vector<unsigned char>>&f){dense28->decode(dense28->index_of(id),v,f);};
    R50Backend b(he,lanes,out/"nodes.jsonl");std::vector<U>x(150528,0);
    if(party==2)pcn_data089::each("AQCF_OWNER_INPUT",x.size(),[&](std::size_t j){require(input[j]>=-1048576&&input[j]<=1048576,"official RGB Q12 preprocessing envelope");x[j]=aqcf2::residue(input[j],aqcf2::M);});
    input.clear();
    if(!admitted.real){hash_check();compact_check();b.public_selftest();}

#if AQCF_SILENTOT_CONTROL
    // Same-session public selftest states must not prewarm a measured graph.
    for(unsigned i=0;i<workers;++i){auto& p=lanes.at(i).silent_control;
      if(p)aqcf_silent_control::report(std::cout,"PUBLIC_SELFTEST_DISCARDED",i,*p);
      p.reset();}
#endif
    pcn_data089::each("AQCF_COMMON315_RESET",4,[&](std::size_t j){require(aqcf_common315_reset(j)==0,"315 fresh measured counters after public states discarded");});
    pcn_data089::each("AQCF_DIRECTION359_RESET",4,[&](std::size_t){direction359::census={};});
    // Public qualification state is never reused by a separately admitted real process.
    aqcf_native4::restore(); // public fixed arithmetic fixtures, never private intermediates
    lanes.flush();const U setup_bytes=lanes.bytes();const double setup=elapsed(process_start);
    U compact_setup_calls=0,compact_setup_rows=0;for(unsigned i=0;i<workers;++i){compact_setup_calls+=lanes.at(i).compact_response_calls;compact_setup_rows+=lanes.at(i).compact_response_rows;}
    std::cout<<"READY\n"<<std::flush;require(::read(0,&command,1)==1&&command=='G',"controller start");
    const auto retained_before299=retained299::census();
    he.first_frame4(); // Same paid boundary before all graph preparation in both configurations.
    auto finalshare=aqcf_r50::execute(b,x);auto final=he.final100(finalshare);lanes.flush();
    auto retained_after299=retained299::census();for(unsigned i=0;i<6;++i)retained_after299[i]-=retained_before299[i];
    const bool retaining299=party==1&&retained;
    require(retained_after299[0]==(retaining299?1205-425*poly458:0)&&retained_after299[1]==(retaining299?2183657:0)&&retained_after299[2]==(retaining299?1424:0)&&retained_after299[3]==(retaining299?8931328:0)&&retained_after299[4]==(retaining299?(2629ULL-425*poly458)*8192:0),"299 complete retained c0 public roster; BN191 excluded");
    std::cout<<"RETAINED299_CENSUS "<<party<<" "<<retained;for(auto v:retained_after299)std::cout<<" "<<v;std::cout<<"\n";
    require(b.completed_nodes==180&&b.affines==72,"all official ResNet50 operations including ArgMax");
    require(!b.phase_pending&&b.phase_calls==49&&b.phase_rows==9006592&&he.raw_phase_rows==b.phase_rows+b.residual_rows+b.pool_phase_rows&&b.residual_calls==16&&b.residual_rows==5519360&&!b.residual_pending,"unique phase accounting");
    require(he.quarter_rows+he.raw_phase_rows==16935401,"every original restoration accounted; no skipped noneligible operator");
    require(he.reserved_returns==no458&&he.metrics.input_ct==ni458&&he.metrics.output_ct==no458,"BN19 replacement complete HE counts");he.census19();
    require(b.pending191<0,"191 no live pending output");
    require(he.enabled191?(b.calls191==18&&b.rows191==5820416&&he.late_layers191==18&&he.late_rows191==5820416):(b.calls191==0&&he.late_layers191==0),"191 complete selected caller census");
    for(auto& state:he.state191)require(!state,"191 no retained affine state");
    U jointrows191=0,rejected191=0;for(unsigned k=0;k<4;++k){U calls=0,rows=0;require(aqcf_silent_joint26_stats(lanes.at(k).control().owned189_handle(),&calls,&rows)==0,"191 native joint stats");
      require(he.enabled191&&!retained307flag?(calls>0&&rows==b.joint_attempts191[k]):(calls==0&&rows==0),"191 exact fresh native attempts");jointrows191+=rows;rejected191+=b.joint_rejects191[k];
      std::cout<<"BN191_OWNER "<<party<<" "<<k<<" "<<he.input_attempts191[k]<<" "<<calls<<" "<<rows<<" "<<b.joint_rejects191[k]<<"\n";}
    require(jointrows191==(retained307flag?0:b.rows191)+rejected191,"191 native accepted/rejected ledger");
    std::cout<<"BN191_CENSUS "<<party<<" "<<b.calls191<<" "<<b.rows191<<" "<<jointrows191<<" "<<rejected191<<" "<<he.late_payload191<<"\n"<<std::flush;
    require(b.enabled216?(b.calls216==16&&b.rows216==5519360):(b.calls216==0&&b.rows216==0),"216 full original residual census");
    require(b.enabled216,"222 strongest217 terminal in both arms");if(admitted.real)require(b.enabled221&&b.enabled233,"233 candidate-only private boundary");
    std::cout<<"FLOOR216_CENSUS "<<party<<" "<<b.enabled216<<" "<<b.calls216<<" "<<b.rows216<<"\n"<<std::flush;
    require(b.phase_calls+b.residual_calls==65&&b.phase_rows+b.residual_rows==14525952,"221 complete original nonlinear consumer coverage");
    std::cout<<"SEED221_CENSUS "<<party<<" "<<b.enabled221<<" "<<(b.enabled221?65:0)<<" "<<(b.enabled221?14525952:0)<<"\n";
    require(b.pool38_lift_elements==802816*(1-b.enabled444)&&b.pool38_max_elements==(pool258?1204224:1605632)*(1-b.enabled444)&&b.pool255_maximum_calls==(pool258?4:8),"258 exact full signed pool DAG census");
    std::cout<<"POOL258_CENSUS "<<party<<" "<<pool258<<" "<<b.pool255_maximum_calls<<" "<<b.pool38_lift_elements<<" "<<b.pool38_max_elements<<"\n";
    require(!b.pool_phase_pending&&!b.pool_quotient_pending&&b.pool_phase_calls==b.enabled444&&b.pool_phase_rows==802816*b.enabled444&&b.pool26_lift_elements==802816*b.enabled444&&b.pool26_max_elements==1204224*b.enabled444&&b.pool_restore_calls==b.enabled444,"444 complete one-use graph splice");
    std::cout<<"POOL444_CENSUS "<<party<<" "<<b.enabled444<<" "<<b.pool_phase_calls<<" "<<b.pool_phase_rows<<" "<<b.pool26_lift_elements<<" "<<b.pool26_max_elements<<" "<<b.pool_restore_calls<<"\n";
    std::cout<<"POOL444_DIRECTION "<<party;for(unsigned lane=0;lane<4;++lane){require(b.pool444_reverse[lane]==U(b.enabled444&&(lane%2)),"444 fixed public direction mapping");std::cout<<" "<<b.pool444_reverse[lane];}std::cout<<"\n";
    require(b.pool230_calls==1&&(!b.enabled233||(b.layer233_calls>0&&b.layer233_rows>14525952)),"233 complete fixed execution coverage");
    std::cout<<"FRONTIER233_CENSUS "<<party<<" "<<b.enabled233<<" "<<b.layer233_calls<<" "<<b.layer233_rows<<" "<<b.pool230_calls<<"\n";
    require(bool(dense28),"dense initialized after first input");dense28->census();aqcf_input371::census(party);pipeline467::census(party);dot500::census(party);output_merge515::census(party);
    const bool c341=party==1&&correction342;
    require(he.correction341_pw_calls==(c341?36:0)&&he.correction341_bn_calls==(c341?18:0)&&he.correction341_pw_rows==(c341?8931328:0)&&he.correction341_bn_rows==(c341?5820416:0),"342 complete exact correction roster");
    std::cout<<"CORRECTION342_CENSUS "<<party<<" "<<correction342<<" "<<he.correction341_pw_calls<<" "<<he.correction341_bn_calls<<" "<<he.correction341_pw_rows<<" "<<he.correction341_bn_rows<<"\n";

    require(he.metrics.pm==(party==1?pm458:0ULL),"role-specific server multiplication count");
    if(party==2){require(final.size()==1002,"all1001 scores and protected ArgMax");std::ofstream f(out/"FINAL1002.i64",std::ios::binary);f.write(reinterpret_cast<const char*>(final.data()),8016);require(bool(f),"complete final result");}
    std::cout<<"CLIENT_INPUT_PARALLEL_RECEIPT workers="<<he.client_input_max_workers<<" layers="<<he.client_input_parallel_layers<<" frames="<<he.client_input_parallel_frames<<" cpus=";
    for(auto cpu:he.client_input_cpus)std::cout<<cpu<<",";std::cout<<"\n"<<std::flush;
    std::cout<<"NONLINEAR_CPU_RECEIPT ";for(auto cpu:lanes.observed_cpu)std::cout<<cpu<<",";std::cout<<"\n"<<std::flush;
    U compact_calls=0,compact_rows=0;for(unsigned i=0;i<workers;++i){compact_calls+=lanes.at(i).compact_response_calls;compact_rows+=lanes.at(i).compact_response_rows;}
    require(compact_calls>compact_setup_calls&&compact_rows>compact_setup_rows,"fullgraph compact core exercised");
    std::cout<<"COMPACT_CORE_RUNTIME {\"online_calls\":"<<compact_calls-compact_setup_calls<<",\"online_rows\":"<<compact_rows-compact_setup_rows<<"}\n"<<std::flush;
    if(party==2){
      require(he.client_return_max_workers==4&&he.client_return_layers==72&&he.client_return_frames==no458,"complete client return parallel core");
      std::cout<<"CLIENT_RETURN_PARALLEL_RECEIPT workers="<<he.client_return_max_workers<<" layers="<<he.client_return_layers<<" frames="<<he.client_return_frames<<" cpus=";
      for(auto cpu:he.client_return_cpus)std::cout<<cpu<<",";std::cout<<" counts=";
      for(auto n:he.client_return_counts)std::cout<<n<<",";std::cout<<"\n"<<std::flush;
    }

#if AQCF_SILENTOT_CONTROL
    std::array<std::array<U,4>,4> tree336c{};
    pcn_data089::each("TREE336_READ",4,[&](std::size_t j){auto& c=aqcf_tree335::census;tree336c[j]={c.calls,c.rows,c.leaf_rows,c.composition_rows};});
    uint64_t seed_core_rows=0,seed_leaf_rows=0,all_core238=0,all_radix238=0;
    for(unsigned i=0;i<workers;++i){auto& p=lanes.at(i).silent_control;
      require(bool(p)&&p->stats().lookup_rows>0,"provider used by all four owning lanes");
      aqcf_pool263_stats z295{};require(aqcf_pool263_read_stats(p->owned189_handle(),&z295)==0,"295 pool ledger read");
      require(z295.live_banks==0&&z295.prepared_banks==z295.consumed_banks&&z295.prepared_rows==z295.consumed_rows,"295 all fresh pool banks consumed once");
      require(carry295?(z295.prepared_banks==4*(8-3*b.enabled444)&&z295.prepared_rows==(8ULL-3*b.enabled444)*301056&&z295.native_rot_rows==(40ULL-15*b.enabled444)*301056&&z295.peak_live_banks==8-3*b.enabled444):(z295.prepared_banks==0&&z295.native_rot_rows==0),"295 complete original pool row census");
      std::cout<<"POOL295_BANK "<<party<<" "<<carry295<<" "<<i<<" "<<z295.prepared_banks<<" "<<z295.consumed_banks<<" "<<z295.prepared_rows<<" "<<z295.consumed_rows<<" "<<z295.native_rot_calls<<" "<<z295.native_rot_rows<<" "<<z295.early_bytes<<" "<<z295.correction_bytes<<" "<<z295.table_bytes<<" "<<z295.live_banks<<" "<<z295.peak_live_banks<<"\n";
      auto late=p->late37_stats();require(b.quotient169?(late.first==0&&late.second==0):(late.second>0),"expected carry provider and no unrelated late banks");std::cout<<"LATE37_STATS lane="<<i<<" calls="<<late.first<<" rows="<<late.second<<" live=0\n";
      auto quotient=p->quotient169_stats();
      require((b.quotient169&&!aqcf2::radix197_enabled())?(quotient.prepared_rows>0&&quotient.prepared_rows==quotient.consumed_rows&&quotient.native_rot_rows==3*quotient.prepared_rows&&quotient.live_banks==0):(quotient.prepared_calls==0),"source-bound quotient169 use and complete lifetime");
      std::cout<<"QUOTIENT169_STATS lane="<<i<<" calls="<<quotient.prepared_calls<<" rows="<<quotient.prepared_rows<<" native_rows="<<quotient.native_rot_rows<<" early_bytes="<<quotient.explicit_early_bytes<<" correction_bytes="<<quotient.explicit_correction_bytes<<" table_bytes="<<quotient.explicit_table_bytes<<" live="<<quotient.live_banks<<" peak_live="<<quotient.peak_live_banks<<"\n";
      auto radix=p->quotient197_stats();
      require(b.enabled221?(radix.prepared_rows>0&&radix.prepared_rows==radix.consumed_rows&&radix.native_rot_rows==4*radix.prepared_rows&&radix.live_banks==0&&radix.peak_live_banks==(mixed238?11:1)):(radix.prepared_calls==0),"221 first-leaf native fixed schedule/lifecycle");
      uint64_t derived=0,leaf=0;require(aqcf_seed221_read_stats(p->owned189_handle(),&derived,&leaf)==0,"221 lifetime closed");require(leaf<=radix.prepared_rows,"238 derived and prescribed bank census");all_core238+=quotient.prepared_rows;all_radix238+=radix.prepared_rows;seed_core_rows+=derived;seed_leaf_rows+=leaf;
      std::cout<<"SEED221_BANK lane="<<i<<" derived_core_rows="<<derived<<" leaf_rows="<<leaf<<"\n";
      std::cout<<"QUOTIENT197_STATS lane="<<i<<" calls="<<radix.prepared_calls<<" rows="<<radix.prepared_rows<<" native_rows="<<radix.native_rot_rows<<" early_bytes="<<radix.explicit_early_bytes<<" correction_bytes="<<radix.explicit_correction_bytes<<" table_bytes="<<radix.explicit_table_bytes<<" live="<<radix.live_banks<<" peak_live="<<radix.peak_live_banks<<"\n";
      std::uint64_t ec249=0,er249=0,erm249=0,eb249=0,eo249=0;
      require(aqcf_elide249_stats(p->owned189_handle(),&ec249,&er249,&erm249,&eb249,&eo249)==0,"250 no provider failure");
      require(elide250||(ec249==0&&er249==0&&erm249==0&&eb249==0&&eo249==0),"250 control did not execute candidate");
      std::cout<<"ELIDE250_SUPPLY "<<party<<" "<<elide250<<" "<<i<<" "<<ec249<<" "<<er249<<" "<<erm249<<" "<<eb249<<" "<<eo249<<"\n";
      std::uint64_t sliced4=0,sliced8=0,wordcalls=0,wordrows=0,wordnative=0;int sf=0,wf=0;
      require(aqcf_slice207_read_stats(p->owned189_handle(),&sliced4,&sliced8,&sf)==0&&aqcf_word208_read_stats(p->owned189_handle(),&wordcalls,&wordrows,&wordnative,&wf)==0,"209 actual producer census");
      require(sf==slices209&&wf==slices209&&sliced8==radix.prepared_rows&&sliced4==(slices209?quotient.prepared_rows:0),"207 complete original core coverage");
      require(slices209?(wordcalls>0&&wordrows>=quotient.prepared_rows/16&&wordnative>=wordrows):(wordcalls==0&&wordrows==0&&wordnative==0),"208 public shape-bound coverage");
      std::cout<<"SLICE209_STATS lane="<<i<<" flag="<<sf<<" core_rows="<<sliced4<<" radix8_rows="<<sliced8<<" word_calls="<<wordcalls<<" word_rows="<<wordrows<<" word_native_rows="<<wordnative<<"\n";
      uint64_t jpc=0,jpr=0,jcc=0,jcr=0;
      require(aqcf_joint304_stats(p->owned189_handle(),&jpc,&jpr,&jcc,&jcr)==0,"305 complete one-use joint banks");
      require(jpc==(joint305?(47+18*retained307flag-26*duplex336+b.enabled444):0)&&jpr==(joint305?(2176384+1455104*retained307flag-420224*duplex336+200704*b.enabled444):0)&&jcc==jpc&&jcr==jpr,"305 full47 consumers without BN191");
      std::cout<<"JOINT305_BANK "<<party<<" "<<i<<" "<<jpc<<" "<<jpr<<" "<<jcc<<" "<<jcr<<"\n";
      auto& t=tree336c[i];require(t[0]==26*duplex336&&t[1]==420224*duplex336&&t[2]==13*t[1]&&t[3]==12*t[1],"336 exact fixed raw-route tree census");
      uint64_t lc=0,lr=0,lrm=0,lb=0;require(aqcf_leaf335_stats(p->owned189_handle(),&lc,&lr,&lrm,&lb)==0,"336 leaf receipt");
      require(lc==t[0]&&lr==t[2]&&lrm==3*lr&&lb==14*lr/8,"336 original row-reduced leaf costs");
      std::cout<<"TREE336_CENSUS "<<party<<" "<<i<<" "<<t[0]<<" "<<t[1]<<" "<<t[2]<<" "<<t[3]<<" "<<lc<<" "<<lr<<" "<<lrm<<" "<<lb<<"\n";
      uint64_t pc354[24]{};require(aqcf_packed354_stats(p->owned189_handle(),pc354)==0,"354 complete codec demand");
      std::cout<<"PACKED354_CENSUS "<<party<<" "<<packed354<<" "<<i;for(auto v:pc354)std::cout<<" "<<v;std::cout<<"\n";
      aqcf_silent_control::report(std::cout,"MEASURED_FRESH_GRAPH",i,*p);}
    require(b.enabled221&&b.enabled233,"238 strongest233 settings in both arms");
    require(seed_leaf_rows==(mixed238?153965056ULL+5820416ULL*retained307flag-18489856ULL*duplex336+8830976ULL*b.enabled444:14525952ULL)&&seed_core_rows==(mixed238?14525952ULL-1680896ULL*duplex336+802816ULL*b.enabled444:226594816ULL),"238 full derived/prescribed row ledger");
    require(all_core238==(mixed238?14525952ULL-1680896ULL*duplex336+802816ULL*b.enabled444:232415232ULL)&&all_radix238==(mixed238?159785472ULL-18489856ULL*duplex336+8830976ULL*b.enabled444:14525952ULL),"238 complete fixed core coverage");
    std::cout<<"MIXED238_CENSUS "<<party<<" "<<mixed238<<" "<<all_core238<<" "<<all_radix238<<" "<<seed_core_rows<<" "<<seed_leaf_rows<<"\n";
    std::cout<<"LEGACY_AUX_RECEIPT "<<s.and_elements<<" "<<s.b2a_elements<<" "<<s.compare_elements<<" "<<s.late_nco_rows<<"\n";
#endif
    std::array<std::array<uint64_t,24>,4> common315stats{};
    pcn_data089::each("AQCF_COMMON315_READ",4,[&](std::size_t j){require(aqcf_common315_read(j,common315stats[j].data())==0,"315 no live cohort at model end");});
    std::array<std::array<uint64_t,4>,4> dir359stats{};
    pcn_data089::each("AQCF_DIRECTION359_READ",4,[&](std::size_t j){const auto&c=direction359::census;dir359stats[j]={c.calls,c.rows,c.reversed_calls,c.reversed_rows};});
    for(unsigned j=0;j<4;++j){std::cout<<"DIRECTION359_CENSUS "<<party<<" "<<direction359flag<<" "<<j;for(auto v:dir359stats[j])std::cout<<" "<<v;std::cout<<"\n";}

    for(unsigned j=0;j<4;++j){std::cout<<"COMMON315 "<<party<<" "<<j;for(auto v:common315stats[j])std::cout<<" "<<v;std::cout<<"\n";}
    rusage usage{};require(getrusage(RUSAGE_SELF,&usage)==0,"resource receipt");
    std::ofstream r(out/"RESULT.json");r<<std::setprecision(17)<<"{\"status\":\"COMPLETE_ADMITTED_TRAINED_GRAPH_CHECK_SCOPE\",\"private_model\":"<<(admitted.real?"true":"false")<<",\"real_image\":"<<(admitted.real?"true":"false")<<",\"conditional_security\":true,\"party\":"<<party
     <<",\"workers\":"<<workers<<",\"nodes\":180,\"affines\":72,\"relu_nodes\":49,\"input_CT\":"<<ni458<<",\"output_CT\":"<<no458<<",\"PM\":"<<pm458<<",\"BN19_scalar_leaf_macs\":"<<(horner?832:15296)<<",\"BN19_key_switches\":"<<(horner?4480:14368)<<",\"HORNER292_spectral_products\":"<<(horner?14464:0)<<",\"HORNER292_extra_zeros\":"<<(horner&&party==1?512:0)
     <<",\"pool444_enabled\":"<<b.enabled444<<",\"pool444_raw_rows\":"<<b.pool_phase_rows<<",\"pool444_raw_calls\":"<<b.pool_phase_calls<<",\"pool26_lift_elements\":"<<b.pool26_lift_elements<<",\"pool26_max_elements\":"<<b.pool26_max_elements<<",\"pool444_restore_calls\":"<<b.pool_restore_calls<<",\"widened_elements\":"<<b.widened_elements<<",\"setup_seconds\":"<<setup<<",\"full_process_seconds\":"<<elapsed(process_start)
     <<",\"service_raw_seconds\":"<<(party==2?he.service_delta.raw:0)<<",\"service_monotonic_seconds\":"<<(party==2?he.service_delta.monotonic:0)<<",\"service_realtime_seconds\":"<<(party==2?he.service_delta.realtime:0)
     <<",\"online_local_io_counter_delta\":"<<lanes.bytes()-setup_bytes<<",\"total_local_io_counter\":"<<lanes.bytes()<<",\"max_rss_kib\":"<<usage.ru_maxrss
     <<",\"maximum_nonlinear_workers\":"<<lanes.maximum_workers<<",\"fresh_zero_returns\":"<<he.metrics.fresh_zero
     <<",\"client_encryptions\":"<<he.metrics.encryptions<<",\"client_masked_decryptions\":"<<he.metrics.decryptions
     <<",\"full_mask_coefficients\":"<<he.metrics.full_mask<<",\"ct_adds\":"<<he.metrics.ct_add<<",\"quarter_recovery_elements\":"<<he.quarter_rows<<",\"raw_phase_consumed_elements\":"<<b.phase_rows<<",\"raw_phase_consumed_calls\":"<<b.phase_calls<<",\"residual_phase_consumed_elements\":"<<b.residual_rows<<",\"residual_phase_consumed_calls\":"<<b.residual_calls<<",\"residual_add_calls\":"<<b.residual_add_calls
     <<",\"normalized_weight_edges\":"<<he.normalized_weight_edges.load()<<",\"pool38_lift_elements\":"<<b.pool38_lift_elements<<",\"pool38_max_elements\":"<<b.pool38_max_elements<<",\"band_action_layers\":"<<he.band_action_layers<<",\"band_action_edges\":"<<he.band_action_edges.load()<<",\"band_action_outputs\":"<<he.band_action_outputs.load()<<",\"joint_coefficient_returns\":"<<he.joint_coefficient_returns.load()<<",\"joint_bulk_mask_returns\":"<<he.joint_bulk_mask_returns.load()<<",\"window_action_edges\":"<<he.window_action_edges.load()<<",\"window_action_layers\":"<<he.window_action_layers<<",\"window_action_inputs\":"<<he.window_action_inputs<<",\"window_action_outputs\":"<<he.window_action_outputs<<",\"tuple_selector_elements\":"<<s.tuple_selector_elements<<",\"spatial_weight_edges\":"<<he.spatial_weight_edges.load()<<",\"HE_phases_seconds\":{";
    bool first=true;for(auto&[k,v]:he.metrics.seconds){if(!first)r<<',';first=false;r<<'"'<<k<<"\":"<<v;}
    r<<"},\"nonlinear_phases_seconds\":{";first=true;for(auto&[k,v]:s.boundary_seconds){if(!first)r<<',';first=false;r<<'"'<<k<<"\":"<<v;}
    r<<"},\"linear_parallel\":{\"maximum_workers\":"<<he.maximum_linear_workers
     <<",\"layers\":"<<he.parallel_layers<<",\"outputs\":"<<he.parallel_outputs
     <<",\"audited_outputs\":"<<he.parallel_audited_outputs<<",\"worker_cpus\":[";
    for(unsigned j=0;j<4;++j){if(j)r<<',';r<<he.linear_worker_cpus[j];}
    r<<"],\"worker_outputs\":[";for(unsigned j=0;j<4;++j){if(j)r<<',';r<<he.linear_worker_outputs[j];}
    r<<"]}}\n";require(bool(r),"result write");require(aqcf_pacer036_bytes()==lanes.bytes(),"all protocol bytes accounted");std::cout<<"PACER170 "<<aqcf_pacer036_bytes()<<" "<<aqcf_pacer036_calls()<<" "<<aqcf_pacer036_chunks()<<" "<<aqcf_pacer036_wait_ns()<<" "<<aqcf_profile170_rate()<<" 16384\n";std::cout<<"DONE\n"<<std::flush;alarm(0);return 0;
}catch(const std::exception&e){std::cerr<<"ADMITTED_TRAINED_HE_FAIL "<<e.what()<<'\n';return 1;}}
