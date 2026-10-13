#pragma once
// Bounded engineering buffering for the isolated native backend callbacks.
// Payload bytes/order and explicit native flushes are preserved. No read-ahead.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <stdexcept>
namespace aqcf_native_stream {
class Writer {
  static constexpr std::size_t capacity=16384;
  std::array<unsigned char,capacity> bytes;
  std::size_t used=0;
public:
  template<class Sink> void drain(Sink& sink) {
    if(used){sink.send(sink.context,bytes.data(),static_cast<int>(used));used=0;}
  }
  template<class Sink> void send(Sink& sink,const void* data,int length) {
    if(length<0)throw std::invalid_argument("negative native stream send");
    if(!length){sink.send(sink.context,data,0);return;}
    if(!data)throw std::invalid_argument("null native stream send");
    const auto* p=static_cast<const unsigned char*>(data);
    std::size_t remaining=static_cast<std::size_t>(length);
    while(remaining){
      if(!used && remaining>=capacity){
        // Already-large sends need no extra copy or delay.
        sink.send(sink.context,p,static_cast<int>(remaining));return;
      }
      const std::size_t n=std::min(capacity-used,remaining);
      std::memcpy(bytes.data()+used,p,n);used+=n;p+=n;remaining-=n;
      if(used==capacity)drain(sink);
    }
  }
  bool empty()const{return used==0;}
};
}
