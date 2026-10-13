// Matched public-only transport instrumentation; no cryptographic operation.
// All linked SCI NetIO sockets use fwrite. Each process is one role.
#include <cstdio>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <cerrno>
#include <climits>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
extern "C" size_t __real_fwrite(const void*,size_t,size_t,FILE*);
namespace {
constexpr std::uint64_t rate=6250000,segment=16384;
bool paced_profile(){
 static const bool enabled=[](){const char* p=std::getenv("AQCF_EXECUTION_PROFILE170");if(!p)std::abort();if(!std::strcmp(p,"LOCAL50"))return true;if(!std::strcmp(p,"CLOUD_PUBLIC")||!std::strcmp(p,"WAN_NATIVE"))return false;std::abort();}();
 return enabled;
}
std::mutex gate;
std::uint64_t next_deadline=0;
std::atomic<std::uint64_t> socket_bytes{0},socket_calls{0},socket_chunks{0},wait_ns{0};
std::uint64_t now(){timespec t{};if(clock_gettime(CLOCK_MONOTONIC_RAW,&t))std::abort();return std::uint64_t(t.tv_sec)*1000000000+t.tv_nsec;}
void reserve(std::uint64_t n){
 std::uint64_t deadline;
 {std::lock_guard<std::mutex> lock(gate);next_deadline=std::max(next_deadline,now())+(n*1000000000+rate-1)/rate;deadline=next_deadline;}
 auto began=now();
 for(;;){auto t=now();if(t>=deadline)break;auto remaining=deadline-t;timespec pause{time_t(remaining/1000000000),long(remaining%1000000000)};while(nanosleep(&pause,&pause)&&errno==EINTR){} }
 wait_ns+=now()-began;
}
}

extern "C" size_t __real_fread(void*,size_t,size_t,FILE*);
extern "C" int __real_fflush(FILE*);
namespace {
bool socket_stream161(FILE* f){
 if(!f)return false;
 struct stat st{};int fd=fileno(f);
 return fd>=0 && fstat(fd,&st)==0 && S_ISSOCK(st.st_mode);
}
[[noreturn]] void failure161(const char* op,FILE* f,size_t wanted,size_t got,int error){
 const int eof=f?feof(f):0, bad=f?ferror(f):0;
 dprintf(STDERR_FILENO,"AQCF_IO161_FATAL op=%s requested=%zu completed=%zu errno=%d eof=%d error=%d\n",op,wanted,got,error,eof,bad);
 _exit(74);
}
size_t checked_fwrite161(const void* p,size_t size,size_t count,FILE* f){
 errno=0;size_t got=__real_fwrite(p,size,count,f);int e=errno;
 if(got!=count)failure161("fwrite",f,count,got,e);
 return got;
}
}
extern "C" size_t __wrap_fread(void* p,size_t size,size_t count,FILE* f){
 if(!size||!count||!socket_stream161(f))return __real_fread(p,size,count,f);
 errno=0;size_t got=__real_fread(p,size,count,f);int e=errno;
 if(got!=count)failure161("fread",f,count,got,e);
 return got;
}
extern "C" int __wrap_fflush(FILE* f){
 const bool guarded=socket_stream161(f);errno=0;int result=__real_fflush(f);int e=errno;
 if(guarded && result!=0)failure161("fflush",f,0,0,e);
 return result;
}

extern "C" size_t __wrap_fwrite(const void* ptr,size_t size,size_t count,FILE* stream){
 if(!size||!count)return __real_fwrite(ptr,size,count,stream);
 struct stat st{};int fd=fileno(stream);if(fd<0||fstat(fd,&st)||!S_ISSOCK(st.st_mode))return __real_fwrite(ptr,size,count,stream);
 // Original NetIO always sends byte elements. Fail rather than silently bypass.
 if(size!=1||count>INT_MAX)std::abort();++socket_calls;
 if(!paced_profile()){auto sent=checked_fwrite161(ptr,size,count,stream);socket_bytes+=sent;++socket_chunks;return sent;}
 const auto* bytes=static_cast<const unsigned char*>(ptr);size_t done=0;
 while(done<count){size_t n=std::min<size_t>(segment,count-done);reserve(n);auto sent=checked_fwrite161(bytes+done,1,n,stream);socket_bytes+=sent;++socket_chunks;done+=sent;if(sent<n)break;}
 return done;
}
extern "C" std::uint64_t aqcf_pacer036_bytes(){return socket_bytes.load();}
extern "C" std::uint64_t aqcf_pacer036_calls(){return socket_calls.load();}
extern "C" std::uint64_t aqcf_pacer036_chunks(){return socket_chunks.load();}
extern "C" std::uint64_t aqcf_pacer036_wait_ns(){return wait_ns.load();}

extern "C" std::uint64_t aqcf_profile170_rate(){return paced_profile()?rate:0;}
