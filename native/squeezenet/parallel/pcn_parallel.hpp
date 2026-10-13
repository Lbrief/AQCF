#pragma once
// Execution-only CPU parallelism; no arithmetic, model or protocol substitution.
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <sched.h>
#include <stdexcept>
#include <thread>
#include <vector>
extern "C" void pcn_ntt085_register();
extern "C" void pcn_ntt085_outer_enter();
extern "C" void pcn_ntt085_outer_leave();
namespace pcn_parallel {
struct OuterGuard{OuterGuard(){pcn_ntt085_outer_enter();}~OuterGuard(){pcn_ntt085_outer_leave();}};
inline std::mutex scheduler_log;
inline unsigned long long cpu_ns(){timespec t{};if(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t))throw std::runtime_error("batch CPU clock");return t.tv_sec*1000000000ULL+t.tv_nsec;}

inline std::atomic<unsigned long long> regions{0}, items{0}, active{0}, peak{0};
inline std::array<std::atomic<unsigned long long>,4> lane_masks{};
inline int workers() {
    static int n=[] { const char* p=std::getenv("PCN_HE_WORKERS");
        int v=p?std::atoi(p):1;
        if(v!=1 && v!=4) throw std::runtime_error("PCN_HE_WORKERS must be 1 or 4");
        return v; }();
    return n;
}
inline std::vector<int> cpus() {
    cpu_set_t a; CPU_ZERO(&a);
    if(sched_getaffinity(0,sizeof(a),&a)) throw std::runtime_error("HE affinity read");
    std::vector<int> v;
    for(int i=0;i<CPU_SETSIZE;++i) if(CPU_ISSET(i,&a)) v.push_back(i);
    if(v.size()<static_cast<unsigned>(workers())) throw std::runtime_error("HE insufficient allocated CPUs");
    return v;
}
template<class F> void each(std::size_t n,F&& f) {
    if(n==0) return;
    ++regions; items+=n;
    if(workers()==1) { for(std::size_t i=0;i<n;++i) f(i); return; }
    const auto allowed=cpus();
    if(allowed.size()!=4)throw std::runtime_error("production batch exact four CPUs");
    pcn_ntt085_register();
    if(n<4){
        // Real small-CT work is partitioned inside hooked scalar NTT, not dummy CTs.
        for(std::size_t i=0;i<n;++i)f(i);
        std::lock_guard<std::mutex> guard(scheduler_log);
        std::cerr<<"PCN_BATCH {\"mode\":\"inner\",\"items\":"<<n<<"}\n";
        return;
    }
    std::array<unsigned long long,4> jobs{},cpu_ticks{};
    std::vector<std::thread> threads;
    std::exception_ptr error;
    std::mutex error_lock;
    std::atomic<bool> cancel{false};
    // Each output element belongs to exactly one worker. No parallel reduction
    // changes the author's per-output accumulation order.
    auto body=[&](unsigned lane) {
        try {
            cpu_set_t a; CPU_ZERO(&a); CPU_SET(allowed[lane],&a);
            if(pthread_setaffinity_np(pthread_self(),sizeof(a),&a)) throw std::runtime_error("HE lane pin");
            int cpu=sched_getcpu();
            if(cpu!=allowed[lane] || cpu>=64) throw std::runtime_error("HE actual CPU mismatch");
            lane_masks[lane].fetch_or(1ULL<<cpu);
            auto current=++active, prior=peak.load();
            while(prior<current && !peak.compare_exchange_weak(prior,current)) {}
            try {
                OuterGuard scope;auto begin=cpu_ns();
                for(std::size_t i=lane;i<n && !cancel;i+=4){f(i);++jobs[lane];}
                cpu_ticks[lane]=cpu_ns()-begin;
                if(!jobs[lane]||!cpu_ticks[lane])throw std::runtime_error("inactive native outer worker");
            }
            catch(...) { --active; throw; }
            --active;
        } catch(...) {
            cancel=true;
            std::lock_guard<std::mutex> guard(error_lock);
            if(!error) error=std::current_exception();
        }
    };
    try { for(unsigned k=0;k<4;++k) threads.emplace_back(body,k); }
    catch(...) { cancel=true; for(auto& t:threads) t.join(); throw; }
    for(auto& t:threads) t.join();
    if(error) std::rethrow_exception(error);
    std::lock_guard<std::mutex> guard(scheduler_log);
    std::cerr<<"PCN_BATCH {\"mode\":\"outer\",\"items\":"<<n<<",\"lanes\":[";
    for(int k=0;k<4;++k){if(k)std::cerr<<',';std::cerr<<"{\"cpu\":"<<allowed[k]<<",\"jobs\":"<<jobs[k]<<",\"cpu_ns\":"<<cpu_ticks[k]<<'}';}
    std::cerr<<"]}\n";
}
inline void report() {
    std::cerr << "PCN_HE_PARALLEL_RUNTIME {\"configured_workers\":" << workers()
        << ",\"parallel_regions\":" << regions << ",\"task_items\":" << items
        << ",\"maximum_live_workers\":" << peak << ",\"lane_cpu_masks\":[";
    for(int i=0;i<4;++i) std::cerr << (i?",":"") << lane_masks[i];
    std::cerr << "]}\n";
}
inline double seconds(std::chrono::steady_clock::time_point t) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();
}
}
