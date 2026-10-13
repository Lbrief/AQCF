#pragma once
#include "program.hpp"
#include "module_basis.hpp"
#include "kernel_action.hpp"
#include "coefficient_wire.hpp"
#include "bilinear_action.hpp"
#include "lifted_pk_zero.hpp"
#include "root_flood.hpp"
#include "quarter_protocol.hpp"
#include "native_auxiliary_lift.hpp"
#include "spectral_ingress.hpp"
#include "signed_weight.hpp"
#include "seeded_wire40.hpp"
#ifndef AQCF_ABLATE_SHORT
#define AQCF_ABLATE_SHORT 0
#endif
static_assert(AQCF_ABLATE_SHORT==0 || AQCF_ABLATE_SHORT==1,"boolean short ablation");
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
#include "sci.hpp"
#include "clock_sample.hpp"
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
    U quotient_eligible_layers=0,full_weight_ntts=0;
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
    static constexpr std::array<U,3> primes={1099510890497ULL,1099511480321ULL,274877562881ULL};
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
    static constexpr U cap=479;
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
    }
    aqcf2::ClockSample service_start{},service_delta{};
    BfvProgram(aqcf2::Sci& session,U t,const Layouts& l,const Program* p,bool module=false,bool stream=true,
      std::string action="full",std::string transport="seal",
      std::string matrix_plan="legacy",std::string zero_plan="separate",std::string query_plan="public",std::string integer_lift="native")
      :s(session),modulus(t),layouts(l),program(p),use_module(module),stream_mac(stream),kernel(std::move(action)),wire(std::move(transport)),
        mac_plan(std::move(matrix_plan)),finalizer(std::move(zero_plan)),query_encryption(std::move(query_plan)),lift_plan(std::move(integer_lift)){
        // Only the frozen Tiny Ring41 entry may select its same-ring full control.
        require(DEGREE==8192&&AFFINES==21&&t==(U(1)<<41)&&quarter_enabled&&data_limbs==2&&
          !use_module&&stream_mac&&kernel=="broadcast"&&mac_plan=="legacy"&&
          finalizer=="prescaled"&&lift_plan=="native"&&query_encryption=="symmetric-seeded"&&wire=="packed-query",
          "isolated Tiny Ring41 HE ablation entry");
        if(AQCF_ABLATE_SHORT)kernel="full";
        const bool current_full_prescaled=AQCF_ABLATE_SHORT&&kernel=="full";
        require(!quarter_enabled||(t==(U(1)<<41)&&lift_plan=="native"&&finalizer=="prescaled"&&query_encryption=="symmetric-seeded"&&wire=="packed-query"),"quarter exact producer contract");
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
        require((mac_plan=="legacy"&&finalizer=="separate")||kernel=="broadcast"||public_generic_prescaled||current_full_prescaled,
          "R9 exact rewrite uses the same admitted broadcast geometry, not fiber conversion");
        require(kernel=="full" || kernel=="broadcast" || kernel=="fiber","kernel mode");
        require(wire=="seal" || wire=="packed-query","wire mode");
        require(!use_module && stream_mac,"R5 freezes QMC OFF and common stream ON; original algorithm lineage remains in the R4 input archive");
        require(t==(U(1)<<41),"public qualified Ring41 HE only");
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
        if(quarter_enabled){const cpp_int error=cpp_int(22)*(cpp_int(1)<<30)+4098;
          require(cpp_int(8)*modulus*error<Q,"quarter deterministic signed-affine error margin");
          require(cap==479,"fixed ResNet18 return roster only");return;}
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
        t=Steady::now();bytes=aqcf_wire40::pack(bytes,{primes[0],primes[1]});metrics.seconds["seeded40_pack"]+=elapsed(t);
        if(!service_started){require(s.party==2,"client input service timer");
            service_start=aqcf2::ClockSample::now();service_started=true;}
        number(bytes.size());t=Steady::now();s.io.io->send_data(bytes.data(),int(bytes.size()));s.flush();
        sent_bytes+=bytes.size();metrics.input_payload_bytes+=bytes.size();metrics.seconds["send_wait"]+=elapsed(t);
    }
    void receive_seeded40(seal::Ciphertext& ct){
        U n=number();require(n==aqcf_wire40::SIZE,"fixed seeded40 length");
        std::string bytes(n,'\0');auto t=Steady::now();s.io.io->recv_data(bytes.data(),int(n));received_bytes+=n;
        metrics.seconds["receive_wait"]+=elapsed(t);t=Steady::now();
        bytes=aqcf_wire40::unpack(bytes,{primes[0],primes[1]});
        metrics.seconds["seeded40_unpack"]+=elapsed(t);t=Steady::now();std::istringstream in(bytes,std::ios::binary);
        ct.load(*context,in);require(in.peek()==std::char_traits<char>::eof(),"seeded40 full native load");
        metrics.seconds["deserialization"]+=elapsed(t);
    }
    template<class Obj>void receive_object(Obj& obj){U n=number();require(n>0 && n<=(8ULL<<20),"SEAL incoming frame length");
        std::string bytes(n,'\0');auto time=Steady::now();s.io.io->recv_data(bytes.data(),int(n));received_bytes+=n;
        metrics.seconds["receive_wait"]+=elapsed(time);time=Steady::now();std::istringstream buffer(bytes,std::ios::binary);
        obj.load(*context,buffer);require(buffer.peek()==std::char_traits<char>::eof(),"SEAL trailing bytes");metrics.seconds["deserialization"]+=elapsed(time);}
    void send_spectral_input(const ingress::Bytes& bytes){
        require(s.party==2&&spectral_secret&&bytes.size()==184432,"client-only exact spectral frame");
        if(!service_started){service_start=aqcf2::ClockSample::now();service_started=true;}
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
          ct.parms_id()==context->first_parms_id() && !ct.is_transparent(),"BFV return representation");
    }
    void protect(seal::Ciphertext& ct,const std::vector<Query>& queries,std::vector<U>& own,bool zero_already_added=false){
        require(s.party==1 && reserved_returns<=cap,"server-only protected return");
        // Zero linear maps can have a transparent pre-mask accumulator. No such
        // object is sent: full mask + fresh asymmetric EncZero + flood follows.
        require(seal::is_valid_for(ct,*context)&&ct.size()==2&&!ct.is_ntt_form()&&ct.parms_id()==context->first_parms_id(),"pre-protection BFV structure");
        if(quarter_enabled){require(zero_already_added,"all original fresh zero consumed before full-Q mask");auto qt=Steady::now();
          quarter_mask(ct,*quarter_context,queries,own);metrics.full_mask+=DEGREE;metrics.c0_add+=data_limbs*DEGREE;
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
        auto t=Steady::now();CoefficientWriter w;
        for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){
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
        U width=0;for(unsigned r=0;r<data_limbs;++r)width+=coefficient_bits(primes[r]);
        const U count=DEGREE+(query?query->size():DEGREE),expected=(count*width+7)/8;
        U n=number();require(n==expected,"fixed public lossless ciphertext frame length");
        std::vector<unsigned char> bytes(n);auto t=Steady::now();
        s.io.io->recv_data(bytes.data(),s.dim(n));received_bytes+=n;metrics.seconds["receive_wait"]+=elapsed(t);
        t=Steady::now();ct.resize(*context,context->first_parms_id(),2);ct.is_ntt_form()=false;
        for(unsigned c=0;c<2;++c)std::fill_n(ct.data(c),data_limbs*DEGREE,U(0));
        CoefficientReader rd(bytes);
        for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){
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
        const auto g=kernel_geometry(l);const bool short_kernel=g.active && kernel!="full";
        const bool fiber=short_kernel && kernel=="fiber";
        // Frozen public policy: only the seven non-quotient layers use R15.
        // Short kernels keep the complete original matrix/finalizer path.
        const bool aux_mode=lift_plan!="native"&&!g.active;
        const bool spectral_input=query_encryption=="spectral-seeded"&&short_kernel&&!fiber&&!aux_mode;
        metrics.seconds["public_action_geometry"]+=elapsed(gt);
        if(g.active)++metrics.quotient_eligible_layers;
        if(short_kernel)++metrics.action_layers;
        const U ni=l.maps.size(),no=l.queries.size();
        expected_inputs+=ni;expected_outputs+=no;original_ranks.push_back(ni);ranks.push_back(ni);
        require(own.size()==l.inputs,"native affine input shape");for(U x:own)require(x<modulus,"canonical own share");
        require(no<=cap-reserved_returns,"return budget before evaluation");reserved_returns+=no;
        std::vector<U> out(l.outputs);
        if(s.party==2){
            number(U(id));number(ni);
            for(U i=0;i<ni;++i){auto t=Steady::now();auto pt=encode(gather(l,i,own,modulus));metrics.seconds["gather_encode"]+=elapsed(t);
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
            require(number()==U(id) && number()==no,"return node/count framing");
            for(U i=0;i<no;++i){seal::Ciphertext ct;receive_cipher(ct,&l.queries[i]);auto t=Steady::now();
                seal::Plaintext pt;if(quarter_enabled){ct.parms_id()=quarter_context->first_parms_id();quarter_decryptor->decrypt(ct,pt);}
                else decryptor->decrypt(ct,pt);++metrics.decryptions;metrics.seconds["masked_decryption"]+=elapsed(t);
                t=Steady::now();for(auto q:l.queries[i]){U v=q.exponent<pt.coeff_count()?pt[q.exponent]:0;
                    require(v<(quarter_enabled?4*modulus:modulus),"canonical masked query");out[q.destination]=v;}
                metrics.seconds["query_extraction"]+=elapsed(t);}
        }else{
            const auto& layer=program->at(id);
            // Also checked before any peer traffic by the role entry point.
            if(g.active)certify_kernel_support(layer,g); // same model admission in both arms
            auto t=Steady::now();
            if(aux_mode){++metrics.auxiliary_layers;if(!auxiliary)auxiliary=std::make_unique<NativeAuxiliaryLift>(*context,public_key);if(lift_plan=="aux-fused")auxiliary->prepare_fused_key();}
            metrics.seconds["R15_PK_and_auxiliary_preparation"]+=elapsed(t);t=Steady::now();std::vector<ActionNTT> transforms;
            std::vector<std::vector<U>> broadcast(data_limbs);
            if(short_kernel){
                for(unsigned r=0;r<data_limbs;++r)transforms.emplace_back(g.d,primes[r]);
                if(!fiber){
                    // Mature control: the same small-weight transforms, expanded
                    // to SEAL's full spectrum. Calibrate ordering on PUBLIC Y.
                    seal::Plaintext monomial;monomial.resize(DEGREE);monomial[g.b]=1;
                    evaluator->transform_to_ntt_inplace(monomial,context->first_parms_id());
                    for(unsigned r=0;r<data_limbs;++r){
                        Edge e;e.coefficients.push_back({g.b,1});auto roots=transforms[r].weight(e,g.b);
                        std::unordered_map<U,U> index;for(U k=0;k<g.d;++k)require(index.emplace(roots[k],k).second,"distinct quotient roots");
                        broadcast[r].resize(DEGREE);std::vector<U> used(g.d,0);
                        for(U j=0;j<DEGREE;++j){auto pos=index.find(monomial[r*DEGREE+j]);
                            require(pos!=index.end(),"SEAL/quotient spectrum matching");const U k=pos->second;
                            require(used[k]<g.b,"public spectrum fiber multiplicity");broadcast[r][j]=k*g.b+used[k]++;}
                        for(U n:used)require(n==g.b,"complete grouped spectrum fibers");
                    }
                }
            }
            metrics.seconds["online_public_transform_tables"]+=elapsed(t);
            require(number()==U(id) && number()==ni,"input node/count framing");
            std::vector<ActionCipher> inputs;inputs.reserve(ni);
            std::vector<aux15::Spectrum> auxiliary_inputs;if(aux_mode)auxiliary_inputs.reserve(ni);
            for(U i=0;i<ni;++i){seal::Ciphertext ct;
                if(spectral_input){t=Steady::now();auto pt=encode(gather(l,i,own,modulus));metrics.seconds["gather_encode"]+=elapsed(t);
                    ct=receive_spectral_input(pt);++metrics.input_restore;
                }else{
                    if(query_encryption=="symmetric-seeded"||query_encryption=="spectral-seeded"){receive_seeded40(ct);check(ct);}else receive_cipher(ct);t=Steady::now();
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
                    for(unsigned c=0;c<2;++c)for(unsigned r=0;r<data_limbs;++r){U* dest=a.v.data()+(c*data_limbs+r)*DEGREE;
                        if(short_kernel)for(U j=0;j<DEGREE;++j)dest[broadcast[r][j]]=ct.data(c)[r*DEGREE+j];
                        else std::copy_n(ct.data(c)+r*DEGREE,DEGREE,dest);}
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
            std::vector<std::vector<U>> parallel_values;
            if(parallel_mode){
                auto wall=Steady::now();parallel_values.resize(no);
                const unsigned count=std::min<U>(linear_workers,no);
                std::vector<std::exception_ptr> errors(count);
                std::vector<std::thread> threads;
                auto work=[&](unsigned lane){try{
                    cpu_set_t cpus;CPU_ZERO(&cpus);CPU_SET(lane,&cpus);
                    require(sched_setaffinity(0,sizeof(cpus),&cpus)==0,"linear worker CPU affinity");
                    linear_worker_cpus[lane]=sched_getcpu();
                    require(linear_worker_cpus[lane]==int(lane),"actual linear CPU evidence");
                    seal::Evaluator lane_evaluator(*context);
                    auto pool=seal::MemoryPoolHandle::New();
                    for(U oi=lane;oi<no;oi+=count){
                        const auto& o=layer.output.at(oi);
                        std::vector<__uint128_t> sums(2*data_limbs*DEGREE,0);
                        for(const auto&e:o.edges){
                            std::vector<std::vector<U>> small;
                            std::vector<U> full;
                            if(short_kernel){for(unsigned r=0;r<data_limbs;++r)small.push_back(transforms[r].weight(e,g.b));}
                            else{
                                full=aqcf_signed_weight::spectrum(*context,e.coefficients);
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
                        ++linear_worker_outputs[lane];
                    }
                }catch(...){errors[lane]=std::current_exception();}};
                try{for(unsigned lane=1;lane<count;++lane)threads.emplace_back(work,lane);}
                catch(...){for(auto&thread:threads)thread.join();throw;}
                work(0);for(auto&thread:threads)thread.join();
                for(auto&e:errors)if(e)std::rethrow_exception(e);
                maximum_linear_workers=std::max(maximum_linear_workers,count);
                ++parallel_layers;parallel_outputs+=no;
                metrics.seconds["parallel_existing_MAC_wall"]+=elapsed(wall);
            }
            number(U(id));number(no);U output_index=0;
            for(const auto& o:layer.output){
                expected_pm+=o.edges.size();expected_adds+=o.edges.empty()?0:o.edges.size()-1;
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
                    if(take_parallel){
                        if(short_kernel)metrics.short_weight_ntts+=data_limbs*o.edges.size();
                        else metrics.full_weight_ntts+=data_limbs*o.edges.size();
                        const U actual_degree=short_kernel?g.d:DEGREE;
                        metrics.weight_butterflies+=data_limbs*(actual_degree/2)*logarithm(actual_degree)*o.edges.size();}}
                else for(const auto& e:o.edges){
                    t=Steady::now();std::vector<std::vector<U>> small;std::vector<U> full;
                    if(short_kernel){for(unsigned r=0;r<data_limbs;++r)small.push_back(transforms[r].weight(e,g.b));
                        metrics.short_weight_ntts+=data_limbs;metrics.weight_butterflies+=data_limbs*(g.d/2)*logarithm(g.d);
                    }else{
                        full=aqcf_signed_weight::spectrum(*context,e.coefficients);
                        metrics.full_weight_ntts+=data_limbs;
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
        if(quarter_enabled){auto qt=Steady::now();quarter_recover_all(s,out);quarter_correction_s+=elapsed(qt);
          metrics.seconds["quarter_2pc_recovery_all_costs"]+=elapsed(qt);quarter_rows+=out.size();}
        if(auxiliary){metrics.auxiliary_stats=auxiliary->engine.stats;metrics.auxiliary_pk_inverse=auxiliary->original_pk_inverse_ntts;}
        metrics.input_ct+=ni;metrics.output_ct+=no;return out;
    }
    std::vector<I> final200(const std::vector<U>& own){
        require(own.size()==200 && std::all_of(requested.begin(),requested.end(),[](bool b){return b;}),"final-only release after all21");
        for(U a:own)require(a<aqcf2::M,"final share canonical");std::vector<I> out;s.flush();
        if(s.party==1)s.io.io->send_data(own.data(),1600);
        else{std::vector<U> peer(200);s.io.io->recv_data(peer.data(),1600);
            require(service_started&&!service_finished,"first ciphertext through final payload timer");
            service_delta=aqcf2::ClockSample::now().since(service_start);service_finished=true;
            out.resize(200);
            for(std::size_t j=0;j<200;++j){require(peer[j]<aqcf2::M,"canonical peer final share");out[j]=aqcf2::centered((peer[j]+own[j])%aqcf2::M,aqcf2::M);}}
        s.flush();return out;
    }
};
} // namespace aqcf3
