#pragma once
// New model port only. No change to the measured MobileNet executable/header.
// Backends must provide protected affine/floor_relu/floor for private use.
#include "types.hpp"
#include <array>
#include <map>
#include <vector>
namespace aqcf_rhombus {
enum class Op { Affine, FloorRelu, Add, Pool };
struct Node {
    int id; Op op; std::array<int,2> parents; unsigned parent_count;
    std::size_t elements; int affine; unsigned output_scale; unsigned pool_spatial;
};
struct Coverage {std::size_t nodes=0,affines=0,relus=0,adds=0,pools=0;
    std::size_t floor_elements=0,relu_elements=0;};

template<class B,std::size_t Count>
auto execute(B& b,const std::array<Node,Count>& graph,
             const std::vector<typename B::Value>& input,int final_id) {
    using V=typename B::Value;
    struct Tensor {std::vector<V> value;unsigned scale=0;};
    aqcf2::require(input.size()==3072,"ResNet50 complete NCHW input");
    std::map<int,Tensor> live;std::map<int,std::size_t> uses;
    live.emplace(0,Tensor{input,12});
    for(const auto& n:graph){
        aqcf2::require(n.parent_count>=1&&n.parent_count<=2,"one or two parents");
        for(unsigned p=0;p<n.parent_count;++p)++uses[n.parents[p]];
    }
    Coverage c;
    for(const auto& n:graph) {
        auto marker=b.begin_node(n);Tensor out;
        auto fetch=[&](unsigned p)->const Tensor& {
            aqcf2::require(p<n.parent_count&&n.parents[p]<n.id,"topological parent");
            auto it=live.find(n.parents[p]);aqcf2::require(it!=live.end(),"live parent");
            return it->second;
        };
        const auto& a=fetch(0);
        switch(n.op) {
        case Op::Affine:
            aqcf2::require(n.parent_count==1&&a.scale==12&&n.output_scale==24,"affine Q12 to Q24");
            out={b.affine(n.affine,a.value,n.elements),24};++c.affines;break;
        case Op::FloorRelu:
            aqcf2::require(n.parent_count==1&&a.scale==24&&n.output_scale==12,"ReLU then floor12");
            // Exact target max(0,floor(a/4096)); no ReLU6 clipping and no
            // truncation before a residual ADD. Approximate opponent rounding
            // remains a separately recorded native numerical behavior.
            out={b.floor_relu(a.value,12),12};++c.relus;
            c.floor_elements+=out.value.size();c.relu_elements+=out.value.size();break;
        case Op::Add: {
            const auto& skip=fetch(1);
            aqcf2::require(n.parent_count==2&&a.scale==24&&n.output_scale==24,
                           "residual main stays Q24 until sum");
            aqcf2::require((skip.scale==12||skip.scale==24)&&a.value.size()==skip.value.size(),"residual shape/scale");
            auto y=a.value;
            for(std::size_t j=0;j<y.size();++j)
                y[j]=b.add(y[j],skip.scale==24?skip.value[j]:b.lift_scale(skip.value[j],12));
            out={std::move(y),24};++c.adds;break;
        }
        case Op::Pool: {
            aqcf2::require(n.parent_count==1&&a.scale==12&&n.pool_spatial==16&&
                           a.value.size()==n.elements*16,"entire 4x4 channel pool");
            std::vector<V> sums(n.elements,b.zero());
            for(std::size_t ch=0;ch<n.elements;++ch)for(unsigned j=0;j<16;++j)
                sums[ch]=b.add(sums[ch],a.value[ch*16+j]);
            out={b.floor(sums,4),12};++c.pools;c.floor_elements+=out.value.size();break;
        }
        default: aqcf2::require(false,"unknown graph operation");
        }
        aqcf2::require(out.scale==n.output_scale&&out.value.size()==n.elements,"output shape/scale");
        b.end_node(n,marker,out.value);++c.nodes;
        aqcf2::require(live.emplace(n.id,std::move(out)).second,"unique node");
        for(unsigned p=0;p<n.parent_count;++p)if(--uses[n.parents[p]]==0)live.erase(n.parents[p]);
    }
    // Native classifier returns Q24 scores. Silently floor12 at this point
    // can erase close margins and is not the target model's final function.
    aqcf2::require(c.nodes==Count&&live.count(final_id)&&live.at(final_id).scale==24&&
                   live.at(final_id).value.size()==100,"all final Q24 scores");
    b.accept_coverage(c);return live.at(final_id).value;
}
}
