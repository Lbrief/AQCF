#pragma once
#include "squeezenet/engine/types.hpp"
#include <openssl/crypto.h>
#include <array>
namespace aqcf_argmax274 {
using namespace aqcf2;
using V=std::vector<U>;
struct Census {U comparisons=0,leaf_rows=0,compose_rows=0,and_rows=0,public_labels=0,private_labels=0,value_mux=0;};
inline thread_local Census census;
inline U compose(U low,U high){return ((high&1)|(((high>>1)&1)&(low&1)))|((((high>>1)&1)&((low>>1)&1))<<1);}
// Six 4-bit digit actions, two groups of three, then their final composition.
// Each lookup returns a FRESH XOR-masked action; no two cofactors are exposed.
// Standard finite-function composition using the already qualified private OT.
template<class B> Bits carry24(B& s,const V& own){
 const size_t n=own.size();require(n&&n<=4096,"274 small tail comparison batch");
 V choice(6*n),table(s.party==1?6*n*16:0);auto pad=s.party==1?s.random_small(6*n):std::vector<uint8_t>{};
 for(size_t k=0;k<6;++k)for(size_t i=0;i<n;++i){const size_t at=k*n+i;U a=(own[i]>>(4*k))&15;choice[at]=a;
  if(s.party==1)for(U b=0;b<16;++b){U sum=a+b;table[at*16+b]=(U(sum>=16)|(U(sum==15)<<1))^pad[at];}}
 auto got=s.lookup_words(table,choice,6*n,4,2,false);s.flush();V action(6*n);
 for(size_t i=0;i<6*n;++i)action[i]=s.party==1?pad[i]:got[i];
 if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));if(!pad.empty())OPENSSL_cleanse(pad.data(),pad.size());
 choice.assign(2*n,0);table.assign(s.party==1?2*n*64:0,0);pad=s.party==1?s.random_small(2*n):std::vector<uint8_t>{};
 for(size_t group=0;group<2;++group)for(size_t i=0;i<n;++i){const size_t at=group*n+i;
  U a=action[(3*group)*n+i]|(action[(3*group+1)*n+i]<<2)|(action[(3*group+2)*n+i]<<4);choice[at]=a;
  if(s.party==1)for(U b=0;b<64;++b){U v=a^b;table[at*64+b]=compose(compose(v&3,(v>>2)&3),(v>>4)&3)^pad[at];}}
 got=s.lookup_words(table,choice,2*n,6,2,false);s.flush();V middle(2*n);
 for(size_t i=0;i<2*n;++i)middle[i]=s.party==1?pad[i]:got[i];
 if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));if(!pad.empty())OPENSSL_cleanse(pad.data(),pad.size());
 choice.assign(n,0);table.assign(s.party==1?n*16:0,0);pad=s.party==1?s.random_small(n):std::vector<uint8_t>{};
 for(size_t i=0;i<n;++i){U a=middle[i]|(middle[n+i]<<2);choice[i]=a;
  if(s.party==1)for(U b=0;b<16;++b){U v=a^b;table[i*16+b]=(compose(v&3,(v>>2)&3)&1)^(pad[i]&1);}}
 got=s.lookup_words(table,choice,n,4,1,false);s.flush();Bits out(n);
 for(size_t i=0;i<n;++i)out.v[i]=s.party==1?(pad[i]&1):got[i];
 if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));if(!pad.empty())OPENSSL_cleanse(pad.data(),pad.size());
 for(auto* v:{&action,&middle,&got})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));
 census.comparisons+=n;census.leaf_rows+=6*n;census.compose_rows+=3*n;out.check();return out;
}
// Ragged AND trees with public arity <=4. Missing inputs are public TRUE.
// Each selected function value is freshly one-time padded; no winner opens.
template<class B> Bits all(B& s,std::vector<std::vector<uint8_t>> values){
 require(!values.empty(),"274 nonempty useful candidates");
 while(std::any_of(values.begin(),values.end(),[](const auto& v){return v.size()>1;})){
  std::vector<std::vector<uint8_t>> next(values.size());std::vector<std::pair<size_t,size_t>> rows;
  for(size_t i=0;i<values.size();++i){require(!values[i].empty(),"274 nonempty comparisons");next[i].resize((values[i].size()+3)/4);
   for(size_t j=0;j<next[i].size();++j)rows.emplace_back(i,4*j);}
  V choice(rows.size()),table(s.party==1?16*rows.size():0);auto pad=s.party==1?s.random_small(rows.size()):std::vector<uint8_t>{};
  for(size_t at=0;at<rows.size();++at){auto [i,begin]=rows[at];U own=0;
   for(size_t k=0;k<4;++k){U bit=begin+k<values[i].size()?values[i][begin+k]:U(s.party==1);require(bit<2,"274 canonical win shares");own|=bit<<k;}choice[at]=own;
   if(s.party==1)for(U b=0;b<16;++b)table[16*at+b]=U((own^b)==15)^(pad[at]&1);}
  auto got=s.lookup_words(table,choice,rows.size(),4,1,false);s.flush();
  for(size_t at=0;at<rows.size();++at){auto [i,begin]=rows[at];next[i][begin/4]=s.party==1?(pad[at]&1):got[at];}
  for(auto& v:values)OPENSSL_cleanse(v.data(),v.size());values=std::move(next);census.and_rows+=rows.size();
  if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));if(!pad.empty())OPENSSL_cleanse(pad.data(),pad.size());
 }
 Bits out(values.size());for(size_t i=0;i<values.size();++i)out.v[i]=values[i][0];out.check();return out;
}
template<class B> Bits winners(B& s,const V& value,size_t begin,size_t count,size_t group){
 V diff;std::vector<size_t> sizes(count);diff.reserve(count*(group-1));
 for(size_t k=0;k<count;++k){const size_t i=begin+k,lo=(i/group)*group,hi=std::min(value.size(),lo+group);
  for(size_t j=lo;j<hi;++j)if(j!=i){diff.push_back((value[i]-value[j]-U(s.party==1&&i>j))&mask(37));++sizes[k];}}
 auto loses=s.xor_(carry24(s,diff),s.local_bit(diff,24));auto wins=s.not_(loses);
 std::vector<std::vector<uint8_t>> pieces(count);size_t at=0;
 for(size_t k=0;k<count;++k){pieces[k].assign(wins.v.begin()+at,wins.v.begin()+at+sizes[k]);at+=sizes[k];}
 require(at==wins.v.size(),"274 complete pair partition");return all(s,std::move(pieces));
}
// Multiplication of an XOR-shared bit by a PUBLIC original class index.
// Unlike the second round, indices here are public; one fresh chosen lookup suffices.
template<class B> Ring public_index(B& s,const Bits& win,size_t begin){
 size_t n=win.v.size();V choices(n),table(s.party==1?2*n:0),pad=s.party==1?s.random_ring_words(n,55):V{};
 for(size_t i=0;i<n;++i){choices[i]=win.v[i];if(s.party==1)for(U b=0;b<2;++b)table[2*i+b]=(U(begin+i)*(win.v[i]^b)-pad[i])&mask(55);}
 auto got=s.lookup_words(table,choices,n,1,55,false);s.flush();Ring out(55,n);for(size_t i=0;i<n;++i)out.v[i]=s.party==1?pad[i]:got[i];
 if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));if(!pad.empty())OPENSSL_cleanse(pad.data(),pad.size()*sizeof(U));census.public_labels+=n;return out;
}
// Exactly1000 classes. Group16 then all63 group winners: two comparison layers.
// Consecutive groups preserve the original index order, including tied maxima.
template<class L> V run(L& lanes,const V& input){
 require(input.size()==1000,"274 complete original SQ classifier");V ids(1000),terms_index(1000);
 auto terms_value=lanes.run(ids,[&](auto& s,const V& part,size_t offset){
  auto win=winners(s,input,offset,part.size(),16);auto labels=public_index(s,win,offset);
  Ring values(37,part.size());std::copy_n(input.begin()+offset,part.size(),values.v.begin());auto selected=s.mux(win,values);
  std::copy(labels.v.begin(),labels.v.end(),terms_index.begin()+offset);census.value_mux+=part.size();return selected.v;});
 constexpr size_t groups=63;V group_ids(groups),index(groups);
 auto value=lanes.run(group_ids,[&](auto&,const V& part,size_t offset){V out(part.size());
  for(size_t k=0;k<part.size();++k){size_t group=offset+k;U v=0,label=0;for(size_t i=group*16;i<std::min<size_t>(1000,group*16+16);++i){v+=terms_value[i];label+=terms_index[i];}
   out[k]=v&mask(37);index[group]=label&mask(55);}return out;});
 auto selected=lanes.run(group_ids,[&](auto& s,const V& part,size_t offset){
  auto win=winners(s,value,offset,part.size(),groups);Ring labels(55,part.size());std::copy_n(index.begin()+offset,part.size(),labels.v.begin());
  census.private_labels+=part.size();return s.mux(win,labels).v;});
 V reduction_ids(4);auto terms=lanes.run(reduction_ids,[&](auto&,const V& part,size_t offset){require(part.size()==1,"274 four useful final reductions");
  U sum=0;for(size_t i=groups*offset/4;i<groups*(offset+1)/4;++i)sum+=selected[i];return V{sum&mask(55)};});
 U sum=0;for(U x:terms)sum+=x;return V{sum&mask(55)};
}
}
