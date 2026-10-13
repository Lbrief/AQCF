#pragma once
// Exact predicate-only radix-16 carry execution, NOT a new OT primitive.
// Only owned words and XOR-shared carry tokens are consumed. No state is opened.
#include "types.hpp"
#include <array>
#include <utility>
namespace aqcf2 {
struct CarryTask {
    const std::vector<U>* owned;
    unsigned width;
    Bits seed;
    bool public_zero_seed;
};
struct CarryTokens {
    std::vector<Bits> value;
    U logical_transitions=0, ot_rows=0, table_bits=0, ot_calls=0;
    U nonlinear_and_elements=0;
};
inline unsigned token_digit_width(unsigned width,unsigned position){
    require(position<width,"token public digit position");
    return std::min(4U,width-position);
}
inline U token_step_plain(U a,U b,U carry,unsigned bits){
    require(bits>=1&&bits<=4&&a<=mask(bits)&&b<=mask(bits)&&carry<2,"token step domain");
    return (a+b+carry)>>bits;
}
inline std::size_t validate_carry_tasks(const std::vector<CarryTask>& tasks){
    require(!tasks.empty()&&tasks.size()<=2,"one/two exact boundary tasks");
    const auto n=tasks[0].seed.v.size();require(n>0&&n<=4096,"fixed original boundary chunk");
    for(const auto&t:tasks){
        require(t.owned&&t.owned->size()==n&&t.width>=1&&t.width<=54&&t.seed.v.size()==n,"token task shape");
        t.seed.check();
        if(t.public_zero_seed)for(auto b:t.seed.v)require(b==0,"public zero seed is actually zero");
    }
    return n;
}
// Main construction: evaluate the carry action on the ONE actual secret token.
// Unlike GT/EQ materialization, no outputs for unused incoming carries survive.
// Width/public-seed buckets only; group membership never uses a secret value.
template<class B> CarryTokens carry_token_execute(B& s,const std::vector<CarryTask>& tasks){
    const auto n=validate_carry_tasks(tasks);CarryTokens result;
    unsigned max_width=0;for(const auto&t:tasks){result.value.push_back(t.seed);max_width=std::max(max_width,t.width);}
    for(unsigned position=0;position<max_width;position+=4){
        // The seed-free first high digit has four choices bits, not five.
        for(unsigned selector_bits=1;selector_bits<=5;++selector_bits){
            std::vector<std::size_t> ids;
            for(std::size_t t=0;t<tasks.size();++t)if(position<tasks[t].width){
                unsigned d=token_digit_width(tasks[t].width,position);
                bool free_seed=position==0&&tasks[t].public_zero_seed;
                if(d+(free_seed?0:1)==selector_bits)ids.push_back(t);
            }
            if(ids.empty())continue;
            const std::size_t rows=n*ids.size();const U entries=U(1)<<selector_bits;
            std::vector<U> choices(rows),table;std::vector<std::uint8_t> masks;
            if(s.party==1){masks=s.random_small(rows);table.resize(rows*entries);}
            for(std::size_t g=0;g<ids.size();++g){const auto id=ids[g];const auto&t=tasks[id];
                const unsigned d=token_digit_width(t.width,position);
                const bool free_seed=position==0&&t.public_zero_seed;
                for(std::size_t j=0;j<n;++j){const auto row=g*n+j;
                    const U digit=((*t.owned)[j]>>position)&mask(d);
                    const U own_carry=result.value[id].v[j];
                    choices[row]=free_seed?digit:((digit<<1)|own_carry);
                    if(s.party==1)for(U c=0;c<entries;++c){
                        const U other_digit=free_seed?c:c>>1;
                        const U carry=free_seed?0:(own_carry^(c&1));
                        table[row*entries+c]=token_step_plain(digit,other_digit,carry,d)^(masks[row]&1);
                    }
                }
            }
            auto got=s.lookup_words(table,choices,rows,selector_bits,1);
            for(std::size_t g=0;g<ids.size();++g)for(std::size_t j=0;j<n;++j){const auto row=g*n+j;
                U value=s.party==1?(masks[row]&1):got[row];require(value<2,"token OT output must be a bit");
                result.value[ids[g]].v[j]=std::uint8_t(value);
            }
            result.logical_transitions+=rows;result.ot_rows+=rows;
            result.table_bits+=rows*entries;++result.ot_calls;
        }
    }
    for(const auto&v:result.value) v.check();
    return result;
}
// Strong mature control: same radix, secret parity seed and boundary tasks,
// but materialize all subsequent digit (generate,propagate) pairs, then apply
// their unary actions with native AND. This generalizes the inherited folded
// scan; it is NOT attributed as a new AQCF contribution.
template<class B> CarryTokens carry_materialized_execute(B& s,const std::vector<CarryTask>& tasks){
    const auto n=validate_carry_tasks(tasks);CarryTokens result;
    unsigned max_width=0;std::vector<std::vector<Bits>> generate(tasks.size()),propagate(tasks.size());
    for(std::size_t t=0;t<tasks.size();++t){
        max_width=std::max(max_width,tasks[t].width);result.value.push_back(tasks[t].seed);
        const auto d=(tasks[t].width+3)/4;generate[t].assign(d,Bits(n));propagate[t].assign(d,Bits(n));
    }
    // First digit consumes the seed directly, so OFF is not burdened with an
    // unnecessary seed-B2A, equality output or first AND gate.
    for(unsigned selector_bits=1;selector_bits<=5;++selector_bits){
        std::vector<std::size_t> ids;
        for(std::size_t t=0;t<tasks.size();++t){unsigned d=token_digit_width(tasks[t].width,0);
            if(d+(tasks[t].public_zero_seed?0:1)==selector_bits)ids.push_back(t);}
        if(ids.empty())continue;
        const auto rows=n*ids.size();const U entries=U(1)<<selector_bits;
        std::vector<U> choices(rows),table;std::vector<std::uint8_t> masks;
        if(s.party==1){masks=s.random_small(rows);table.resize(rows*entries);}
        for(std::size_t g=0;g<ids.size();++g){const auto id=ids[g];const auto&t=tasks[id];
            unsigned d=token_digit_width(t.width,0);
            for(std::size_t j=0;j<n;++j){auto row=g*n+j;U digit=(*t.owned)[j]&mask(d);
                choices[row]=t.public_zero_seed?digit:(digit<<1)|t.seed.v[j];
                if(s.party==1)for(U c=0;c<entries;++c){U b=t.public_zero_seed?c:c>>1;
                    U cy=t.public_zero_seed?0:(U(t.seed.v[j])^(c&1));
                    table[row*entries+c]=token_step_plain(digit,b,cy,d)^(masks[row]&1);}}
        }
        auto got=s.lookup_words(table,choices,rows,selector_bits,1);
        for(std::size_t g=0;g<ids.size();++g)for(std::size_t j=0;j<n;++j){auto row=g*n+j;
            U v=s.party==1?(masks[row]&1):got[row];require(v<2,"materialized first token");result.value[ids[g]].v[j]=v;}
        result.logical_transitions+=rows;result.ot_rows+=rows;result.table_bits+=rows*entries;++result.ot_calls;
    }
    // All higher digit relations can be generated in parallel by actual width.
    for(unsigned d=1;d<=4;++d){
        std::vector<std::pair<std::size_t,unsigned>> nodes;
        for(std::size_t t=0;t<tasks.size();++t)for(unsigned p=4;p<tasks[t].width;p+=4)
            if(token_digit_width(tasks[t].width,p)==d)nodes.emplace_back(t,p/4);
        if(nodes.empty())continue;
        const auto rows=n*nodes.size();const U entries=U(1)<<d;
        std::vector<U> choices(rows),table;std::vector<std::uint8_t> masks;
        if(s.party==1){masks=s.random_small(rows);table.resize(rows*entries);}
        for(std::size_t g=0;g<nodes.size();++g){auto [t,p]=nodes[g];
            for(std::size_t j=0;j<n;++j){auto row=g*n+j;U a=((*tasks[t].owned)[j]>>(p*4))&mask(d);choices[row]=a;
                if(s.party==1)for(U b=0;b<entries;++b){U sum=a+b;
                    table[row*entries+b]=((U(sum>=entries))|(U(sum==entries-1)<<1))^(masks[row]&3);}}
        }
        auto got=s.lookup_words(table,choices,rows,d,2);
        for(std::size_t g=0;g<nodes.size();++g){auto [t,p]=nodes[g];for(std::size_t j=0;j<n;++j){auto row=g*n+j;
            U v=s.party==1?(masks[row]&3):got[row];require(v<4,"materialized digit bits");
            generate[t][p].v[j]=v&1;propagate[t][p].v[j]=(v>>1)&1;}}
        result.logical_transitions+=rows;result.ot_rows+=rows;result.table_bits+=2*rows*entries;++result.ot_calls;
    }
    for(unsigned p=1;p*4<max_width;++p){
        std::vector<std::size_t> ids;std::vector<std::pair<Bits,Bits>> products;
        for(std::size_t t=0;t<tasks.size();++t)if(p*4<tasks[t].width){ids.push_back(t);products.emplace_back(propagate[t][p],result.value[t]);}
        auto got=s.and_batch(products);
        for(std::size_t g=0;g<ids.size();++g){auto t=ids[g];result.value[t]=s.xor_(generate[t][p],got[g]);}
        result.nonlinear_and_elements+=n*ids.size();
    }
    for(const auto&v:result.value) v.check();
    return result;
}
} // namespace aqcf2
