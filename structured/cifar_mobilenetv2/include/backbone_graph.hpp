#pragma once
// Model-neutral AQCF execution schedule. Each linear calls the AQCF backend;
// this header never delegates to an external opponent's HE implementation.
#include "types.hpp"
#include <map>
#include <array>
#include <vector>
namespace aqcf_backbone {
enum class Op { Affine, Clip6, Add, Pool };
struct Node {
    int id; Op op; std::array<int,2> parents; unsigned parent_count;
    std::size_t elements; int affine; unsigned affine_input_scale;
    unsigned post_shift; unsigned output_scale; unsigned pool_spatial;
};
struct Coverage {std::size_t nodes=0,affines=0,clips=0,adds=0,pools=0;
    std::size_t floor_elements=0,clip_elements=0;};

template<class B,std::size_t Count>
auto execute(B& b,const std::array<Node,Count>& graph,
             const std::vector<typename B::Value>& input,int final_id) {
    using V=typename B::Value;
    struct Tensor {std::vector<V> value;unsigned scale=12;unsigned pending_shift=0;};
    aqcf2::require(input.size()==3072,"complete NCHW input");
    std::map<int,Tensor> live;std::map<int,std::size_t> uses;std::map<int,Op> consumer;
    live.emplace(0,Tensor{input,12,0});
    for(const auto& n:graph)for(unsigned p=0;p<n.parent_count;++p) {++uses[n.parents[p]];consumer[n.parents[p]]=n.op;}
    Coverage coverage;
    for(const auto& n:graph) {
        auto marker=b.begin_node(n);Tensor out;
        auto fetch=[&](unsigned p)->const Tensor& {
            aqcf2::require(p<n.parent_count && n.parents[p]<n.id,"topological graph");
            auto it=live.find(n.parents[p]);aqcf2::require(it!=live.end(),"missing live tensor");return it->second;
        };
        const auto& first=fetch(0);
        auto scaled=[&](const Tensor& t,unsigned target) {
            aqcf2::require(t.pending_shift==0,"deferred floor only reaches its unique clip");
            if(t.scale==target)return t.value;
            if(t.scale>target) {coverage.floor_elements+=t.value.size();return b.floor(t.value,t.scale-target);}
            auto y=t.value;for(auto& v:y)v=b.lift_scale(v,target-t.scale);return y;
        };
        switch(n.op) {
          case Op::Affine: {
            auto x=scaled(first,n.affine_input_scale);
            out.value=b.affine(n.affine,x);out.scale=n.affine_input_scale+12;
            if(n.post_shift) {
                coverage.floor_elements+=out.value.size();
                if(uses[n.id]==1 && consumer[n.id]==Op::Clip6) out.pending_shift=n.post_shift;
                else {out.value=b.floor(out.value,n.post_shift);out.scale-=n.post_shift;}
            }
            ++coverage.affines;break;
          }
          case Op::Clip6:
            aqcf2::require(first.scale-first.pending_shift==12,"clip must receive Q12, no omitted floor");
            out.value=first.pending_shift?b.floor_clip6(first.value,first.pending_shift):b.clip6(first.value);out.scale=12;
            ++coverage.clips;coverage.clip_elements+=out.value.size();break;
          case Op::Add: {
            auto a=scaled(first,n.output_scale),other=scaled(fetch(1),n.output_scale);
            aqcf2::require(a.size()==other.size(),"full residual shape");
            for(std::size_t j=0;j<a.size();++j)a[j]=b.add(a[j],other[j]);
            out.value=std::move(a);out.scale=n.output_scale;++coverage.adds;break;
          }
          case Op::Pool:
            aqcf2::require(first.scale==12 && n.pool_spatial>0 && first.value.size()==n.elements*n.pool_spatial,"full channel-major pool");
            out.value.assign(n.elements,b.zero());
            for(std::size_t c=0;c<n.elements;++c)for(unsigned j=0;j<n.pool_spatial;++j)
                out.value[c]=b.add(out.value[c],first.value[c*n.pool_spatial+j]);
            if(n.post_shift) {out.value=b.floor(out.value,n.post_shift);coverage.floor_elements+=out.value.size();}
            out.scale=12;++coverage.pools;break;
        }
        aqcf2::require(out.value.size()==n.elements && out.scale-out.pending_shift==n.output_scale,"node output/scale mismatch");
        ++coverage.nodes;b.end_node(n,marker);
        aqcf2::require(live.emplace(n.id,std::move(out)).second,"unique produced node");
        for(unsigned p=0;p<n.parent_count;++p)if(--uses[n.parents[p]]==0)live.erase(n.parents[p]);
    }
    aqcf2::require(coverage.nodes==Count && live.count(final_id) && live.at(final_id).scale==12 && live.at(final_id).pending_shift==0 && live.at(final_id).value.size()==100,"complete final scores");
    b.accept_coverage(coverage);return live.at(final_id).value;
}
} // namespace aqcf_backbone
