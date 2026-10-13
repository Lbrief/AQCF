#pragma once
// Experimental public-support automorphism action. Structural zero skipping is
// selected from layer geometry, NEVER from private weight values. Full ciphertext
// degree, output RNS spectra and protocol remain unchanged. NTT pruning and ring
// automorphisms are established tools; no priority claim is made here.
#include "resnet50/core/public_tile_weight.hpp"
#include <array>
#include <numeric>
#include <tuple>
namespace aqcf3::spatial_action {
struct Stage {U half;bool dense=false;std::vector<std::uint16_t> both,left,right;std::vector<ActionMultiplier> roots;};
struct PrimePlan {
 U q,unit;std::vector<U> position,reverse;std::vector<unsigned char> allowed,neg;
 std::vector<ActionMultiplier> twist;std::vector<Stage> stages;
 PrimePlan(U prime,U ell,U row,U kernel,U channels,U u):q(prime),unit(u),position(DEGREE),reverse(DEGREE),allowed(DEGREE,0),neg(DEGREE),twist(DEGREE){
  require(u>0&&u<2*DEGREE&&(u&1),"public automorphism unit");
  for(U k=1;k<DEGREE;++k)reverse[k]=(reverse[k>>1]>>1)|((k&1)?DEGREE>>1:0);
  std::vector<unsigned char> active(DEGREE,0);
  for(U c=0;c<channels;++c)for(U y=0;y<kernel;++y)for(U x=0;x<kernel;++x){U e=c*ell+y*row+x;require(e<DEGREE&&!allowed[e],"public noncolliding spatial support");allowed[e]=1;
   U mapped=(e*u)%(2*DEGREE);neg[e]=mapped>=DEGREE;position[e]=mapped%DEGREE;active[reverse[position[e]]]=1;}
  U generator=2;while(generator<4096&&action_pow(generator,(q-1)/2,q)!=q-1)++generator;require(generator<4096,"nonresidue bound");
  U psi=action_pow(generator,(q-1)/(2*DEGREE),q),omega=U(__uint128_t(psi)*psi%q),v=1;
  require(action_pow(psi,DEGREE,q)==q-1,"full-degree primitive root");
  for(U k=0;k<DEGREE;++k){twist[k]=ActionMultiplier(v,q);v=U(__uint128_t(v)*psi%q);}
  for(U half=1;half<DEGREE;half*=2){Stage s;s.half=half;std::vector<unsigned char> next(DEGREE,0);
   for(U start=0;start<DEGREE;start+=2*half)for(U j=0;j<half;++j){U a=start+j;bool x=active[a],y=active[a+half];
    if(x&&y)s.both.push_back(a);else if(x)s.left.push_back(a);else if(y)s.right.push_back(a);if(x||y)next[a]=next[a+half]=1;}
   s.dense=s.both.size()==DEGREE/2;U root=action_pow(omega,DEGREE/(2*half),q),w=1;s.roots.resize(half);
   for(U j=0;j<half;++j){s.roots[j]=ActionMultiplier(w,q);w=U(__uint128_t(w)*root%q);}
   active=std::move(next);stages.push_back(std::move(s));
  }
  require(std::all_of(active.begin(),active.end(),[](auto x){return x==1;}),"full spectrum materialized");
 }
 U lazy(U y,const ActionMultiplier& w)const{U t=U((__uint128_t(y)*w.scaled)>>64);return y*w.w-t*q;}
 std::vector<U> weight(const Edge& e)const{
  std::vector<U> values(DEGREE,0);
  for(const auto& a:e.coefficients){require(a.exponent<DEGREE&&allowed[a.exponent],"private coefficient outside fixed public spatial support");U m=magnitude(a.value);require(m<q,"signed coefficient bound");
   bool sign=(a.value<0)!=bool(neg[a.exponent]);U w=sign?q-m:m,p=position[a.exponent];values[reverse[p]]=twist[p].apply(w,q);}
  for(const auto& s:stages){U h=s.half;
   auto both=[&](U a){U u=values[a];if(u>=2*q)u-=2*q;U v=lazy(values[a+h],s.roots[a&(h-1)]);values[a]=u+v;values[a+h]=u+2*q-v;};
   if(s.dense){for(U start=0;start<DEGREE;start+=2*h)for(U j=0;j<h;++j)both(start+j);}
   else{
    for(U a:s.both)both(a);
    for(U a:s.left)values[a+h]=values[a];
    for(U a:s.right){U v=lazy(values[a+h],s.roots[a&(h-1)]);values[a]=v;values[a+h]=2*q-v;}
   }
  }
  for(auto& v:values){if(v>=2*q)v-=2*q;if(v>=q)v-=q;}return values;
 }
};
struct Plan {
 std::vector<PrimePlan> plans;std::vector<std::vector<U>> indices;
 Plan(const seal::SEALContext& cx,U ell,U row,U kernel,U channels,U unit){
  U inverse=1;while(inverse<2*DEGREE&&(inverse*unit)%(2*DEGREE)!=1)inverse+=2;require(inverse<2*DEGREE,"automorphism inverse");
  std::vector<Coefficient> coefficient{{inverse%DEGREE,inverse>=DEGREE?-1:1}};auto roots=aqcf_signed_weight::spectrum(cx,coefficient);
  const auto& qs=cx.first_context_data()->parms().coeff_modulus();
  for(unsigned r=0;r<qs.size();++r){U q=qs[r].value();plans.emplace_back(q,ell,row,kernel,channels,unit);
   ActionNTT ntt(DEGREE,q);Edge y;y.coefficients.push_back({1,1});auto ref=ntt.weight(y,1);std::unordered_map<U,U> positions;
   for(U k=0;k<DEGREE;++k)require(positions.emplace(ref[k],k).second,"unique primitive roots");std::vector<U> index(DEGREE);std::vector<bool> used(DEGREE,false);
   for(U j=0;j<DEGREE;++j){auto it=positions.find(roots[r*DEGREE+j]);require(it!=positions.end()&&!used[it->second],"exact root automorphism bijection");index[j]=it->second;used[it->second]=true;}indices.push_back(std::move(index));
  }
 }
 std::vector<U> weight(const Edge& e)const{std::vector<U> result(plans.size()*DEGREE);
  for(unsigned r=0;r<plans.size();++r){auto small=plans[r].weight(e);for(U j=0;j<DEGREE;++j)result[r*DEGREE+j]=small[indices[r][j]];}return result;}
};
inline std::vector<U> spectrum(const seal::SEALContext& cx,const Edge& e,U ell,U row,U kernel,U channels,U unit){
 using Key=std::tuple<const seal::SEALContext*,U,U,U,U,U>;thread_local std::map<Key,std::unique_ptr<Plan>> plans;
 Key key{&cx,ell,row,kernel,channels,unit};auto it=plans.find(key);if(it==plans.end())it=plans.emplace(key,std::make_unique<Plan>(cx,ell,row,kernel,channels,unit)).first;
 return it->second->weight(e);
}
}
