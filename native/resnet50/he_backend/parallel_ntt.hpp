#pragma once
#include "resnet50/engine/persistent_worker_owner.hpp"
// Scheduling-only candidate derived from the pinned SEAL 4.1.2 DWT operation order.
// NOT production-linked or qualified. No RNG, ciphertext padding, or cached secrets.
#include <seal/util/ntt.h>
#include <array>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <fstream>
#include <mutex>
#include <pthread.h>
#include <sched.h>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <time.h>
#include <utility>
#include <vector>

namespace pcn_ntt4 {
using U = std::uint64_t;
using Operand = seal::util::MultiplyUIntModOperand;
using Arithmetic = seal::util::Arithmetic<U, Operand, Operand>;
inline void require(bool ok, const char *why) {
    if (!ok) throw std::runtime_error(why);
}
inline U thread_ns() {
    timespec ts{};
    require(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts)==0, "NTT4 thread clock");
    return U(ts.tv_sec)*1000000000ULL+U(ts.tv_nsec);
}
inline int topology_value(int cpu, const char *name) {
    std::ifstream in("/sys/devices/system/cpu/cpu"+std::to_string(cpu)+"/topology/"+name);
    int value=-1; require(bool(in>>value), "NTT4 physical topology missing"); return value;
}
inline void admission(const std::array<int,4> &cpus) {
    cpu_set_t allowed; CPU_ZERO(&allowed);
    require(sched_getaffinity(0,sizeof(allowed),&allowed)==0, "NTT4 read affinity");
    require(CPU_COUNT(&allowed)==4, "NTT4 exactly four admitted CPUs");
    std::set<int> unique;
    std::set<std::pair<int,int>> physical;
    for (int cpu:cpus) {
        require(cpu>=0 && cpu<CPU_SETSIZE && CPU_ISSET(cpu,&allowed), "NTT4 CPU admission");
        unique.insert(cpu);
        physical.emplace(topology_value(cpu,"physical_package_id"),topology_value(cpu,"core_id"));
    }
    require(unique.size()==4 && physical.size()==4, "NTT4 four distinct physical cores");
}
struct Lane {
    U jobs=0, cpu_ns=0;
    int actual_cpu=-1;
};
struct Receipt {
    std::size_t degree=0;
    bool inverse=false, canonical=false;
    // Stage0 checks bounds; stages1..logN are real butterflies; final optional reduction.
    std::vector<std::array<Lane,4>> stages;
};
class Barrier {
    std::mutex lock_;
    std::condition_variable changed_;
    unsigned arrivals_=0, epoch_=0;
    bool broken_=false;
public:
    void abort() {
        std::lock_guard<std::mutex> lock(lock_); broken_=true; changed_.notify_all();
    }
    void wait() {
        std::unique_lock<std::mutex> lock(lock_);
        require(!broken_,"NTT4 cancelled barrier");
        unsigned epoch=epoch_;
        if (++arrivals_==4) { arrivals_=0; ++epoch_; changed_.notify_all(); }
        else changed_.wait(lock,[&]{return broken_ || epoch_!=epoch;});
        require(!broken_,"NTT4 cancelled barrier");
    }
};

// On any exception caller MUST discard values: no partial polynomial may be used.
// Joining is unconditional, including a failed thread launch or broken barrier.
template<class Values> inline Receipt transform(Values &values, const seal::util::NTTTables &tables,
                         const std::array<int,4> &cpus, bool inverse, bool canonical,
                         int public_test_failure_lane=-1) {
    const int logn=tables.coeff_count_power();
    require(logn>=3 && logn<=16,"NTT4 bounded public transform size");
    const std::size_t n=std::size_t(1)<<logn;
    const U p=tables.modulus().value();
    require(values.size()==n && p>2 && p<(U(1)<<61),"NTT4 shape/modulus");
    require(public_test_failure_lane>=-1 && public_test_failure_lane<4,"NTT4 failure hook");
    admission(cpus);
    Receipt result; result.degree=n; result.inverse=inverse; result.canonical=canonical;
    result.stages.resize(1+std::size_t(logn)+(canonical?1:0));
    Barrier barrier;
    std::mutex error_lock;
    std::exception_ptr first_error;
    std::vector<aqcf_owner4::Task> threads;
    const auto *roots=inverse?tables.get_from_inv_root_powers():tables.get_from_root_powers();
    const Arithmetic arith(tables.modulus());
    auto worker=[&](unsigned lane) {
        try {
            cpu_set_t pin; CPU_ZERO(&pin); CPU_SET(cpus[lane],&pin);
            require(pthread_setaffinity_np(pthread_self(),sizeof(pin),&pin)==0,"NTT4 pin");
            require(sched_getcpu()==cpus[lane],"NTT4 actual CPU");
            barrier.wait();
            auto stage=[&](std::size_t id,std::size_t count,auto &&fn) {
                const std::size_t begin=count*lane/4,end=count*(lane+1)/4;
                require(begin<end,"NTT4 empty real-work lane");
                const U start=thread_ns();
                for (std::size_t i=begin;i<end;++i) fn(i);
                Lane record; record.jobs=end-begin; record.cpu_ns=thread_ns()-start;
                record.actual_cpu=sched_getcpu();
                require(record.cpu_ns>0 && record.actual_cpu==cpus[lane],"NTT4 lane CPU evidence");
                result.stages[id][lane]=record;
                // This barrier is necessary: the next stage reads prior-stage output.
                barrier.wait();
            };
            stage(0,n,[&](std::size_t i) {
                require(values[i]<(inverse?2*p:4*p),"NTT4 input range");
            });
            if (public_test_failure_lane==int(lane)) throw std::runtime_error("NTT4 public injected lane failure");
            std::size_t stage_id=1;
            if (!inverse) {
                for (std::size_t m=1,gap=n/2;m<n;m*=2,gap/=2,++stage_id) {
                    stage(stage_id,n/2,[&](std::size_t k) {
                        const std::size_t group=k/gap,j=k%gap,index=2*group*gap+j;
                        const U u=arith.guard(values[index]);
                        const U v=arith.mul_root(values[index+gap],roots[m+group]);
                        values[index]=arith.add(u,v);
                        values[index+gap]=arith.sub(u,v);
                    });
                }
            } else {
                std::size_t root_offset=1;
                const Operand inverse_degree=tables.inv_degree_modulo();
                const Operand final_root=arith.mul_root_scalar(roots[n-1],inverse_degree);
                for (std::size_t m=n/2,gap=1;m;m/=2,gap*=2,++stage_id) {
                    stage(stage_id,n/2,[&](std::size_t k) {
                        const std::size_t group=k/gap,j=k%gap,index=2*group*gap+j;
                        const U u=m==1?arith.guard(values[index]):values[index];
                        const U v=values[index+gap];
                        if (m==1) {
                            values[index]=arith.mul_scalar(arith.guard(arith.add(u,v)),inverse_degree);
                            values[index+gap]=arith.mul_root(arith.sub(u,v),final_root);
                        } else {
                            values[index]=arith.guard(arith.add(u,v));
                            values[index+gap]=arith.mul_root(arith.sub(u,v),roots[root_offset+group]);
                        }
                    });
                    root_offset+=m;
                }
            }
            if (canonical) stage(stage_id,n,[&](std::size_t i) {
                U v=values[i];
                if (!inverse && v>=2*p) v-=2*p;
                if (v>=p) v-=p;
                values[i]=v;
            });
        } catch (...) {
            { std::lock_guard<std::mutex> lock(error_lock); if (!first_error) first_error=std::current_exception(); }
            barrier.abort();
        }
    };
    try { for (unsigned lane=0;lane<4;++lane) threads.emplace_back(worker,lane); }
    catch (...) {
        { std::lock_guard<std::mutex> lock(error_lock); if (!first_error) first_error=std::current_exception(); }
        barrier.abort();
    }
    for (auto &thread:threads) thread.join();
    if (first_error) std::rethrow_exception(first_error);
    return result;
}
} // namespace pcn_ntt4
