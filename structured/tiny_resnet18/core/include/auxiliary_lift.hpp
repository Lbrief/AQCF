#pragma once
// R15: exact bounded integer lift of a plaintext-weight action. This header
// does NOT define an encryption scheme or change the ciphertext modulus.
#include "kernel_action.hpp"
#ifdef AQCF_ROOT_NATIVE_AUX_WIDE
#include "native_aux_wide.hpp"
#endif
#include <array>
#include <openssl/crypto.h>
#include <chrono>
#include <memory>
#include <map>
#ifdef __AVX2__
#include <immintrin.h>
#endif
namespace aqcf3::aux15 {
#ifdef AQCF_ROOT_NATIVE_AUX_WIDE
using WideNTT = NativeAuxWide;
#else
using WideNTT = ActionNTT;
#endif
inline constexpr U L=1152921504606830593ULL, S=65537ULL;
inline constexpr std::array<U,5> Q=pairprofile::Q;
inline constexpr U L1_LIMIT=U(1)<<30;
inline constexpr __uint128_t CRT_MOD=__uint128_t(L)*S;
inline constexpr W GLOBAL_BOUND=W(Q[0]/2)*(L1_LIMIT+DEGREE);
inline U reduce_l(__uint128_t v){
    constexpr U mask=(U(1)<<60)-1,c=16383;
    v=(v&mask)+(v>>60)*c;
    v=(v&mask)+(v>>60)*c;
    U r=U(v);return r>=L?r-L:r;
}
inline U mod(W v,U q){W r=v%W(q);return U(r<0?r+q:r);}
inline U lift_l(I v){return v<0?L-U(-v):U(v);}
inline std::uint32_t lift_s(I v){I x=v%I(S);return x<0?x+S:x;}
inline I centered(U x,U q){require(x<q,"canonical centered lift");return x>q/2?I(x)-I(q):I(x);}
inline W recover(U a,U b){
    require(a<L&&b<S,"canonical CRT limbs");
    static const U il=action_pow(L%S,S-2,S);
    U k=U((b+S-a%S)*il%S);
    __uint128_t v=__uint128_t(L)*k+a;
    W z=v>CRT_MOD/2?W(v)-W(CRT_MOD):W(v);
    require(z<=GLOBAL_BOUND&&z>=-GLOBAL_BOUND,"public exact-lift envelope");
    return z;
}
struct Stats {
    U input_wide=0,input_narrow=0,weight_wide=0,weight_narrow=0;
    U u_wide=0,u_narrow=0,output_wide=0,output_narrow=0;
    U wide_products=0,narrow_products=0,crt_coefficients=0,pk_preprocess_ntts=0;
    U fresh=0;
};
// The narrow stream uses ordinary Fermat-prime arithmetic, not a new primitive.
class NarrowNTT {
    U n_;std::vector<unsigned> rev_;std::vector<std::uint32_t> twist_,untwist_;
    std::vector<std::vector<std::uint32_t>> root_,iroot_;
    static std::uint32_t mul(std::uint32_t a,std::uint32_t b){
        U p=U(a)*b;I v=I(p&65535)-I(p>>16);return v<0?v+S:v;
    }
#ifdef __AVX2__
    static __m256i v_mul(__m256i x,__m256i y){
        const __m256i one=_mm256_set1_epi32(1),s=_mm256_set1_epi32(65537),top=_mm256_set1_epi32(65536);
        __m256i p=_mm256_mullo_epi32(x,y);
        __m256i z=_mm256_sub_epi32(_mm256_and_si256(p,_mm256_set1_epi32(65535)),_mm256_srli_epi32(p,16));
        z=_mm256_add_epi32(z,_mm256_and_si256(_mm256_srai_epi32(z,31),s));
        // Only 65536*65536 overflows a 32-bit product. Its residue is 1.
        auto carry=_mm256_and_si256(one,_mm256_and_si256(_mm256_cmpeq_epi32(x,top),_mm256_cmpeq_epi32(y,top)));
        return _mm256_add_epi32(z,carry);
    }
    static void v_scale(std::uint32_t* a,const std::vector<std::uint32_t>& w,U n){
        U j=0;for(;j+8<=n;j+=8){auto x=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(a+j));auto y=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(w.data()+j));_mm256_storeu_si256(reinterpret_cast<__m256i*>(a+j),v_mul(x,y));}
        for(;j<n;++j)a[j]=mul(a[j],w[j]);
    }
#endif
public:
    explicit NarrowNTT(U n):n_(n),rev_(n),twist_(n),untwist_(n){
        require(n>=2&&n<=DEGREE&&!(n&(n-1)),"narrow power-of-two degree");
        const U psi=action_pow(3,(S-1)/(2*n),S),ip=action_pow(psi,S-2,S);
        require(action_pow(psi,n,S)==S-1,"narrow primitive root");
        U p=1,r=1,invn=action_pow(n,S-2,S);
        for(U j=0;j<n;++j){twist_[j]=p;untwist_[j]=r*invn%S;p=p*psi%S;r=r*ip%S;
            if(j)rev_[j]=(rev_[j>>1]>>1)|((j&1)?n>>1:0);}
        U omega=psi*psi%S,iw=action_pow(omega,S-2,S);
        for(U len=2;len<=n;len<<=1){
            std::vector<std::uint32_t>a(len/2),b(len/2);
            U w=action_pow(omega,n/len,S),v=action_pow(iw,n/len,S),x=1,y=1;
            for(U j=0;j<len/2;++j){a[j]=x;b[j]=y;x=x*w%S;y=y*v%S;}
            root_.push_back(std::move(a));iroot_.push_back(std::move(b));
        }
    }
    U degree()const{return n_;}
    void transform(std::uint32_t* a,bool inverse)const {
        if(!inverse){
#ifdef __AVX2__
            v_scale(a,twist_,n_);
#else
            for(U j=0;j<n_;++j)a[j]=mul(a[j],twist_[j]);
#endif
        }
        for(U j=0;j<n_;++j)if(j<rev_[j])std::swap(a[j],a[rev_[j]]);
        U st=0;
        for(U len=2;len<=n_;len<<=1,++st){
            const auto& ws=inverse?iroot_[st]:root_[st];
            for(U k=0;k<n_;k+=len){U j=0;
#ifdef __AVX2__
                for(;j+8<=len/2;j+=8){
                    auto x=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(a+k+j));
                    auto y=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(a+k+j+len/2));
                    auto w=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ws.data()+j));
                    auto v=v_mul(y,w),sum=_mm256_add_epi32(x,v),diff=_mm256_sub_epi32(x,v);
                    sum=_mm256_sub_epi32(sum,_mm256_and_si256(_mm256_cmpgt_epi32(sum,_mm256_set1_epi32(65536)),_mm256_set1_epi32(65537)));
                    diff=_mm256_add_epi32(diff,_mm256_and_si256(_mm256_srai_epi32(diff,31),_mm256_set1_epi32(65537)));
                    _mm256_storeu_si256(reinterpret_cast<__m256i*>(a+k+j),sum);_mm256_storeu_si256(reinterpret_cast<__m256i*>(a+k+j+len/2),diff);
                }
#endif
                for(;j<len/2;++j){
                const std::uint32_t x=a[k+j],v=mul(a[k+j+len/2],ws[j]);
                U plus=U(x)+v;a[k+j]=plus>=S?plus-S:plus;
                a[k+j+len/2]=x>=v?x-v:x+S-v;
                }
            }
        }
        if(inverse){
#ifdef __AVX2__
            v_scale(a,untwist_,n_);
#else
            for(U j=0;j<n_;++j)a[j]=mul(a[j],untwist_[j]);
#endif
        }
    }
};
struct Spectrum {
    std::vector<U> wide;
    std::vector<std::uint32_t> narrow;
    explicit Spectrum(U n=0):wide(n),narrow(n){}
};
class Engine {
    U n_;WideNTT big_;NarrowNTT small_;
    // beta is centered p^-1 times the ORIGINAL coefficient public key.
    Spectrum beta_;std::vector<U> last_;
    std::array<U,4> pinv_{},lmodq_{},crtmodq_{};
    std::array<ActionMultiplier,4> pmul_;
    std::map<U,std::unique_ptr<WideNTT>> weight_big_;
    std::map<U,std::unique_ptr<NarrowNTT>> weight_small_;
public:
    Stats stats;
    explicit Engine(U n=DEGREE):n_(n),big_(n,L),small_(n) {
        require(n>=2&&n<=DEGREE&&!(n&(n-1)),"aux engine degree");
        require(CRT_MOD/2>__uint128_t(GLOBAL_BOUND),"unambiguous exact lift");
        for(U r=0;r<4;++r){pinv_[r]=action_pow(Q[4]%Q[r],Q[r]-2,Q[r]);pmul_[r]=ActionMultiplier(pinv_[r],Q[r]);lmodq_[r]=L%Q[r];crtmodq_[r]=U(CRT_MOD%Q[r]);}
    }
    U degree()const{return n_;}
    void prepare_public_key(const std::vector<U>& coefficient_pk){
        require(coefficient_pk.size()==10*n_,"original coefficient PK shape");
        beta_=Spectrum(8*n_);last_.resize(2*n_);
        for(U c=0;c<2;++c)for(U r=0;r<5;++r){
            for(U j=0;j<n_;++j){
                const U a=coefficient_pk[(c*5+r)*n_+j];require(a<Q[r],"PK canonical");
                if(r==4)last_[c*n_+j]=lift_l(centered(a,Q[4]));
                else {
                    const I b=centered(U(__uint128_t(a)*pinv_[r]%Q[r]),Q[r]);
                    beta_.wide[(c*4+r)*n_+j]=lift_l(b);
                    beta_.narrow[(c*4+r)*n_+j]=lift_s(b);
                }
            }
            if(r==4)big_.transform(last_.data()+c*n_,1,false);
            else {big_.transform(beta_.wide.data()+(c*4+r)*n_,1,false);
                small_.transform(beta_.narrow.data()+(c*4+r)*n_,false);}
        }
        stats.pk_preprocess_ntts+=18;
    }
    Spectrum input(const std::vector<U>& coefficient_data){
        require(coefficient_data.size()==8*n_,"original data shape");
        Spectrum a(8*n_);
        for(U h=0;h<8;++h){
            for(U j=0;j<n_;++j){I v=centered(coefficient_data[h*n_+j],Q[h%4]);
                a.wide[h*n_+j]=lift_l(v);a.narrow[h*n_+j]=lift_s(v);}
            big_.transform(a.wide.data()+h*n_,1,false);
            small_.transform(a.narrow.data()+h*n_,false);
        }
        stats.input_wide+=8;stats.input_narrow+=8;return a;
    }
    // No new weight-dependent online path. 'stride' is the already-admitted
    // public quotient-kernel geometry, or 1 for an original full-degree layer.
    Spectrum accumulate(const std::vector<Spectrum>& inputs,const Output& output,U stride=1){
        require(stride>=1&&n_%stride==0&&n_/stride>=2,"public action stride");
        require(inputs.size()>0&&inputs.size()<=64,"public accumulation bound");
        const U d=n_/stride;
        if(d!=n_ && !weight_big_.count(d)){weight_big_[d]=std::make_unique<WideNTT>(d,L);weight_small_[d]=std::make_unique<NarrowNTT>(d);}
        const WideNTT& wb=d==n_?big_:*weight_big_.at(d);
        const NarrowNTT& ws=d==n_?small_:*weight_small_.at(d);
        std::vector<__uint128_t> sums(8*n_);
        std::vector<U> narrow_sums(8*n_);
        U l1=0;require(output.edges.size()<=inputs.size(),"original unique-edge count");std::vector<bool> seen_input(inputs.size(),false);
        for(const auto&e:output.edges){
            require(e.input<inputs.size()&&!seen_input[e.input],"public unique input index");seen_input[e.input]=true;
            require(inputs[e.input].wide.size()==8*n_&&inputs[e.input].narrow.size()==8*n_,"input spectra shape");
            Spectrum w(d);
            std::vector<bool> seen(d,false);
            for(const auto&a:e.coefficients){
                require(a.exponent<n_&&a.exponent%stride==0,"unchanged kernel support");
                U k=a.exponent/stride;require(!seen[k],"unique private coefficient");seen[k]=true;
                U m=magnitude(a.value);require(m<=L1_LIMIT-l1,"original output L1 bound");l1+=m;
                w.wide[k]=lift_l(a.value);w.narrow[k]=lift_s(a.value);
            }
            wb.transform(w.wide.data(),1,false);ws.transform(w.narrow.data(),false);
            ++stats.weight_wide;++stats.weight_narrow;
            const auto&a=inputs[e.input];
            for(U h=0;h<8;++h)for(U j=0;j<n_;++j){
                const U at=h*n_+j,k=j%d;
                sums[at]+=__uint128_t(a.wide[at])*w.wide[k];
                narrow_sums[at]+=U(a.narrow[at])*w.narrow[k];
            }
            stats.wide_products+=8*n_;stats.narrow_products+=8*n_;
        }
        Spectrum result(8*n_);
        for(U j=0;j<8*n_;++j){result.wide[j]=reduce_l(sums[j]);result.narrow[j]=narrow_sums[j]%S;}
        return result;
    }
    // Public coupled-coins tests call this API. Production MUST allocate fresh
    // original SEAL ZeroCoins for each call in the native adapter.
    std::vector<U> finish(Spectrum acc,const std::vector<I>& ternary_u,
        const std::vector<U>& e0,const std::vector<U>& e1){
        require(beta_.wide.size()==8*n_&&last_.size()==2*n_,"PK prepared");
        require(acc.wide.size()==8*n_&&acc.narrow.size()==8*n_&&ternary_u.size()==n_&&e0.size()==5*n_&&e1.size()==5*n_,"zero coin shape");
        Spectrum u(n_);
        for(U j=0;j<n_;++j){require(ternary_u[j]>=-1&&ternary_u[j]<=1,"original ternary support");u.wide[j]=lift_l(ternary_u[j]);u.narrow[j]=lift_s(ternary_u[j]);}
        big_.transform(u.wide.data(),1,false);small_.transform(u.narrow.data(),false);
        ++stats.u_wide;++stats.u_narrow;
        std::vector<I> z(2*n_);
        std::vector<U> tmp(n_);
        for(U c=0;c<2;++c){
            const auto&err=c?e1:e0;
            for(U j=0;j<n_;++j)tmp[j]=reduce_l(__uint128_t(last_[c*n_+j])*u.wide[j]);
            big_.transform(tmp.data(),1,true);++stats.output_wide;
            for(U j=0;j<n_;++j){
                I v=centered(tmp[j],L);
                require(v<=I(n_*(Q[4]/2))&&v>=-I(n_*(Q[4]/2)),"single-wide PK product envelope");
                require(err[4*n_+j]<Q[4],"canonical p error");
                z[c*n_+j]=centered(mod(W(v)+err[4*n_+j],Q[4]),Q[4]);
            }
        }
        std::vector<U> out(8*n_);
        for(U h=0;h<8;++h){
            U* a=acc.wide.data()+h*n_;auto* b=acc.narrow.data()+h*n_;
            for(U j=0;j<n_;++j){
                a[j]=reduce_l(__uint128_t(beta_.wide[h*n_+j])*u.wide[j]+a[j]);
                b[j]=(U(beta_.narrow[h*n_+j])*u.narrow[j]+b[j])%S;
            }
            big_.transform(a,1,true);small_.transform(b,true);
            ++stats.output_wide;++stats.output_narrow;
            const U c=h/4,r=h%4,q=Q[r];const auto&err=c?e1:e0;
            for(U j=0;j<n_;++j){
                require(err[r*n_+j]<q,"canonical data error");
                const W f=recover(a[j],b[j]);
                // |center_p(z)| < every data q: exactly two conditional corrections.
                I diff=I(err[r*n_+j])-z[c*n_+j];if(diff<0)diff+=I(q);if(diff>=I(q))diff-=I(q);
                const U correction=pmul_[r].apply(U(diff),q);
                // Reconstruct modulo q without signed 128-bit division.
                static const U inv_l_s=action_pow(L%S,S-2,S);
                const U k=(U(b[j])+S-a[j]%S)*inv_l_s%S;
                U fq=(a[j]%q+k*lmodq_[r])%q;
                if(f<0)fq=fq>=crtmodq_[r]?fq-crtmodq_[r]:fq+q-crtmodq_[r];
                U v=fq+correction;out[h*n_+j]=v>=q?v-q:v;
            }
        }
        stats.wide_products+=10*n_;stats.narrow_products+=8*n_;stats.crt_coefficients+=8*n_;++stats.fresh;
        OPENSSL_cleanse(u.wide.data(),u.wide.size()*sizeof(U));
        OPENSSL_cleanse(u.narrow.data(),u.narrow.size()*sizeof(std::uint32_t));
        OPENSSL_cleanse(tmp.data(),tmp.size()*sizeof(U));
        return out;
    }
    // Same auxiliary action without fusion: strong algebraic ablation. The
    // caller then adds an independently generated ORIGINAL zero ciphertext.
    std::vector<U> finish_action_only(Spectrum acc){
        require(acc.wide.size()==8*n_&&acc.narrow.size()==8*n_,"action shape");
        std::vector<U> out(8*n_);
        for(U h=0;h<8;++h){big_.transform(acc.wide.data()+h*n_,1,true);
            small_.transform(acc.narrow.data()+h*n_,true);++stats.output_wide;++stats.output_narrow;
            for(U j=0;j<n_;++j)out[h*n_+j]=mod(recover(acc.wide[h*n_+j],acc.narrow[h*n_+j]),Q[h%4]);}
        stats.crt_coefficients+=8*n_;return out;
    }
};
}
