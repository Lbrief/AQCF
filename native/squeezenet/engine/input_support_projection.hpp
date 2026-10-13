#pragma once
#include "squeezenet/engine/squeezenet_input_rounding.hpp"
#include "squeezenet/engine/kernel_action.hpp"
#include <array>
#include <atomic>
namespace aqcf_support402 {
using aqcf3::U;using aqcf3::require;using Plan=std::vector<uint16_t>;
inline bool enabled(){const char*p=std::getenv("AQCF_SUPPORT402");require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"402 explicit public support arm");return p[0]=='1';}
inline bool eligible(U id){switch(id){case 1:case 4:case 7:case 10:case 13:case 16:case 19:case 22:case 25:return true;default:return false;}}
inline std::vector<Plan> plans(const aqcf3::Layout& l){
 std::vector<Plan> out(l.maps.size());if(!eligible(l.id))return out;
 const auto g=aqcf3::kernel_geometry(l);require(g.active&&g.b>=64&&g.b<=4096,"402 public unpaired pointwise class");
 for(size_t i=0;i<l.maps.size();++i){std::vector<bool> residue(g.b,false);for(const auto&t:l.maps[i])residue[t.exponent%g.b]=true;
  require(std::any_of(residue.begin(),residue.end(),[](bool x){return x;}),"402 nonempty public support");
  if(std::all_of(residue.begin(),residue.end(),[](bool x){return x;}))continue;
  for(U j=0;j<8192;++j)if(residue[j%g.b])out[i].push_back(uint16_t(j));
  require(out[i].size()%8==0&&out[i].size()<8192,"402 byte-aligned proper public support");
 }return out;
}
// All candidate private-edge checks happen before any peer connection. Neither
// a weight value nor a private sparsity pattern changes a transmitted frame.
inline void certify(const aqcf3::Program& program){
 for(const auto& z:program){const auto& l=z.layout;if(!eligible(l.id))continue;
  const auto g=aqcf3::kernel_geometry(l);const auto p=plans(l);
  for(const auto& o:z.output)for(const auto&e:o.edges){
   for(const auto&a:e.coefficients)require(a.exponent%g.b==0,"402 private kernel outside public stride class");
   if(p[e.input].empty())continue;
   for(const auto&q:o.queries)require(std::binary_search(p[e.input].begin(),p[e.input].end(),uint16_t(q.exponent)),"402 queried coefficient outside public input fiber support");
  }
 }
}
inline size_t frame_size(const Plan& p){return p.empty()?aqcf_input379::frame_size():8+aqcf_input376::prefix+71*p.size()/8+aqcf_input376::suffix;}
inline std::string pack(const std::string& raw,const Plan& p){
 if(p.empty())return aqcf_input379::pack(raw);
 using namespace aqcf_input376;bound();aqcf_wire288::validate(raw);std::string out(frame_size(p),'\0');std::memcpy(out.data(),"AQSP402A",8);
 std::memcpy(out.data()+8,raw.data(),prefix);std::memcpy(out.data()+out.size()-suffix,raw.data()+raw_size-suffix,suffix);
 W pending=0;unsigned live=0;size_t at=8+prefix;
 for(U j:p){const W z=crt(aqcf_wire288::word(raw.data()+prefix+8*j),aqcf_wire288::word(raw.data()+prefix+8*(N+j)));
  const W word=(z+(W(1)<<(shift-1)))>>shift;need(word<(W(1)<<bits),"402 fixed rounded word");pending|=word<<live;live+=bits;
  while(live>=8){out[at++]=char(U(pending)&255);pending>>=8;live-=8;}
 }
 need(!live&&!pending&&at==out.size()-suffix,"402 exact public frame");return out;
}
inline std::string unpack(const std::string& wire,const Plan& p){
 if(p.empty())return aqcf_input379::unpack(wire);
 using namespace aqcf_input376;bound();need(wire.size()==frame_size(p)&&wire.compare(0,8,"AQSP402A")==0,"402 public frame identity");
 std::string out(raw_size,'\0');std::memcpy(out.data(),wire.data()+8,prefix);std::memcpy(out.data()+raw_size-suffix,wire.data()+wire.size()-suffix,suffix);
 const W maxword=(Q-1+(W(1)<<(shift-1)))>>shift;W pending=0;unsigned live=0;size_t at=8+prefix;
 for(U j:p){while(live<bits){pending|=W(static_cast<unsigned char>(wire[at++]))<<live;live+=8;}
  const W word=pending&((W(1)<<bits)-1);pending>>=bits;live-=bits;need(word<=maxword,"402 canonical rounded word");const W z=word<<shift;
  aqcf_wire288::putword(out.data()+prefix+8*j,U(z%q0));aqcf_wire288::putword(out.data()+prefix+8*(N+j),U(z%q1));
 }
 need(!live&&!pending&&at==wire.size()-suffix,"402 exact decode cursor");aqcf_wire288::validate(out);return out;
}
}
