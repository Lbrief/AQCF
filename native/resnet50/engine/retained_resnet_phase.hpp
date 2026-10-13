#pragma once
#include "resnet50/protocol/native_ot_bridge.h"
#include "resnet50/engine/directed_phase.hpp"
#include <exception>
extern "C" int aqcf_common315_phase_begin(aqcf_silent_handle*,int,int);
extern "C" int aqcf_common315_phase_end(aqcf_silent_handle*,int);
namespace phase315 {
struct Phase {
 aqcf_silent_handle* h;int exceptions;
 Phase(aqcf_silent_handle* p,int n,bool normal,int logical=0):h(p),exceptions(std::uncaught_exceptions()){aqcf2::require((logical?direction359::aqcf_common359_phase_begin(h,n,normal,logical,0):aqcf_common315_phase_begin(h,n,normal))==0,aqcf_silent_error(h));}
 ~Phase() noexcept(false){bool abort=std::uncaught_exceptions()>exceptions;int rc=aqcf_common315_phase_end(h,abort);if(rc&&!abort)throw std::runtime_error(aqcf_silent_error(h));}
 Phase(const Phase&)=delete;Phase&operator=(const Phase&)=delete;
};
}
