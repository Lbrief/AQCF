#pragma once
// Public, division-free row-module elimination over Z[X]/(X^8192+1).
// No model coefficients or input/share values participate in planning.
#include "squeezenet/engine/program.hpp"
#include <chrono>
#include <limits>
#include <tuple>
namespace aqcf3 {
struct BasisMix { U basis,shift; I multiplier; };
struct ModulePlan {
    std::vector<std::vector<Term>> basis;
    std::vector<std::vector<BasisMix>> transform; // G_i = sum_j T_ij B_j
    bool contracted=false;
    U operations=0;
    std::string reason;
};
struct ModuleLimits {
    U work=4000000, max_terms=64*DEGREE, max_mass=64, mix_mass=8;
};
inline bool source_order(const Term&a,const Term&b){return std::tie(a.source,a.exponent)<std::tie(b.source,b.exponent);}
inline std::vector<Term> canonical_row(std::vector<Term> a,U n=DEGREE){
    std::sort(a.begin(),a.end(),source_order); std::vector<Term> r;
    for(auto t:a){require(t.exponent<n,"module exponent");if(!t.multiplier)continue;
        if(!r.empty()&&r.back().source==t.source&&r.back().exponent==t.exponent){
            W z=W(r.back().multiplier)+t.multiplier;
            require(z>=std::numeric_limits<I>::min()&&z<=std::numeric_limits<I>::max(),"public coefficient overflow");
            r.back().multiplier=I(z);if(!z)r.pop_back();
        }else r.push_back(t);
    }return r;
}
inline bool bounded_row(const std::vector<Term>& r,const ModuleLimits& lim,U n){
    if(r.size()>lim.max_terms){return false;}
    std::vector<U> mass(n,0);
    for(auto t:r){U v=magnitude(t.multiplier);if(v>lim.max_mass-mass[t.exponent])return false;mass[t.exponent]+=v;}
    return true;
}
inline ModulePlan identity_module(const Layout& l,const std::string& reason){
    ModulePlan p;p.basis=l.maps;p.reason=reason;p.transform.resize(l.maps.size());
    for(U i=0;i<l.maps.size();++i){p.transform[i].push_back({i,0,1});}
    return p;
}
inline ModulePlan module_plan(const Layout& l,ModuleLimits lim={},U n=DEGREE){
    require(n>0&&(n&(n-1))==0,"module power-of-two degree");
    ModulePlan p;std::vector<std::pair<U,Term>> pivots; // basis index, unit pivot
    for(U i=0;i<l.maps.size();++i){
        auto row=canonical_row(l.maps[i],n);std::vector<BasisMix> mix;U mix_mass=0;
        for(const auto& entry:pivots){
            U bi=entry.first;Term pivot=entry.second;std::vector<Term> factors;
            auto it=std::lower_bound(row.begin(),row.end(),Term{pivot.source,0,0},source_order);
            while(it!=row.end()&&it->source==pivot.source){factors.push_back(*it);++it;}
            for(auto f:factors){
                U shift=(f.exponent+n-pivot.exponent)%n;
                I sign=(pivot.exponent+shift>=n)?-1:1;
                I multiplier=f.multiplier*pivot.multiplier*sign; // inverse of +/- monomial
                U am=magnitude(multiplier);
                if(am>lim.mix_mass-mix_mass)return identity_module(l,"PUBLIC_MIX_LIMIT");
                mix_mass+=am;mix.push_back({bi,shift,multiplier});
                if(p.basis[bi].size()>lim.work-p.operations)return identity_module(l,"PUBLIC_WORK_LIMIT");
                p.operations+=p.basis[bi].size();
                auto next=row;next.reserve(row.size()+p.basis[bi].size());
                for(auto t:p.basis[bi]){
                    U exp=t.exponent+shift;I sg=exp>=n?-1:1;if(exp>=n)exp-=n;
                    W v=-W(multiplier)*sg*t.multiplier;
                    if(v<std::numeric_limits<I>::min()||v>std::numeric_limits<I>::max())return identity_module(l,"PUBLIC_INTEGER_LIMIT");
                    next.push_back({t.source,exp,I(v)});
                }
                row=canonical_row(std::move(next),n);
                if(!bounded_row(row,lim,n))return identity_module(l,"PUBLIC_MAP_LIMIT");
            }
        }
        if(!row.empty()){
            U bi=p.basis.size();if(mix_mass==lim.mix_mass)return identity_module(l,"PUBLIC_MIX_LIMIT");
            mix.push_back({bi,0,1});
            // Only a source whose complete entry is a unit polynomial is a pivot.
            // Nonunits, including 2 and 1+X, are NEVER divided by.
            for(std::size_t j=0;j<row.size();){std::size_t k=j+1;while(k<row.size()&&row[k].source==row[j].source)++k;
                if(k==j+1 && magnitude(row[j].multiplier)==1){pivots.push_back({bi,row[j]});break;}j=k;}
            p.basis.push_back(std::move(row));
        }
        p.transform.push_back(std::move(mix));
    }
    if(p.basis.empty()||p.basis.size()>=l.maps.size())return identity_module(l,"NO_RANK_CONTRACTION");
    p.contracted=true;p.reason="UNIT_PIVOT_CONTRACTION";return p;
}
inline void verify_module_plan(const Layout& l,const ModulePlan& p,U n=DEGREE){
    require(p.transform.size()==l.maps.size(),"module certificate rows");
    const ModuleLimits limits{};
    for(const auto& row:p.basis){for(auto t:row)require(t.source<l.inputs&&t.exponent<n,"certificate map bounds");require(bounded_row(row,limits,n),"certificate map mass");}
    for(const auto& row:p.transform){U mass=0;for(auto t:row){require(magnitude(t.multiplier)<=limits.mix_mass-mass,"certificate transform mass");mass+=magnitude(t.multiplier);}}

    for(U i=0;i<l.maps.size();++i){std::vector<Term> rebuilt;
        for(auto m:p.transform[i]){require(m.basis<p.basis.size()&&m.shift<n,"module certificate index");
            for(auto t:p.basis[m.basis]){U e=t.exponent+m.shift;I sg=e>=n?-1:1;if(e>=n)e-=n;
                W v=W(m.multiplier)*sg*t.multiplier;
                require(v>=std::numeric_limits<I>::min()&&v<=std::numeric_limits<I>::max(),"certificate coefficient overflow");
                rebuilt.push_back({t.source,e,I(v)});}}
        auto a=canonical_row(std::move(rebuilt),n),b=canonical_row(l.maps[i],n);
        require(a.size()==b.size(),"module identity size");
        for(U j=0;j<a.size();++j)require(a[j].source==b[j].source&&a[j].exponent==b[j].exponent&&a[j].multiplier==b[j].multiplier,"module exact identity");
    }
}
inline std::vector<Edge> contract_output(const Output& o,const ModulePlan& p,U n=DEGREE){
    // Private coefficient substitution, NOT a new fit/model. All r columns kept,
    // including zero columns: no weight-dependent network schedule or PM count.
    std::vector<std::vector<W>> sums(p.basis.size(),std::vector<W>(n,0));
    for(const auto& e:o.edges){require(e.input<p.transform.size(),"module private edge index");
        for(auto t:p.transform[e.input])for(auto a:e.coefficients){
            U ex=a.exponent+t.shift;I sign=ex>=n?-1:1;if(ex>=n)ex-=n;
            sums[t.basis][ex]+=W(a.value)*t.multiplier*sign;
        }}
    std::vector<Edge> edges;W norm=0;
    for(U j=0;j<sums.size();++j){Edge e;e.input=j;
        for(U k=0;k<n;++k){W v=sums[j][k];norm+=v<0?-v:v;
            require(norm<=(W(1)<<30),"private folded norm exceeds inherited H109 admission; stop, no secret fallback");
            e.coefficients.push_back({k,I(v)}); // fixed N entries, including zeros
        }edges.push_back(std::move(e));
    }return edges;
}
} // namespace aqcf3
