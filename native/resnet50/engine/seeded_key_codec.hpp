#pragma once
#include <seal/seal.h>
#include <seal/serialization.h>
#include <array>
#include <sstream>
#include <vector>
#include <stdexcept>
namespace key343 {
using Bytes=std::vector<unsigned char>;
inline void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
template<class T>Bytes save_seeded(const T& x){
 need(seal::Serialization::IsSupportedComprMode(seal::compr_mode_type::zstd),"343 bundled ZSTD support");
 std::ostringstream o(std::ios::binary);x.save(o,seal::compr_mode_type::zstd);auto s=o.str();return Bytes(s.begin(),s.end());
}
template<class T>void load(const seal::SEALContext& cx,const Bytes& b,T& key,std::size_t cap){
 need(b.size()>=16&&b.size()<=cap,"343 unchanged bounded key frame");
 seal::Serialization::SEALHeader h;
 seal::Serialization::LoadHeader(reinterpret_cast<const seal::seal_byte*>(b.data()),b.size(),h,false);
 need(seal::Serialization::IsValidHeader(h)&&h.size==b.size()&&h.compr_mode==seal::compr_mode_type::zstd,"343 exact current SEAL compressed frame");
 std::istringstream in(std::string(b.begin(),b.end()),std::ios::binary);
 key.load(cx,in);need(in.peek()==std::char_traits<char>::eof(),"343 no trailing key data");
 need(seal::is_valid_for(key,cx),"343 original valid key context");
}
inline std::array<Bytes,2> generate(seal::KeyGenerator& kg,const seal::SEALContext& cx,
 seal::PublicKey& pk,seal::GaloisKeys& gk,const std::vector<std::uint32_t>& elements){
 need(elements.size()==7,"343 original seven Galois elements");
 auto wirePK=save_seeded(kg.create_public_key());
 auto wireGK=save_seeded(kg.create_galois_keys(elements));
 load(cx,wirePK,pk,1ULL<<20);load(cx,wireGK,gk,16ULL<<20);
 need(gk.size()==7,"343 seven original keys");for(auto e:elements)need(gk.has_key(e),"343 exact Galois roster");
 return {std::move(wirePK),std::move(wireGK)};
}
}

