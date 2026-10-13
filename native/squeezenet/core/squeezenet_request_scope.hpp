#pragma once
#include <exception>
#include <stdexcept>
#include <cstdint>
extern "C" int aqcf_common317_scope(int,int);
extern "C" int aqcf_common317_end(int);
extern "C" int aqcf_common317_reset();
extern "C" int aqcf_common317_read(int,uint64_t*);
namespace scope317 {
struct Scope {
 int exceptions;
 Scope(int lane,bool eligible):exceptions(std::uncaught_exceptions()) {if(aqcf_common317_scope(lane,eligible)!=0)throw std::runtime_error("317 begin owned scope");}
 ~Scope() noexcept(false){bool abort=std::uncaught_exceptions()>exceptions;int rc=aqcf_common317_end(abort);if(rc&&!abort)throw std::runtime_error("317 end owned scope");}
 Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
};
}
