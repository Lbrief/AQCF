#pragma once
// Same predicates, same online call sites. No model values or secrets logged.
// Parallel is a join-before-return callback, with disjoint index ranges and
// exception propagation. The production callback is BfvProgram::chunks4.
#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <vector>
namespace aqcf_validation4 {
inline void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}

template<class Maps,class Queries,class U,class Parallel>
void geometry(const Maps& maps,const Queries& queries,U b,U j,U f,Parallel parallel){
    parallel("PUBLIC_ACTION_INPUT_GEOMETRY",maps.size(),[&](std::size_t lo,std::size_t hi){
        for(auto i=lo;i<hi;++i){const auto& m=maps[i];check(!m.empty(),"nonempty tail input");
            for(const auto& t:m)check(t.exponent<j*b&&t.multiplier==1,"zero-filled channel-fiber input");}
    });
    parallel("PUBLIC_ACTION_OUTPUT_GEOMETRY",queries.size(),[&](std::size_t lo,std::size_t hi){
        for(auto i=lo;i<hi;++i){const auto& q=queries[i];check(!q.empty(),"nonempty tail query");
            for(const auto& t:q)check(t.exponent<f*j*b&&(t.exponent/b)%j==j-1,"fiber output coset");}
    });
}

template<class Outputs,class U,class Parallel>
void support(const Outputs& output,U b,Parallel parallel){
    check(b!=0,"nonzero admitted private kernel stride");
    parallel("PRIVATE_COEFFICIENT_SUPPORT",output.size(),[&](std::size_t lo,std::size_t hi){
        for(auto i=lo;i<hi;++i)for(const auto& e:output[i].edges)for(const auto& a:e.coefficients)
            check(a.exponent%b==0,"private kernel outside admitted public action class");
    });
}

template<class Queries,class U,class Parallel>
void destinations(const Queries& queries,std::size_t outputs,U degree,Parallel parallel){
    // Never use vector<bool> or a non-atomic shared byte for duplicate detection.
    // Default-constructed C++17 atomics are explicitly initialized before any use.
    std::vector<std::atomic<unsigned char>> seen(outputs);
    parallel("CLIENT_DESTINATION_ZERO",outputs,[&](std::size_t lo,std::size_t hi){
        for(auto i=lo;i<hi;++i)seen[i].store(0,std::memory_order_relaxed);
    });
    parallel("CLIENT_DESTINATION_MARK",queries.size(),[&](std::size_t lo,std::size_t hi){
        for(auto i=lo;i<hi;++i)for(const auto& q:queries[i]){
            check(q.destination<outputs&&q.exponent<degree,"disjoint complete client destinations");
            check(seen[q.destination].exchange(1,std::memory_order_relaxed)==0,
                  "disjoint complete client destinations");
        }
    });
    parallel("CLIENT_DESTINATION_COVERAGE",outputs,[&](std::size_t lo,std::size_t hi){
        for(auto i=lo;i<hi;++i)check(seen[i].load(std::memory_order_relaxed)==1,"all client outputs covered");
    });
    // Thread joins between phases establish completion. Atomics prevent same-
    // destination races on invalid fixtures, and exchange cannot wrap as a count.
}
}
