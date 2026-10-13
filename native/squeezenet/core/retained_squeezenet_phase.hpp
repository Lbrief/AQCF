#pragma once
#include "squeezenet/protocol/native_ot_bridge.h"
#include <exception>
extern "C" int aqcf_common317_phase_begin(aqcf_silent_handle*,int,int);
extern "C" int aqcf_common317_phase_end(aqcf_silent_handle*,int);
namespace phase317 {
struct Phase {
 aqcf_silent_handle* h;int exceptions;
 Phase(aqcf_silent_handle* p,int n,int kind):h(p),exceptions(std::uncaught_exceptions()){aqcf2::require(aqcf_common317_phase_begin(h,n,kind)==0,aqcf_silent_error(h));}
 ~Phase() noexcept(false){bool abort=std::uncaught_exceptions()>exceptions;int rc=aqcf_common317_phase_end(h,abort);if(rc&&!abort)throw std::runtime_error(aqcf_silent_error(h));}
 Phase(const Phase&)=delete;Phase&operator=(const Phase&)=delete;
};
}
