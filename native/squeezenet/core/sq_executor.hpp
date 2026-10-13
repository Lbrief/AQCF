#include "squeezenet/core/pool_geometry.hpp"
#ifndef AQCF_FUSED_QUARTER_ENTRY
#define AQCF_FUSED_QUARTER_ENTRY 0
#endif
#pragma once
#include "squeezenet/core/sq_schedule.hpp"
#include <unordered_map>
namespace aqcf_sq {
// Same original graph in NCHW. Bias shift/add are fused into each HE affine;
// their logical accounting nodes are retained and explicitly labelled fused.
template<class B> std::vector<aqcf3::U> execute(B& b,const std::vector<aqcf3::U>& input){
 using U=aqcf3::U;using V=std::vector<U>;using aqcf3::require;
 require(input.size()==154587,"full227x227x3 real input");
 std::unordered_map<int,V> tensors;tensors[0]=b.copy_tensor(input,"EXEC_INPUT_COPY");
 std::unordered_map<int,unsigned> certified_nonnegative_bits;
 auto get=[&](int id)->const V&{return tensors.at(id);};
  unsigned conv=0,relu=0,maxpool=0,avgpool=0,concat=0,bias=0,scale=0,classes=0;
  bool pending_quotient_pool=false,quotient_pool_consumed=false;
 for(const auto& n:graph){
  auto started=b.begin_node(n);const auto&q=n.args;V y;std::size_t elements=1;for(int x:n.shape)elements*=x;
  if(n.affine>=0){require(n.kind=="Conv2DWrapper","affine operator binding");auto cert=certified_nonnegative_bits.find(q[13]);
   if(cert==certified_nonnegative_bits.end())y=b.affine(n.affine,get(q[13]),elements);
   else y=b.affine_with_nonnegative_certificate(n.affine,get(q[13]),elements,cert->second);
   if(AQCF_FUSED_QUARTER_ENTRY){require(b.phase_pending&&b.phase_tensor_id==-1,"bind unique affine phase tensor");b.phase_tensor_id=n.out;}
    ++conv;}
  else if(n.kind=="ScaleUp1"){require(q[2]==12,"exact bias shift fused in previous HE call");y.assign(elements,0);++scale;}
  else if(n.kind=="MatAddBroadCast4"){
    if(AQCF_FUSED_QUARTER_ENTRY){require(b.phase_pending&&b.phase_tensor_id==q[4],"only identity bias alias of raw phase");b.phase_tensor_id=n.out;}
    y=b.copy_tensor(get(q[4]),"EXEC_FUSED_BIAS_ALIAS");++bias;}
  else if(n.kind=="Relu4"){
   require(q[6]==12&&(q[7]==0||q[7]==1),"native ReLU scaling");
   if(n.index==4){
    require(pending_quotient_pool&&!quotient_pool_consumed&&q[4]==59&&q[7]==1,"unique adjacent quotient consumer");
    y=b.quotient_pool_finish(get(q[4]));pending_quotient_pool=false;quotient_pool_consumed=true;
    std::cout<<"QUOTIENT_POOL_FINISH node4 exact_ReLU37_conjugated_lift25\n"<<std::flush;
   }else {
     if(AQCF_FUSED_QUARTER_ENTRY)require(b.phase_pending&&b.phase_tensor_id==q[4]&&q[7]==1,"fixed raw affine ReLU consumer");
     y=b.boundary(get(q[4]),q[7]?12:0,true);
    }
   ++relu;
  }
  else if(n.kind=="Concat2T444"){
   require(q[0]==1&&q[4]==1&&q[9]==1&&q[14]==3&&q[5]==q[10]&&q[6]==q[11],"native channel concat shape");
   y=b.concat_tensor(get(q[8]),get(q[13]));++concat;
  }else if(n.kind=="MaxPool"){
   require(q[0]==1&&q[12]==1&&q[15]==q[3],"original maximum pool geometry");
   aqcf_pool_geometry::Shape shape{q[13],q[14],q[3],q[1],q[2],q[4],q[5],q[6],q[8],q[10],q[11]};
   auto found=certified_nonnegative_bits.find(q[16]);
   unsigned bits=found==certified_nonnegative_bits.end()?0:found->second;
   require((n.index==3&&bits==0)||((n.index==31||n.index==58)&&bits==24),"bound exact public producer certificates");
   std::cout<<"POOL_CERTIFICATE "<<n.index<<" bits "<<bits<<"\n"<<std::flush;
   if(bits){
    y=aqcf_pool_geometry::separable(get(q[16]),shape,[&](const V&a,const V&c){return b.pool_range_maximum(a,c,bits);},[&](const char* phase,std::size_t n,auto work){b.tensor_work(phase,n,work);});
   }else{
    require(n.index==3&&n.out==59&&q[16]==56&&!pending_quotient_pool,"fixed sole-consumer quotient pair");
    require(graph.at(4).kind=="Relu4"&&graph.at(4).args[4]==59&&graph.at(4).args[7]==1,"following floor12 ReLU");
    if(AQCF_FUSED_QUARTER_ENTRY)require(b.phase_pending&&b.phase_tensor_id==q[16],"fixed raw first-pool producer");
     auto x=b.quotient_pool_lift(get(q[16]));
    y=aqcf_pool_geometry::separable(x,shape,[&](const V&a,const V&c){return b.quotient_pool_maximum(a,c);},[&](const char* phase,std::size_t n,auto work){b.tensor_work(phase,n,work);});
    pending_quotient_pool=true;
    std::cout<<"QUOTIENT_POOL_PAIR node3_to4 ReLU_conjugated_nonnegative24_in25 pending_lift37\n"<<std::flush;
   }
   b.mask37_tensor(y,"EXEC_POOL_MASK37");++maxpool;
  }else if(n.kind=="AvgPool"){
   const int oh=q[1],ow=q[2],ch=q[3],kh=q[4],kw=q[5],pt=q[6],pl=q[8],sh=q[10],sw=q[11],ih=q[13],iw=q[14];
   require(q[0]==1&&q[12]==1&&q[15]==ch,"original pool geometry");
   const auto& x=get(q[16]);y.resize(elements);
   // Independent output-channel reductions: original ky/kx order and modulo37
   // after EVERY add are retained, including original zero padding.
   b.tensor_work("EXEC_AVGPOOL_SUM",elements,[&](std::size_t lo,std::size_t hi){
    for(auto j=lo;j<hi;++j){const int w=j%ow,h=(j/ow)%oh,c=j/(ow*oh);U sum=0;
     for(int ky=0;ky<kh;++ky)for(int kx=0;kx<kw;++kx){
      const int sy=h*sh-pt+ky,sx=w*sw-pl+kx;
      const U v=sy>=0&&sy<ih&&sx>=0&&sx<iw?x[(c*ih+sy)*iw+sx]:0;
      sum=(sum+v)&aqcf2::mask(37);
     }y[j]=sum;
    }
   });
   require(kh*kw==169&&oh==1&&ow==1&&ch==1000,"full13x13 average");auto cert=certified_nonnegative_bits.find(q[16]);
   if(cert!=certified_nonnegative_bits.end()&&cert->second==24)y=b.divide169_nonnegative24(y);
   else y=b.divide169(y);++avgpool;
  }else if(n.kind=="ArgMax3"){
   require(q[6]==1000&&q[8]==3,"all1000classes secure ArgMax");auto cert=certified_nonnegative_bits.find(q[7]);
   if(cert!=certified_nonnegative_bits.end()&&cert->second==24)y=b.argmax_nonnegative24(get(q[7]));
   else y=b.argmax(get(q[7]));++classes;
  }else throw std::runtime_error("unhandled original SqNet operator "+n.kind);
  // Only public operator semantics create a certificate. Unknown producers
  // receive none; activation magnitudes and shares are never inspected.
  if(n.kind=="Relu4")certified_nonnegative_bits[n.out]=q[7]?24:36;
  else if(n.kind=="Concat2T444"){
   auto a=certified_nonnegative_bits.find(q[8]),c=certified_nonnegative_bits.find(q[13]);
   if(a!=certified_nonnegative_bits.end()&&c!=certified_nonnegative_bits.end())
    certified_nonnegative_bits[n.out]=std::max(a->second,c->second);
  }else if(n.kind=="AvgPool"){
   auto a=certified_nonnegative_bits.find(q[16]);
   if(a!=certified_nonnegative_bits.end()&&a->second==24)certified_nonnegative_bits[n.out]=24;
  }else if(n.kind=="MaxPool"){
   auto a=certified_nonnegative_bits.find(q[16]);
   if(a!=certified_nonnegative_bits.end())certified_nonnegative_bits[n.out]=a->second;
  }
  require(y.size()==elements,"no partial tensor output");tensors[n.out]=std::move(y);b.end_node(n,started,tensors[n.out]);
 }
  require(conv==26&&scale==26&&bias==26&&relu==26&&maxpool==3&&avgpool==1&&concat==8&&classes==1,"complete native117 logical operations");
  require(quotient_pool_consumed&&!pending_quotient_pool,"exactly one full quotient producer-consumer pair");
 require(b.certified_lift_affines==25,"exactly25 eligible affine producers; first signed input retained");
 require(certified_nonnegative_bits.at(299)==24,"final public score range");
 auto scores=b.final_scores_nonnegative55(get(299));auto label=get(302);
 require(scores.size()==1000&&label.size()==1,"all protected scores and class");scores.push_back(label[0]);return scores;
}
}
