#pragma once
// Transport scheduling only. A ciphertext becomes readable ONLY after its
// fresh zero, complete coefficient mask and validity checks have finished.
#include <condition_variable>
#include <mutex>
#include <vector>
#include <stdexcept>
namespace aqcf3 {
class OrderedPublication {
 std::mutex mutex_;std::condition_variable cv_;std::vector<unsigned char> ready_;
 bool failed_=false;
public:
 explicit OrderedPublication(std::size_t n):ready_(n,0){}
 void publish(std::size_t i){
  {std::lock_guard<std::mutex> lock(mutex_);
   if(i>=ready_.size()||ready_[i])throw std::runtime_error("duplicate/out-of-range protected publication");
   ready_[i]=1;}
  cv_.notify_all();
 }
 void fail(){ {std::lock_guard<std::mutex> lock(mutex_);failed_=true;}cv_.notify_all(); }
 bool wait(std::size_t i){
  std::unique_lock<std::mutex> lock(mutex_);
  if(i>=ready_.size())throw std::runtime_error("publication read bounds");
  cv_.wait(lock,[&]{return failed_||ready_[i];});return !failed_;
 }
};
}
