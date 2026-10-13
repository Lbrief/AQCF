#include "resnet50/engine/bounded_fma_accumulator.hpp"
namespace dot500_kernel {
void* create(const seal::SEALContext& c,const seal::Ciphertext& b){return new dot499::Accumulator(c,b);}
void destroy(void* p) noexcept{delete static_cast<dot499::Accumulator*>(p);}
void add(void* p,const seal::Ciphertext& c,const seal::Plaintext& b){static_cast<dot499::Accumulator*>(p)->add(c,b);}
seal::Ciphertext finish(void* p){return static_cast<dot499::Accumulator*>(p)->finish();}
std::size_t terms(const void* p){return static_cast<const dot499::Accumulator*>(p)->terms;}
}
