#pragma once
// Mature Winograd inner-product identity, NOT a new cryptographic primitive.
// Both implementations consume the same already prepared matrix and input bank.
// Arithmetic is on public-modulus ciphertext coefficients on the SERVER only.
#include "types.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace aqcf3 {
struct BilinearCounts {
    std::uint64_t ordinary_products=0, core_products=0, row_products=0, column_products=0;
    std::uint64_t factor_additions=0, output_reductions=0;
    std::uint64_t total_products() const { return core_products+row_products+column_products; }
};
inline void matrix_shape(const std::vector<std::uint64_t>& a,
                         const std::vector<std::uint64_t>& b,
                         std::size_t rows,std::size_t inner,std::size_t columns,
                         std::uint64_t q) {
    aqcf2::require(rows && inner && columns && rows<=512 && inner<=4096 && columns<=16384,
                   "bounded public matrix dimensions");
    aqcf2::require(q>2 && q<(std::uint64_t(1)<<44),"original <=44-bit RNS modulus");
    aqcf2::require(a.size()==rows*inner && b.size()==inner*columns,"dense action matrix shape");
    for(auto v:a)aqcf2::require(v<q,"canonical private weight spectrum");
    for(auto v:b)aqcf2::require(v<q,"canonical encrypted input spectrum");
}
inline std::vector<std::uint64_t> matrix_direct(
        const std::vector<std::uint64_t>& a,const std::vector<std::uint64_t>& b,
        std::size_t rows,std::size_t inner,std::size_t columns,std::uint64_t q,
        BilinearCounts& cost) {
    matrix_shape(a,b,rows,inner,columns,q);
    std::vector<std::uint64_t> out(rows*columns);
    std::vector<__uint128_t> sums(columns);
    for(std::size_t i=0;i<rows;++i){
        std::fill(sums.begin(),sums.end(),__uint128_t(0));
        for(std::size_t k=0;k<inner;++k){const auto w=a[i*inner+k];const auto* x=b.data()+k*columns;
            for(std::size_t j=0;j<columns;++j)sums[j]+=__uint128_t(w)*x[j];}
        for(std::size_t j=0;j<columns;++j)out[i*columns+j]=std::uint64_t(sums[j]%q);
    }
    cost.ordinary_products+=std::uint64_t(rows)*inner*columns;
    cost.core_products+=std::uint64_t(rows)*inner*columns;
    cost.output_reductions+=std::uint64_t(rows)*columns;
    return out;
}
inline std::vector<std::uint64_t> matrix_winograd(
        const std::vector<std::uint64_t>& a,const std::vector<std::uint64_t>& b,
        std::size_t rows,std::size_t inner,std::size_t columns,std::uint64_t q,
        BilinearCounts& cost) {
    matrix_shape(a,b,rows,inner,columns,q);
    const std::size_t pairs=inner/2;
    std::vector<std::uint64_t> column(columns),row(rows),out(rows*columns);
    std::vector<__uint128_t> sums(columns,0);
    // BOTH corrections are required. They are prepared and charged in this call.
    for(std::size_t k=0;k<pairs;++k){const auto* u=b.data()+(2*k)*columns;const auto* v=u+columns;
        for(std::size_t j=0;j<columns;++j)sums[j]+=__uint128_t(u[j])*v[j];}
    for(std::size_t j=0;j<columns;++j)column[j]=std::uint64_t(sums[j]%q);
    for(std::size_t i=0;i<rows;++i){__uint128_t acc=0;
        for(std::size_t k=0;k<pairs;++k)acc+=__uint128_t(a[i*inner+2*k])*a[i*inner+2*k+1];
        row[i]=std::uint64_t(acc%q);
    }
    for(std::size_t i=0;i<rows;++i){
        std::fill(sums.begin(),sums.end(),__uint128_t(0));
        for(std::size_t k=0;k<pairs;++k){const auto w0=a[i*inner+2*k],w1=a[i*inner+2*k+1];
            const auto* u=b.data()+(2*k)*columns;const auto* v=u+columns;
            for(std::size_t j=0;j<columns;++j)sums[j]+=__uint128_t(w0+v[j])*(w1+u[j]);
        }
        if(inner&1){const auto w=a[i*inner+inner-1];const auto* x=b.data()+(inner-1)*columns;
            for(std::size_t j=0;j<columns;++j)sums[j]+=__uint128_t(w)*x[j];}
        for(std::size_t j=0;j<columns;++j){auto v=std::uint64_t(sums[j]%q);
            v=v>=row[i]?v-row[i]:v+q-row[i];
            out[i*columns+j]=v>=column[j]?v-column[j]:v+q-column[j];}
    }
    // inner<=4096, factors<2^45: every unsigned accumulator is <2^102.
    cost.ordinary_products+=std::uint64_t(rows)*inner*columns;
    cost.core_products+=std::uint64_t(rows)*(pairs+(inner&1))*columns;
    cost.row_products+=std::uint64_t(rows)*pairs;
    cost.column_products+=std::uint64_t(pairs)*columns;
    cost.factor_additions+=2ULL*rows*pairs*columns;
    cost.output_reductions+=std::uint64_t(rows)*columns+rows+columns;
    return out;
}
} // namespace aqcf3

namespace aqcf3 {
// Mature Strassen bilinear decomposition. Keep its name and credit; no
// ciphertext-ciphertext multiplication and no new ciphertext/noise level.
inline std::vector<std::uint64_t> matrix_strassen(
    const std::vector<std::uint64_t>& a,const std::vector<std::uint64_t>& b,
    std::size_t rows,std::size_t inner,std::size_t columns,std::uint64_t q,
    BilinearCounts& cost,unsigned depth=2) {
    matrix_shape(a,b,rows,inner,columns,q);
    if(!depth||rows<4||inner<4||columns<8)return matrix_direct(a,b,rows,inner,columns,q,cost);
    const auto m=(rows+1)/2,n=(inner+1)/2,c=(columns+1)/2;
    auto block=[](const std::vector<std::uint64_t>& x,std::size_t nr,std::size_t nc,
                  std::size_t r0,std::size_t c0,std::size_t rr,std::size_t cc){
        std::vector<std::uint64_t> z(rr*cc,0);
        for(std::size_t i=0;i<rr&&r0+i<nr;++i)for(std::size_t j=0;j<cc&&c0+j<nc;++j)z[i*cc+j]=x[(r0+i)*nc+c0+j];
        return z;};
    auto mix=[q](const auto& x,const auto& y,bool sub){
        aqcf2::require(x.size()==y.size(),"Strassen linear shape");std::vector<std::uint64_t> z(x.size());
        for(std::size_t i=0;i<x.size();++i){auto v=sub?x[i]+q-y[i]:x[i]+y[i];if(v>=q)v-=q;z[i]=v;}return z;};
    auto a11=block(a,rows,inner,0,0,m,n),a12=block(a,rows,inner,0,n,m,n),
         a21=block(a,rows,inner,m,0,m,n),a22=block(a,rows,inner,m,n,m,n);
    auto b11=block(b,inner,columns,0,0,n,c),b12=block(b,inner,columns,0,c,n,c),
         b21=block(b,inner,columns,n,0,n,c),b22=block(b,inner,columns,n,c,n,c);
    auto mul=[&](const auto& x,const auto& y){return matrix_strassen(x,y,m,n,c,q,cost,depth-1);};
    auto p1=mul(mix(a11,a22,false),mix(b11,b22,false));
    auto p2=mul(mix(a21,a22,false),b11);
    auto p3=mul(a11,mix(b12,b22,true));
    auto p4=mul(a22,mix(b21,b11,true));
    auto p5=mul(mix(a11,a12,false),b22);
    auto p6=mul(mix(a21,a11,true),mix(b11,b12,false));
    auto p7=mul(mix(a12,a22,true),mix(b21,b22,false));
    std::vector<std::uint64_t> out(rows*columns);
    auto plus=[q](std::uint64_t x,std::uint64_t y){auto v=x+y;return v>=q?v-q:v;};
    auto minus=[q](std::uint64_t x,std::uint64_t y){return x>=y?x-y:x+q-y;};
    for(std::size_t i=0;i<m;++i)for(std::size_t j=0;j<c;++j){auto k=i*c+j;
        if(i<rows&&j<columns)out[i*columns+j]=plus(minus(plus(p1[k],p4[k]),p5[k]),p7[k]);
        if(i<rows&&j+c<columns)out[i*columns+j+c]=plus(p3[k],p5[k]);
        if(i+m<rows&&j<columns)out[(i+m)*columns+j]=plus(p2[k],p4[k]);
        if(i+m<rows&&j+c<columns)out[(i+m)*columns+j+c]=plus(plus(minus(p1[k],p2[k]),p3[k]),p6[k]);}
    cost.factor_additions+=5*m*n+5*n*c+8*m*c;
    return out;
}
} // namespace aqcf3
