#pragma once
#include "resnet50/engine/directed_phase_view.hpp"
#include "resnet50/engine/prefix_lut_adapter.hpp"
namespace direction359 {
inline aqcf2::Ring consume(aqcf2::Sci&s,const std::vector<aqcf2::U>&v,int lane,const std::vector<aqcf2::U>*add=nullptr,bool bn=false){
 aqcf2::require(lane>=0&&lane<4,"359 public owner lane");const bool rev=enabled()&&(lane==1||lane==3);
 View view(s,rev);++census.calls;census.rows+=v.size();if(rev){++census.reversed_calls;census.reversed_rows+=v.size();}
 // BN retains its original joint304 path even on small tensors.
 return bn?aqcf_joint304::consume(view,v,add):candidate336::consume(view,v,add);
}
}
