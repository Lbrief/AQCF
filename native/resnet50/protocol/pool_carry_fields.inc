#include "resnet50/protocol/pool_carry.h"
#include "resnet50/protocol/pool_radix_math.hpp"
struct Pool263Bank {
 int sender=0,rows=0;
 std::vector<uint8_t> pads,k0,k1,alpha,R,r,D;
 ~Pool263Bank(){for(auto* v:{&pads,&k0,&k1,&alpha,&R,&r,&D})if(!v->empty())OPENSSL_cleanse(v->data(),v->size());}
};
