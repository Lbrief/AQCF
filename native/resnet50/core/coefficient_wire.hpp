#pragma once
// COMMON, mature lossless coefficient bit-packing / c0 query omission.
// Not an originality claim. The complete output is masked, freshly randomized
// and flooded BEFORE this deterministic projection is taken.
#include "resnet50/core/program.hpp"
namespace aqcf3 {
inline unsigned coefficient_bits(U q){unsigned b=0;for(U v=q-1;v;v>>=1)++b;return b;}
class CoefficientWriter {
    std::vector<unsigned char> bytes_;
    __uint128_t buffer_=0; unsigned bits_=0;
public:
    explicit CoefficientWriter(std::size_t capacity=0){bytes_.reserve(capacity);}
    void append(U value,unsigned width) {
        require(width>0&&width<64&&(value>>width)==0,"canonical packed coefficient");
        buffer_|=__uint128_t(value)<<bits_;bits_+=width;
        while(bits_>=8){bytes_.push_back(unsigned(buffer_&255));buffer_>>=8;bits_-=8;}
    }
    std::vector<unsigned char> finish() {
        if(bits_) bytes_.push_back(unsigned(buffer_&255));
        bits_=0;buffer_=0;return std::move(bytes_);
    }
};
class CoefficientReader {
    const std::vector<unsigned char>& bytes_;std::size_t at_=0;
    __uint128_t buffer_=0;unsigned bits_=0;
public:
    explicit CoefficientReader(const std::vector<unsigned char>& b):bytes_(b){}
    U take(unsigned width,U modulus) {
        require(width>0&&width<64,"packed coefficient width");
        while(bits_<width){require(at_<bytes_.size(),"truncated coefficient frame");buffer_|=__uint128_t(bytes_[at_++])<<bits_;bits_+=8;}
        U v=U(buffer_&((__uint128_t(1)<<width)-1));buffer_>>=width;bits_-=width;
        require(v<modulus,"noncanonical packed RNS coefficient");return v;
    }
    void end()const{require(at_==bytes_.size()&&buffer_==0,"coefficient frame tail");}
};
} // namespace aqcf3
