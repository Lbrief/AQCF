#pragma once
// Exact separation of a private channel kernel V(X^b) from full-degree
// ciphertext fibers. Small NTTs and Shoup multiplication are established tools;
// no priority claim is made for those tools. The source and cost ablations
// distinguish spectral broadcast from the action-level transposition.
#include "resnet50/core/program.hpp"
#include <numeric>
#include <limits>
namespace aqcf3 {
struct KernelGeometry {
    U b=1, d=DEGREE, channels_per_input=0;
    bool active=false;
};
inline KernelGeometry kernel_geometry(const Layout& l) {
    KernelGeometry z;
    if(l.queries.empty() || l.queries[0].empty()) return z;
    const auto& q=l.queries[0];
    U b=1;
    while(b<q.size() && q[b].exponent==q[0].exponent+b) ++b;
    if(b<64 || b>1024 || (b&(b-1)) || DEGREE%b) return z;
    const U length=l.maps.at(0).size();
    if(length==0 || length%b || length>DEGREE) return z;
    std::atomic<bool> bad{false};
    require(l.maps.size()+l.queries.size()>=4,"four public geometry records");
    pcn_data089::each("AQCF_PUBLIC_GEOMETRY",l.maps.size()+l.queries.size(),[&](std::size_t i){
      if(i<l.maps.size()){const auto&m=l.maps[i];if(m.size()!=length){bad=true;return;}
        for(U j=0;j<m.size();++j)if(m[j].exponent!=j||m[j].multiplier!=1)bad=true;
      }else{const auto&qq=l.queries[i-l.maps.size()];if(qq.empty()||qq.size()%b){bad=true;return;}
        for(U j=0;j<qq.size();j+=b){if(qq[j].exponent%length!=length-b)bad=true;
          for(U k=0;k<b;++k)if(qq[j+k].exponent!=qq[j].exponent+k)bad=true;}}
    });if(bad)return z;
    z.b=b; z.d=DEGREE/b; z.channels_per_input=length/b; z.active=true;
    return z;
}
inline void certify_kernel_support(const Layer& l,const KernelGeometry& g) {
    if(!g.active) return;
    // This is a model-class admission, BEFORE peer traffic. It is not a
    // weight-dependent online fallback. No support, values or private digest
    // is written to a public receipt.
    pcn_data089::each("AQCF_PRIVATE_SUPPORT",l.output.size(),[&](std::size_t oi){for(const auto& e:l.output[oi].edges)
        for(const auto& a:e.coefficients)require(a.exponent%g.b==0,"private kernel outside admitted public action class");});
}
inline U action_pow(U a,U e,U q) {
    U r=1;
    for(;e;e>>=1,a=U(__uint128_t(a)*a%q))
        if(e&1) r=U(__uint128_t(r)*a%q);
    return r;
}
struct ActionMultiplier {
    U w=0, scaled=0;
    ActionMultiplier()=default;
    ActionMultiplier(U value,U q):w(value),scaled(U((__uint128_t(value)<<64)/q)){}
    U apply(U x,U q)const {
        // x,w < q < 2^44. Exact Shoup remainder lies in [0,2q).
        const U quotient=U((__uint128_t(x)*scaled)>>64);
        U v=x*w-quotient*q;
        if(v>=q) v-=q;
        return v;
    }
};
class ActionNTT {
    U d_, q_;
    std::vector<U> reverse_;
    std::vector<ActionMultiplier> twist_, untwist_;
    std::vector<std::vector<ActionMultiplier>> roots_, inverse_roots_;
public:
    ActionNTT(U d,U q):d_(d),q_(q),reverse_(d),twist_(d),untwist_(d) {
        require(d>=2 && d<=DEGREE && !(d&(d-1)) && (q-1)%(2*d)==0,
                "action transform degree");
        U generator=2;
        while(generator<4096 && action_pow(generator,(q-1)/2,q)!=q-1) ++generator;
        require(generator<4096,"public quadratic nonresidue search bound");
        const U psi=action_pow(generator,(q-1)/(2*d),q);
        require(action_pow(psi,d,q)==q-1,"action primitive root");
        const U ipsi=action_pow(psi,q-2,q), id=action_pow(d,q-2,q);
        U t=1,it=1;
        for(U j=0;j<d;++j) {
            twist_[j]=ActionMultiplier(t,q);
            untwist_[j]=ActionMultiplier(U(__uint128_t(it)*id%q),q);
            t=U(__uint128_t(t)*psi%q); it=U(__uint128_t(it)*ipsi%q);
            if(j) reverse_[j]=(reverse_[j>>1]>>1)|((j&1)?d>>1:0);
        }
        const U omega=U(__uint128_t(psi)*psi%q), iw=action_pow(omega,q-2,q);
        for(U len=2;len<=d;len*=2) {
            std::vector<ActionMultiplier> a(len/2),b(len/2);
            U r=action_pow(omega,d/len,q),ri=action_pow(iw,d/len,q),v=1,vi=1;
            for(U j=0;j<len/2;++j) {
                a[j]=ActionMultiplier(v,q); b[j]=ActionMultiplier(vi,q);
                v=U(__uint128_t(v)*r%q); vi=U(__uint128_t(vi)*ri%q);
            }
            roots_.push_back(std::move(a)); inverse_roots_.push_back(std::move(b));
        }
    }
    U degree()const{return d_;}
    U modulus()const{return q_;}
    void transform(U* a,U lanes,bool inverse)const {
        // a[k*lanes+r] is coefficient k of fiber r. No conversion of an
        // RLWE ciphertext to a lower-dimensional encryption scheme occurs.
        require(a && lanes>=1 && d_*lanes<=DEGREE,"action array geometry");
        if(!inverse) for(U k=0;k<d_;++k) for(U r=0;r<lanes;++r)
            a[k*lanes+r]=twist_[k].apply(a[k*lanes+r],q_);
        for(U k=0;k<d_;++k) if(k<reverse_[k])
            for(U r=0;r<lanes;++r) std::swap(a[k*lanes+r],a[reverse_[k]*lanes+r]);
        U stage=0;
        for(U len=2;len<=d_;len*=2,++stage) {
            const auto& roots=inverse?inverse_roots_[stage]:roots_[stage];
            for(U start=0;start<d_;start+=len) for(U j=0;j<len/2;++j) {
                U* x=a+(start+j)*lanes; U* y=a+(start+j+len/2)*lanes;
                const auto& w=roots[j];
                for(U r=0;r<lanes;++r) {
                    const U u=x[r],v=w.apply(y[r],q_);
                    U plus=u+v; if(plus>=q_) plus-=q_;
                    x[r]=plus; y[r]=u>=v?u-v:u+q_-v;
                }
            }
        }
        if(inverse) for(U k=0;k<d_;++k) for(U r=0;r<lanes;++r)
            a[k*lanes+r]=untwist_[k].apply(a[k*lanes+r],q_);
    }
    std::vector<U> weight(const Edge& edge,U stride)const {
        std::vector<U> w(d_,0);
        for(const auto& a:edge.coefficients) {
            require(a.exponent%stride==0 && a.exponent/stride<d_,"kernel support certificate");
            // The original per-output L1 cap is 2^30 < every data prime.
            const U m=magnitude(a.value); require(m<q_,"signed kernel coefficient bound");
            w[a.exponent/stride]=a.value<0?q_-m:m;
        }
        transform(w.data(),1,false); return w;
    }
};
struct ActionCipher {
    // PRIVATE computational representation. Never serialized, never marked as
    // SEAL's full negacyclic NTT domain, never sent to the client.
    std::vector<U> v;
};
} // namespace aqcf3
