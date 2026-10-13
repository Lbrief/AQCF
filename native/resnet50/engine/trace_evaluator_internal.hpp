#pragma once
#include <stdexcept>
namespace aqcf_internal514 {
inline thread_local bool active=false;
struct Scope {
 Scope(){if(active)throw std::logic_error("514 nested internal scope");active=true;}
 ~Scope(){active=false;}
 Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
};
inline void require_closed(){if(active)throw std::logic_error("514 internal scope cannot export");}
}
