#include <map>
struct Late37Bank {
 int sender=0,rows=0;
 std::vector<uint8_t> pads,k0,k1,choice,carry;
 ~Late37Bank(){for(auto* v:{&pads,&k0,&k1,&choice,&carry})if(!v->empty())OPENSSL_cleanse(v->data(),v->size());}
};
