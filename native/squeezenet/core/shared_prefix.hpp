#pragma once
#include "squeezenet/core/direct_token_core.hpp"
#include "squeezenet/core/direct_ring_relu.hpp"
#include "squeezenet/core/wave_policy.hpp"
namespace aqcf2 {
// The larger public wave must not bypass the frozen 65536-row KKOT bound.
// Split independent lookup rows, not the dependent carry path. Each subcall
// advances the existing OT stream and consumes fresh material exactly once.
template<class B> std::vector<U> prefix_wave_lookup(B& s,std::vector<U>& table,
 const std::vector<U>& choice,std::size_t n,unsigned in,unsigned out,bool compact){
    require(n>0&&choice.size()==n,"prefix lookup row shape");
    const std::size_t stride=compact?1:(U(1)<<in);
    require(s.party!=1||table.size()==n*stride,"prefix sender table shape");
    if(n<=65536)return s.lookup_words(table,choice,n,in,out,compact);
    require(n<=8*aqcf_wan_wave::relu_limit,"bounded independent prefix packets");
    std::vector<U> result(n);
    for(std::size_t begin=0;begin<n;begin+=65536){
        const auto len=std::min<std::size_t>(65536,n-begin);
        std::vector<U> c(choice.begin()+begin,choice.begin()+begin+len),t;
        if(s.party==1)t.assign(table.begin()+begin*stride,table.begin()+(begin+len)*stride);
        auto got=s.lookup_words(t,c,len,in,out,compact);
        std::copy(got.begin(),got.end(),result.begin()+begin);
        if(!t.empty())OPENSSL_cleanse(t.data(),t.size()*sizeof(U));
    }
    return result;
}
template<class B> LinkedCarry linked_cofactor_prefix_execute(B& s,const std::vector<CarryTask>& tasks,unsigned tap_width,Bits& tapped){

 require(tasks.size()==1,"direct prefix single task");
 const auto& task=tasks[0];require(task.owned,"direct prefix owned input");
 auto r=s.boundary_profile("direct_token_prefix",[&]{return aqcf_direct_carry20::run(s,*task.owned,task.width,task.seed,task.public_zero_seed,tap_width);});
 LinkedCarry result;result.value.push_back(r.carry);tapped=r.tap;
 result.packet_rows=r.rows;result.packet_table_bits=r.bits;result.packet_calls=r.calls;
 result.selector_rows=0;return result;

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
