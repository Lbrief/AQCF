#pragma once
#include <exception>
#include <stdexcept>
#include <cstdint>
extern "C" int aqcf_common315_scope(int,int);
extern "C" int aqcf_common315_end(int);
extern "C" int aqcf_common315_reset(int);
extern "C" int aqcf_common315_read(int,uint64_t*);
namespace scope315 {
struct Scope {
 int exceptions;
 Scope(int lane,bool eligible):exceptions(std::uncaught_exceptions()) {if(aqcf_common315_scope(lane,eligible)!=0)throw std::runtime_error("315 begin owned scope");}
 ~Scope() noexcept(false){bool abort=std::uncaught_exceptions()>exceptions;int rc=aqcf_common315_end(abort);if(rc&&!abort)throw std::runtime_error("315 end owned scope");}
 Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
};
}
