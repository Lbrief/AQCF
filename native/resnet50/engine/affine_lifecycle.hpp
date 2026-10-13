#pragma once
#include <array>
#include <cstdint>
#include <mutex>
#include <stdexcept>
namespace aqcf_wave {
// Metadata only; no secret value or random generator is stored or exported.
// The production owner invokes events after actual send/decode completion.
class AffineLifecycle {
public:
 enum class Phase {fresh,input_sent,response_complete,ready,consumed,wiped,failed};
 struct Identity {std::uint64_t node,input,output;};
private:
 std::mutex lock;std::array<Phase,72> phase{};
 std::array<std::uint64_t,2> request{};bool bound=false,cancelled=false;
 void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
public:
 void bind(std::array<std::uint64_t,2> nonce){std::lock_guard<std::mutex> hold(lock);
  need(!bound&&!cancelled,"affine request binds once");request=nonce;bound=true;}
 void step(unsigned id,Phase from,Phase to){std::lock_guard<std::mutex> hold(lock);
  need(bound&&!cancelled&&id>0&&id<72,"affine live request/id");need(phase[id]==from,"affine one-use transition");
  need((from==Phase::fresh&&to==Phase::input_sent)||(from==Phase::input_sent&&to==Phase::response_complete)||
       (from==Phase::response_complete&&to==Phase::ready)||(from==Phase::ready&&to==Phase::consumed)||
       (from==Phase::consumed&&to==Phase::wiped),"affine legal transition");phase[id]=to;
 }
 void consume(unsigned id,std::array<std::uint64_t,2> nonce,Identity got,Identity expected){
  std::lock_guard<std::mutex> hold(lock);need(bound&&!cancelled&&id>0&&id<72,"affine consumption request/id");
  need(nonce==request&&got.node==expected.node&&got.input==expected.input&&got.output==expected.output,"affine nonce/SSA identity");
  need(phase[id]==Phase::ready,"affine ready and not previously consumed");phase[id]=Phase::consumed;
 }
 void fail() noexcept {std::lock_guard<std::mutex> hold(lock);cancelled=true;
  for(auto& p:phase)if(p!=Phase::wiped)p=Phase::failed;}
 bool complete(){std::lock_guard<std::mutex> hold(lock);if(!bound||cancelled)return false;
  for(unsigned i=1;i<72;++i)if(phase[i]!=Phase::wiped)return false;return true;}
};
}
