#pragma once
#include <atomic>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <pthread.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sched.h>
#include <omp.h>
namespace trace267 {
using Fn=void(*)(void*);std::vector<Fn> allowed;std::atomic<unsigned long> seq{0};
long tid(){return syscall(SYS_gettid);}unsigned long long cpu(){timespec t{};clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t);return 1000000000ULL*t.tv_sec+t.tv_nsec;}
unsigned long long birth(){std::ifstream f("/proc/self/task/"+std::to_string(tid())+"/stat");std::string x;std::getline(f,x);auto p=x.rfind(')');if(p==std::string::npos)_Exit(92);std::istringstream s(x.substr(p+2));std::string v;for(int field=3;field<=22;++field){if(!(s>>v))_Exit(92);if(field==22)return std::stoull(v);}return 0;}
void event(const char* what,long parent=-1,void* fn=nullptr,const char* creator="",unsigned long long jobs=0){char out[1600];int n=snprintf(out,sizeof(out),"{\"seq\":%lu,\"event\":\"%s\",\"pid\":%ld,\"tid\":%ld,\"birth\":%llu,\"parent\":%ld,\"cpu_ns\":%llu,\"cpu\":%d,\"callback\":\"%p\",\"creator\":\"%s\",\"jobs\":%llu}\n",seq.fetch_add(1),what,long(getpid()),tid(),birth(),parent,cpu(),sched_getcpu(),fn,creator,jobs);if(n<=0||n>=int(sizeof(out))||write(2,out,n)!=n)_Exit(93);}
struct Start{void*(*fn)(void*);void* p;long parent;std::string creator;};
void ending(void* p){auto s=static_cast<Start*>(p);event("THREAD_EXIT",s->parent,reinterpret_cast<void*>(s->fn));delete s;}
void* enter(void* p){auto s=static_cast<Start*>(p);event("THREAD_ENTER",s->parent,reinterpret_cast<void*>(s->fn),s->creator.c_str());void* out;pthread_cleanup_push(ending,p);out=s->fn(s->p);pthread_cleanup_pop(1);return out;}
struct Scope{Fn fn;void* p;long parent;};
void invoke(void* p){auto s=static_cast<Scope*>(p);event("IO_ENTER",s->parent,reinterpret_cast<void*>(s->fn));s->fn(s->p);event("IO_EXIT",s->parent,reinterpret_cast<void*>(s->fn));}
struct Finalizer{~Finalizer(){omp_pause_resource_all(omp_pause_hard);event("PROCESS_EXIT");}};
}
extern "C" int pthread_create(pthread_t* t,const pthread_attr_t* a,void*(*f)(void*),void* p){using Real=int(*)(pthread_t*,const pthread_attr_t*,void*(*)(void*),void*);static auto real=reinterpret_cast<Real>(dlsym(RTLD_NEXT,"pthread_create"));if(!real)_Exit(94);void* stack[16];int count=backtrace(stack,16);std::ostringstream ss;for(int i=0;i<count;++i){if(i)ss<<' ';ss<<stack[i];}auto c=new trace267::Start{f,p,trace267::tid(),ss.str()};int rc=real(t,a,trace267::enter,c);if(rc)delete c;return rc;}
extern "C" void GOMP_parallel(void(*fn)(void*),void* p,unsigned n,unsigned flags){using Real=void(*)(void(*)(void*),void*,unsigned,unsigned);static auto real=reinterpret_cast<Real>(dlsym(RTLD_NEXT,"GOMP_parallel"));if(!real)_Exit(95);if(std::find(trace267::allowed.begin(),trace267::allowed.end(),fn)==trace267::allowed.end()){trace267::event("DENIED_CALLBACK",-1,reinterpret_cast<void*>(fn));_Exit(91);}trace267::Scope s{fn,p,trace267::tid()};trace267::event("IO_DISPATCH",s.parent,reinterpret_cast<void*>(fn));real(trace267::invoke,&s,n,flags);trace267::event("IO_JOIN",s.parent,reinterpret_cast<void*>(fn));}
