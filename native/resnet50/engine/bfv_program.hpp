#include <iomanip>
#include "resnet50/engine/parallel_public_transform.hpp"
#include "resnet50/core/linear18_roster.hpp"
#include "resnet50/engine/affine_lifecycle.hpp"
#include "resnet50/core/batch_norm_affine_core.hpp"
#include "resnet50/engine/batch_norm_retained_core.hpp"
#include "resnet50/engine/protected_input.hpp"
#include "resnet50/engine/resnet_affine_pipeline.hpp"
#include <condition_variable>
#include <mutex>
#pragma once
#include "resnet50/core/program.hpp"
#include "resnet50/core/ordered_publication.hpp"
#include "resnet50/core/module_basis.hpp"
#include "resnet50/core/kernel_action.hpp"
#include "resnet50/core/coefficient_wire.hpp"
#include "resnet50/core/joint_crt_wire.hpp"
#include "resnet50/engine/retained_component_policy.hpp"
#include "resnet50/engine/packed_affine_correction.hpp"
#include "resnet50/core/bilinear_action.hpp"
#include "resnet50/core/lifted_pk_zero.hpp"
#include "resnet50/core/root_flood.hpp"
#include "resnet50/core/quarter_protocol.hpp"
#include "resnet50/core/producer_bulk_mask.hpp"
#include "resnet50/core/native_auxiliary_lift.hpp"
#include "resnet50/core/spectral_ingress.hpp"
#include "resnet50/core/signed_weight.hpp"
#include "resnet50/core/public_tile_weight.hpp"
#include "resnet50/core/stride_normalized_weight.hpp"
#include "resnet50/core/windowed_quotient_action.hpp"
#include "resnet50/core/r50_stride_rules.hpp"
#include <atomic>
#include "resnet50/core/spatial_sparse_action.hpp"
#include "resnet50/core/r50_spatial_rules.hpp"
#include "resnet50/core/r50_public_tile_spacing.hpp"
#include "resnet50/core/seeded_wire38.hpp"
#if !defined(AQCF_QUARTER_ADMITTED_REAL) || !defined(AQCF_RHOMBUS_ADMITTED)
#error "Separate source-bound ResNet50 conditional admission required"
#endif
#ifndef AQCF_EXACT_QUARTER_BUILD
#error "Build with new explicit exact-quarter admission"
#endif
#if (defined(AQCF_QUARTER_QUAL_PUBLIC) + defined(AQCF_QUARTER_ADMITTED_REAL)) != 1
#error "One and only one qualification/admission mode"
#endif
static_assert(AQCF_QUARTER_PROFILE==2 && AQCF_QUERY_FINALIZER_PUBLIC==0,"qualified two-limb full-mask producer only");
#include <seal/util/ntt.h>
#include <unordered_map>
#include "resnet50/core/sci.hpp"
#include "resnet50/core/clock_sample.hpp"
#include <seal/seal.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <chrono>
#include <map>
#include <memory>
#include <sstream>
#include <thread>
#include <exception>
#include <sched.h>
namespace aqcf3 {
using Steady=std::chrono::steady_clock;
inline double elapsed(Steady::time_point t){return std::chrono::duration<double>(Steady::now()-t).count();}
struct HEMetrics {
    U input_ct=0,output_ct=0,pm=0,ct_add=0,bias_add=0,input_restore=0,
      fresh_zero=0,full_mask=0,query_mask=0,c0_add=0,encryptions=0,decryptions=0,zero_outputs=0;
    U action_layers=0,short_weight_ntts=0,weight_butterflies=0,cipher_butterflies=0,
      input_payload_bytes=0,output_payload_bytes=0;
    std::map<std::string,double> seconds;
    BilinearCounts bilinear; LiftedZeroStats lifted; U prepared_action_layers=0;
    U auxiliary_layers=0,auxiliary_inputs=0,auxiliary_outputs=0,auxiliary_pk_inverse=0;
    aux15::Stats auxiliary_stats;
};
class BfvProgram {
public:
    static constexpr bool quarter_enabled=AQCF_QUARTER_PROFILE!=0;
    static constexpr unsigned data_limbs=AQCF_QUARTER_PROFILE==2?2:4;
#if AQCF_QUARTER_PROFILE==2
    static constexpr std::array<U,3> primes={274877153281ULL,274877202433ULL,274877562881ULL};
#else
    static constexpr std::array<U,5> primes=pairprofile::Q;
#endif
    // User T1/T4 round uses H115; all arms identical. The mixed-history ledger
    // retains every H113-equivalent old reservation; no reset or reclamation.
    // validate_budget retains the inherited per-session
    // inequalities. tools/run_round.py separately admits the ENTIRE declared
    // past+planned return ledger; object recreation does not reset that ledger.
#if defined(AQCF_R17_REAL_N1) || (defined(AQCF_INGRESS_PUBLIC) || defined(AQCF_EXACT_QUARTER_BUILD))
    static constexpr unsigned flood_exponent=pairprofile::H_EXP, flood_multiplier=pairprofile::H_MULT;
    // Full ResNet50 trained export. PUBLIC only; no private run admission.
    static constexpr U cap=15323;
#else
    static constexpr unsigned flood_exponent=115, flood_multiplier=3;
    static constexpr U cap=2048;
#endif
    static constexpr U Estar=4400193998851ULL;
    aqcf2::Sci& s; U modulus; const Layouts& layouts; const Program* program;
    std::shared_ptr<seal::SEALContext> context;
    std::unique_ptr<seal::Encryptor> encryptor;std::unique_ptr<seal::Evaluator> evaluator;
    std::unique_ptr<seal::Decryptor> decryptor;seal::PublicKey public_key;
    std::unique_ptr<seal::SecretKey> spectral_secret;
    std::unique_ptr<seal::SecretKey> client_query_secret;
    unsigned client_input_workers=1,client_input_max_workers=1;
    U client_input_parallel_layers=0,client_input_parallel_frames=0;
    std::array<int,4> client_input_cpus{{-1,-1,-1,-1}};
    U sent_bytes=0,received_bytes=0,reserved_returns=0;HEMetrics metrics; U quarter_rows=0; double quarter_correction_s=0;
    std::shared_ptr<seal::SEALContext> quarter_context;std::unique_ptr<seal::Decryptor> quarter_decryptor;
    bool use_module=false,stream_mac=true;
    std::string kernel="full",wire="seal";
    std::string mac_plan="legacy", finalizer="separate", query_encryption="public";
    U expected_inputs=0,expected_outputs=0,expected_pm=0,expected_adds=0,contracted_layers=0;
    std::string lift_plan="native";
    std::unique_ptr<NativeAuxiliaryLift> auxiliary;
    std::unique_ptr<PrescaledOriginalZero> original_prescaled;
    std::vector<U> ranks,original_ranks;
    std::array<bool,AFFINES> requested{};
    // Client-clock service boundary. No final reconstruction/argmax is inside.
    bool service_started=false,service_finished=false;
    unsigned linear_workers=1,maximum_linear_workers=1;
    bool parallel_audit=false;
    U parallel_layers=0,parallel_outputs=0,parallel_audited_outputs=0;
    std::array<U,4> linear_worker_outputs{};
    std::array<int,4> linear_worker_cpus{{-1,-1,-1,-1}};
    void configure_linear_workers(unsigned n,bool audit){
        require(n==1||n==4,"fixed public linear resource budget");
        linear_workers=s.party==1?n:1;parallel_audit=audit;
        client_input_workers=s.party==2?n:1;
    }
    bool defer_raw_phase=false; U raw_phase_rows=0;
    std::function<void(std::vector<U>&)> recover_four;
    std::function<U()> all_sent4;
    bool frame_counter_started4=false;
    U first_frame_sent4=0,last_payload_sent4=0;
    double final_recovery4=0;
    void first_frame4(){
        if(frame_counter_started4)return;require(bool(all_sent4),"bound all-channel sent counter");
        first_frame_sent4=all_sent4();frame_counter_started4=true;
        if(s.party==2){service_start=aqcf2::ClockSample::now();service_started=true;std::cout<<"WAN_SERVICE_BEGIN\n"<<std::flush;std::cout<<std::setprecision(17)<<"CLOCK_BOUNDARY037 BEGIN "<<service_start.raw<<" "<<service_start.monotonic<<"\n"<<std::flush;}
        std::cout<<"AQCF_FIRST_FRAME_COUNTER "<<s.party<<" "<<first_frame_sent4<<"\n"<<std::flush;
    }

    aqcf2::ClockSample service_start{},service_delta{};
    BfvProgram(aqcf2::Sci& session,U t,const Layouts& l,const Program* p,bool module=false,bool stream=true,
      std::string action="full",std::string transport="seal",
      std::string matrix_plan="legacy",std::string zero_plan="separate",std::string query_plan="public",std::string integer_lift="native")
      :s(session),modulus(t),layouts(l),program(p),use_module(module),stream_mac(stream),kernel(std::move(action)),wire(std::move(transport)),
        mac_plan(std::move(matrix_plan)),finalizer(std::move(zero_plan)),query_encryption(std::move(query_plan)),lift_plan(std::move(integer_lift)){
        require(!quarter_enabled||(t==(U(1)<<37)&&lift_plan=="native"&&finalizer=="prescaled"&&query_encryption=="symmetric-seeded"&&wire=="packed-query"),"quarter exact producer contract");
        require(lift_plan=="native"||lift_plan=="aux-separate"||lift_plan=="aux-fused","public R15 lift plan");
        require(lift_plan=="native"||(t==aqcf2::M&&kernel=="broadcast"&&(finalizer=="lifted"||finalizer=="prescaled")),"R15 preserves original short-kernel/lifted combination");
        require(mac_plan=="legacy"||mac_plan=="matrix"||mac_plan=="winograd"||mac_plan=="strassen","public MAC plan");
        require(finalizer=="separate"||finalizer=="lifted"||finalizer=="prescaled","public zero-finalization plan");
        require(query_encryption=="public"||query_encryption=="symmetric-seeded"||query_encryption=="spectral-seeded","public input encryption plan");
        bool public_generic_prescaled=false;
#ifdef AQCF_ROOT_GENERIC_ZERO_PUBLIC
        // The full kernel is ALREADY in ordinary SEAL NTT order; it needs no
        // broadcast permutation before the same original zero finalizer.
        // Public-only qualification covers complete ciphertext equality.
        public_generic_prescaled=mac_plan=="legacy"&&kernel=="full"&&finalizer=="prescaled"&&lift_plan=="native"&&t==aqcf2::M;
#endif
        require((mac_plan=="legacy"&&finalizer=="separate")||kernel=="broadcast"||public_generic_prescaled,
          "R9 exact rewrite uses the same admitted broadcast geometry, not fiber conversion");
        require(kernel=="full" || kernel=="broadcast" || kernel=="fiber","kernel mode");
        require(wire=="seal" || wire=="packed-query","wire mode");
        require(!use_module && stream_mac,"R5 freezes QMC OFF and common stream ON; original algorithm lineage remains in the R4 input archive");
        require(t==(U(1)<<37),"public qualified Ring37 HE only");
        require((s.party==1)==(p!=nullptr),"private program role separation");
        seal::EncryptionParameters params(seal::scheme_type::bfv);params.set_poly_modulus_degree(DEGREE);
        params.set_plain_modulus(t);std::vector<seal::Modulus> qs;for(U q:primes)qs.emplace_back(q);params.set_coeff_modulus(qs);
        context=std::make_shared<seal::SEALContext>(params,true,seal::sec_level_type::tc128);
        require(context->parameters_set(),"BFV context rejected");
        const auto key=context->key_context_data(),data=context->first_context_data();
        require(key && data && key->qualifiers().sec_level==seal::sec_level_type::tc128 &&
          key->parms().coeff_modulus().size()==data_limbs+1 && data->parms().coeff_modulus().size()==data_limbs,"BFV key/data/security profile");
        for(std::size_t i=0;i<primes.size();++i)require(key->parms().coeff_modulus()[i].value()==primes[i],"explicit modulus roster");
        require(root_flood_halfwidth()==(W(flood_multiplier)<<flood_exponent),"actual sampler must equal advertised protection before traffic");
        validate_budget();auto began=Steady::now();
        if(quarter_enabled){auto qp=params;qp.set_plain_modulus(4*t);quarter_context=std::make_shared<seal::SEALContext>(qp,true,seal::sec_level_type::tc128);
          require(quarter_context->parameters_set(),"quarter native context");}
        if(s.party==2){seal::KeyGenerator keys(*context);keys.create_public_key(public_key);
          client_query_secret=std::make_unique<seal::SecretKey>(keys.secret_key());
          decryptor=std::make_unique<seal::Decryptor>(*context,keys.secret_key());
          if(quarter_enabled){auto qsk=keys.secret_key();qsk.data().parms_id()=quarter_context->key_parms_id();
            quarter_decryptor=std::make_unique<seal::Decryptor>(*quarter_context,qsk);}
          if(query_encryption=="symmetric-seeded"||query_encryption=="spectral-seeded")encryptor=std::make_unique<seal::Encryptor>(*context,public_key,keys.secret_key());
          if(query_encryption=="spectral-seeded")spectral_secret=std::make_unique<seal::SecretKey>(keys.secret_key());
          metrics.seconds["keygen"]+=elapsed(began);send_object(public_key);}
        else receive_object(public_key);
        if(!encryptor)encryptor=std::make_unique<seal::Encryptor>(*context,public_key);
        evaluator=std::make_unique<seal::Evaluator>(*context);
    }
    void validate_budget()const{
        using boost::multiprecision::cpp_int;cpp_int Q=1;for(unsigned i=0;i<data_limbs;++i)Q*=primes[i];
        if(quarter_enabled){const cpp_int error=cpp_int(22)*(cpp_int(1)<<30)+4098+(cpp_int(1)<<34)+cpp_int(DEGREE)*(cpp_int(1)<<21);
          require(primes[0]==joint_crt_wire::q0&&primes[1]==joint_crt_wire::q1&&DEGREE==8192,"joint return fixed profile");
          require(cpp_int(8)*modulus*error<Q,"quarter deterministic signed-affine error margin");
          require(cap==15323,"fixed complete ResNet50 return roster only");return;}
        const cpp_int H=cpp_int(flood_multiplier)<<flood_exponent,E=Estar;
#ifdef AQCF_R17_REAL_N1
        // Retain 3143408/12 old H113-equivalent returns; add exactly 934/15.
        require((cpp_int(3930194)*64+1834)*DEGREE*E*(cpp_int(1)<<40)<(cpp_int(960)<<113),
          "retained completed history plus the entire matched pair");
        require(cpp_int(3930194)*64+1834<cpp_int(960)*262016,"no cleared historical budget");
#else
        require((cpp_int(12)*261795+1868)*DEGREE*E*(cpp_int(1)<<40)<H,
          "entire retained R8 history plus both R9 one-image reservations");
#endif
        require(cpp_int(4098)*(cpp_int(1)<<30)+1+4097+1==E,"public error recurrence");
        require(cpp_int(cap)*DEGREE*E*(cpp_int(1)<<40)<H,"single-arm statistical envelope");
        // Divide by H107: unchanged baseline loss plus 3/4 of that envelope.
        require(cpp_int(7)*cap*DEGREE*E*(cpp_int(1)<<40)<(cpp_int(1)<<109),"four-arm composed statistical envelope");
        require(cpp_int(4)*modulus*(E+H)<Q,"BFV all-level decoding margin");
    }
    void number(U v){flush18();output18()->send_data(&v,8);flush18();sent_bytes+=8;}
    U number(){U v;flush18();s.io.io->recv_data(&v,8);received_bytes+=8;return v;}
    template<class Obj>void send_object(const Obj& obj){auto time=Steady::now();std::ostringstream buffer(std::ios::binary);
        obj.save(buffer); // Fixed compiled SEAL default for ALL new controls.
        auto bytes=buffer.str();metrics.seconds["serialization"]+=elapsed(time);
        require(!bytes.empty() && bytes.size()<=(8ULL<<20),"SEAL message length");number(bytes.size());time=Steady::now();
        s.io.io->send_data(bytes.data(),int(bytes.size()));flush18();sent_bytes+=bytes.size();metrics.seconds["send_wait"]+=elapsed(time);}
    void send_prepacked_seeded_query(const std::string& bytes){
        require(s.party==2&&bytes.size()==aqcf_wire38::SIZE,"unchanged fixed seeded frame");
        require(service_started,"parallel preparation cannot precede first real encrypted frame");
        number(bytes.size());auto t=Steady::now();s.io.io->send_data(bytes.data(),int(bytes.size()));flush18();
        sent_bytes+=bytes.size();metrics.input_payload_bytes+=bytes.size();metrics.seconds["send_wait"]+=elapsed(t);
    }
    void send_pipeline_seeded(const Layout& l,const std::vector<U>& own,U ni){
        require(s.party==2&&client_input_workers==4&&client_query_secret&&service_started,"client T4 after first frame");
        require(ni>1&&quarter_enabled&&query_encryption=="symmetric-seeded","qualified seeded input batch");
        const unsigned count=std::min<U>(client_input_workers,ni-1);require(count==4,"R50 input batch needs four real frames");std::vector<std::string> frames(ni);
        std::mutex ready_mutex;std::condition_variable ready_cv;std::vector<bool> ready(ni,false);bool failed=false;
        std::vector<std::exception_ptr> errors(count);std::vector<aqcf_owner4::Task> workers;auto wall=Steady::now();
        for(unsigned lane=0;lane<count;++lane)workers.emplace_back(lane,[&,lane]{try{
            cpu_set_t cpus;CPU_ZERO(&cpus);CPU_SET(aqcf_native4::cpu(lane),&cpus);
            require(sched_setaffinity(0,sizeof(cpus),&cpus)==0,"client encryption CPU pin");
            client_input_cpus[lane]=sched_getcpu();require(client_input_cpus[lane]==aqcf_native4::cpu(lane),"client physical-core receipt");
            aqcf_native4::Outer audit4(lane,"CLIENT_INPUT");
            seal::Encryptor local(*context,public_key,*client_query_secret);
            for(U i=1+lane;i<ni;i+=count){
                ++audit4.jobs;
                auto pt=encode(gather(l,i,own,modulus));auto encrypted=local.encrypt_symmetric(pt);
                std::ostringstream buffer(std::ios::binary);encrypted.save(buffer,seal::compr_mode_type::none);
                auto bytes=buffer.str();require(bytes.size()>=data_limbs*DEGREE*8&&bytes.size()<data_limbs*DEGREE*8+512,"fixed seeded serializer");
                frames[i]=aqcf_wire38::pack(bytes,{primes[0],primes[1]});
                require(frames[i].size()==aqcf_wire38::SIZE,"fixed packed query size");
                {std::lock_guard<std::mutex> lock(ready_mutex);ready[i]=true;}ready_cv.notify_all();
            }
        }catch(...){
            {std::lock_guard<std::mutex> lock(ready_mutex);errors[lane]=std::current_exception();failed=true;}ready_cv.notify_all();
        }});
        std::exception_ptr send_error;
        try{for(U i=1;i<ni;++i){
            std::unique_lock<std::mutex> lock(ready_mutex);ready_cv.wait(lock,[&]{return ready[i]||failed;});
            if(failed)break;lock.unlock();send_prepacked_seeded_query(frames[i]);
        }}catch(...){send_error=std::current_exception();}
        for(auto& w:workers)w.join();for(auto& e:errors)if(e)std::rethrow_exception(e);
        if(send_error)std::rethrow_exception(send_error);
        metrics.encryptions+=ni-1;metrics.seconds["client_encrypt_and_ordered_send_pipeline_wall"]+=elapsed(wall);
        ++client_input_parallel_layers;client_input_parallel_frames+=ni-1;client_input_max_workers=std::max(client_input_max_workers,count);
    }
    void public_client_parallel_selftest(){
        if(s.party!=2||client_input_workers!=4)return;
        require(bool(client_query_secret),"client owns secret only");
        std::array<std::exception_ptr,4> errors{};std::vector<aqcf_owner4::Task> workers;
        for(unsigned lane=0;lane<4;++lane)workers.emplace_back(lane,[&,lane]{try{
            cpu_set_t pin;CPU_ZERO(&pin);CPU_SET(aqcf_native4::cpu(lane),&pin);require(!sched_setaffinity(0,sizeof(pin),&pin),"public input fixture pin");pcn_parallel::OuterGuard scope;
            seal::Encryptor local(*context,public_key,*client_query_secret);seal::Decryptor dec(*context,*client_query_secret);
            for(unsigned j=0;j<8;++j){std::vector<U> v(DEGREE);for(U k=0;k<DEGREE;++k)v[k]=(k*17+lane*997+j)&(modulus-1);
                auto pt=encode(v);auto encrypted=local.encrypt_symmetric(pt);std::ostringstream o(std::ios::binary);encrypted.save(o,seal::compr_mode_type::none);
                auto packed=aqcf_wire38::pack(o.str(),{primes[0],primes[1]});auto unpacked=aqcf_wire38::unpack(packed,{primes[0],primes[1]});
                std::istringstream input(unpacked,std::ios::binary);seal::Ciphertext ct;ct.load(*context,input);check(ct);seal::Plaintext recovered;dec.decrypt(ct,recovered);
                for(U k=0;k<DEGREE;++k)require((k<recovered.coeff_count()?recovered[k]:0)==v[k],"public parallel seeded input exactness");
            }
        }catch(...){errors[lane]=std::current_exception();}});
        for(auto& w:workers)w.join();for(auto& e:errors)if(e)std::rethrow_exception(e);
        std::cout<<"PUBLIC_CLIENT_PARALLEL_SEEDED_PASS ciphertexts=32 coefficients=262144 private_calls=0\n"<<std::flush;
    }
    template<class Obj>void send_seeded_query(const Obj& obj){
        auto t=Steady::now();std::ostringstream buffer(std::ios::binary);
        // No content-adaptive compression: the seeded frame length depends only
        // on the fixed public N/level/SEAL serialization version.
        obj.save(buffer,seal::compr_mode_type::none);auto bytes=buffer.str();
        metrics.seconds["serialization"]+=elapsed(t);
        require(bytes.size()>=data_limbs*DEGREE*8&&bytes.size()<data_limbs*DEGREE*8+512,"fixed seeded public frame family");
        t=Steady::now();bytes=aqcf_wire38::pack(bytes,{primes[0],primes[1]});metrics.seconds["seeded38_pack"]+=elapsed(t);
        require(s.party==2,"client input service timer");first_frame4();
        number(bytes.size());t=Steady::now();s.io.io->send_data(bytes.data(),int(bytes.size()));flush18();
        sent_bytes+=bytes.size();metrics.input_payload_bytes+=bytes.size();metrics.seconds["send_wait"]+=elapsed(t);
    }
    // Conventional concurrency of the unchanged seeded load/share restore/NTT.
    // Network and framing remain on the original main thread. No novelty claim.
    struct IngressPhase{double unpack=0,load=0,gather=0,restore=0,ntt=0;};
    unsigned server_ingress_max_workers=1;
    U server_ingress_frames=0,server_ingress_layers=0;
    std::array<int,4> server_ingress_cpus{{-1,-1,-1,-1}};
    ActionCipher decode_ingress(const std::string& frame,const seal::Plaintext& share,
        bool short_kernel,const std::vector<std::vector<U>>& broadcast,
        seal::Evaluator& local,IngressPhase& phase,const window_action::Plan* window=nullptr)const{
        auto t=Steady::now();auto bytes=aqcf_wire38::unpack(frame,{primes[0],primes[1]});
        phase.unpack+=elapsed(t);t=Steady::now();std::istringstream input(bytes,std::ios::binary);
        seal::Ciphertext ct;ct.load(*context,input);require(input.peek()==std::char_traits<char>::eof(),"parallel native load consumed frame");
        check(ct);phase.load+=elapsed(t);t=Steady::now();local.add_plain_inplace(ct,share);
        phase.restore+=elapsed(t);t=Steady::now();
        if(window){
            require(!short_kernel,"window ingress original coefficient order");
            std::vector<U> coefficients(2*data_limbs*DEGREE);
            for(unsigned c=0;c<2;++c)std::copy_n(ct.data(c),data_limbs*DEGREE,coefficients.data()+c*data_limbs*DEGREE);
            ActionCipher a;a.v=window->prepare_coefficients(coefficients);phase.ntt+=elapsed(t);return a;
        }
        local.transform_to_ntt_inplace(ct);
        ActionCipher a;a.v.resize(2*data_limbs*DEGREE);
        for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){U* dest=a.v.data()+(c*data_limbs+r)*DEGREE;
            if(short_kernel)for(U j=0;j<DEGREE;++j)dest[broadcast[r][j]]=ct.data(c)[r*DEGREE+j];
            else std::copy_n(ct.data(c)+r*DEGREE,DEGREE,dest);}
        phase.ntt+=elapsed(t);return a;
    }
    std::vector<ActionCipher> receive_ingress_pipeline(const Layout& l,const std::vector<U>& own,
        U ni,bool short_kernel,const std::vector<std::vector<U>>& broadcast,const window_action::Plan* window=nullptr){
        require(s.party==1&&linear_workers==4&&ni>1,"server ingress bounded T4");
        const unsigned count=std::min<U>(linear_workers,ni);require(count==4,"R50 ingress needs four real frames");
        std::vector<ActionCipher> inputs(ni);std::vector<std::string> frames(ni);
        std::vector<bool> ready(ni,false);bool failed=false;
        std::mutex mu;std::condition_variable cv;std::vector<std::exception_ptr> errors(count);
        std::vector<IngressPhase> phase(count);std::vector<aqcf_owner4::Task> workers;auto wall=Steady::now();
        auto work=[&](unsigned lane){try{
            cpu_set_t cpus;CPU_ZERO(&cpus);CPU_SET(aqcf_native4::cpu(lane),&cpus);
            require(sched_setaffinity(0,sizeof(cpus),&cpus)==0,"server ingress physical CPU pin");
            server_ingress_cpus[lane]=sched_getcpu();require(server_ingress_cpus[lane]==aqcf_native4::cpu(lane),"server ingress CPU evidence");
            aqcf_native4::Outer audit4(lane,"SERVER_INGRESS");
            seal::Evaluator local(*context);
            for(U i=lane;i<ni;i+=count){
                {std::unique_lock<std::mutex> lock(mu);cv.wait(lock,[&]{return ready[i]||failed;});if(failed)return;}
                auto t=Steady::now();auto share=encode(gather(l,i,own,modulus));phase[lane].gather+=elapsed(t);
                ++audit4.jobs;
                inputs[i]=decode_ingress(frames[i],share,short_kernel,broadcast,local,phase[lane],window);
                frames[i].clear();
            }
        }catch(...){std::lock_guard<std::mutex> lock(mu);errors[lane]=std::current_exception();failed=true;cv.notify_all();}};
        try{for(unsigned lane=0;lane<count;++lane)workers.emplace_back(work,lane);}
        catch(...){ {std::lock_guard<std::mutex> lock(mu);failed=true;}cv.notify_all();for(auto&w:workers)w.join();throw;}
        std::exception_ptr network_error;
        try{for(U i=0;i<ni;++i){
            first_frame4();U n=number();require(n==aqcf_wire38::SIZE,"unchanged parallel input frame size");
            frames[i].resize(n);auto t=Steady::now();s.io.io->recv_data(frames[i].data(),int(n));received_bytes+=n;
            metrics.seconds["receive_wait"]+=elapsed(t);
            {std::lock_guard<std::mutex> lock(mu);ready[i]=true;}cv.notify_all();
        }}catch(...){network_error=std::current_exception();{std::lock_guard<std::mutex> lock(mu);failed=true;}cv.notify_all();}
        for(auto&w:workers)w.join();if(network_error)std::rethrow_exception(network_error);
        for(auto&e:errors)if(e)std::rethrow_exception(e);
        metrics.seconds["server_receive_restore_NTT_pipeline_wall"]+=elapsed(wall);
        for(auto&p:phase){metrics.seconds["ingress_worker_unpack_not_wall"]+=p.unpack;metrics.seconds["ingress_worker_load_not_wall"]+=p.load;
            metrics.seconds["ingress_worker_gather_not_wall"]+=p.gather;metrics.seconds["ingress_worker_restore_not_wall"]+=p.restore;metrics.seconds["ingress_worker_NTT_not_wall"]+=p.ntt;}
        server_ingress_max_workers=std::max(server_ingress_max_workers,count);server_ingress_frames+=ni;++server_ingress_layers;
        std::cout<<"SERVER_INPUT_PIPELINE_RECEIPT workers="<<count<<" frames="<<ni<<" cpus=";
        for(unsigned lane=0;lane<count;++lane)std::cout<<server_ingress_cpus[lane]<<',';std::cout<<'\n'<<std::flush;
        return inputs;
    }
    void public_server_ingress_selftest(){
        if(s.party!=1||linear_workers!=4)return;
        // Independently generated PUBLIC-FIXTURE key, not the client's runtime SK.
        seal::KeyGenerator keys(*context);seal::PublicKey pk;keys.create_public_key(pk);
        seal::Encryptor enc(*context,pk,keys.secret_key());seal::Evaluator serial(*context);
        std::vector<std::string> frame(32);std::vector<seal::Plaintext> shares(32);
        std::vector<ActionCipher> expected(32),got(32);std::vector<std::vector<U>> mapping(data_limbs,std::vector<U>(DEGREE));
        for(auto&m:mapping)for(U j=0;j<DEGREE;++j)m[j]=(j*5)%DEGREE;
        for(U i=0;i<32;++i){std::vector<U> clear(DEGREE),own(DEGREE);
            for(U j=0;j<DEGREE;++j){clear[j]=(j*17+i*997)&(modulus-1);own[j]=(j*337+i*113)&(modulus-1);}
            auto encrypted=enc.encrypt_symmetric(encode(clear));std::ostringstream o(std::ios::binary);encrypted.save(o,seal::compr_mode_type::none);
            frame[i]=aqcf_wire38::pack(o.str(),{primes[0],primes[1]});shares[i]=encode(own);
            auto raw=aqcf_wire38::unpack(frame[i],{primes[0],primes[1]});std::istringstream stream(raw,std::ios::binary);
            seal::Ciphertext ct;ct.load(*context,stream);check(ct);serial.add_plain_inplace(ct,shares[i]);serial.transform_to_ntt_inplace(ct);
            auto& out=expected[i].v;out.resize(2*data_limbs*DEGREE);
            for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r)for(U j=0;j<DEGREE;++j)
                out[(c*data_limbs+r)*DEGREE+(i%2?mapping[r][j]:j)]=ct.data(c)[r*DEGREE+j];
        }
        std::array<std::exception_ptr,4> error{};std::vector<aqcf_owner4::Task> workers;
        for(unsigned lane=0;lane<4;++lane)workers.emplace_back(lane,[&,lane]{try{
            cpu_set_t pin;CPU_ZERO(&pin);CPU_SET(aqcf_native4::cpu(lane),&pin);require(!sched_setaffinity(0,sizeof(pin),&pin),"public ingress fixture pin");pcn_parallel::OuterGuard scope;
            seal::Evaluator local(*context);IngressPhase phase;
            for(U i=lane;i<32;i+=4)got[i]=decode_ingress(frame[i],shares[i],i%2,mapping,local,phase);
        }catch(...){error[lane]=std::current_exception();}});
        for(auto&w:workers)w.join();for(auto&e:error)if(e)std::rethrow_exception(e);
        for(U i=0;i<32;++i)require(got[i].v==expected[i].v,"public concurrent ingress equals original serial words");
        std::cout<<"PUBLIC_SERVER_INGRESS_PASS ciphertexts=32 NTT_words=1048576 private_calls=0\n"<<std::flush;
        // Same public-key-fixture check in BOTH arms. No runtime secret key is
        // used and no private input/model is loaded for this arithmetic check.
        const std::array<std::pair<U,U>,5> wc{{{196,40},{49,162},{49,164},{49,165},{784,10}}};
        U window_checks=0;
        for(U ci=0;ci<wc.size();++ci){auto [stride,support]=wc[ci];window_action::Plan plan(*context,stride,support);stride_weight::Plan old(*context,stride,support);
            std::vector<__uint128_t> sums(4*plan.slots,0),oracle(4*DEGREE,0);IngressPhase phase;
            for(U e=0;e<2;++e){auto x=decode_ingress(frame[ci*2+e],shares[ci*2+e],false,mapping,serial,phase,&plan);
                auto y=decode_ingress(frame[ci*2+e],shares[ci*2+e],false,mapping,serial,phase);
                Edge edge;for(U k=0;k<support;++k)edge.coefficients.push_back({k*stride,I((k*337+e*997)%65535)-32767});
                auto weight=plan.weight(edge);plan.add(sums,x.v,weight);auto reference=old.weight(edge);
                for(U c=0;c<2;++c)for(U r=0;r<2;++r)for(U j=0;j<DEGREE;++j){U off=(c*2+r)*DEGREE+j;oracle[off]+=__uint128_t(y.v[off])*reference[r*DEGREE+j];}
            }
            auto got=plan.finish_spectrum(sums);for(U j=0;j<4*DEGREE;++j){require(got[j]==U(oracle[j]%primes[(j/DEGREE)%2]),"public encrypted window ingress/MAC exact");++window_checks;}
        }
        std::cout<<"PUBLIC_WINDOW_INGRESS_MAC_PASS coefficients="<<window_checks<<" private_calls=0\n"<<std::flush;
        U band_checks=0;aqcf_band::Accumulator bands(4*DEGREE);std::vector<__uint128_t> reference(4*DEGREE,0);
        for(U e=0;e<17;++e){IngressPhase phase;auto a=decode_ingress(frame[e],shares[e],false,mapping,serial,phase);Edge edge;
            for(U k=0;k<6;++k)edge.coefficients.push_back({k*1372,I((k*997+e*113)%65535)-32767});
            stride_weight::Plan plan(*context,1372,6);auto w=plan.weight(edge);
            for(U c=0;c<2;++c)for(U r=0;r<2;++r){U off=(c*2+r)*DEGREE;bands.add(off,a.v.data()+off,w.data()+r*DEGREE,DEGREE);
                for(U j=0;j<DEGREE;++j)reference[off+j]+=__uint128_t(a.v[off+j])*w[r*DEGREE+j];}
        }
        for(U j=0;j<4*DEGREE;++j){require(bands.word(j)==reference[j],"public complete encrypted-input band product exact before reduction");++band_checks;}
        std::cout<<"PUBLIC_BAND_MAC_PASS accumulator_words="<<band_checks<<" private_calls=0\n"<<std::flush;
        U joint_checks=0,mask_checks=0;PrescaledOriginalZero final(*context,public_key);
        for(U test=0;test<4;++test){std::vector<U> spec(4*DEGREE);
            for(U j=0;j<spec.size();++j)spec[j]=U((reference[j]+test)%primes[(j/DEGREE)%2]);
            auto coeff=spec;for(U r=0;r<4;++r)seal::util::inverse_ntt_negacyclic_harvey(coeff.data()+r*DEGREE,context->first_context_data()->small_ntt_tables()[r%2]);
            ZeroCoins public_coins(*context);LiftedZeroStats a,b;
            auto x=final.with_coins(spec,public_coins,a,false),y=final.with_coins(coeff,public_coins,b,true);
            for(U j=0;j<4*DEGREE;++j){require(x.data()[j]==y.data()[j],"public full output producer coefficient equality");++joint_checks;}
            seal::prng_seed_type seed{};seed[0]=test+773;auto old_rng=seal::Blake2xbPRNGFactory().create(seed);
            std::vector<U> oldmask(2*DEGREE),newmask(2*DEGREE);
            for(U r=0;r<2;++r){U q=primes[r],cut=(U(0)-q)%q;for(U j=0;j<DEGREE;++j){U v;do{old_rng->generate(sizeof(v),reinterpret_cast<seal::seal_byte*>(&v));}while(v<cut);oldmask[r*DEGREE+j]=v%q;}}
            bulk_full_mask(seal::Blake2xbPRNGFactory().create(seed),newmask);
            for(U j=0;j<2*DEGREE;++j){require(oldmask[j]==newmask[j],"public complete original mask stream equality");++mask_checks;}
        }
        std::cout<<"PUBLIC_PRODUCER_JOINT_PASS ciphertext_words="<<joint_checks<<" mask_words="<<mask_checks<<" private_calls=0\n"<<std::flush;
    }

    void receive_seeded38(seal::Ciphertext& ct){
        first_frame4();U n=number();require(n==aqcf_wire38::SIZE,"fixed seeded38 length");
        std::string bytes(n,'\0');auto t=Steady::now();s.io.io->recv_data(bytes.data(),int(n));received_bytes+=n;
        metrics.seconds["receive_wait"]+=elapsed(t);t=Steady::now();
        bytes=aqcf_wire38::unpack(bytes,{primes[0],primes[1]});
        metrics.seconds["seeded38_unpack"]+=elapsed(t);t=Steady::now();std::istringstream in(bytes,std::ios::binary);
        ct.load(*context,in);require(in.peek()==std::char_traits<char>::eof(),"seeded38 full native load");
        metrics.seconds["deserialization"]+=elapsed(t);
    }
    template<class Obj>void receive_object(Obj& obj){U n=number();require(n>0 && n<=(8ULL<<20),"SEAL incoming frame length");
        std::string bytes(n,'\0');auto time=Steady::now();s.io.io->recv_data(bytes.data(),int(n));received_bytes+=n;
        metrics.seconds["receive_wait"]+=elapsed(time);time=Steady::now();std::istringstream buffer(bytes,std::ios::binary);
        obj.load(*context,buffer);require(buffer.peek()==std::char_traits<char>::eof(),"SEAL trailing bytes");metrics.seconds["deserialization"]+=elapsed(time);}
    void send_spectral_input(const ingress::Bytes& bytes){
        require(s.party==2&&spectral_secret&&bytes.size()==184432,"client-only exact spectral frame");
        if(!service_started){service_start=aqcf2::ClockSample::now();service_started=true;}
        number(bytes.size());auto t=Steady::now();s.io.io->send_data(bytes.data(),int(bytes.size()));flush18();
        sent_bytes+=bytes.size();metrics.input_payload_bytes+=bytes.size();metrics.seconds["send_wait"]+=elapsed(t);
    }
    seal::Ciphertext receive_spectral_input(const seal::Plaintext& own){
        require(s.party==1&&!spectral_secret,"server cannot own spectral SK");U size=number();require(size==184432,"fixed spectral frame length");
        ingress::Bytes frame(size);auto t=Steady::now();s.io.io->recv_data(frame.data(),int(size));received_bytes+=size;metrics.seconds["receive_wait"]+=elapsed(t);
        t=Steady::now();auto ct=ingress::consume(*context,frame,own);metrics.seconds["spectral_seed_unpack_and_share_restore"]+=elapsed(t);return ct;
    }
    U coefficient_residue(I value)const{return modulus==aqcf2::M?(U(value)&(aqcf2::M-1)):aqcf2::residue(W(value),modulus);}
    seal::Plaintext encode(const std::vector<U>& values)const{
        require(values.size()==DEGREE,"complete coefficient vector");seal::Plaintext p;p.resize(DEGREE);
        pcn_span091::each("AQCF_PLAINTEXT_ENCODE",DEGREE,[&](std::size_t j){require(values[j]<modulus,"canonical coefficient share");p[j]=values[j];});return p;
    }
    void check(const seal::Ciphertext& ct)const{
        require(seal::is_valid_for(ct,*context) && ct.size()==2 && !ct.is_ntt_form() &&
          ct.parms_id()==context->first_parms_id() && !ct.is_transparent(),"BFV return representation");
    }
    void protect(seal::Ciphertext& ct,const std::vector<Query>& queries,std::vector<U>& own,bool zero_already_added=false){
        require(s.party==1 && reserved_returns<=cap,"server-only protected return");
        // Zero linear maps can have a transparent pre-mask accumulator. No such
        // object is sent: full mask + fresh asymmetric EncZero + flood follows.
        require(seal::is_valid_for(ct,*context)&&ct.size()==2&&!ct.is_ntt_form()&&ct.parms_id()==context->first_parms_id(),"pre-protection BFV structure");
        if(quarter_enabled){require(zero_already_added,"all original fresh zero consumed before full-Q mask");auto qt=Steady::now();
          if(retained299::enabled())retained299::spatial(ct,*quarter_context,queries,own);else{quarter_mask(ct,*quarter_context,queries,own);metrics.full_mask+=DEGREE;metrics.c0_add+=data_limbs*DEGREE;}
          metrics.seconds["quarter_full_Q_mask_and_scale"]+=elapsed(qt);check(ct);return;}
        auto time=Steady::now();seal::Plaintext mask;mask.resize(DEGREE);
        // Only c0[queries] is serialized in this mode. c1 and the fresh PK zero
        // stay full-ring; there is no post-protection homomorphic operation.
        // Full SEAL serialization is deliberately excluded from this rewrite.
        const bool project=AQCF_QUERY_FINALIZER_PUBLIC && wire=="packed-query";
        std::vector<U> selected;
        if(project){std::array<bool,DEGREE> seen{};selected.reserve(queries.size());
            for(const auto&q:queries){require(q.exponent<DEGREE,"public query position");
                if(!seen[q.exponent]){seen[q.exponent]=true;selected.push_back(q.exponent);}}
            std::fill_n(mask.data(),DEGREE,U(0));}
        auto rng=seal::Blake2xbPRNGFactory().create();U threshold=(U(0)-modulus)%modulus;
        auto sample_mask=[&](U j){U value;do{rng->generate(sizeof(value),reinterpret_cast<seal::seal_byte*>(&value));}while(value<threshold);mask[j]=value%modulus;};
        if(project)for(U j:selected)sample_mask(j);else for(U j=0;j<DEGREE;++j)sample_mask(j);
        for(const auto& q:queries)own[q.destination]=mask[q.exponent];evaluator->sub_plain_inplace(ct,mask);
        if(project)metrics.query_mask+=selected.size();else metrics.full_mask+=DEGREE;
        metrics.seconds[project?"query_projected_mask":"full_coefficient_mask"]+=elapsed(time);
        if(!zero_already_added){time=Steady::now();seal::Ciphertext zero;encryptor->encrypt_zero(context->first_parms_id(),zero);check(zero);
        evaluator->add_inplace(ct,zero);++metrics.fresh_zero;metrics.seconds["fresh_PK_zero"]+=elapsed(time);}
        time=Steady::now();rng=seal::Blake2xbPRNGFactory().create();
        auto add_noise=[&](U j){const W error=root_flood_sample(rng);
            for(unsigned r=0;r<data_limbs;++r){U delta=aqcf2::residue(error,primes[r]);U& v=ct.data(0)[r*DEGREE+j];
                v+=delta;if(v>=primes[r])v-=primes[r];++metrics.c0_add;}};
        if(project)for(U j:selected)add_noise(j);else for(U j=0;j<DEGREE;++j)add_noise(j);
        metrics.seconds["c0_finalizer"]+=elapsed(time);check(ct); // No post-flood arithmetic/modswitch.
    }
    // Full public-key frames retain SEAL serialization. Only data ciphertexts
    // use the common lossless codec; all masks and all four RNS limbs remain.
    std::vector<unsigned char> pack_cipher4(const seal::Ciphertext& ct,const std::vector<Query>* query){
        check(ct);
        if(query){require(quarter_enabled&&data_limbs==2,"joint return only on admitted quarter profile");
            std::vector<U> indices;indices.reserve(query->size());for(auto q:*query)indices.push_back(q.exponent);
            return joint_crt_wire::encode(ct.data(0),ct.data(1),indices,retained299::enabled());}
        CoefficientWriter w;
        for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){
            const U* a=ct.data(c)+r*DEGREE;const unsigned bits=coefficient_bits(primes[r]);
            if(c==0 && query){for(auto q:*query)w.append(a[q.exponent],bits);}
            else for(U j=0;j<DEGREE;++j)w.append(a[j],bits);
        }
        auto bytes=w.finish();return bytes;
    }
    void send_packed4(const std::vector<unsigned char>& bytes,bool query){
        number(bytes.size());auto t=Steady::now();output18()->send_data(bytes.data(),s.dim(bytes.size()));flush18();
        sent_bytes+=bytes.size();metrics.seconds["send_wait"]+=elapsed(t);
        (query?metrics.output_payload_bytes:metrics.input_payload_bytes)+=bytes.size();
    }
    void send_cipher(const seal::Ciphertext& ct,const std::vector<Query>* query=nullptr){
        if(wire=="seal"){send_object(ct);return;}
        auto t=Steady::now();auto bytes=pack_cipher4(ct,query);metrics.seconds["ciphertext_wire_encode"]+=elapsed(t);send_packed4(bytes,bool(query));
    }
    void receive_cipher(seal::Ciphertext& ct,const std::vector<Query>* query=nullptr){
        if(wire=="seal"){receive_object(ct);check(ct);return;}
        U width=0;for(unsigned r=0;r<data_limbs;++r)width+=coefficient_bits(primes[r]);
        const U count=DEGREE+(query?query->size():DEGREE),expected=query?joint_crt_wire::size(query->size(),retained299::enabled()):(count*width+7)/8;
        U n=number();require(n==expected,"fixed public ciphertext frame length; integer-exact return codec");
        std::vector<unsigned char> bytes(n);auto t=Steady::now();
        s.io.io->recv_data(bytes.data(),s.dim(n));received_bytes+=n;metrics.seconds["receive_wait"]+=elapsed(t);
        t=Steady::now();ct.resize(*context,context->first_parms_id(),2);ct.is_ntt_form()=false;
        for(unsigned c=0;c<2;++c)std::fill_n(ct.data(c),data_limbs*DEGREE,U(0));
        if(query){std::vector<U> indices;indices.reserve(query->size());for(auto q:*query)indices.push_back(q.exponent);
            joint_crt_wire::decode(bytes,ct.data(0),ct.data(1),indices,retained299::enabled());check(ct);
            metrics.seconds["joint_exact_wire_decode"]+=elapsed(t);metrics.output_payload_bytes+=n;return;}
        CoefficientReader rd(bytes);
        for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){
            U* a=ct.data(c)+r*DEGREE;const unsigned bits=coefficient_bits(primes[r]);
            if(c==0 && query){for(auto q:*query)a[q.exponent]=rd.take(bits,primes[r]);}
            else for(U j=0;j<DEGREE;++j)a[j]=rd.take(bits,primes[r]);
        }
        rd.end();check(ct);metrics.seconds["lossless_wire_decode"]+=elapsed(t);
        (query?metrics.output_payload_bytes:metrics.input_payload_bytes)+=n;
    }
    // Ordinary execution optimization: one network owner, independent client
    // decode/decrypt workers. No secret sent out, no mask or payload omitted.
    unsigned client_return_max_workers=1;
    U client_return_layers=0,client_return_frames=0;
    std::array<int,4> client_return_cpus{{-1,-1,-1,-1}};
    std::array<U,4> client_return_counts{};
    void receive_return_pipeline(const Layout& l,std::vector<U>& out,std::vector<std::vector<unsigned char>>* buffered18=nullptr){
        require(s.party==2&&client_query_secret&&quarter_enabled&&wire=="packed-query"&&client_input_workers==4,"fixed client return worker contract");
        const U no=l.queries.size();require(no>0,"nonempty return roster");
        // Verify public scatter ownership before concurrent writes.
        std::vector<unsigned char> visited(out.size(),0);
        for(const auto& qs:l.queries)for(const auto&q:qs){
            require(q.destination<out.size()&&q.exponent<DEGREE&&!visited[q.destination],"unique public output owner");visited[q.destination]=1;
        }
        require(std::all_of(visited.begin(),visited.end(),[](auto v){return v==1;}),"complete public output coverage");
        auto wall=Steady::now();const unsigned count=std::min<U>(4,no);require(count==4,"R50 returns need four real frames");
        std::vector<std::vector<unsigned char>> frames(no);
        OrderedPublication ready(no);std::vector<aqcf_owner4::Task> threads;
        std::vector<std::exception_ptr> errors(count);
        struct Phases{double decode=0,decrypt=0,extract=0;U count=0;};
        std::vector<Phases> phases(count);
        auto worker=[&](unsigned lane){try{
            cpu_set_t cpus;CPU_ZERO(&cpus);CPU_SET(aqcf_native4::cpu(lane),&cpus);
            require(sched_setaffinity(0,sizeof(cpus),&cpus)==0,"client return worker CPU affinity");
            client_return_cpus[lane]=sched_getcpu();require(client_return_cpus[lane]==aqcf_native4::cpu(lane),"actual client return CPU");
            aqcf_native4::Outer audit4(lane,"CLIENT_RETURN");
            auto sk=*client_query_secret;sk.data().parms_id()=quarter_context->key_parms_id();
            seal::Decryptor local(*quarter_context,sk);
            for(U i=lane;i<no;i+=count){
                require(ready.wait(i),"client ciphertext publication failed");
                auto t=Steady::now();seal::Ciphertext ct;ct.resize(*context,context->first_parms_id(),2);ct.is_ntt_form()=false;
                for(unsigned c=0;c<2;++c)std::fill_n(ct.data(c),data_limbs*DEGREE,U(0));
                const auto& qs=l.queries[i];std::vector<U> indices;indices.reserve(qs.size());
                for(auto q:qs)indices.push_back(q.exponent);
                joint_crt_wire::decode(frames[i],ct.data(0),ct.data(1),indices,retained299::enabled());check(ct);phases[lane].decode+=elapsed(t);
                t=Steady::now();ct.parms_id()=quarter_context->first_parms_id();seal::Plaintext pt;local.decrypt(ct,pt);
                phases[lane].decrypt+=elapsed(t);t=Steady::now();
                for(auto q:qs){U v=q.exponent<pt.coeff_count()?pt[q.exponent]:0;
                    require(v<4*modulus,"canonical unchanged masked query");out[q.destination]=v;
                }
                ++audit4.jobs;
                phases[lane].extract+=elapsed(t);++phases[lane].count;
                std::vector<unsigned char>().swap(frames[i]);
            }
        }catch(...){errors[lane]=std::current_exception();ready.fail();}};
        std::exception_ptr owner_error;
        try{
            for(unsigned lane=0;lane<count;++lane)threads.emplace_back(worker,lane);
            U width=0;for(unsigned r=0;r<data_limbs;++r)width+=coefficient_bits(primes[r]);
            for(U i=0;i<no;++i){
                const U expected=joint_crt_wire::size(l.queries[i].size(),retained299::enabled());
                if(buffered18){require(buffered18->size()==no&&buffered18->at(i).size()==expected,"bound buffered return");frames[i]=std::move(buffered18->at(i));metrics.output_payload_bytes+=expected;}
                else{U n=number();require(n==expected,"fixed public ciphertext frame length; integer-exact return codec");
                frames[i].resize(n);auto t=Steady::now();s.io.io->recv_data(frames[i].data(),s.dim(n));
                received_bytes+=n;metrics.output_payload_bytes+=n;metrics.seconds["receive_wait"]+=elapsed(t);}
                ready.publish(i);
            }
        }catch(...){owner_error=std::current_exception();ready.fail();}
        for(auto&th:threads)th.join();
        if(owner_error)std::rethrow_exception(owner_error);
        for(auto&e:errors)if(e)std::rethrow_exception(e);
        U complete=0;
        for(unsigned lane=0;lane<count;++lane){const auto&p=phases[lane];complete+=p.count;client_return_counts[lane]+=p.count;
            metrics.seconds["client_return_worker_decode_not_wall"]+=p.decode;
            metrics.seconds["client_return_worker_decrypt_not_wall"]+=p.decrypt;
            metrics.seconds["client_return_worker_extract_not_wall"]+=p.extract;
        }
        require(complete==no,"all masked returns consumed exactly once");metrics.decryptions+=no;
        client_return_max_workers=std::max(client_return_max_workers,count);++client_return_layers;client_return_frames+=no;
        metrics.seconds["client_return_receive_decode_decrypt_pipeline_wall"]+=elapsed(wall);
    }

    std::atomic<U> joint_coefficient_returns{0},joint_bulk_mask_returns{0};
    U band_action_layers=0;
    std::atomic<U> band_action_edges{0},band_action_outputs{0};
    U window_action_layers=0,window_action_inputs=0,window_action_outputs=0;
    std::atomic<U> window_action_edges{0};
    std::atomic<U> normalized_weight_edges{0},spatial_weight_edges{0};
    std::vector<U> evaluate_weight(unsigned id,unsigned oi,const Edge& edge){
#if AQCF_STRIDE_NORMALIZED
        const auto rule=r50_stride(id,oi);
        if(rule.stride){++normalized_weight_edges;return stride_weight::spectrum(*context,edge,rule.stride,rule.support,r50_public_tile_spacing(id,oi));}
#endif
#if AQCF_STRIDE_NORMALIZED
        const auto spatial=r50_spatial(id,oi);
        if(spatial.ell){++spatial_weight_edges;return spatial_action::spectrum(*context,edge,spatial.ell,spatial.row,spatial.k,spatial.channels,spatial.unit);}
#endif
        return tile_weight::spectrum(*context,edge,r50_public_tile_spacing(id,oi));
    }
    static unsigned logarithm(U n){unsigned k=0;for(;n>1;n>>=1)++k;return k;}
    std::vector<U> direct_affine(int id,const std::vector<U>& own){
        require(id>=0 && std::size_t(id)<AFFINES && !requested[id],"affine once, complete port");
        if(selected28&&selected28(unsigned(id)))return produce28(unsigned(id),own);
        if(auto g=aqcf_bn19::find(id))return direct_bn19(*g,own);
        requested[id]=true;
        const auto& l=layouts[id];const auto gt=Steady::now();
        const auto g=kernel_geometry(l);const bool short_kernel=g.active && kernel!="full";
        const bool fiber=short_kernel && kernel=="fiber";
        // Frozen public policy: only the seven non-quotient layers use R15.
        // Short kernels keep the complete original matrix/finalizer path.
        const bool aux_mode=lift_plan!="native"&&!g.active;
        const bool spectral_input=query_encryption=="spectral-seeded"&&short_kernel&&!fiber&&!aux_mode;
        metrics.seconds["public_action_geometry"]+=elapsed(gt);
        if(short_kernel)++metrics.action_layers;
        const U ni=l.maps.size(),no=l.queries.size();
        expected_inputs+=ni;expected_outputs+=no;original_ranks.push_back(ni);ranks.push_back(ni);
        require(own.size()==l.inputs,"native affine input shape");pcn_data089::each("AQCF_SHARE_CHECK",own.size(),[&](std::size_t j){require(own[j]<modulus,"canonical own share");});
        require(no<=cap-reserved_returns,"return budget before evaluation");reserved_returns+=no;
        std::vector<U> out(l.outputs);
        if(s.party==2){
            number(U(id));number(ni);
            for(U i=0;i<ni;++i){
                if(i==1&&client_input_workers==4&&quarter_enabled&&query_encryption=="symmetric-seeded"){
                    send_pipeline_seeded(l,own,ni);break;
                }
                auto t=Steady::now();auto pt=encode(gather(l,i,own,modulus));metrics.seconds["gather_encode"]+=elapsed(t);
                t=Steady::now();
                if(spectral_input){
                    require(bool(spectral_secret),"client spectral secret key");ingress::Randomness random(*context);
                    auto ct=ingress::produce(*context,*spectral_secret,pt,random);auto frame=ingress::serialize(*context,ct,random.seed,45);
                    ++metrics.encryptions;metrics.seconds["spectral_input_encrypt_and_pack"]+=elapsed(t);send_spectral_input(frame);
                }else if(query_encryption=="symmetric-seeded"||query_encryption=="spectral-seeded"){
                    // Established SEAL client-side symmetric seeded encryption.
                    // The server still owns only PK. This is a common control option,
                    // NOT attributed to the new accumulator-finalizer identity.
                    auto ct=encryptor->encrypt_symmetric(pt);++metrics.encryptions;
                    metrics.seconds["input_encryption"]+=elapsed(t);send_seeded_query(ct);
                }else{seal::Ciphertext ct;encryptor->encrypt(pt,ct);++metrics.encryptions;
                    metrics.seconds["input_encryption"]+=elapsed(t);send_cipher(ct);}}
            if(!preparing18){require(number()==U(id) && number()==no,"return node/count framing");
            if(client_input_workers==4&&quarter_enabled&&wire=="packed-query")receive_return_pipeline(l,out);
            else for(U i=0;i<no;++i){seal::Ciphertext ct;receive_cipher(ct,&l.queries[i]);auto t=Steady::now();
                seal::Plaintext pt;if(quarter_enabled){ct.parms_id()=quarter_context->first_parms_id();quarter_decryptor->decrypt(ct,pt);}
                else decryptor->decrypt(ct,pt);++metrics.decryptions;metrics.seconds["masked_decryption"]+=elapsed(t);
                t=Steady::now();for(auto q:l.queries[i]){U v=q.exponent<pt.coeff_count()?pt[q.exponent]:0;
                    require(v<(quarter_enabled?4*modulus:modulus),"canonical masked query");out[q.destination]=v;}
                metrics.seconds["query_extraction"]+=elapsed(t);}}
        }else{
            const auto& layer=program->at(id);
            // Also checked before any peer traffic by the role entry point.
            if(short_kernel)certify_kernel_support(layer,g);
            auto t=Steady::now();
            if(aux_mode){++metrics.auxiliary_layers;if(!auxiliary)auxiliary=std::make_unique<NativeAuxiliaryLift>(*context,public_key);if(lift_plan=="aux-fused")auxiliary->prepare_fused_key();}
            metrics.seconds["R15_PK_and_auxiliary_preparation"]+=elapsed(t);t=Steady::now();std::vector<ActionNTT> transforms;
            std::vector<std::vector<U>> broadcast(data_limbs);
            if(short_kernel){
                require(!fiber,"source-bound R50 broadcast profile");
                auto tables=prepare_public_transform4(g.b,g.d,*context,aqcf_native4::allocation,
                  [](const char* label,const auto& receipt){std::cerr<<"AQCF_TABLE4 {\"label\":\""<<label<<"\",\"lanes\":[";
                    for(unsigned k=0;k<4;++k){require(receipt.items[k]>0&&receipt.cpu_ns[k]>0&&receipt.cpus[k]==aqcf_native4::cpu(k),"actual four table workers");
                      if(k)std::cerr<<',';std::cerr<<"{\"cpu\":"<<receipt.cpus[k]<<",\"jobs\":"<<receipt.items[k]<<",\"cpu_ns\":"<<receipt.cpu_ns[k]<<'}';}
                    std::cerr<<"]}\n";});
                transforms=std::move(tables.transforms);broadcast=std::move(tables.broadcast);
            }
            metrics.seconds["online_public_transform_tables"]+=elapsed(t);
            std::unique_ptr<window_action::Plan> window;
#if AQCF_WINDOW_ACTION
            const auto wr=r50_stride(id,0);
            if(!short_kernel&&!fiber&&!aux_mode&&!spectral_input&&mac_plan=="legacy"&&linear_workers==4&&!parallel_audit&&ni>1&&no>1&&(wr.stride==196||wr.stride==49)){
                U support=wr.support;for(U oi=1;oi<no;++oi){auto r=r50_stride(id,oi);require(r.stride==wr.stride,"public common stride only");support=std::max<U>(support,r.support);}
                require(quarter_enabled&&finalizer=="prescaled"&&query_encryption=="symmetric-seeded","unchanged complete producer gate");
                auto began=Steady::now();window=std::make_unique<window_action::Plan>(*context,wr.stride,support);
                metrics.seconds["window_public_plan_setup"]+=elapsed(began);++window_action_layers;window_action_inputs+=ni;window_action_outputs+=no;
            }
#endif
            require(number()==U(id) && number()==ni,"input node/count framing");
            std::vector<ActionCipher> inputs;inputs.reserve(ni);
            std::vector<aux15::Spectrum> auxiliary_inputs;if(aux_mode)auxiliary_inputs.reserve(ni);
            const bool take_ingress=linear_workers>1&&query_encryption=="symmetric-seeded"&&!spectral_input&&!aux_mode&&!fiber&&ni>1;
            if(take_ingress){
                inputs=receive_ingress_pipeline(l,own,ni,short_kernel,broadcast,window.get());
                metrics.input_restore+=ni;metrics.cipher_butterflies+=window?ni*2*data_limbs*(window->slots/2)*logarithm(window->L):ni*2*data_limbs*(DEGREE/2)*logarithm(DEGREE);
            }else for(U i=0;i<ni;++i){seal::Ciphertext ct;
                if(spectral_input){t=Steady::now();auto pt=encode(gather(l,i,own,modulus));metrics.seconds["gather_encode"]+=elapsed(t);
                    ct=receive_spectral_input(pt);++metrics.input_restore;
                }else{
                    if(query_encryption=="symmetric-seeded"||query_encryption=="spectral-seeded"){receive_seeded38(ct);check(ct);}else receive_cipher(ct);t=Steady::now();
                    auto pt=encode(gather(l,i,own,modulus));metrics.seconds["gather_encode"]+=elapsed(t);t=Steady::now();
                    evaluator->add_plain_inplace(ct,pt);++metrics.input_restore;metrics.seconds["input_share_restore"]+=elapsed(t);
                }
                t=Steady::now();
                if(aux_mode){auxiliary_inputs.push_back(auxiliary->input(ct));++metrics.auxiliary_inputs;metrics.seconds["R15_input_lift_and_both_NTTs"]+=elapsed(t);continue;}
                ActionCipher a;a.v.resize((2*data_limbs)*DEGREE);
                if(fiber){
                    for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){U* v=a.v.data()+(c*data_limbs+r)*DEGREE;
                        std::copy_n(ct.data(c)+r*DEGREE,DEGREE,v);transforms[r].transform(v,g.b,false);}
                }else{
                    if(!spectral_input)evaluator->transform_to_ntt_inplace(ct);
                    pcn_data089::each("AQCF_ROOT_INGRESS_COPY",2*data_limbs*DEGREE,[&](std::size_t pos){
                        const U c=pos/(data_limbs*DEGREE),r=(pos/DEGREE)%data_limbs,j=pos%DEGREE;
                        a.v[(c*data_limbs+r)*DEGREE+(short_kernel?broadcast[r][j]:j)]=ct.data(c)[r*DEGREE+j];});
                }
                metrics.cipher_butterflies+=(spectral_input?data_limbs:2*data_limbs)*(DEGREE/2)*logarithm(fiber?g.d:DEGREE);
                metrics.seconds[fiber?"input_fiber_NTT":"input_NTT"]+=elapsed(t);inputs.push_back(std::move(a));
            }
            // Public geometry determines the matrix path; zeros in the PRIVATE
            // edge bank are never used to select a faster/publicly shorter path.
            const bool matrix_mode=short_kernel&&mac_plan!="legacy";
            std::vector<std::vector<U>> action_outputs;
            if(matrix_mode){
                auto prep=Steady::now();++metrics.prepared_action_layers;
                std::vector<U> weights(data_limbs*g.d*no*ni,0);
                for(U oi=0;oi<no;++oi)for(const auto& e:layer.output[oi].edges){
                    for(unsigned r=0;r<data_limbs;++r){auto w=transforms[r].weight(e,g.b);
                        for(U k=0;k<g.d;++k)weights[((r*g.d+k)*no+oi)*ni+e.input]=w[k];}
                    metrics.short_weight_ntts+=data_limbs;metrics.weight_butterflies+=data_limbs*(g.d/2)*logarithm(g.d);
                }
                metrics.seconds["common_matrix_weight_preparation"]+=elapsed(prep);
                prep=Steady::now();action_outputs.assign(no,std::vector<U>((2*data_limbs)*DEGREE));
                std::vector<U> a(no*ni),b(ni*2*g.b);
                for(unsigned r=0;r<data_limbs;++r)for(U k=0;k<g.d;++k){
                    std::copy_n(weights.data()+(r*g.d+k)*no*ni,no*ni,a.data());
                    for(U i=0;i<ni;++i)for(unsigned c=0;c<2;++c)
                        std::copy_n(inputs[i].v.data()+(c*data_limbs+r)*DEGREE+k*g.b,g.b,b.data()+(i*2+c)*g.b);
                    // Corrections, factor sums, reductions and copies are all charged.
                    auto y=mac_plan=="strassen"?matrix_strassen(a,b,no,ni,2*g.b,primes[r],metrics.bilinear):
                           mac_plan=="winograd"?matrix_winograd(a,b,no,ni,2*g.b,primes[r],metrics.bilinear):
                                                 matrix_direct(a,b,no,ni,2*g.b,primes[r],metrics.bilinear);
                    for(U oi=0;oi<no;++oi)for(unsigned c=0;c<2;++c)
                        std::copy_n(y.data()+(oi*2+c)*g.b,g.b,action_outputs[oi].data()+(c*data_limbs+r)*DEGREE+k*g.b);
                }
                metrics.seconds["matrix_MAC_all_preparation_and_copies"]+=elapsed(prep);
            }
            // Conventional parallel execution of the EXISTING exact MAC.
            // Only local deterministic arithmetic runs on workers. No worker
            // touches protocol, shared evaluator, RNG, finalizer or network state.
            const bool parallel_mode=linear_workers>1&&!fiber&&!aux_mode&&!matrix_mode&&no>1;
            bool band_mode=false;
#if AQCF_BAND_ACTION
            if(id==0||id==2||id==3||id==4||id==5||id==7||id==8||id==9||id==11||id==12||id==13||id==15||id==16||id==17||id==18||id==20||id==21||id==23||id==25||id==26||id==27||id==29||id==30||id==31||id==33||id==34||id==35||id==36||id==38||id==39||id==40||id==42||id==43||id==44||id==46||id==47||id==48||id==50||id==51||id==52||id==54||id==55||id==56||id==58||id==59||id==60||id==61||id==63||id==64||id==65||id==67||id==68||id==69||id==71){
                require(parallel_mode&&!short_kernel&&!parallel_audit&&quarter_enabled&&finalizer=="prescaled","fixed public full producer band mode");
                for(const auto&o:layer.output)require(o.edges.size()<=65536,"public exact band fan-in bound");
                band_mode=true;++band_action_layers;
            }
#endif
            std::vector<std::vector<U>> parallel_values;
            const bool whole_producer=parallel_mode&&!parallel_audit&&quarter_enabled&&finalizer=="prescaled";
            const bool coefficient_window=bool(window)&&whole_producer;
            std::vector<seal::Ciphertext> protected_outputs;
            std::vector<std::vector<unsigned char>> protected_frames(no);
            std::array<double,4> pack_seconds4{};
            std::vector<LiftedZeroStats> producer_stats;
            std::vector<double> producer_seconds;
            if(whole_producer){
                // Validate publicly fixed writes: no two workers own one slot.
                std::vector<unsigned char> written(out.size(),0);
                for(const auto& o:layer.output)for(const auto& q:o.queries){
                    require(q.destination<out.size()&&q.exponent<DEGREE,"parallel public query bounds");
                    require(!written[q.destination],"parallel output shares must be disjoint");written[q.destination]=1;
                }
                require(std::all_of(written.begin(),written.end(),[](auto x){return x==1;}),"complete destination ownership");
                protected_outputs.resize(no);producer_stats.resize(linear_workers);producer_seconds.resize(linear_workers,0);
            }
            if(parallel_mode){
                auto wall=Steady::now();parallel_values.resize(no);
                const unsigned count=std::min<U>(linear_workers,no);require(count==4,"R50 producer needs four real outputs");
                std::vector<std::exception_ptr> errors(count);
                std::vector<aqcf_owner4::Task> threads;
                OrderedPublication publication(no);
                auto work=[&](unsigned lane){try{
                    cpu_set_t cpus;CPU_ZERO(&cpus);CPU_SET(aqcf_native4::cpu(lane),&cpus);
                    require(sched_setaffinity(0,sizeof(cpus),&cpus)==0,"linear worker CPU affinity");
                    linear_worker_cpus[lane]=sched_getcpu();
                    require(linear_worker_cpus[lane]==aqcf_native4::cpu(lane),"actual linear CPU evidence");
            aqcf_native4::Outer audit4(lane,"SERVER_PRODUCER");
                    seal::Evaluator lane_evaluator(*context);
                    auto pool=seal::MemoryPoolHandle::New();
                    // Each worker owns its finalizer and every fresh coin bank.
                    std::unique_ptr<PrescaledOriginalZero> lane_finalizer;
                    if(whole_producer)lane_finalizer=std::make_unique<PrescaledOriginalZero>(*context,public_key);
                    for(U oi=lane;oi<no;oi+=count){
                        const auto& o=layer.output.at(oi);
                        auto& value=parallel_values[oi];
                        if(band_mode){
                            aqcf_band::Accumulator sums(4*(window?window->slots:DEGREE));
                            for(const auto&e:o.edges){auto weight=window?window->weight(e):evaluate_weight(id,oi,e);
                                if(window){window->add(sums,inputs.at(e.input).v,weight);++window_action_edges;++normalized_weight_edges;}
                                else for(U c=0;c<2;++c)for(U r=0;r<2;++r){U at=(c*2+r)*DEGREE;sums.add(at,inputs.at(e.input).v.data()+at,weight.data()+r*DEGREE,DEGREE);}
                                ++band_action_edges;
                            }
                            if(window)value=coefficient_window?window->finish_coefficients(sums):window->finish_spectrum(sums);else{value.resize(4*DEGREE);for(U j=0;j<4*DEGREE;++j)value[j]=U(sums.word(j)%primes[(j/DEGREE)%2]);}
                            ++band_action_outputs;
                        }else if(window){
                            std::vector<__uint128_t> sums(4*window->slots,0);
                            for(const auto&e:o.edges){auto weight=window->weight(e);window->add(sums,inputs.at(e.input).v,weight);++window_action_edges;++normalized_weight_edges;}
                            value=window->finish_spectrum(sums);
                        }else{
                        std::vector<__uint128_t> sums(2*data_limbs*DEGREE,0);
                        for(const auto&e:o.edges){
                            std::vector<std::vector<U>> small;
                            std::vector<U> full;
                            if(short_kernel){for(unsigned r=0;r<data_limbs;++r)small.push_back(transforms[r].weight(e,g.b));}
                            else{
                                full=evaluate_weight(id,oi,e);
                            }
                            const auto& input=inputs.at(e.input).v;
                            for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){
                                U off=(c*data_limbs+r)*DEGREE;
                                if(short_kernel){for(U k=0;k<g.d;++k){const U w=small[r][k];
                                    for(U j=0;j<g.b;++j){U at=off+k*g.b+j;sums[at]+=__uint128_t(input[at])*w;}}}
                                else for(U j=0;j<DEGREE;++j)sums[off+j]+=__uint128_t(input[off+j])*full[r*DEGREE+j];
                            }
                        }
                        value.resize(2*data_limbs*DEGREE);
                        for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r)for(U j=0;j<DEGREE;++j){
                            U off=(c*data_limbs+r)*DEGREE;
                            value[off+j]=U(sums[off+(short_kernel?broadcast[r][j]:j)]%primes[r]);
                        }
                        }
                        if(whole_producer){
                            auto started=Steady::now();
                            auto& ct=protected_outputs[oi];ct=lane_finalizer->finish(value,producer_stats[lane],coefficient_window);if(coefficient_window)++joint_coefficient_returns;
                            seal::Plaintext bias;bias.resize(DEGREE);
                            for(const auto& q:o.queries)bias[q.exponent]=aqcf2::residue(W(q.bias),modulus);
                            lane_evaluator.add_plain_inplace(ct,bias);
                            // Unchanged full coefficient mask; independent fresh PRNG
                            // per call. No Sci/session/network operation in a worker.
                            if(retained299::enabled())retained299::spatial(ct,*quarter_context,o.queries,out);else quarter_mask_bulk(ct,*quarter_context,o.queries,out);++joint_bulk_mask_returns;check(ct);
                            producer_seconds[lane]+=elapsed(started);
                            auto pack_start4=Steady::now();protected_frames[oi]=pack_cipher4(ct,&o.queries);
                            pack_seconds4[lane]+=elapsed(pack_start4);
                            publication.publish(oi);
                        }
                ++audit4.jobs;
                        ++linear_worker_outputs[lane];
                    }
                }catch(...){errors[lane]=std::current_exception();publication.fail();}};
                if(whole_producer){
                    std::exception_ptr consumer_error;
                    try{
                        // All compute workers still occupy exactly 0,2,4,6.
                        // The network owner shares CPU0; it adds no physical core.
                        for(unsigned lane=0;lane<count;++lane)threads.emplace_back(work,lane);
                        number(U(id));number(no);
                        for(U oi=0;oi<no;++oi){
                            if(!publication.wait(oi))throw std::runtime_error("protected producer failed");
                            send_packed4(protected_frames[oi],true);
                            std::vector<unsigned char>().swap(protected_frames[oi]);
                        }
                    }catch(...){consumer_error=std::current_exception();publication.fail();}
                    for(auto&thread:threads)thread.join();
                    for(auto&e:errors)if(e)std::rethrow_exception(e);
                    if(consumer_error)std::rethrow_exception(consumer_error);
                    metrics.seconds["ordered_protected_production_and_transport_wall"]+=elapsed(wall);
                }else{
                    try{for(unsigned lane=0;lane<count;++lane)threads.emplace_back(work,lane);}
                    catch(...){for(auto&thread:threads)thread.join();throw;}
                    for(auto&thread:threads)thread.join();
                }
                for(auto&e:errors)if(e)std::rethrow_exception(e);
                maximum_linear_workers=std::max(maximum_linear_workers,count);
                ++parallel_layers;parallel_outputs+=no;
                if(!whole_producer)metrics.seconds["parallel_existing_MAC_wall"]+=elapsed(wall);
                if(whole_producer){
                    for(unsigned lane=0;lane<count;++lane){
                        const auto& z=producer_stats[lane];
                        metrics.lifted.fresh_zeros+=z.fresh_zeros;metrics.lifted.u_forward_ntts+=z.u_forward_ntts;
                        metrics.lifted.inverse_ntts+=z.inverse_ntts;metrics.lifted.avoided_inverse_ntts+=z.avoided_inverse_ntts;
                        metrics.lifted.pk_dyadic_products+=z.pk_dyadic_products;
                        metrics.lifted.accumulator_scale_products+=z.accumulator_scale_products;
                        metrics.lifted.modulus_switch_components+=z.modulus_switch_components;
                        metrics.seconds["parallel_complete_producer_worker_seconds_not_wall"]+=producer_seconds[lane];
                    }
                    metrics.fresh_zero+=no;if(!retained299::enabled()){metrics.full_mask+=no*DEGREE;metrics.c0_add+=no*data_limbs*DEGREE;}
                    metrics.bias_add+=no;
                }
            }
            if(!whole_producer){number(U(id));number(no);}U output_index=0;
            for(auto v:pack_seconds4)metrics.seconds["protected_pack_worker_sum_not_wall"]+=v;
            for(const auto& o:layer.output){
                expected_pm+=o.edges.size();expected_adds+=o.edges.empty()?0:o.edges.size()-1;
                if(whole_producer){
                    metrics.pm+=o.edges.size();metrics.ct_add+=o.edges.empty()?0:o.edges.size()-1;
                    if(o.edges.empty())++metrics.zero_outputs;
                    if(short_kernel)metrics.short_weight_ntts+=data_limbs*o.edges.size();
                    metrics.weight_butterflies+=data_limbs*(g.d/2)*logarithm(g.d)*o.edges.size();
                    // Same complete outputs were already sent in original order.
                    ++output_index;continue;
                }
                if(aux_mode){
                    t=Steady::now();auto a=auxiliary->engine.accumulate(auxiliary_inputs,o,1);
                    metrics.seconds["R15_shared_weights_and_both_MACs"]+=elapsed(t);t=Steady::now();
                    auto acc=auxiliary->finish(std::move(a),lift_plan=="aux-fused");
                    metrics.seconds["R15_zero_sampling_inverse_CRT_moddown"]+=elapsed(t);
                    ++metrics.fresh_zero;++metrics.auxiliary_outputs;
                    metrics.pm+=o.edges.size();metrics.ct_add+=o.edges.empty()?0:o.edges.size()-1;
                    if(o.edges.empty())++metrics.zero_outputs;
                    ++output_index;check(acc);t=Steady::now();
                    seal::Plaintext bias;bias.resize(DEGREE);for(auto q:o.queries)bias[q.exponent]=aqcf2::residue(W(q.bias),modulus);
                    evaluator->add_plain_inplace(acc,bias);++metrics.bias_add;metrics.seconds["Q32_bias_once"]+=elapsed(t);
                    protect(acc,o.queries,out,true);send_cipher(acc,&o.queries);continue;
                }
                std::vector<__uint128_t> sums((2*data_limbs)*DEGREE,0);bool initialized=false;
                const bool take_parallel=parallel_mode&&!parallel_audit;
                if(matrix_mode||take_parallel){metrics.pm+=o.edges.size();metrics.ct_add+=o.edges.empty()?0:o.edges.size()-1;initialized=!o.edges.empty();
                    if(take_parallel){if(short_kernel)metrics.short_weight_ntts+=data_limbs*o.edges.size();
                        metrics.weight_butterflies+=data_limbs*(g.d/2)*logarithm(g.d)*o.edges.size();}}
                else for(const auto& e:o.edges){
                    t=Steady::now();std::vector<std::vector<U>> small;std::vector<U> full;
                    if(short_kernel){for(unsigned r=0;r<data_limbs;++r)small.push_back(transforms[r].weight(e,g.b));
                        metrics.short_weight_ntts+=data_limbs;metrics.weight_butterflies+=data_limbs*(g.d/2)*logarithm(g.d);
                    }else{
                        full=evaluate_weight(id,output_index,e);
                        const U span=r50_public_tile_spacing(id,output_index);metrics.weight_butterflies+=data_limbs*((DEGREE/span)/2)*logarithm(DEGREE/span);
                    }
                    metrics.seconds[short_kernel?"private_weight_short_NTT":"private_weight_tile_spectrum_including_public_tables"]+=elapsed(t);t=Steady::now();
                    const auto& input=inputs.at(e.input).v;
                    for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){const U off=(c*data_limbs+r)*DEGREE;
                        if(short_kernel){for(U k=0;k<g.d;++k){const U w=small[r][k];for(U lane=0;lane<g.b;++lane){
                            const U at=off+k*g.b+lane;sums[at]+=__uint128_t(input[at])*w;}}}
                        else {for(U j=0;j<DEGREE;++j)sums[off+j]+=__uint128_t(input[off+j])*full[r*DEGREE+j];}
                    }
                    ++metrics.pm;if(initialized)++metrics.ct_add;initialized=true;metrics.seconds["PM_CT_add"]+=elapsed(t);
                }
                t=Steady::now();std::vector<U> value((2*data_limbs)*DEGREE);
                if(take_parallel)value=std::move(parallel_values[output_index]);
                else if(matrix_mode)value=std::move(action_outputs[output_index]);
                else for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r)for(U j=0;j<DEGREE;++j){
                    U at=(c*data_limbs+r)*DEGREE+j;value[at]=U(sums[at]%primes[r]);}
                ++output_index;metrics.seconds["PM_CT_add"]+=elapsed(t);t=Steady::now();
                // Undo only the public spectral permutation. Do not invert before
                // the lifted finalizer: those inverse transforms are what it removes.
                if(short_kernel&&!fiber&&!take_parallel)for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){
                    U* v=value.data()+(c*data_limbs+r)*DEGREE;std::vector<U> ordinary(DEGREE);
                    for(U j=0;j<DEGREE;++j)ordinary[j]=v[broadcast[r][j]];
                    std::copy(ordinary.begin(),ordinary.end(),v);}
                metrics.seconds["common_output_spectrum_order"]+=elapsed(t);t=Steady::now();
                if(parallel_mode&&parallel_audit){
                    require(value==parallel_values.at(output_index-1),"parallel MAC must match frozen serial accumulator bit for bit");
                    ++parallel_audited_outputs;
                }
                seal::Ciphertext acc;
                if(finalizer=="prescaled"){
                    if(!original_prescaled)original_prescaled=std::make_unique<PrescaledOriginalZero>(*context,public_key);
                    acc=original_prescaled->finish(value,metrics.lifted);
                    ++metrics.fresh_zero;metrics.seconds["common_prescaled_original_zero_and_inverse"]+=elapsed(t);
                }else if(finalizer=="lifted"){
                    acc=lifted_pk_zero(*context,public_key,value,metrics.lifted);
                    ++metrics.fresh_zero;metrics.seconds["lifted_PK_zero_inverse_and_moddown"]+=elapsed(t);
                }else{
                    for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){U* v=value.data()+(c*data_limbs+r)*DEGREE;
                        if(fiber)transforms[r].transform(v,g.b,true);
                        else seal::util::inverse_ntt_negacyclic_harvey(v,context->first_context_data()->small_ntt_tables()[r]);}
                    metrics.cipher_butterflies+=2*data_limbs*(DEGREE/2)*logarithm(fiber?g.d:DEGREE);
                    metrics.seconds[fiber?"output_inverse_fiber_NTT":"output_inverse_NTT_or_zero"]+=elapsed(t);t=Steady::now();
                    encryptor->encrypt_zero(context->first_parms_id(),acc);check(acc);++metrics.fresh_zero;
                    metrics.seconds["fresh_PK_zero"]+=elapsed(t);t=Steady::now();
                    for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r)for(U j=0;j<DEGREE;++j){
                        U& z=acc.data(c)[r*DEGREE+j];z+=value[(c*data_limbs+r)*DEGREE+j];if(z>=primes[r])z-=primes[r];}
                    metrics.seconds["fresh_zero_coefficient_add"]+=elapsed(t);
                }
                if(!initialized)++metrics.zero_outputs;
                check(acc);t=Steady::now();
                seal::Plaintext bias;bias.resize(DEGREE);for(auto q:o.queries)bias[q.exponent]=aqcf2::residue(W(q.bias),modulus);
                evaluator->add_plain_inplace(acc,bias);++metrics.bias_add;metrics.seconds["Q32_bias_once"]+=elapsed(t);
                protect(acc,o.queries,out,true);send_cipher(acc,&o.queries);
            }
        }
        if(preparing18){require(defer_raw_phase,"prepared phases defer original recovery");}
        else if(quarter_enabled&&defer_raw_phase){require(modulus==(U(1)<<37),"exact raw Ring39 phase only");raw_phase_rows+=out.size();}
        else if(quarter_enabled){auto qt=Steady::now();require(bool(recover_four),"four-lane recovery bound");recover_four(out);quarter_correction_s+=elapsed(qt);
          metrics.seconds["quarter_2pc_recovery_all_costs"]+=elapsed(qt);quarter_rows+=out.size();}
        if(auxiliary){metrics.auxiliary_stats=auxiliary->engine.stats;metrics.auxiliary_pk_inverse=auxiliary->original_pk_inverse_ntts;}
        metrics.input_ct+=ni;metrics.output_ct+=no;return out;
    }
    #include "resnet50/engine/batch_norm_retained_members.inc"
    #include "resnet50/engine/batch_norm_affine_members.inc"
    #include "resnet50/engine/online_affine_members.inc"
    std::vector<I> final100(const std::vector<U>& own){
        require(prepared18&&prepared_count18==71&&consumed_count18==71&&std::all_of(consumed18.begin(),consumed18.end(),[](bool x){return x;}),"all original affine consumers exactly once");
        std::cout<<"LINEAR18_FINAL "<<s.party<<" "<<prepared_count18<<" "<<consumed_count18<<" "<<correction_bytes18<<"\n"<<std::flush;
        require(own.size()==1002 && std::all_of(requested.begin(),requested.end(),[](bool b){return b;}),"final-only release after all53");
        pcn_data089::each("AQCF_FINAL_SHARE_CHECK",own.size(),[&](std::size_t j){require(own[j]<aqcf2::M,"final share canonical");});std::vector<I> out;flush18();
        if(s.party==1){s.io.io->send_data(own.data(),8016);flush18();last_payload_sent4=all_sent4();}
        else{std::vector<U> peer(1002);s.io.io->recv_data(peer.data(),8016);
            require(service_started&&!service_finished,"first ciphertext through final payload timer");
            const auto service_end037=aqcf2::ClockSample::now();service_delta=service_end037.since(service_start);service_finished=true;last_payload_sent4=all_sent4();
            std::cout<<"WAN_SERVICE_END\n"<<std::flush;std::cout<<std::setprecision(17)<<"CLOCK_BOUNDARY037 END "<<service_end037.raw<<" "<<service_end037.monotonic<<"\n"<<std::flush;auto recovery_begin4=Steady::now();out.resize(1002);
            pcn_data089::each("AQCF_FINAL_RECOVER",1002,[&](std::size_t j){require(peer[j]<aqcf2::M,"canonical peer final share");out[j]=aqcf2::centered((peer[j]+own[j])%aqcf2::M,aqcf2::M);});final_recovery4=elapsed(recovery_begin4);}
        require(frame_counter_started4&&last_payload_sent4>=first_frame_sent4,"complete exact sent counter interval");
        std::cout<<"AQCF_SERVICE_METERS {\"party\":"<<s.party<<",\"first_frame_sent\":"<<first_frame_sent4
          <<",\"last_payload_sent\":"<<last_payload_sent4<<",\"online_sent_bytes\":"<<last_payload_sent4-first_frame_sent4
          <<",\"final_recovery_seconds\":"<<std::setprecision(17)<<final_recovery4<<"}\n"<<std::flush;
        flush18();return out;
    }
};
} // namespace aqcf3
