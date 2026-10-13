#pragma once
#include "resnet50/engine/r50_schedule.hpp"
#include <unordered_map>
namespace aqcf_r50 {
// Exact native topology; vectors use NCHW rather than the author's NHWC layout.
template<class B> std::vector<aqcf3::U> execute(B& b,const std::vector<aqcf3::U>& input){
 using U=aqcf3::U;using V=std::vector<U>;using aqcf3::require;
 std::unordered_map<int,V> tensors;tensors[0]=b.copy_tensor(input,"R50_EXEC_INPUT_COPY");
 auto get=[&](int id)->const V&{return tensors.at(id);};
 for(const auto& n:graph){
  auto started=b.begin_node(n);const auto&q=n.args;V y;std::size_t elements=1;for(int v:n.shape)elements*=v;
  if(n.affine>=0){int src=n.kind=="FusedBatchNorm4411"?q[4]:n.kind=="MatMul2D"?q[3]:q[13];y=b.affine(n.affine,get(src),elements,n.index,n.out,src);}
  else if(n.kind=="Relu4"){require(q[7]==1&&q[6]==12,"native ReLU scaling");y=b.boundary(get(q[4]),12,true,false,n.index,q[4]);}
  else if(n.kind=="ScaleDown4")y=b.boundary(get(q[4]),q[5],false,false,n.index,q[4]);
  else if(n.kind=="ScaleUp4"){const auto& x=get(q[4]);y.resize(x.size());b.tensor_work("R50_SCALE_UP",y.size(),[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)y[j]=(x[j]<<q[5])&aqcf2::mask(37);});}
  else if(n.kind=="MatAdd4")y=b.residual_add(get(q[4]),get(q[5]),n.index,q[4],q[5],n.out);
  else if(n.kind=="Pad442"){
   require(q[0]==1&&q[4]==1&&q[3]==q[7]&&n.padding.size()==8,"pad geometry");const auto&p=n.padding;require(p[0]==0&&p[1]==0&&p[6]==0&&p[7]==0,"spatial padding only");
   int oh=q[1],ow=q[2],ih=q[5],iw=q[6],ch=q[3];require(oh==ih+p[2]+p[3]&&ow==iw+p[4]+p[5],"pad size");y.resize(elements);const auto&x=get(q[8]);
   b.tensor_work("R50_PAD",elements,[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j){const int xx=int(j%ow),yy=int((j/ow)%oh),c=int(j/(std::size_t(oh)*ow));const int sy=yy-p[2],sx=xx-p[4];y[j]=(sy>=0&&sy<ih&&sx>=0&&sx<iw)?x[(c*ih+sy)*iw+sx]:0;}});
  }else if(n.kind=="Squeeze24")y=b.copy_tensor(get(q[q.size()-2]),"R50_SQUEEZE_COPY");
  else if(n.kind=="ScaleUp1"){require(q[0]==1001&&q[2]==12,"private FC bias folded exactly");y.assign(elements,0);}
  else if(n.kind=="MatAddBroadCast2"){require(q[0]==1&&q[1]==1001,"complete class head");y=b.copy_tensor(get(q[2]),"R50_HEAD_COPY");}
  else if(n.kind=="MaxPool"||n.kind=="AvgPool"){
   int oh=q[1],ow=q[2],ch=q[3],kh=q[4],kw=q[5],pt=q[6],pl=q[8],sh=q[10],sw=q[11],ih=q[13],iw=q[14];
   require(q[0]==1&&q[12]==1&&q[15]==ch,"pool geometry");auto x=n.kind=="MaxPool"?b.pool_lift(get(q[16])):b.copy_tensor(get(q[16]),"R50_AVG_INPUT_COPY");y.assign(elements,0);bool first=true;
   if(n.kind=="MaxPool"&&b.enabled255){
    require(n.index==2&&n.out==259&&q==std::vector<int>({1,56,56,64,3,3,0,1,0,1,2,2,1,112,112,64,256,259})&&n.shape==std::vector<int>({1,56,56,64}),"258 source-bound original signed37 max pooling geometry");
    y=aqcf_pool255::reduce(b,x,ch,ih,iw);
   }else for(int ky=0;ky<kh;++ky)for(int kx=0;kx<kw;++kx){
    V v(elements);b.tensor_work("R50_POOL_GATHER",elements,[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j){const int xx=int(j%ow),yy=int((j/ow)%oh),c=int(j/(std::size_t(oh)*ow));const int sy=yy*sh-pt+ky,sx=xx*sw-pl+kx;v[j]=(sy>=0&&sy<ih&&sx>=0&&sx<iw)?x[(c*ih+sy)*iw+sx]:0;}});
    if(n.kind=="MaxPool"){if(first)y=std::move(v);else y=b.pool_maximum(y,v);}else b.tensor_work("R50_AVG_ADD",elements,[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)y[j]=(y[j]+v[j])&aqcf2::mask(37);});first=false;
   }
   if(n.kind=="MaxPool")b.mask37_tensor(y,"R50_POOL_PROJECT");else{
    // Public graph certificate, never inferred from private values/statistics.
    const auto& producer=graph.at(173);
    require(n.index==174&&n.out==767&&q[16]==765&&elements==2048&&
      oh==1&&ow==1&&ch==2048&&kh==7&&kw==7&&ih==7&&iw==7&&
      q[6]==0&&q[7]==0&&q[8]==0&&q[9]==0&&sh==1&&sw==1&&
      producer.index==173&&producer.kind=="Relu4"&&producer.out==765&&
      producer.args==std::vector<int>({1,7,7,2048,761,765,12,1})&&
      producer.shape==std::vector<int>({1,7,7,2048}),"AvgPool174 source-bound nonnegative range certificate");
    y=b.divide49_certified173(y);
   }
  }else if(n.kind=="ArgMax1"){require(q[0]==1&&q[1]==1&&q[2]==1001&&q[4]==1,"all classes protected ArgMax");y=b.argmax(get(q[3]));}
  else throw std::runtime_error("unhandled official R50 node "+n.kind);
  require(y.size()==elements,"complete R50 output shape");tensors[n.out]=std::move(y);b.end_node(n,started,tensors[n.out]);
 }
 require(!b.residual_pending&&!b.residual_added&&b.residual_other.empty()&&b.residual_calls==16&&b.residual_add_calls==16&&b.residual_rows==5519360,"all residual chains consumed");
 require(!b.phase_pending&&b.phase_calls==49&&b.phase_rows==9006592,"all source-bound phase pairs consumed exactly once");
 require(b.avg29_calls==1&&b.avg29_rows==2048,"certified exact reciprocal division consumed once");
 auto scores=b.boundary(get(776),0,false,true);auto label=get(graph.back().out);require(scores.size()==1001&&label.size()==1,"scores plus protected class");scores.push_back(label[0]);return scores;
}
}
