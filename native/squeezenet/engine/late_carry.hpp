#pragma once
// Application contraction of the R8 two-cofactor packet + selector into R7's
// one-bit transition. Batched random NCO makes unknown carry choices legal.
// This construction is equivalent to R7 + standard NCO preprocessing. It is
// deliberately NOT claimed to be independent of that mature strong control.
#include "squeezenet/engine/carry_token.hpp"
#include "squeezenet/engine/late_ot_bank.hpp"
#include <type_traits>
#include <utility>
namespace aqcf2 {
template<class B,class=void> struct HasLateNCO:std::false_type {};
template<class B> struct HasLateNCO<B,std::void_t<decltype(std::declval<B&>().random_n_ot(std::size_t{},unsigned{}))>>:std::true_type {};
template<class B> RandomNBitOT prepare_late_bank(B& s,std::size_t n,unsigned in){
    if constexpr(HasLateNCO<B>::value)return s.random_n_ot(n,in);
    else throw std::runtime_error("selected backend has no random NCO implementation");
}
struct LateCarry {
    std::vector<Bits> value;
    U rows=0,alternatives=0,table_bits=0,choice_bits=0,preparation_calls=0,online_calls=0;
};
template<class B> LateCarry late_carry_execute(B& s,const std::vector<CarryTask>& tasks,bool cyclic=false){
    const auto n=validate_carry_tasks(tasks); LateCarry out;
    struct Step{std::size_t task;unsigned digit,width,in;};
    std::vector<std::vector<Step>> stages; std::array<std::size_t,6> totals{};
    std::vector<std::vector<Bits>> masks(tasks.size());
    unsigned depth=0;
    for(std::size_t t=0;t<tasks.size();++t){unsigned d=(tasks[t].width+3)/4;
        depth=std::max(depth,d);masks[t].assign(d,Bits(n));out.value.push_back(tasks[t].seed);
        if(s.party==1){auto r=s.random_small(n*d);for(unsigned j=0;j<d;++j)
            for(std::size_t k=0;k<n;++k)masks[t][j].v[k]=r[j*n+k]&1;}
    }
    // Bank order equals consumption order within each public alphabet.
    for(unsigned j=0;j<depth;++j)for(unsigned in=2;in<=5;++in){std::vector<Step> stage;
        for(std::size_t t=0;t<tasks.size();++t)if(j<masks[t].size()){
            unsigned d=token_digit_width(tasks[t].width,4*j);
            unsigned ib=d+unsigned(j!=0||!tasks[t].public_zero_seed);
            require(ib>=2&&ib<=5,"frozen RTE6/16 public digit alphabet");
            if(ib==in){stage.push_back({t,j,d,ib});totals[in]+=n;}
        }
        if(!stage.empty())stages.push_back(std::move(stage));
    }
    std::array<RandomNBitOT,6> bank;
    s.boundary_profile("late_all_random_NCO_preparation",[&](){
        for(unsigned in=2;in<=5;++in)if(totals[in]){
            bank[in]=prepare_late_bank(s,totals[in],in);++out.preparation_calls;
        }return 0;
    });
    s.boundary_profile("late_transition_consumption",[&](){
        for(const auto& stage:stages){const unsigned in=stage[0].in;const U N=U(1)<<in;
            const auto rows=stage.size()*n;std::vector<U> choice(rows),table;
            if(s.party==1)table.resize(rows*N);
            for(std::size_t z=0;z<stage.size();++z){auto st=stage[z];const auto& task=tasks[st.task];
                for(std::size_t k=0;k<n;++k){const auto at=z*n+k;
                    const U a=((*task.owned)[k]>>(4*st.digit))&mask(st.width);
                    const bool seed0=st.digit==0&&task.public_zero_seed;
                    choice[at]=seed0?a:(a<<1)|out.value[st.task].v[k];
                    if(s.party==1)for(U c=0;c<N;++c){const U b=seed0?c:c>>1;
                        const U carry=seed0?0:out.value[st.task].v[k]^(c&1);
                        table[at*N+c]=((a+b+carry)>>st.width)^masks[st.task][st.digit].v[k];
                    }
                }
            }
            auto got=late_take(s,bank[in],table,choice,cyclic);
            for(std::size_t z=0;z<stage.size();++z)for(std::size_t k=0;k<n;++k){auto st=stage[z];
                out.value[st.task].v[k]=s.party==1?masks[st.task][st.digit].v[k]:got[z*n+k];
            }
            out.rows+=rows;out.alternatives+=rows*N;out.table_bits+=rows*N;
            out.choice_bits+=rows*in;++out.online_calls;
            if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));
        }return 0;
    });
    for(unsigned in=2;in<=5;++in)if(totals[in])bank[in].finish();
    for(auto& lane:masks)for(auto& b:lane)OPENSSL_cleanse(b.v.data(),b.v.size());
    for(const auto& b:out.value)b.check();
    return out;
}
} // namespace aqcf2
