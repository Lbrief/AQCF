#pragma once
#include "squeezenet/engine/types.hpp"
#include <utility>
namespace aqcf2 {
// A comparison state is EQ=0, GT=1, LT=2, additively shared in Z/3Z.
// This is NOT an XOR Bits object and is never locally cast to Ring55.
struct Trit { std::vector<U> v; explicit Trit(std::size_t n=0):v(n){} };
inline unsigned small_bits(U n){unsigned b=0;for(U v=n-1;v;v>>=1)++b;return std::max(1U,b);}
inline U trit_pow(unsigned a,U base=3){U v=1;while(a--)v*=base;return v;}
inline U state_open(U a,U b,bool xor_state){return xor_state?(a^b):(a+b)%3;}
inline U state_mask(U a,U r,bool xor_state){return xor_state?(a^r):(a+3-r)%3;}
inline U compare_trit(U a,U b){return a==b?0:a>b?1:2;}
inline U join_trit(U low,U high){return high?high:low;}
template<class B> std::vector<U> random_trits(B&s,std::size_t n){
    std::vector<U> out;out.reserve(n);
    while(out.size()<n){auto r=s.random_small(std::max<std::size_t>(256,n-out.size()));
        for(auto v:r)if(v<3&&out.size()<n)out.push_back(v);}
    return out;
}
template<class B> std::vector<Trit> trit_leaves(B&s,const std::vector<U>& x,unsigned width,unsigned radix=5,bool xor_state=false){
    require(width>=1&&width<=59&&radix==5,"trit leaf contract");
    const U base=xor_state?4:3;const U dm=mask(radix),wm=mask(width);unsigned d=(width+radix-1)/radix;std::size_t n=x.size(),rows=n*d;
    std::vector<U> choices(rows),table,masks;
    if(s.party==1){table.resize(rows*(U(1)<<radix));if(xor_state){auto rr=s.random_small(rows);masks.assign(rr.begin(),rr.end());}else masks=random_trits(s,rows);}
    for(unsigned j=0;j<d;++j)for(std::size_t k=0;k<n;++k){auto at=j*n+k;U v=x[k]&wm;
        if(s.party==2){v=wm-v;}
        U digit=(v>>(j*radix))&dm;choices[at]=digit;
        if(s.party==1)for(U c=0;c<=dm;++c)table[at*(dm+1)+c]=state_mask(compare_trit(digit,c),masks[at],xor_state);
    }
    auto got=s.lookup_words(table,choices,rows,radix,2);std::vector<Trit> out(d,Trit(n));
    for(unsigned j=0;j<d;++j)for(std::size_t k=0;k<n;++k){U v=s.party==1?masks[j*n+k]:got[j*n+k];require(v<base,"canonical state leaf");out[j].v[k]=v;}
    return out;
}
// Parallel 3-ary level. Groups of 2 and 3 are batched separately to avoid
// padding two-state nodes to the larger OT alphabet. A lone node is carried.
template<class B> std::vector<Trit> trit_level(B&s,const std::vector<Trit>& in,bool xor_state=false){
    const U base=xor_state?4:3;
    require(in.size()>3,"trit level size");std::size_t n=in[0].v.size();
    const std::size_t groups=(in.size()+2)/3;std::vector<Trit> out(groups,Trit(n));
    for(unsigned arity:{3U,2U}){
        std::vector<std::size_t> ids;for(std::size_t j=0;j<groups;++j)if(std::min<std::size_t>(3,in.size()-3*j)==arity)ids.push_back(j);
        if(ids.empty()){continue;}
        U alphabet=trit_pow(arity,base);unsigned bits=small_bits(alphabet);U entries=U(1)<<bits;
        std::size_t rows=ids.size()*n;std::vector<U> choices(rows),table,masks;
        if(s.party==1){table.resize(rows*entries);if(xor_state){auto rr=s.random_small(rows);masks.assign(rr.begin(),rr.end());}else masks=random_trits(s,rows);}
        for(std::size_t gi=0;gi<ids.size();++gi)for(std::size_t k=0;k<n;++k){auto at=gi*n+k;U code=0,pow=1;
            for(unsigned j=0;j<arity;++j){U v=in[3*ids[gi]+j].v[k];require(v<base,"state input");code+=pow*v;pow*=base;}choices[at]=code;
            if(s.party==1)for(U c=0;c<entries;++c){U state=0,t=c;
                if(c<alphabet)for(unsigned j=0;j<arity;++j){U v=state_open(in[3*ids[gi]+j].v[k],t%base,xor_state);t/=base;state=join_trit(state,v);}
                table[at*entries+c]=state_mask(state,masks[at],xor_state);
            }
        }
        auto got=s.lookup_words(table,choices,rows,bits,2);
        for(std::size_t gi=0;gi<ids.size();++gi)for(std::size_t k=0;k<n;++k){U v=s.party==1?masks[gi*n+k]:got[gi*n+k];require(v<base,"canonical joined state");out[ids[gi]].v[k]=v;}
    }
    if(in.size()%3==1){out.back()=in.back();}
    return out;
}
// Last lookup returns an XOR bit directly. With parity, it computes
// GT XOR (EQ AND parity), the exact RTE carry/tie decision.
template<class B> Bits trit_root(B&s,const std::vector<Trit>& states,const Bits* parity,bool xor_state=false){
    const U base=xor_state?4:3;
    unsigned arity=unsigned(states.size());require(arity>=1&&arity<=3,"trit root arity");std::size_t n=states[0].v.size();
    if(parity){require(parity->v.size()==n,"trit root parity");parity->check();}
    U alphabet=trit_pow(arity,base),choices_n=alphabet*(parity?2:1);unsigned bits=small_bits(choices_n);U entries=U(1)<<bits;
    std::vector<U> choices(n),table;std::vector<std::uint8_t> random;
    if(s.party==1){table.resize(n*entries);random=s.random_small(n);}
    for(std::size_t k=0;k<n;++k){U code=0,pow=1;
        for(unsigned j=0;j<arity;++j){require(states[j].v[k]<base,"root input state");code+=pow*states[j].v[k];pow*=base;}
        if(parity){code+=alphabet*parity->v[k];}
        choices[k]=code;
        if(s.party==1)for(U c=0;c<entries;++c){U bit=0;
            if(c<choices_n){U state=0,t=c%alphabet;
                for(unsigned j=0;j<arity;++j){state=join_trit(state,state_open(states[j].v[k],t%base,xor_state));t/=base;}
                bit=U(state==1);if(parity&&state==0)bit^=U(parity->v[k])^(c/alphabet);
            }
            table[k*entries+c]=bit^(random[k]&1);
        }
    }
    auto got=s.lookup_words(table,choices,n,bits,1);Bits out(n);
    for(std::size_t k=0;k<n;++k){out.v[k]=s.party==1?(random[k]&1):std::uint8_t(got[k]);}
    out.check();return out;
}
template<class B> Bits ternary_carry(B&s,const std::vector<U>& x,unsigned w,const Bits* parity=nullptr,bool xor_state=false){
    auto st=trit_leaves(s,x,w,5,xor_state);while(st.size()>3)st=trit_level(s,st,xor_state);return trit_root(s,st,parity,xor_state);
}
template<class B> Bits ternary_msb(B&s,const Ring& x,bool xor_state=false){
    x.check();require(x.width>=2,"trit msb width");auto c=ternary_carry(s,x.v,x.width-1,nullptr,xor_state);
    return s.xor_(c,s.local_bit(x.v,x.width-1));
}
// Strong traditional control: same radix-5 leaves, ordinary XOR GT/EQ shares,
// balanced binary tree, native SCI AND. No deliberately enlarged comparator.
template<class B> std::pair<Bits,Bits> binary5_carry_eq(B&s,const std::vector<U>& x,unsigned width){
    require(width>=1&&width<=59,"binary5 width");unsigned digits=(width+4)/5;std::size_t n=x.size(),rows=n*digits;
    std::vector<U> choices(rows),table;std::vector<std::uint8_t> masks;
    if(s.party==1){table.resize(rows*32);masks=s.random_small(rows);}
    for(unsigned d=0;d<digits;++d)for(std::size_t j=0;j<n;++j){auto at=d*n+j;U v=x[j]&mask(width);if(s.party==2)v=mask(width)-v;
        U a=(v>>(5*d))&31;choices[at]=a;if(s.party==1)for(U b=0;b<32;++b)table[at*32+b]=(U(a>b)|(U(a==b)<<1))^(masks[at]&3);
    }
    auto got=s.lookup_words(table,choices,rows,5,2);struct Node{Bits g,e;};std::vector<Node> level;
    for(unsigned d=0;d<digits;++d){Node z{Bits(n),Bits(n)};for(std::size_t j=0;j<n;++j){U v=s.party==1?masks[d*n+j]:got[d*n+j];z.g.v[j]=v&1;z.e.v[j]=(v>>1)&1;}level.push_back(std::move(z));}
    while(level.size()>1){std::vector<std::pair<Bits,Bits>> pairs;
        for(std::size_t j=0;j+1<level.size();j+=2){pairs.emplace_back(level[j+1].e,level[j].g);pairs.emplace_back(level[j+1].e,level[j].e);}
        auto a=s.and_batch(pairs);std::vector<Node> next;
        for(std::size_t j=0;j+1<level.size();j+=2)next.push_back({s.xor_(level[j+1].g,a[j]),a[j+1]});
        if(level.size()%2){next.push_back(std::move(level.back()));}
        level=std::move(next);
    }return {level[0].g,level[0].e};
}
} // namespace aqcf2
