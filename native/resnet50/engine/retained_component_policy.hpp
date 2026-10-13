#pragma once
#include "resnet50/engine/retained_resnet_component.hpp"
#include <atomic>
#include <cstdlib>
#include <cstring>
namespace retained299 {
inline bool enabled(){const char*p=std::getenv("AQCF_RETAINED299");retain298::need(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"299 explicit fixed retained arm");return p[0]=='1';}
inline std::atomic<uint64_t> spatial_returns{0},spatial_words{0},pointwise_returns{0},pointwise_words{0},coefficients{0},saved_bytes{0};
inline void record(bool pointwise,uint64_t words){
 (pointwise?pointwise_returns:spatial_returns)++;(pointwise?pointwise_words:spatial_words)+=words;coefficients+=8192;
 const uint64_t bits=pointwise?49:41; saved_bytes+=(bits*words+7)/8;
}
template<class Query>void spatial(seal::Ciphertext& ct,const seal::SEALContext& quarter,const std::vector<Query>& queries,std::vector<uint64_t>& own){
 auto phase=retain298::first(ct,quarter);for(const auto&q:queries){retain298::need(q.exponent<8192&&q.destination<own.size(),"299 unchanged spatial scatter");own[q.destination]=phase[q.exponent];}
 OPENSSL_cleanse(phase.data(),phase.size()*sizeof(uint64_t));record(false,queries.size());
}
inline std::array<uint64_t,6> census(){return {spatial_returns.load(),spatial_words.load(),pointwise_returns.load(),pointwise_words.load(),coefficients.load(),saved_bytes.load()};}
}
