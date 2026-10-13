#pragma once
#define _GNU_SOURCE
#include "resnet50/engine/persistent_worker_owner.hpp"
#include "resnet50/vendor/bolt_sci/BuildingBlocks/truncation.h"
#include <atomic>
#include <algorithm>
#include <vector>
#include <dlfcn.h>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/syscall.h>
#include <arpa/inet.h>
#include <omp.h>

namespace trace037 {
using Fn=void(*)(void*);
std::vector<Fn> allowed;
std::atomic<unsigned long> seq{0};
long tid(){return syscall(SYS_gettid);}
unsigned long long cpu(){timespec t{};clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t);return 1000000000ULL*t.tv_sec+t.tv_nsec;}
unsigned long long birth(){
 std::ifstream f("/proc/self/task/"+std::to_string(tid())+"/stat");std::string x;std::getline(f,x);
 auto p=x.rfind(')');if(p==std::string::npos)_Exit(92);std::istringstream s(x.substr(p+2));std::string v;
 for(int field=3;field<=22;++field){if(!(s>>v))_Exit(92);if(field==22)return std::stoull(v);}return 0;
}
void event(const char* what,long parent=-1,int lane=-1,Fn fn=nullptr,unsigned long long jobs=0){
 char name[16]{};pthread_getname_np(pthread_self(),name,sizeof(name));char out[512];
 int n=snprintf(out,sizeof(out),"{\"seq\":%lu,\"event\":\"%s\",\"pid\":%ld,\"tid\":%ld,\"birth\":%llu,\"parent\":%ld,\"lane\":%d,\"cpu_ns\":%llu,\"cpu\":%d,\"name\":\"%s\",\"callback\":\"%p\",\"jobs\":%llu}\n",seq.fetch_add(1),what,long(getpid()),tid(),birth(),parent,lane,cpu(),sched_getcpu(),name,reinterpret_cast<void*>(fn),jobs);
 if(n<=0||n>=int(sizeof(out))||write(STDERR_FILENO,out,n)!=n)_Exit(93);
}
struct Start{void*(*fn)(void*);void* data;long parent;};
void ending(void* p){auto s=static_cast<Start*>(p);event("THREAD_EXIT",s->parent);delete s;}
void* enter(void* p){auto s=static_cast<Start*>(p);event("THREAD_ENTER",s->parent);void* result;
 pthread_cleanup_push(ending,p);result=s->fn(s->data);pthread_cleanup_pop(1);return result;
}
struct Scope{Fn fn;void* data;long owner;int lane;};
void invoke(void* p){auto s=static_cast<Scope*>(p);event("IO_ENTER",s->owner,s->lane,s->fn);s->fn(s->data);event("IO_EXIT",s->owner,s->lane,s->fn);}
}
extern "C" int pthread_create(pthread_t* t,const pthread_attr_t* a,void*(*f)(void*),void* p){
 using Real=int(*)(pthread_t*,const pthread_attr_t*,void*(*)(void*),void*);
 static auto real=reinterpret_cast<Real>(dlsym(RTLD_NEXT,"pthread_create"));if(!real)_Exit(94);
 auto context=new trace037::Start{f,p,trace037::tid()};int rc=real(t,a,trace037::enter,context);if(rc)delete context;return rc;
}
extern "C" void GOMP_parallel(void(*fn)(void*),void* data,unsigned count,unsigned flags){
 using Real=void(*)(void(*)(void*),void*,unsigned,unsigned);
 static auto real=reinterpret_cast<Real>(dlsym(RTLD_NEXT,"GOMP_parallel"));if(!real)_Exit(95);
 if(std::find(trace037::allowed.begin(),trace037::allowed.end(),fn)==trace037::allowed.end()||aqcf_owner4::current_lane<0){trace037::event("DENIED_CALLBACK",-1,-1,fn);_Exit(91);}
 trace037::Scope scope{fn,data,trace037::tid(),aqcf_owner4::current_lane};
 trace037::event("IO_DISPATCH",scope.owner,scope.lane,fn);real(trace037::invoke,&scope,count,flags);trace037::event("IO_JOIN",scope.owner,scope.lane,fn);
}
