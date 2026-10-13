#pragma once
#include "direct_ring_relu.hpp"
namespace aqcf2 {
template<class B> LinkedCarry linked_cofactor_prefix_execute(B& s,const std::vector<CarryTask>& tasks,unsigned tap_width,Bits& tapped){
    const auto n=validate_carry_tasks(tasks);LinkedCarry out;
    require(tasks.size()==1&&tap_width>=4&&tap_width%4==0&&tap_width<tasks[0].width,"public single-path tap shape");
    struct Lane {std::vector<Bits> masks;std::vector<std::vector<U>> packet;std::vector<std::size_t> offset;};
    std::vector<Lane> lanes(tasks.size());std::size_t selectors=0;unsigned depth=0;
    for(std::size_t t=0;t<tasks.size();++t){const auto d=(tasks[t].width+3)/4;depth=std::max(depth,d);
        auto& l=lanes[t];l.masks.assign(d,Bits(n));l.packet.assign(d,std::vector<U>(n));l.offset.resize(d);
        if(s.party==1){auto r=s.random_small(d*n);for(unsigned j=0;j<d;++j)for(std::size_t k=0;k<n;++k)l.masks[j].v[k]=r[j*n+k]&1;}
        for(unsigned j=1;j<d;++j){l.offset[j]=selectors;selectors+=n;}
        out.value.push_back(tasks[t].seed);
    }
    // V0,V1 are separate from ROT key pairs, and from every R_j. Key reuse
    // here would let a receiver decrypt both cofactors and expose propagation.
    std::vector<std::uint8_t> pad0(selectors),pad1(selectors);
    if(s.party==1&&selectors){auto v=s.random_small(selectors);for(std::size_t i=0;i<selectors;++i){pad0[i]=v[i]&1;pad1[i]=(v[i]>>1)&1;}}
    s.boundary_profile("linked_packet_preparation",[&](){
        // Same first-digit treatment as strong S, including secret parity seed.
        for(unsigned ib=1;ib<=5;++ib){std::vector<std::size_t> ids;
            for(std::size_t t=0;t<tasks.size();++t)if(std::min(4U,tasks[t].width)+(tasks[t].public_zero_seed?0:1)==ib)ids.push_back(t);
            if(ids.empty())continue;
            const auto rows=ids.size()*n;const U count=U(1)<<ib;std::vector<U> choice(rows),table;
            if(s.party==1)table.resize(rows*count);
            for(std::size_t j=0;j<ids.size();++j){auto t=ids[j];unsigned d=std::min(4U,tasks[t].width);
                for(std::size_t k=0;k<n;++k){auto at=j*n+k;U a=(*tasks[t].owned)[k]&mask(d);
                    choice[at]=tasks[t].public_zero_seed?a:(a<<1)|tasks[t].seed.v[k];
                    if(s.party==1)for(U c=0;c<count;++c){U b=tasks[t].public_zero_seed?c:c>>1;
                        U seed=tasks[t].public_zero_seed?0:(tasks[t].seed.v[k]^(c&1));
                        table[at*count+c]=token_step_plain(a,b,seed,d)^lanes[t].masks[0].v[k];}}
            }
            auto got=s.lookup_words(table,choice,rows,ib,1);
            for(std::size_t j=0;j<ids.size();++j)for(std::size_t k=0;k<n;++k)
                out.value[ids[j]].v[k]=s.party==1?lanes[ids[j]].masks[0].v[k]:got[j*n+k];
            out.packet_rows+=rows;out.packet_table_bits+=rows*count;++out.packet_calls;
        }
        // All higher packets are independent of the selected carry path.
        for(unsigned d=1;d<=4;++d){std::vector<std::pair<std::size_t,unsigned>> ids;
            for(std::size_t t=0;t<tasks.size();++t)for(unsigned j=1;j<lanes[t].masks.size();++j)
                if(token_digit_width(tasks[t].width,4*j)==d)ids.emplace_back(t,j);
            if(ids.empty())continue;
            const auto rows=ids.size()*n;const U count=U(1)<<d;std::vector<U> choice(rows),table;
            if(s.party==1)table.resize(rows*count);
            for(std::size_t x=0;x<ids.size();++x){auto [t,j]=ids[x];
                for(std::size_t k=0;k<n;++k){auto at=x*n+k,pi=lanes[t].offset[j]+k;U a=((*tasks[t].owned)[k]>>(4*j))&mask(d);choice[at]=a;
                    if(s.party==1)for(U b=0;b<count;++b){U r=lanes[t].masks[j-1].v[k],next=lanes[t].masks[j].v[k];
                        // a,b are already bounded d-bit digits; r/next are masked bits.
                        // Avoid revalidating identical loop invariants per table cell.
                        const U sum=a+b;
                        U u0=((sum+r)>>d)^next^pad0[pi];
                        U u1=((sum+(r^1))>>d)^next^pad1[pi];
                        table[at*count+b]=u0|(u1<<1);}}
            }
            auto got=s.lookup_words(table,choice,rows,d,2);
            if(s.party==2)for(std::size_t x=0;x<ids.size();++x){auto [t,j]=ids[x];for(std::size_t k=0;k<n;++k)lanes[t].packet[j][k]=got[x*n+k];}
            out.packet_rows+=rows;out.packet_table_bits+=2*rows*count;++out.packet_calls;
        }
        return 0;
    });
    if(tap_width==4)tapped=out.value[0];
    // All heavy binary OTs are batched here, with fresh RANDOM choices. This
    // is standard ROT-to-chosen-OT; generation time/bytes are NOT moved offline.
    auto rot=s.boundary_profile("linked_selector_preparation",[&](){return s.random_ot_bits(selectors);});
    std::vector<std::uint8_t> used(selectors,0);
    s.boundary_profile("linked_selector_consumption",[&](){
        for(unsigned j=1;j<depth;++j){std::vector<std::size_t> ids;
            for(std::size_t t=0;t<tasks.size();++t)if(j<lanes[t].masks.size())ids.push_back(t);
            const auto rows=ids.size()*n;require(rows>0,"public selector stage");
            std::vector<std::uint8_t> e(rows),reply(2*rows);
            for(std::size_t x=0;x<ids.size();++x)for(std::size_t k=0;k<n;++k){auto t=ids[x],at=x*n+k,pi=lanes[t].offset[j]+k;
                require(!used[pi],"ROT reuse forbidden");used[pi]=1;
                if(s.party==2)e[at]=out.value[t].v[k]^rot.t[pi];
                else require(out.value[t].v[k]==lanes[t].masks[j-1].v[k],"linked predecessor invariant");}
            e=s.transfer_masked_bits(2,e);
            if(s.party==1)for(std::size_t x=0;x<ids.size();++x)for(std::size_t k=0;k<n;++k){auto t=ids[x],at=x*n+k,pi=lanes[t].offset[j]+k;
                reply[2*at]=(e[at]?pad1[pi]:pad0[pi])^rot.k0[pi];
                reply[2*at+1]=(e[at]?pad0[pi]:pad1[pi])^rot.k1[pi];}
            reply=s.transfer_masked_bits(1,reply);
            for(std::size_t x=0;x<ids.size();++x)for(std::size_t k=0;k<n;++k){auto t=ids[x],at=x*n+k,pi=lanes[t].offset[j]+k;
                if(s.party==1)out.value[t].v[k]=lanes[t].masks[j].v[k];
                else {U incoming=out.value[t].v[k],selected=reply[2*at+rot.t[pi]]^rot.kt[pi];
                    out.value[t].v[k]=((lanes[t].packet[j][k]>>incoming)&1)^selected;}
                // Remove the live one-use keys after this transition.
                rot.k0[pi]=rot.k1[pi]=rot.t[pi]=rot.kt[pi]=pad0[pi]=pad1[pi]=0;
            }
            if(4*(j+1)==tap_width)tapped=out.value[0];
            out.selector_rows+=rows;
        }
        return 0;
    });
    require(out.selector_rows==selectors&&std::all_of(used.begin(),used.end(),[](auto v){return v==1;}),"complete single-use selection bank");
    require(tapped.v.size()==n,"carry tap emitted once");tapped.check();
    for(const auto& b:out.value)b.check();
    return out;
}
}

namespace aqcf_tiny_comm {
template<class B> aqcf2::Ring shared_prefix_relu(B&s,const aqcf2::Ring&in,unsigned f){
 using namespace aqcf2;
 if(f%4||f<4||f>=40)return direct_ring_relu(s,in,f);
 require(in.width==55&&f<=30&&!in.v.empty()&&in.v.size()<=4096,"prefix floor ReLU");
 const auto n=in.v.size();std::vector<U> own(n);Ring coarse(41-f,n);
 for(size_t j=0;j<n;++j){own[j]=in.v[j]&mask(41);coarse.v[j]=own[j]>>f;}
 std::vector<CarryTask> tasks{{&own,40,s.constant(false,n),true}};Bits low;
 auto c=linked_cofactor_prefix_execute(s,tasks,f,low);
 s.note_linked_execution(n,n,c.packet_rows,c.packet_table_bits,c.selector_rows);
 return boundary_relu_emit(s,coarse,low,c.value[0]);
}
}
