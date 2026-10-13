#pragma once
#include "resnet50/engine/joint_quotient_adapter.hpp"
#include "resnet50/engine/prefix_tree.hpp"
#include <cstdlib>
#include <cstring>
extern "C" int aqcf_common336_tree_begin(aqcf_silent_handle*,int,int);
extern "C" int aqcf_leaf335_stats(const aqcf_silent_handle*,uint64_t*,uint64_t*,uint64_t*,uint64_t*);
namespace candidate336 {
using namespace aqcf2;
inline bool enabled(){const char* p=std::getenv("AQCF_DUPLEX336");require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"336 explicit source-bound arm");return p[0]=='1';}
struct Phase {
 aqcf_silent_handle* h;int exceptions;
 Phase(aqcf_silent_handle* p,int n,bool normal,int logical):h(p),exceptions(std::uncaught_exceptions()){require(direction359::aqcf_common359_phase_begin(h,n,normal,logical,1)==0,aqcf_silent_error(h));}
 ~Phase() noexcept(false){bool abort=std::uncaught_exceptions()>exceptions;int rc=aqcf_common315_phase_end(h,abort);if(rc&&!abort)throw std::runtime_error(aqcf_silent_error(h));}
 Phase(const Phase&)=delete;Phase&operator=(const Phase&)=delete;
};
template<class B>Ring consume(B& s,const std::vector<U>& phase,const std::vector<U>* add=nullptr){
 if(!enabled()||phase.size()>25088)return aqcf_joint304::consume(s,phase,add);
 const size_t n=phase.size();Phase paid(s.control().owned189_handle(),int(n),!add,s.party);
 auto p=aqcf_tree335::prepare(s,phase,add);Ring coarse(25,n);for(size_t i=0;i<n;++i)coarse.v[i]=p.own37[i]>>12;
 return add?floor216::emit(s,coarse,p.low,p.high):aqcf_terminal249::emit(s,coarse,p.low,p.high);
}
}
