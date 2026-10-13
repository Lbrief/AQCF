#pragma once
// SOURCE PREPARATION ONLY. Not permission to dispatch a model or private request.
// Same integer operations, disjoint tensor outputs; no random state or network.
#include "resnet50/engine/compute_workers.hpp"
#include "resnet50/engine/native_worker_runtime.hpp"
#include <cstddef>
#include <iostream>
namespace aqcf_tensor4 {
template<class Work>void run(int party,const char* phase,std::size_t n,Work work){
    aqcf_compute4::check(party==1||party==2,"tensor4 party");
    aqcf_compute4::check(n>=4,"tensor4 requires four nonempty real ranges");
    const auto cpus=aqcf_native4::allocation;
    const auto e=aqcf_compute4::ordered<int>(4,cpus,[&](std::size_t k,unsigned lane){
        aqcf_compute4::check(k==lane,"one tensor range per lane");
        const auto lo=n*k/4,hi=n*(k+1)/4;
        aqcf_compute4::check(hi>lo,"nonempty tensor range");
        work(lo,hi);return 0;
    },[](std::size_t,int){});
    std::cout<<"AQCF_R50_TENSOR4 "<<phase<<" "<<n;
    for(unsigned k=0;k<4;++k){
        aqcf_compute4::check(e.cpus[k]==cpus[k]&&e.items[k]==1&&e.cpu_ns[k]>0,
                             "four useful tensor workers");
        std::cout<<" "<<e.cpus[k]<<":"<<(n*(k+1)/4-n*k/4)<<":"<<e.cpu_ns[k];
    }
    std::cout<<"\n"<<std::flush;
}
}
