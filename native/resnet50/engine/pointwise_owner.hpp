#pragma once
#include "resnet50/engine/pointwise_roster.hpp"
#include "resnet50/core/batch_norm_affine_core.hpp"
namespace pw25 {
inline U source_index(const Shape& s,U ch,U y,U x){return ch*s.L*s.stride*s.stride+y*s.stride*s.W*s.stride+x*s.stride;}
inline aqcf3::Layout layout(const Shape& s){
 aqcf3::Layout l;l.id=s.id;l.inputs=s.Ci*s.L*s.stride*s.stride;l.outputs=s.Co*s.L;
 for(const auto& t:s.tiles){for(U u=0;u<t.Ugroups;++u){std::vector<aqcf3::Term> m;for(U r=0;r<std::min(t.J,s.Ci-u*t.J);++r)for(U y=0;y<t.h;++y)for(U x=0;x<t.w;++x)m.push_back({source_index(s,u*t.J+r,t.y+y,t.x+x),(t.J-1-r)*t.L+y*t.w+x,1});std::sort(m.begin(),m.end(),[](const auto&a,const auto&b){return std::tie(a.exponent,a.source)<std::tie(b.exponent,b.source);});l.maps.push_back(std::move(m));}
  for(U v=0;v<t.Vgroups;++v){std::vector<aqcf3::Query> q;for(U f=0;f<std::min(t.F,s.Co-v*t.F);++f)for(U y=0;y<t.h;++y)for(U x=0;x<t.w;++x)q.push_back({(v*t.F+f)*s.L+(t.y+y)*s.W+t.x+x,(f*t.J+t.J-1)*t.L+y*t.w+x,0});l.queries.push_back(std::move(q));}}
 aqcf3::require(l.maps.size()==s.old_inputs&&l.queries.size()==s.old_outputs,"pointwise original layout counts");return l;
}
inline void bind_layout(const aqcf3::Layout& old,const Shape& s){auto want=layout(s);aqcf3::require(old.id==want.id&&old.inputs==want.inputs&&old.outputs==want.outputs&&old.maps.size()==want.maps.size()&&old.queries.size()==want.queries.size(),"original pointwise dimensions");for(U m=0;m<old.maps.size();++m){aqcf3::require(old.maps[m].size()==want.maps[m].size(),"original pointwise input coverage");for(U j=0;j<old.maps[m].size();++j){auto&a=old.maps[m][j];auto&b=want.maps[m][j];aqcf3::require(a.source==b.source&&a.exponent==b.exponent&&a.multiplier==b.multiplier,"original pointwise sample and exponent");}}
 for(U m=0;m<old.queries.size();++m){aqcf3::require(old.queries[m].size()==want.queries[m].size(),"original pointwise output coverage");for(U j=0;j<old.queries[m].size();++j){auto&a=old.queries[m][j];auto&b=want.queries[m][j];aqcf3::require(a.destination==b.destination&&a.exponent==b.exponent,"original pointwise destination and exponent");}}}
struct Owner{std::vector<I>w,bias;};
inline Owner bind_owner(const aqcf3::Layer& old,const Shape& s){
 bind_layout(old.layout,s);aqcf3::require(old.output.size()==s.old_outputs,"owner output count");Owner owner;owner.w.resize(s.Ci*s.Co);owner.bias.resize(s.Co);std::vector<unsigned char>seen(s.Ci*s.Co),seen_bias(s.Co);U ib=0,ob=0;
 for(const auto&t:s.tiles){for(U v=0;v<t.Vgroups;++v){const auto&o=old.output[ob+v];std::vector<const aqcf3::Edge*>edges(t.Ugroups,nullptr);__int128 mass=0;
   for(const auto&e:o.edges){aqcf3::require(e.input>=ib&&e.input<ib+t.Ugroups&&!edges[e.input-ib],"original owner edge identity");edges[e.input-ib]=&e;for(const auto&c:e.coefficients)mass+=c.value<0?-__int128(c.value):__int128(c.value);}
   aqcf3::require(mass<=(__int128(1)<<30),"unchanged original output L1 cap");
   for(U u=0;u<t.Ugroups;++u){U nr=std::min(t.J,s.Ci-u*t.J),nf=std::min(t.F,s.Co-v*t.F);std::vector<I>local(nr*nf);std::vector<bool>once(nr*nf);
    if(edges[u])for(const auto&c:edges[u]->coefficients){aqcf3::require(c.exponent%t.L==0,"pointwise coefficient exact lattice");U pos=c.exponent/t.L,r=pos%t.J,f=pos/t.J;aqcf3::require(r<nr&&f<nf&&!once[f*nr+r],"owner coefficient index unique");once[f*nr+r]=true;local[f*nr+r]=c.value;}
    for(U f=0;f<nf;++f)for(U r=0;r<nr;++r){U at=(v*t.F+f)*s.Ci+u*t.J+r;I value=local[f*nr+r];if(seen[at])aqcf3::require(owner.w[at]==value,"same original weight across spatial tiles");else{owner.w[at]=value;seen[at]=1;}}}
   aqcf3::require(o.queries.size()==old.layout.queries[ob+v].size(),"all original biased queries");for(U j=0;j<o.queries.size();++j){auto&q=o.queries[j];auto&pub=old.layout.queries[ob+v][j];aqcf3::require(q.destination==pub.destination&&q.exponent==pub.exponent,"biased public query identity");U ch=q.destination/s.L;if(seen_bias[ch])aqcf3::require(owner.bias[ch]==q.bias,"same original bias spatially");else{owner.bias[ch]=q.bias;seen_bias[ch]=1;}}}
  ib+=t.Ugroups;ob+=t.Vgroups;}
 aqcf3::require(std::all_of(seen.begin(),seen.end(),[](auto x){return x;})&&std::all_of(seen_bias.begin(),seen_bias.end(),[](auto x){return x;}),"complete owner parameter bijection");
 for(U o=0;o<s.Co;++o){__int128 m=0;for(U i=0;i<s.Ci;++i){I x=owner.w[o*s.Ci+i];m+=x<0?-__int128(x):__int128(x);}aqcf3::require(m<=(__int128(1)<<30),"derived original row L1 unchanged");}return owner;
}
template<class W,class B> aqcf3::Layer fixture(const Shape&s,W weight,B bias){aqcf3::Layer a;a.layout=layout(s);U ib=0,ob=0;for(const auto&t:s.tiles){for(U v=0;v<t.Vgroups;++v){aqcf3::Output out;for(U u=0;u<t.Ugroups;++u){aqcf3::Edge e;e.input=ib+u;for(U f=0;f<std::min(t.F,s.Co-v*t.F);++f)for(U r=0;r<std::min(t.J,s.Ci-u*t.J);++r){I w=weight(v*t.F+f,u*t.J+r);if(w)e.coefficients.push_back({(f*t.J+r)*t.L,w});}if(!e.coefficients.empty())out.edges.push_back(std::move(e));}out.queries=a.layout.queries[ob+v];for(auto&q:out.queries)q.bias=bias(q.destination/s.L);a.output.push_back(std::move(out));}ib+=t.Ugroups;ob+=t.Vgroups;}return a;}
}

