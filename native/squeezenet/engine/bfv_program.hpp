#include "squeezenet/engine/squeezenet_pipeline.hpp"
#include "squeezenet/engine/owned_squeezenet_pipeline.hpp"
#ifndef AQCF_CPU_BASE
#define AQCF_CPU_BASE 0
#endif
#pragma once
#include "squeezenet/engine/packed_phase_wire.hpp"
#include "squeezenet/engine/squeezenet_input_rounding.hpp"
#include "squeezenet/engine/input_support_projection.hpp"
#include "squeezenet/engine/retained_squeezenet_component.hpp"
#include "squeezenet/engine/program.hpp"
#include "squeezenet/engine/module_basis.hpp"
#include "squeezenet/engine/kernel_action.hpp"
#include "squeezenet/engine/parallel_public_transform.hpp"
#include "squeezenet/engine/coefficient_wire.hpp"
#include <atomic>
#include <sys/random.h>
#include <cerrno>
#ifndef AQCF_UNIFORM_INPUT_SHARE
#define AQCF_UNIFORM_INPUT_SHARE 0
#endif
#include "squeezenet/engine/parallel_ingress.hpp"
#ifndef AQCF_HE_PROJECTED_OUTPUT
#define AQCF_HE_PROJECTED_OUTPUT 0
#endif
#include "squeezenet/engine/bilinear_action.hpp"
#include "squeezenet/engine/lifted_pk_zero.hpp"
#include "squeezenet/engine/root_flood.hpp"
#include "squeezenet/engine/quarter_protocol.hpp"
#include "squeezenet/engine/native_auxiliary_lift.hpp"
#include "squeezenet/engine/spectral_ingress.hpp"
#if !defined(AQCF_QUARTER_ADMITTED_REAL) || !defined(AQCF_BACKBONE_ADMITTED)
#error "Separate source-bound MobileNetV2 conditional admission required"
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
#include "squeezenet/engine/sci.hpp"
#include "squeezenet/engine/clock_sample.hpp"
#include <seal/seal.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <chrono>
#include <map>
#include <memory>
#include <sstream>
#include <thread>
#include <exception>
#include <functional>
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
    static constexpr bool projected_output=AQCF_HE_PROJECTED_OUTPUT;
    static constexpr U plaintext_modulus=projected_output?(U(1)<<37):aqcf2::M;
    std::atomic<U> narrowed_returns{0},retained303_returns{0},retained303_coefficients{0},retained303_queries{0},retained303_saved_bytes{0};
    void mask_return303(seal::Ciphertext& ct,const seal::SEALContext& quarter,const std::vector<Query>& query,std::vector<U>& own){
      if(retain296::enabled()){
        require(projected_output&&wire=="packed-query","303 SQ projected return only");
        retain296::first(ct,quarter,query,own);
        ++retained303_returns;retained303_coefficients+=DEGREE;retained303_queries+=query.size();
        retained303_saved_bytes+=(53*(DEGREE+query.size())+7)/8-(53*DEGREE+7)/8;
      }else quarter_mask(ct,quarter,query,own);
    }
    static constexpr bool quarter_enabled=AQCF_QUARTER_PROFILE!=0;
    static constexpr unsigned data_limbs=AQCF_QUARTER_PROFILE==2?2:4;
#if AQCF_QUARTER_PROFILE==2
    static constexpr std::array<U,3> primes=projected_output?std::array<U,3>{9007199254429697ULL,2199023190017ULL,549755731969ULL}:std::array<U,3>{140737488125953ULL,140737488273409ULL,274877562881ULL};
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
    // Full MobileNetV2 trained export. PUBLIC only; no private run admission.
    static constexpr U cap=4288;
#else
    static constexpr unsigned flood_exponent=115, flood_multiplier=3;
    static constexpr U cap=2048;
#endif
    static constexpr U Estar=4400193998851ULL;
    std::function<void(std::vector<U>&)> recovery_workers;
    bool defer_quarter_recovery=false;U deferred_quarter_rows=0;
    aqcf2::Sci& s; U modulus; const Layouts& layouts; const Program* program;
    std::shared_ptr<seal::SEALContext> context;
    std::unique_ptr<seal::Encryptor> encryptor;std::unique_ptr<seal::Evaluator> evaluator;
    std::unique_ptr<seal::Decryptor> decryptor;seal::PublicKey public_key;
    std::unique_ptr<seal::SecretKey> spectral_secret, client_secret;
    std::array<std::unique_ptr<seal::Encryptor>,4> input_encryptors;
    std::array<std::unique_ptr<seal::Decryptor>,4> output_decryptors;
    std::map<std::string,aqcf_compute4::Evidence> compute4_receipts;
    std::array<int,4> compute_cpus()const{return aqcf_native4::allocation;}
    void record_compute4(const char* phase,int id,const aqcf_compute4::Evidence& e){
      auto& sum=compute4_receipts[phase];
      std::cout<<"COMPUTE4_NODE "<<phase<<" "<<id;
      for(unsigned k=0;k<4;++k){require(e.cpus[k]==compute_cpus()[k]&&e.items[k]>0&&e.cpu_ns[k]>0,"actual four useful workers");
        sum.items[k]+=e.items[k];sum.cpu_ns[k]+=e.cpu_ns[k];sum.cpus[k]=e.cpus[k];
        std::cout<<" "<<e.cpus[k]<<":"<<e.items[k]<<":"<<e.cpu_ns[k];}
      std::cout<<"\n"<<std::flush;
    }
    template<class Work>void chunks4(const char* phase,int id,std::size_t n,Work work){
      require(n>=4,"four nonempty arithmetic chunks");
      auto e=aqcf_compute4::ordered<int>(4,compute_cpus(),[&](std::size_t k,unsigned lane){
        require(k==lane,"one real chunk per worker");work(n*k/4,n*(k+1)/4);return 0;
      },[](std::size_t,int){});record_compute4(phase,id,e);
    }
    static constexpr bool uniform_input_share=AQCF_UNIFORM_INPUT_SHARE;
    std::array<U,2> uniform_request_nonce{};
    std::string uniform_own_admission;
    std::array<int,AFFINES> uniform_tensor_ids{};
    bool uniform_session_bound=false;
    int uniform_held_first=-1;
    seal::parms_id_type uniform_held_parms{};
    std::vector<U> uniform_held_own;
    std::vector<seal::Ciphertext> uniform_ordinary_inputs;
    U uniform_shared_pairs=0,uniform_saved_inputs=0,uniform_metadata_bytes=0;
    U pipeline416_layers=0,pipeline416_outputs=0;
    static bool uniform_first(int id){return id==2||id==5||id==8||id==11||id==14||id==17||id==20||id==23;}
    static bool uniform_second(int id){return uniform_first(id-1);}
    void bind_uniform_request(const std::string& own_proof,const std::array<int,AFFINES>& tensor_ids){
      if(!uniform_input_share)return;
      require(!uniform_session_bound&&digest_string(own_proof),"new one-use owner admission for uniform input");
      for(bool used:requested)require(!used,"no affine before fresh request binding");
      uniform_own_admission=own_proof;uniform_tensor_ids=tensor_ids;
      chunks4("UNIFORM_PUBLIC_MAP_IDENTITY",-1,8,[&](std::size_t lo,std::size_t hi){for(auto pair=lo;pair<hi;++pair){int first=2+3*int(pair);
        const auto& a=layouts[first];const auto& b=layouts[first+1];
        require(tensor_ids[first]>=0&&tensor_ids[first]==tensor_ids[first+1],"same public executor tensor identity");
        require(a.inputs==b.inputs&&a.maps.size()==b.maps.size(),"full uniform public map shape");
        for(U i=0;i<a.maps.size();++i){require(a.maps[i].size()==b.maps[i].size(),"full uniform map row count");
          for(U j=0;j<a.maps[i].size();++j){const auto& x=a.maps[i][j];const auto& y=b.maps[i][j];
            require(x.source==y.source&&x.exponent==y.exponent&&x.multiplier==y.multiplier,"same full map, no digest shortcut");}}
      }});
      // Fresh public session identity, independently sampled from OS entropy;
      // not from the secret encryption/OT generators and never a cache key on disk.
      if(s.party==2){unsigned char* p=reinterpret_cast<unsigned char*>(uniform_request_nonce.data());std::size_t done=0;
        while(done<sizeof(uniform_request_nonce)){auto n=::getrandom(p+done,sizeof(uniform_request_nonce)-done,0);
          if(n<0&&errno==EINTR)continue;require(n>0,"fresh uniform request nonce");done+=std::size_t(n);}
        number(uniform_request_nonce[0]);number(uniform_request_nonce[1]);
        require(number()==uniform_request_nonce[0]&&number()==uniform_request_nonce[1],"uniform request echo");
      }else{uniform_request_nonce[0]=number();uniform_request_nonce[1]=number();
        number(uniform_request_nonce[0]);number(uniform_request_nonce[1]);}
      uniform_metadata_bytes+=32;uniform_session_bound=true;
    }
    bool prepare_uniform_input(int id,const std::vector<U>& own){
      if(!uniform_input_share)return false;
      require(uniform_session_bound&&!uniform_own_admission.empty(),"one admitted live context/request only");
      if(uniform_first(id)){
        require(uniform_held_first==-1&&uniform_held_own.empty()&&uniform_ordinary_inputs.empty(),"no abandoned shared input");
        uniform_held_first=id;uniform_held_parms=context->first_parms_id();uniform_held_own.resize(own.size());
        chunks4("UNIFORM_INPUT_IDENTITY_SAVE",id,own.size(),[&](std::size_t a,std::size_t b){
          for(auto j=a;j<b;++j)uniform_held_own[j]=own[j];});
        if(s.party==1)uniform_ordinary_inputs.resize(layouts[id].maps.size());
        return false;
      }
      if(uniform_second(id)){
        require(uniform_held_first==id-1&&uniform_held_parms==context->first_parms_id()&&uniform_held_own.size()==own.size(),"same exact live input/context");
        require(uniform_tensor_ids[id]==uniform_tensor_ids[id-1],"same tensor source");
        chunks4("UNIFORM_INPUT_IDENTITY_CHECK",id,own.size(),[&](std::size_t a,std::size_t b){
          for(auto j=a;j<b;++j)require(uniform_held_own[j]==own[j],"same complete owner share, never send shares or their hash");});
        if(s.party==1)require(uniform_ordinary_inputs.size()==layouts[id].maps.size(),"complete retained input ciphertexts");
        return true;
      }
      require(uniform_held_first==-1,"unexpected affine while shared input is live");return false;
    }
    void finish_uniform_input(int id,bool reuse,U nominal_inputs){
      if(!uniform_input_share)return;
      if(reuse){require(uniform_second(id)&&uniform_held_first==id-1,"consume exact paired input once");
        ++uniform_shared_pairs;uniform_saved_inputs+=nominal_inputs;
        uniform_ordinary_inputs.clear();uniform_held_own.clear();uniform_held_first=-1;uniform_held_parms={};
        std::cout<<"UNIFORM_INPUT_PAIR_CONSUMED "<<id-1<<" "<<id<<" "<<nominal_inputs<<"\n"<<std::flush;
      }
    }
    void verify_uniform_complete()const{
      if(!uniform_input_share)return;
      require(uniform_session_bound&&uniform_held_first==-1&&uniform_held_own.empty()&&uniform_ordinary_inputs.empty(),"no live shared-input cache at request end");
      require(uniform_shared_pairs==8&&uniform_saved_inputs==120&&uniform_metadata_bytes==656,"complete bound uniform input ledger");
    }


    using WireCounters=std::array<U,12>;
    std::function<WireCounters()> wire_counter_snapshot;
    WireCounters cipher_wire_begin{},cipher_wire_end{};
    bool cipher_wire_started=false,cipher_wire_finished=false;
    void begin_cipher_wire(){
        require(bool(wire_counter_snapshot)&&!cipher_wire_started&&!cipher_wire_finished,"fresh frame-range counter start");
        cipher_wire_begin=wire_counter_snapshot();cipher_wire_started=true;
    }
    void end_cipher_wire(){
        require(bool(wire_counter_snapshot)&&cipher_wire_started&&!cipher_wire_finished,"complete frame-range counter end");
        cipher_wire_end=wire_counter_snapshot();
        for(unsigned k=0;k<12;++k)require(cipher_wire_end[k]>=cipher_wire_begin[k],"monotonic send counters");
        cipher_wire_finished=true;
    }
    U cipher_wire_send_bytes()const{
        require(cipher_wire_started&&cipher_wire_finished,"no partial counter evidence");U sum=0;
        for(unsigned k=0;k<12;++k){U d=cipher_wire_end[k]-cipher_wire_begin[k];require(sum<=U(-1)-d,"counter sum overflow");sum+=d;}return sum;
    }
    std::atomic<U> wire288_encoded{0},wire288_decoded{0};
    std::atomic<U> support402_frames{0},support402_coefficients{0},support402_saved_bytes{0};
    struct PreparedInput4{std::string bytes;double gather=0,crypto=0,codec=0;};
    using WireBytes4=std::vector<unsigned char>;
    void send_seeded_bytes4(const std::string& bytes,U expected402){
      require(bytes.size()==expected402,"402 exact public input frame size");
      if(!service_started){require(s.party==2,"client timer");begin_cipher_wire();service_start=aqcf2::ClockSample::now();service_started=true;std::cout<<"WAN_SERVICE_BEGIN\n"<<std::flush;}
      number(bytes.size());auto t=Steady::now();s.io.io->send_data(bytes.data(),s.dim(bytes.size()));s.flush();
      sent_bytes+=bytes.size();metrics.input_payload_bytes+=bytes.size();metrics.seconds["send_wait"]+=elapsed(t);
    }
    U packed_size4(const std::vector<Query>& query)const{
      U width=0;for(unsigned r=0;r<(projected_output?1:data_limbs);++r)width+=coefficient_bits(primes[r]);
      return ((DEGREE+(retain296::enabled()?0:query.size()))*width+7)/8;
    }
    WireBytes4 receive_frame4(U expected,bool packed){
      if(!packed&&!cipher_wire_started){require(s.party==1,"server input receive boundary");begin_cipher_wire();}
      U n=number();require(n>0&&n<=(8ULL<<20),"bounded unchanged frame");
      if(expected)require(n==expected,"fixed packed return size");
      else require(n==aqcf_input379::frame_size(),"290 exact received input size");
      WireBytes4 bytes(n);auto t=Steady::now();s.io.io->recv_data(bytes.data(),s.dim(n));received_bytes+=n;
      metrics.seconds["receive_wait"]+=elapsed(t);if(packed)metrics.output_payload_bytes+=n;return bytes;
    }
    WireBytes4 pack4(const seal::Ciphertext& ct,const std::vector<Query>& query)const{
      check(ct);const unsigned limbs=ct.coeff_modulus_size();require(limbs==(projected_output?1:data_limbs),"unchanged packed limbs");
      CoefficientWriter w;
      for(unsigned c=retain296::enabled()?1:0;c<2;++c)for(unsigned r=0;r<limbs;++r){const U* a=ct.data(c)+r*DEGREE;unsigned bits=coefficient_bits(primes[r]);
        if(c==0){for(auto q:query)w.append(a[q.exponent],bits);}else for(U j=0;j<DEGREE;++j)w.append(a[j],bits);}
      return w.finish();
    }
    seal::Ciphertext unpack4(const WireBytes4& bytes,const std::vector<Query>& query)const{
      require(bytes.size()==packed_size4(query),"exact public packed size");const unsigned limbs=projected_output?1:data_limbs;
      seal::Ciphertext ct;ct.resize(*context,limbs==1?context->last_parms_id():context->first_parms_id(),2);ct.is_ntt_form()=false;
      for(unsigned c=0;c<2;++c)std::fill_n(ct.data(c),limbs*DEGREE,U(0));CoefficientReader rd(bytes);
      for(unsigned c=retain296::enabled()?1:0;c<2;++c)for(unsigned r=0;r<limbs;++r){U* a=ct.data(c)+r*DEGREE;unsigned bits=coefficient_bits(primes[r]);
        if(c==0){for(auto q:query)a[q.exponent]=rd.take(bits,primes[r]);}else for(U j=0;j<DEGREE;++j)a[j]=rd.take(bits,primes[r]);}
      rd.end();check(ct);return ct;
    }
    void send_packed4(const WireBytes4& bytes){number(bytes.size());auto t=Steady::now();
      s.io.io->send_data(bytes.data(),s.dim(bytes.size()));s.flush();sent_bytes+=bytes.size();
      metrics.output_payload_bytes+=bytes.size();metrics.seconds["send_wait"]+=elapsed(t);}

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
        require(n==4&&!audit,"new overlay qualifies only explicit four-worker mode");
        linear_workers=n;parallel_audit=audit;
        require(query_encryption=="symmetric-seeded"&&wire=="packed-query"&&quarter_enabled&&kernel=="broadcast"&&mac_plan=="legacy"&&finalizer=="prescaled","frozen accepted path only");
        if(s.party==2){require(bool(client_secret),"client-local secret only");auto qsk=*client_secret;qsk.data().parms_id()=quarter_context->key_parms_id();
          auto evidence=aqcf_compute4::ordered<int>(4,compute_cpus(),[&](std::size_t k,unsigned lane){
            require(k==lane,"independent cipher worker ownership");
            input_encryptors[k]=std::make_unique<seal::Encryptor>(*context,public_key,*client_secret);
            output_decryptors[k]=std::make_unique<seal::Decryptor>(*quarter_context,qsk);return 0;
          },[](std::size_t,int){});record_compute4("CLIENT_CRYPTO_OBJECTS",-1,evidence);}

    }
    aqcf2::ClockSample service_start{},service_delta{};
    BfvProgram(aqcf2::Sci& session,U t,const Layouts& l,const Program* p,bool module=false,bool stream=true,
      std::string action="full",std::string transport="seal",
      std::string matrix_plan="legacy",std::string zero_plan="separate",std::string query_plan="public",std::string integer_lift="native")
      :s(session),modulus(t),layouts(l),program(p),use_module(module),stream_mac(stream),kernel(std::move(action)),wire(std::move(transport)),
        mac_plan(std::move(matrix_plan)),finalizer(std::move(zero_plan)),query_encryption(std::move(query_plan)),lift_plan(std::move(integer_lift)){
        require(!quarter_enabled||(t==plaintext_modulus&&lift_plan=="native"&&finalizer=="prescaled"&&query_encryption=="symmetric-seeded"&&wire=="packed-query"),"quarter exact producer contract");
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
        require(t==P || t==plaintext_modulus,"explicit public candidate plaintext module");
        require(!projected_output||(quarter_enabled&&t==(U(1)<<37)&&kernel=="broadcast"&&mac_plan=="legacy"),"projected affine fixed scope");
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
        if(s.party==2){seal::KeyGenerator keys(*context);keys.create_public_key(public_key);client_secret=std::make_unique<seal::SecretKey>(keys.secret_key());
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
        if(aqcf_input379::enabled()){aqcf_input376::bound();require(projected_output&&modulus==(U(1)<<37)&&data_limbs==2,"379 original projected SQ context only");}
        using boost::multiprecision::cpp_int;cpp_int Q=1;for(unsigned i=0;i<data_limbs;++i)Q*=primes[i];
        if(quarter_enabled){const cpp_int error=cpp_int(22)*(cpp_int(1)<<30)+4098;
          require(cpp_int(8)*modulus*error<Q,"quarter deterministic signed-affine error margin");
          if(projected_output){const cpp_int after=(error+primes[1]-1)/primes[1]+4098;
            require(cpp_int(8)*modulus*after<primes[0],"conditional projected-output quarter margin");}
          require(cap==4288,"fixed full SqueezeNet26 return roster only");return;}
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
    void number(U v){s.flush();s.io.io->send_data(&v,8);s.flush();sent_bytes+=8;}
    U number(){U v;s.flush();s.io.io->recv_data(&v,8);received_bytes+=8;return v;}
    template<class Obj>void send_object(const Obj& obj){auto time=Steady::now();std::ostringstream buffer(std::ios::binary);
        obj.save(buffer); // Fixed compiled SEAL default for ALL new controls.
        auto bytes=buffer.str();metrics.seconds["serialization"]+=elapsed(time);
        require(!bytes.empty() && bytes.size()<=(8ULL<<20),"SEAL message length");number(bytes.size());time=Steady::now();
        s.io.io->send_data(bytes.data(),int(bytes.size()));s.flush();sent_bytes+=bytes.size();metrics.seconds["send_wait"]+=elapsed(time);}
    template<class Obj>void send_seeded_query(const Obj& obj){
        auto t=Steady::now();std::ostringstream buffer(std::ios::binary);
        // No content-adaptive compression: the seeded frame length depends only
        // on the fixed public N/level/SEAL serialization version.
        obj.save(buffer,seal::compr_mode_type::none);auto bytes=buffer.str();
        metrics.seconds["serialization"]+=elapsed(t);
        require(bytes.size()>=data_limbs*DEGREE*8&&bytes.size()<data_limbs*DEGREE*8+512,"fixed seeded public frame family");
        if(!service_started){require(s.party==2,"client input service timer");
            service_start=aqcf2::ClockSample::now();service_started=true;std::cout<<"WAN_SERVICE_BEGIN\n"<<std::flush;}
        number(bytes.size());t=Steady::now();s.io.io->send_data(bytes.data(),int(bytes.size()));s.flush();
        sent_bytes+=bytes.size();metrics.input_payload_bytes+=bytes.size();metrics.seconds["send_wait"]+=elapsed(t);
    }
    template<class Obj>void receive_object(Obj& obj){U n=number();require(n>0 && n<=(8ULL<<20),"SEAL incoming frame length");
        std::string bytes(n,'\0');auto time=Steady::now();s.io.io->recv_data(bytes.data(),int(n));received_bytes+=n;
        metrics.seconds["receive_wait"]+=elapsed(time);time=Steady::now();std::istringstream buffer(bytes,std::ios::binary);
        obj.load(*context,buffer);require(buffer.peek()==std::char_traits<char>::eof(),"SEAL trailing bytes");metrics.seconds["deserialization"]+=elapsed(time);}
    void send_spectral_input(const ingress::Bytes& bytes){
        require(s.party==2&&spectral_secret&&bytes.size()==184432,"client-only exact spectral frame");
        if(!service_started){service_start=aqcf2::ClockSample::now();service_started=true;std::cout<<"WAN_SERVICE_BEGIN\n"<<std::flush;}
        number(bytes.size());auto t=Steady::now();s.io.io->send_data(bytes.data(),int(bytes.size()));s.flush();
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
        for(std::size_t j=0;j<DEGREE;++j){require(values[j]<modulus,"canonical coefficient share");p[j]=values[j];}return p;
    }
    void check(const seal::Ciphertext& ct)const{
        require(seal::is_valid_for(ct,*context) && ct.size()==2 && !ct.is_ntt_form() &&
          (ct.parms_id()==context->first_parms_id() || (projected_output&&ct.parms_id()==context->last_parms_id()&&ct.coeff_modulus_size()==1)) && !ct.is_transparent(),"BFV return representation");
    }
    void project_return(seal::Ciphertext& ct){
        if(!projected_output)return;
        require(ct.parms_id()==context->first_parms_id()&&ct.coeff_modulus_size()==2,"narrow only original evaluated return");
        // Worker-local evaluator: no shared mutable evaluation state.
        seal::Evaluator output_evaluator(*context);output_evaluator.mod_switch_to_next_inplace(ct);
        require(ct.parms_id()==context->last_parms_id()&&ct.coeff_modulus_size()==1,"exact chosen output level");
        narrowed_returns.fetch_add(1,std::memory_order_relaxed);
    }
    void protect(seal::Ciphertext& ct,const std::vector<Query>& queries,std::vector<U>& own,bool zero_already_added=false){
        require(s.party==1 && reserved_returns<=cap,"server-only protected return");
        // Zero linear maps can have a transparent pre-mask accumulator. No such
        // object is sent: full mask + fresh asymmetric EncZero + flood follows.
        require(seal::is_valid_for(ct,*context)&&ct.size()==2&&!ct.is_ntt_form()&&ct.parms_id()==context->first_parms_id(),"pre-protection BFV structure");
        if(quarter_enabled){require(zero_already_added,"all original fresh zero consumed before full-Q mask");auto qt=Steady::now();
          project_return(ct);mask_return303(ct,*quarter_context,queries,own);if(!retain296::enabled()){metrics.full_mask+=DEGREE;metrics.c0_add+=ct.coeff_modulus_size()*DEGREE;}
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
    void send_cipher(const seal::Ciphertext& ct,const std::vector<Query>* query=nullptr){
        check(ct);
        if(wire=="seal"){send_object(ct);return;}
        const unsigned wire_limbs=ct.coeff_modulus_size();
        require(wire_limbs==(projected_output&&query?1:data_limbs),"fixed public output level");
        auto t=Steady::now();CoefficientWriter w;
        for(unsigned c=retain296::enabled()&&query?1:0;c<2;++c)for(unsigned r=0;r<wire_limbs;++r){
            const U* a=ct.data(c)+r*DEGREE;const unsigned bits=coefficient_bits(primes[r]);
            if(c==0 && query){for(auto q:*query)w.append(a[q.exponent],bits);}
            else for(U j=0;j<DEGREE;++j)w.append(a[j],bits);
        }
        auto bytes=w.finish();metrics.seconds["lossless_wire_encode"]+=elapsed(t);
        number(bytes.size());t=Steady::now();s.io.io->send_data(bytes.data(),s.dim(bytes.size()));s.flush();
        sent_bytes+=bytes.size();metrics.seconds["send_wait"]+=elapsed(t);
        (query?metrics.output_payload_bytes:metrics.input_payload_bytes)+=bytes.size();
    }
    void receive_cipher(seal::Ciphertext& ct,const std::vector<Query>* query=nullptr){
        if(wire=="seal"){receive_object(ct);check(ct);return;}
        const unsigned wire_limbs=projected_output&&query?1:data_limbs;
        U width=0;for(unsigned r=0;r<wire_limbs;++r)width+=coefficient_bits(primes[r]);
        const U count=DEGREE+(query?(retain296::enabled()?0:query->size()):DEGREE),expected=(count*width+7)/8;
        U n=number();require(n==expected,"fixed public lossless ciphertext frame length");
        std::vector<unsigned char> bytes(n);auto t=Steady::now();
        s.io.io->recv_data(bytes.data(),s.dim(n));received_bytes+=n;metrics.seconds["receive_wait"]+=elapsed(t);
        t=Steady::now();ct.resize(*context,wire_limbs==1?context->last_parms_id():context->first_parms_id(),2);ct.is_ntt_form()=false;
        for(unsigned c=0;c<2;++c)std::fill_n(ct.data(c),wire_limbs*DEGREE,U(0));
        CoefficientReader rd(bytes);
        for(unsigned c=retain296::enabled()&&query?1:0;c<2;++c)for(unsigned r=0;r<wire_limbs;++r){
            U* a=ct.data(c)+r*DEGREE;const unsigned bits=coefficient_bits(primes[r]);
            if(c==0 && query){for(auto q:*query)a[q.exponent]=rd.take(bits,primes[r]);}
            else for(U j=0;j<DEGREE;++j)a[j]=rd.take(bits,primes[r]);
        }
        rd.end();check(ct);metrics.seconds["lossless_wire_decode"]+=elapsed(t);
        (query?metrics.output_payload_bytes:metrics.input_payload_bytes)+=n;
    }
    static unsigned logarithm(U n){unsigned k=0;for(;n>1;n>>=1)++k;return k;}
    std::vector<U> affine(int id,const std::vector<U>& own){
        require(id>=0 && std::size_t(id)<AFFINES && !requested[id],"affine once, complete port");requested[id]=true;
        const auto& l=layouts[id];const auto gt=Steady::now();
        auto validation_parallel=[&](const char* phase,std::size_t n,auto work){chunks4(phase,id,n,work);};
        const auto g=kernel_geometry_checked(l,validation_parallel);const bool short_kernel=g.active && kernel!="full";
        const bool fiber=short_kernel && kernel=="fiber";
        // Frozen public policy: only the seven non-quotient layers use R15.
        // Short kernels keep the complete original matrix/finalizer path.
        const bool aux_mode=lift_plan!="native"&&!g.active;
        const bool spectral_input=query_encryption=="spectral-seeded"&&short_kernel&&!fiber&&!aux_mode;
        metrics.seconds["public_action_geometry"]+=elapsed(gt);
        if(short_kernel)++metrics.action_layers;
        const U ni=l.maps.size(),no=l.queries.size();
        const auto support_start402=Steady::now();
        const auto plans402=aqcf_support402::enabled()?aqcf_support402::plans(l):std::vector<aqcf_support402::Plan>(ni);
        metrics.seconds["public_support402_plan_online_wall"]+=elapsed(support_start402);
        expected_inputs+=ni;expected_outputs+=no;original_ranks.push_back(ni);ranks.push_back(ni);
        require(own.size()==l.inputs,"native affine input shape");
        chunks4("INPUT_SHARE_CANONICAL",id,own.size(),[&](std::size_t a,std::size_t b){for(auto j=a;j<b;++j)require(own[j]<modulus,"canonical own share");});
        const bool reuse_uniform=prepare_uniform_input(id,own);
        require(ni>=4&&no>=4,"every affine has four nonempty ciphertext lanes");
        require(no<=cap-reserved_returns,"return budget before evaluation");reserved_returns+=no;
        std::vector<U> out(l.outputs);
        if(s.party==2){
            number(U(id));number(ni);
            if(uniform_input_share){number(uniform_request_nonce[0]);number(uniform_request_nonce[1]);number(U(reuse_uniform));uniform_metadata_bytes+=24;}
            auto wall=Steady::now();
            if(!reuse_uniform){
            auto ep=aqcf_compute4::ordered<PreparedInput4>(ni,compute_cpus(),[&](std::size_t i,unsigned lane){
                PreparedInput4 r;auto t=Steady::now();auto pt=encode(gather(l,i,own,modulus));r.gather=elapsed(t);t=Steady::now();
                auto ct=input_encryptors[lane]->encrypt_symmetric(pt);r.crypto=elapsed(t);t=Steady::now();
                std::ostringstream buffer(std::ios::binary);ct.save(buffer,seal::compr_mode_type::none);r.bytes=buffer.str();if(aqcf_wire288::enabled()){r.bytes=aqcf_support402::pack(r.bytes,plans402[i]);if(!plans402[i].empty()){++support402_frames;support402_coefficients+=8192-plans402[i].size();support402_saved_bytes+=aqcf_input379::frame_size()-r.bytes.size();}wire288_encoded.fetch_add(1,std::memory_order_relaxed);}r.codec=elapsed(t);return r;
            },[&](std::size_t i,PreparedInput4 r){++metrics.encryptions;
                metrics.seconds["gather_encode_worker_seconds_not_wall"]+=r.gather;
                metrics.seconds["input_encryption_worker_seconds_not_wall"]+=r.crypto;
                metrics.seconds["serialization_worker_seconds_not_wall"]+=r.codec;send_seeded_bytes4(r.bytes,aqcf_support402::frame_size(plans402[i]));});
            metrics.seconds["parallel_client_input_with_send_wall"]+=elapsed(wall);record_compute4("CLIENT_GATHER_ENCRYPT_SERIALIZE",id,ep);
            maximum_linear_workers=4;for(unsigned k=0;k<4;++k){linear_worker_cpus[k]=ep.cpus[k];linear_worker_outputs[k]+=ep.items[k];}
            } // genuinely omit only duplicate paired fresh-input transmission
            require(number()==U(id)&&number()==no,"return node/count framing");
            auto coverage_start=Steady::now();
            aqcf_validation4::destinations(l.queries,out.size(),U(DEGREE),validation_parallel);
            metrics.seconds["client_destination_validation_wall"]+=elapsed(coverage_start);
            std::array<double,4> decode_s{},decrypt_s{},extract_s{};wall=Steady::now();
            auto dp=aqcf_compute4::incoming<WireBytes4>(no,compute_cpus(),[&](std::size_t i){return receive_frame4(packed_size4(l.queries[i]),true);},
              [&](std::size_t i,unsigned lane,WireBytes4 bytes){auto t=Steady::now();auto ct=unpack4(bytes,l.queries[i]);decode_s[lane]+=elapsed(t);t=Steady::now();
                ct.parms_id()=projected_output?quarter_context->last_parms_id():quarter_context->first_parms_id();
                seal::Plaintext pt;output_decryptors[lane]->decrypt(ct,pt);decrypt_s[lane]+=elapsed(t);t=Steady::now();
                for(auto q:l.queries[i]){U v=q.exponent<pt.coeff_count()?pt[q.exponent]:0;require(v<4*modulus,"canonical masked query");out[q.destination]=v;}
                extract_s[lane]+=elapsed(t);});
            metrics.decryptions+=no;metrics.seconds["parallel_client_receive_decode_decrypt_wall"]+=elapsed(wall);
            for(unsigned k=0;k<4;++k){metrics.seconds["lossless_wire_decode_worker_seconds_not_wall"]+=decode_s[k];
              metrics.seconds["masked_decryption_worker_seconds_not_wall"]+=decrypt_s[k];metrics.seconds["query_extraction_worker_seconds_not_wall"]+=extract_s[k];}
            record_compute4("CLIENT_DECODE_DECRYPT_EXTRACT",id,dp);
        }else{
            const auto& layer=program->at(id);
            // Keep the original in-graph check, including every coefficient;
            // no assumption that a pretraffic comment eliminated this cost.
            auto support_start=Steady::now();
            if(short_kernel)aqcf_validation4::support(layer.output,g.b,validation_parallel);
            metrics.seconds["private_support_validation_wall"]+=elapsed(support_start);
            auto t=Steady::now();
            if(aux_mode){++metrics.auxiliary_layers;if(!auxiliary)auxiliary=std::make_unique<NativeAuxiliaryLift>(*context,public_key);if(lift_plan=="aux-fused")auxiliary->prepare_fused_key();}
            metrics.seconds["R15_PK_and_auxiliary_preparation"]+=elapsed(t);t=Steady::now();std::vector<ActionNTT> transforms;
            std::vector<std::vector<U>> broadcast(data_limbs);
            if(short_kernel){
                require(!fiber&&data_limbs==2,"public-table4 frozen broadcast path only");
                auto table4=prepare_public_transform4(g.b,g.d,*context,compute_cpus(),
                    [&](const char* phase,const aqcf_compute4::Evidence& e){record_compute4(phase,id,e);});
                transforms=std::move(table4.transforms);broadcast=std::move(table4.broadcast);
            }
            metrics.seconds["online_public_transform_tables"]+=elapsed(t);
            require(number()==U(id) && number()==ni,"input node/count framing");
            if(uniform_input_share){require(number()==uniform_request_nonce[0]&&number()==uniform_request_nonce[1]&&number()==U(reuse_uniform),"same live request and expected fresh/reuse frame");uniform_metadata_bytes+=24;}
            std::vector<ActionCipher> inputs;inputs.reserve(ni);
            std::vector<aux15::Spectrum> auxiliary_inputs;if(aux_mode)auxiliary_inputs.reserve(ni);
            require(!fiber&&!aux_mode&&!spectral_input,"freeze accepted server path");
            inputs.resize(ni);std::array<std::unique_ptr<seal::Evaluator>,4> restore_eval;
            for(auto& e:restore_eval)e=std::make_unique<seal::Evaluator>(*context);
            std::array<double,4> load_s{},gather_s{},restore_s{},ntt_s{};auto input_wall=Steady::now();
            if(reuse_uniform){
              require(!short_kernel,"paired spatial branch requires ordinary SEAL input order");
              auto ie=aqcf_compute4::ordered<int>(ni,compute_cpus(),[&](std::size_t i,unsigned){
                const auto& ct=uniform_ordinary_inputs[i];require(ct.is_ntt_form()&&ct.parms_id()==context->first_parms_id()&&ct.size()==2&&ct.coeff_modulus_size()==data_limbs,"retained full ordinary NTT identity");
                auto& a=inputs[i];a.v.resize(2*data_limbs*DEGREE);
                for(unsigned c=0;c<2;++c)std::copy_n(ct.data(c),data_limbs*DEGREE,a.v.data()+c*data_limbs*DEGREE);
                return 0;
              },[](std::size_t,int){});
              record_compute4("SERVER_SHARED_INPUT_VIEW",id,ie);
              metrics.seconds["shared_input_view_copy_wall"]+=elapsed(input_wall);
            }else{
            auto ie=aqcf_compute4::incoming<WireBytes4>(ni,compute_cpus(),[&](std::size_t i){return receive_frame4(aqcf_support402::frame_size(plans402[i]),false);},
              [&](std::size_t i,unsigned lane,WireBytes4 bytes){auto began=Steady::now();
                std::string raw(bytes.begin(),bytes.end());if(aqcf_wire288::enabled()){raw=aqcf_support402::unpack(raw,plans402[i]);if(!plans402[i].empty()){++support402_frames;support402_coefficients+=8192-plans402[i].size();support402_saved_bytes+=aqcf_input379::frame_size()-bytes.size();}wire288_decoded.fetch_add(1,std::memory_order_relaxed);}std::istringstream buffer(raw,std::ios::binary);seal::Ciphertext ct;ct.load(*context,buffer);
                require(buffer.peek()==std::char_traits<char>::eof(),"seeded trailing bytes");check(ct);
                require(ct.parms_id()==context->first_parms_id()&&ct.coeff_modulus_size()==data_limbs,"full original input level");
                load_s[lane]+=elapsed(began);began=Steady::now();auto pt=encode(gather(l,i,own,modulus));gather_s[lane]+=elapsed(began);
                began=Steady::now();restore_eval[lane]->add_plain_inplace(ct,pt);restore_s[lane]+=elapsed(began);began=Steady::now();
                restore_eval[lane]->transform_to_ntt_inplace(ct);
                if(uniform_input_share&&uniform_first(id))uniform_ordinary_inputs[i]=ct; // BEFORE grouped permutation; copy counted below
                auto& a=inputs[i];a.v.resize(2*data_limbs*DEGREE);
                for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){U* dest=a.v.data()+(c*data_limbs+r)*DEGREE;
                  if(short_kernel)for(U j=0;j<DEGREE;++j)dest[broadcast[r][j]]=ct.data(c)[r*DEGREE+j];
                  else std::copy_n(ct.data(c)+r*DEGREE,DEGREE,dest);}
                ntt_s[lane]+=elapsed(began);});
            record_compute4("SERVER_INPUT_LOAD_RESTORE_NTT",id,ie);metrics.input_restore+=ni;
            metrics.cipher_butterflies+=ni*2*data_limbs*(DEGREE/2)*logarithm(DEGREE);
            metrics.seconds["parallel_server_input_with_receive_wall"]+=elapsed(input_wall);
            for(unsigned k=0;k<4;++k){metrics.seconds["deserialization_worker_seconds_not_wall"]+=load_s[k];
              metrics.seconds["server_gather_encode_worker_seconds_not_wall"]+=gather_s[k];
              metrics.seconds["input_share_restore_worker_seconds_not_wall"]+=restore_s[k];metrics.seconds["input_NTT_worker_seconds_not_wall"]+=ntt_s[k];}
            } // fresh receive/restore branch
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
            std::vector<std::vector<U>> parallel_values;
            const bool whole_producer=parallel_mode&&!parallel_audit&&quarter_enabled&&finalizer=="prescaled";
            std::vector<seal::Ciphertext> protected_outputs;
            std::vector<LiftedZeroStats> producer_stats;
            std::vector<double> producer_seconds;
            if(whole_producer){
                // Validate publicly fixed writes: no two workers own one slot.
                aqcf_validation4::destinations(l.queries,out.size(),U(DEGREE),validation_parallel);
                if(!aqcf_pipeline416::enabled())protected_outputs.resize(no);producer_stats.resize(linear_workers);producer_seconds.resize(linear_workers,0);
            }
            const bool pipeline416=aqcf_pipeline416::enabled();
            if(pipeline416){
                require(whole_producer,"416 requires original complete four-worker producer");
                auto pipeline_started=Steady::now();
                struct Owner416 {
                    aqcf_native4::Outer audit;
                    seal::Evaluator evaluator;
                    seal::MemoryPoolHandle pool;
                    PrescaledOriginalZero finalizer;
                    Owner416(unsigned lane,const seal::SEALContext& ctx,const seal::PublicKey& pk)
                    :audit(lane,"SQ_SERVER_PRODUCER"),evaluator(ctx),pool(seal::MemoryPoolHandle::New()),finalizer(ctx,pk){}
                };
                number(U(id));number(no);
                auto pe=aqcf_compute4::ordered_owned416<WireBytes4>(no,compute_cpus(),
                  [&](unsigned lane){linear_worker_cpus[lane]=sched_getcpu();return std::make_unique<Owner416>(lane,*context,public_key);},
                  [&](std::size_t oi,unsigned lane,Owner416& owner){
                    seal::Ciphertext ct;
                        const auto& o=layer.output.at(oi);
                        std::vector<__uint128_t> sums(2*data_limbs*DEGREE,0);
                        for(const auto&e:o.edges){
                            std::vector<std::vector<U>> small;
                            seal::Plaintext full(owner.pool);
                            if(short_kernel){for(unsigned r=0;r<data_limbs;++r)small.push_back(transforms[r].weight(e,g.b));}
                            else{
                                full.resize(DEGREE);for(auto a:e.coefficients)full[a.exponent]=coefficient_residue(a.value);
                                owner.evaluator.transform_to_ntt_inplace(full,context->first_parms_id(),owner.pool);
                            }
                            const auto& input=inputs.at(e.input).v;
                            for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){
                                U off=(c*data_limbs+r)*DEGREE;
                                if(short_kernel){for(U k=0;k<g.d;++k){const U w=small[r][k];
                                    for(U j=0;j<g.b;++j){U at=off+k*g.b+j;sums[at]+=__uint128_t(input[at])*w;}}}
                                else for(U j=0;j<DEGREE;++j)sums[off+j]+=__uint128_t(input[off+j])*full[r*DEGREE+j];
                            }
                        }
                        std::vector<U> value(2*data_limbs*DEGREE);
                        for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r)for(U j=0;j<DEGREE;++j){
                            U off=(c*data_limbs+r)*DEGREE;
                            value[off+j]=U(sums[off+(short_kernel?broadcast[r][j]:j)]%primes[r]);
                        }
                        if(whole_producer){
                            auto started=Steady::now();
                            ct=owner.finalizer.finish(value,producer_stats[lane]);
                            seal::Plaintext bias;bias.resize(DEGREE);
                            for(const auto& q:o.queries)bias[q.exponent]=aqcf2::residue(W(q.bias),modulus);
                            owner.evaluator.add_plain_inplace(ct,bias);
                            // Unchanged full coefficient mask; independent fresh PRNG
                            // per call. No Sci/session/network operation in a worker.
                            project_return(ct);mask_return303(ct,*quarter_context,o.queries,out);check(ct);
                            producer_seconds[lane]+=elapsed(started);
                        }

                        ++owner.audit.jobs;++linear_worker_outputs[lane];
                        return pack4(ct,o.queries);

                  },[&](std::size_t,WireBytes4 bytes){send_packed4(bytes);},8);
                record_compute4("SERVER_OUTPUT_PACK",id,pe);
                metrics.seconds["pipeline416_complete_producer_pack_send_wall"]+=elapsed(pipeline_started);
                ++pipeline416_layers;pipeline416_outputs+=no;
                maximum_linear_workers=std::max(maximum_linear_workers,unsigned(4));
                ++parallel_layers;parallel_outputs+=no;
                for(unsigned lane=0;lane<4;++lane){
                    const auto& z=producer_stats[lane];
                    metrics.lifted.fresh_zeros+=z.fresh_zeros;metrics.lifted.u_forward_ntts+=z.u_forward_ntts;
                    metrics.lifted.inverse_ntts+=z.inverse_ntts;metrics.lifted.avoided_inverse_ntts+=z.avoided_inverse_ntts;
                    metrics.lifted.pk_dyadic_products+=z.pk_dyadic_products;
                    metrics.lifted.accumulator_scale_products+=z.accumulator_scale_products;
                    metrics.lifted.modulus_switch_components+=z.modulus_switch_components;
                    metrics.seconds["parallel_complete_producer_worker_seconds_not_wall"]+=producer_seconds[lane];
                }
                metrics.fresh_zero+=no;metrics.bias_add+=no;
                if(!retain296::enabled()){metrics.full_mask+=no*DEGREE;metrics.c0_add+=no*(projected_output?1:data_limbs)*DEGREE;}
            }
            if(parallel_mode&&!pipeline416){
                auto wall=Steady::now();parallel_values.resize(no);
                const unsigned count=std::min<U>(linear_workers,no);require(count==4,"four real SQ producer outputs");
                std::vector<std::exception_ptr> errors(count);
                std::vector<std::thread> threads;
                auto work=[&](unsigned lane){try{
                    cpu_set_t cpus;CPU_ZERO(&cpus);CPU_SET(aqcf_native4::cpu(lane),&cpus);
                    require(sched_setaffinity(0,sizeof(cpus),&cpus)==0,"linear worker CPU affinity");
                    linear_worker_cpus[lane]=sched_getcpu();
                    require(linear_worker_cpus[lane]==aqcf_native4::cpu(lane),"actual linear CPU evidence");
                    aqcf_native4::Outer audit4(lane,"SQ_SERVER_PRODUCER");
                    seal::Evaluator lane_evaluator(*context);
                    auto pool=seal::MemoryPoolHandle::New();
                    // Each worker owns its finalizer and every fresh coin bank.
                    std::unique_ptr<PrescaledOriginalZero> lane_finalizer;
                    if(whole_producer)lane_finalizer=std::make_unique<PrescaledOriginalZero>(*context,public_key);
                    for(U oi=lane;oi<no;oi+=count){
                        const auto& o=layer.output.at(oi);
                        std::vector<__uint128_t> sums(2*data_limbs*DEGREE,0);
                        for(const auto&e:o.edges){
                            std::vector<std::vector<U>> small;
                            seal::Plaintext full(pool);
                            if(short_kernel){for(unsigned r=0;r<data_limbs;++r)small.push_back(transforms[r].weight(e,g.b));}
                            else{
                                full.resize(DEGREE);for(auto a:e.coefficients)full[a.exponent]=coefficient_residue(a.value);
                                lane_evaluator.transform_to_ntt_inplace(full,context->first_parms_id(),pool);
                            }
                            const auto& input=inputs.at(e.input).v;
                            for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){
                                U off=(c*data_limbs+r)*DEGREE;
                                if(short_kernel){for(U k=0;k<g.d;++k){const U w=small[r][k];
                                    for(U j=0;j<g.b;++j){U at=off+k*g.b+j;sums[at]+=__uint128_t(input[at])*w;}}}
                                else for(U j=0;j<DEGREE;++j)sums[off+j]+=__uint128_t(input[off+j])*full[r*DEGREE+j];
                            }
                        }
                        auto& value=parallel_values[oi];value.resize(2*data_limbs*DEGREE);
                        for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r)for(U j=0;j<DEGREE;++j){
                            U off=(c*data_limbs+r)*DEGREE;
                            value[off+j]=U(sums[off+(short_kernel?broadcast[r][j]:j)]%primes[r]);
                        }
                        if(whole_producer){
                            auto started=Steady::now();
                            auto& ct=protected_outputs[oi];ct=lane_finalizer->finish(value,producer_stats[lane]);
                            seal::Plaintext bias;bias.resize(DEGREE);
                            for(const auto& q:o.queries)bias[q.exponent]=aqcf2::residue(W(q.bias),modulus);
                            lane_evaluator.add_plain_inplace(ct,bias);
                            // Unchanged full coefficient mask; independent fresh PRNG
                            // per call. No Sci/session/network operation in a worker.
                            project_return(ct);mask_return303(ct,*quarter_context,o.queries,out);check(ct);
                            producer_seconds[lane]+=elapsed(started);
                        }
                        ++audit4.jobs;++linear_worker_outputs[lane];
                    }
                }catch(...){errors[lane]=std::current_exception();}};
                try{for(unsigned lane=1;lane<count;++lane)threads.emplace_back(work,lane);}
                catch(...){for(auto&thread:threads)thread.join();throw;}
                work(0);for(auto&thread:threads)thread.join();aqcf_native4::restore();
                for(auto&e:errors)if(e)std::rethrow_exception(e);
                maximum_linear_workers=std::max(maximum_linear_workers,count);
                ++parallel_layers;parallel_outputs+=no;
                metrics.seconds[whole_producer?"parallel_existing_MAC_and_complete_producer_wall":"parallel_existing_MAC_wall"]+=elapsed(wall);
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
                    metrics.fresh_zero+=no;if(!retain296::enabled()){metrics.full_mask+=no*DEGREE;metrics.c0_add+=no*(projected_output?1:data_limbs)*DEGREE;}
                    metrics.bias_add+=no;
                }
            }
            require(whole_producer,"no silent serial producer fallback");
            U output_index=0;
            if(!pipeline416){number(U(id));number(no);
            auto pack_wall=Steady::now();
            auto pe=aqcf_compute4::ordered<WireBytes4>(no,compute_cpus(),[&](std::size_t i,unsigned){return pack4(protected_outputs[i],layer.output[i].queries);},
              [&](std::size_t,WireBytes4 bytes){send_packed4(bytes);});
            record_compute4("SERVER_OUTPUT_PACK",id,pe);metrics.seconds["parallel_output_pack_with_send_wall"]+=elapsed(pack_wall);
            }
            for(const auto& o:layer.output){
                expected_pm+=o.edges.size();expected_adds+=o.edges.empty()?0:o.edges.size()-1;
                if(whole_producer){
                    metrics.pm+=o.edges.size();metrics.ct_add+=o.edges.empty()?0:o.edges.size()-1;
                    if(o.edges.empty())++metrics.zero_outputs;
                    if(short_kernel)metrics.short_weight_ntts+=data_limbs*o.edges.size();
                    metrics.weight_butterflies+=data_limbs*(g.d/2)*logarithm(g.d)*o.edges.size();
                    // Preserve the exact old output order and framing; network
                    // stays on the main thread after all workers have joined.
                    ++output_index;continue; // already sent by ordered four-worker packing above
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
                    t=Steady::now();std::vector<std::vector<U>> small;seal::Plaintext full;
                    if(short_kernel){for(unsigned r=0;r<data_limbs;++r)small.push_back(transforms[r].weight(e,g.b));
                        metrics.short_weight_ntts+=data_limbs;metrics.weight_butterflies+=data_limbs*(g.d/2)*logarithm(g.d);
                    }else{
                        full.resize(DEGREE);for(auto a:e.coefficients)full[a.exponent]=coefficient_residue(a.value);
                        evaluator->transform_to_ntt_inplace(full,context->first_parms_id());
                        metrics.weight_butterflies+=data_limbs*(DEGREE/2)*logarithm(DEGREE);
                    }
                    metrics.seconds[short_kernel?"private_weight_short_NTT":"private_weight_encode_NTT"]+=elapsed(t);t=Steady::now();
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
        if(quarter_enabled){
          if(defer_quarter_recovery){
            chunks4("RAW_QUARTER_CANONICAL",id,out.size(),[&](std::size_t a,std::size_t b){for(auto j=a;j<b;++j)require(out[j]<(U(1)<<57),"complete unmodified raw quarter phase");});
            deferred_quarter_rows+=out.size();
          }else{auto qt=Steady::now();require(bool(recovery_workers),"secure recovery worker hook required");recovery_workers(out);quarter_correction_s+=elapsed(qt);
            metrics.seconds["quarter_2pc_recovery_all_costs"]+=elapsed(qt);quarter_rows+=out.size();}
        }
        if(auxiliary){metrics.auxiliary_stats=auxiliary->engine.stats;metrics.auxiliary_pk_inverse=auxiliary->original_pk_inverse_ntts;}
        if(projected_output)metrics.lifted.modulus_switch_components+=2*no;
        metrics.input_ct+=reuse_uniform?0:ni;metrics.output_ct+=no;finish_uniform_input(id,reuse_uniform,ni);return out;
    }
    std::vector<I> final1001(const std::vector<U>& own){
        const U modulus=aqcf2::M; // HE-only quotient; final share protocol unchanged.
        require(own.size()==1001 && std::all_of(requested.begin(),requested.end(),[](bool b){return b;}),"final-only release after all26");
        chunks4("FINAL_SHARE_CANONICAL",26,own.size(),[&](std::size_t a,std::size_t b){for(auto j=a;j<b;++j)require(own[j]<modulus,"final share canonical");});
        std::vector<I> out;s.flush();
        if(s.party==1){s.io.io->send_data(own.data(),8008);s.flush();end_cipher_wire();}
        else{std::vector<U> peer(1001);s.io.io->recv_data(peer.data(),8008);
            require(service_started&&!service_finished,"first ciphertext through final payload timer");
            service_delta=aqcf2::ClockSample::now().since(service_start);service_finished=true;end_cipher_wire();std::cout<<"WAN_SERVICE_END\n"<<std::flush;
            const auto final_recovery_begin=aqcf2::ClockSample::now();
            out.resize(1001);
            chunks4("FINAL_RECONSTRUCTION_AFTER_ONLINE",26,1001,[&](std::size_t a,std::size_t b){for(auto j=a;j<b;++j){
              require(peer[j]<modulus,"canonical peer final share");out[j]=aqcf2::centered((peer[j]+own[j])%modulus,modulus);}});
            metrics.seconds["final_recovery_RAW_after_online"]=aqcf2::ClockSample::now().since(final_recovery_begin).raw;}
        s.flush();return out;
    }
};
} // namespace aqcf3
