#include "resnet50/engine/native_worker_runtime.hpp"
#pragma once
// Exact readers for the EXISTING AQCFGPS1 / AQCFCL01 integer payloads.
// The stored header remains P; a separate runtime profile chooses P or Ring55.
#include "resnet50/core/types.hpp"
#include "resnet50/core/pair_profile.hpp"
#include <openssl/evp.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
namespace aqcf3 {
using aqcf2::U; using aqcf2::I; using aqcf2::W; using aqcf2::P;
using aqcf2::require;
// Isolated MobileNetV2 port; frozen original core remains unchanged.
constexpr std::size_t DEGREE=8192, AFFINES=72;
inline bool digest_string(const std::string& h) {
    return h.size()==64 && h.find_first_not_of("0123456789abcdef")==std::string::npos;
}
inline std::string sha256(const std::vector<unsigned char>& bytes) {
    unsigned char out[EVP_MAX_MD_SIZE]{}; unsigned n=0;
    auto* c=EVP_MD_CTX_new(); require(c!=nullptr,"digest allocation");
    const bool ok=EVP_DigestInit_ex(c,EVP_sha256(),nullptr)==1 &&
      EVP_DigestUpdate(c,bytes.data(),bytes.size())==1 && EVP_DigestFinal_ex(c,out,&n)==1;
    EVP_MD_CTX_free(c); require(ok && n==32,"digest operation");
    std::ostringstream s; s<<std::hex<<std::setfill('0');
    for(unsigned j=0;j<n;++j){s<<std::setw(2)<<unsigned(out[j]);}
    return s.str();
}
inline std::vector<unsigned char> bound_bytes(const std::filesystem::path& p,
        const std::string& hash, std::size_t maximum) {
    require(p.is_absolute() && std::filesystem::is_regular_file(p) &&
            !std::filesystem::is_symlink(p) && digest_string(hash),"asset path/digest");
    const auto size=std::filesystem::file_size(p); require(size>0 && size<=maximum,"asset length");
    std::vector<unsigned char> b(size); std::ifstream f(p,std::ios::binary);
    f.read(reinterpret_cast<char*>(b.data()),std::streamsize(b.size()));
    require(std::size_t(f.gcount())==b.size() && f.peek()==std::char_traits<char>::eof(),"asset complete read");
    require(sha256(b)==hash,"asset commitment mismatch"); return b;
}
class Reader {
    const std::vector<unsigned char>& b_; std::size_t at_=0;
public:
    explicit Reader(const std::vector<unsigned char>& b):b_(b){}
    Reader(const std::vector<unsigned char>& b,std::size_t start):b_(b),at_(start){require(start<=b.size(),"reader start");}
    std::size_t position()const{return at_;}
    void skip_words(U n){require(n<=(b_.size()-at_)/8,"bounded framing skip");at_+=std::size_t(n)*8;}
    void end_at(std::size_t end){require(at_==end,"exact layer packet consumption");}
    U word(){ require(at_<=b_.size() && b_.size()-at_>=8,"truncated integer packet");
        U u=0;for(unsigned j=0;j<8;++j)u|=U(b_[at_++])<<(8*j);return u; }
    I signed_word(){U u=word();I v;std::memcpy(&v,&u,sizeof(v));return v;}
    void magic(const char* expected){require(b_.size()-at_>=8 && std::memcmp(b_.data()+at_,expected,8)==0,"packet magic");at_+=8;}
    void end(){require(at_==b_.size(),"trailing packet bytes");}
    U bounded(U maximum,const char* why){U n=word();require(n<=maximum,why);return n;}
};
struct Term { U source, exponent; I multiplier; };
struct Query { U destination, exponent; I bias=0; };
struct Coefficient { U exponent; I value; };
struct Edge { U input; std::vector<Coefficient> coefficients; };
struct Output { std::vector<Edge> edges; std::vector<Query> queries; };
struct Layout { U id=0,inputs=0,outputs=0; std::vector<std::vector<Term>> maps;
                std::vector<std::vector<Query>> queries; };
struct Layer { Layout layout; std::vector<Output> output; };
using Layouts=std::array<Layout,AFFINES>;
using Program=std::array<Layer,AFFINES>;
inline U magnitude(I x){return U(x<0?-W(x):W(x));}
inline std::vector<Term> read_map(Reader& r,U inputs) {
    U n=r.bounded(64*DEGREE,"map term cap"); std::vector<Term> map; map.reserve(n);
    std::array<U,DEGREE> mass{}; std::pair<U,U> previous{};
    for(U j=0;j<n;++j){Term t{r.word(),r.word(),r.signed_word()};
        require(t.source<inputs && t.exponent<DEGREE && t.multiplier!=0 &&
                t.multiplier>=-16 && t.multiplier<=16,"signed map term");
        auto key=std::make_pair(t.exponent,t.source); require(j==0 || previous<key,"map canonical order");
        previous=key;mass[t.exponent]+=magnitude(t.multiplier);
        require(mass[t.exponent]<=64,"map multiplier mass");map.push_back(t);}
    return map;
}
inline void base_header(Reader& r,const char* magic) {
    r.magic(magic);require(r.word()==1 && r.word()==DEGREE && r.word()==P && r.word()==AFFINES,
      "original integer packet version/N/P/layer count");
}
struct LayerFrame4 { std::size_t begin,end; U id; };
inline std::array<LayerFrame4,AFFINES> layer_frames4(const std::vector<unsigned char>& bytes,bool model){
 Reader r(bytes);base_header(r,model?"AQCFGPS1":"AQCFCL01");std::array<LayerFrame4,AFFINES> frames;std::array<bool,AFFINES> seen{};
 for(U j=0;j<AFFINES;++j){const auto begin=r.position();const U id=r.word();
  require(id<AFFINES&&!seen[id],"unique bounded layer identity");seen[id]=true;r.skip_words(2);
  U ni=r.bounded(DEGREE,"frame input cap"),no=r.bounded(DEGREE,"frame output cap");require(ni&&no,"nonempty frame");
  for(U k=0;k<ni;++k){U n=r.bounded(64*DEGREE,"frame map cap");r.skip_words(3*n);}
  U edges=0;
  for(U k=0;k<no;++k){
   if(model){U ne=r.bounded(ni,"frame edge cap"),nq=r.bounded(DEGREE,"frame query cap");edges+=ne;require(edges<=32768&&nq,"frame resources");
    for(U v=0;v<ne;++v){r.skip_words(1);U nnz=r.bounded(DEGREE,"frame coefficient cap");r.skip_words(2*nnz);}r.skip_words(3*nq);
   }else{U nq=r.bounded(DEGREE,"frame public query cap");require(nq,"nonempty public queries");r.skip_words(2*nq);}
  }frames[j]={begin,r.position(),id};
 }r.end();return frames;
}
inline Layouts parse_layout(const std::vector<unsigned char>& bytes) {
    const auto frames=layer_frames4(bytes,false);Layouts all;
    pcn_data089::each("AQCF_OWNER_PARSE_LAYOUT",AFFINES,[&](std::size_t j){Reader r(bytes,frames[j].begin);Layout l;l.id=r.word();l.inputs=r.word();l.outputs=r.word();
        U ni=r.bounded(DEGREE,"input CT cap"),no=r.bounded(DEGREE,"output CT cap");
        require(l.id<AFFINES && l.id==frames[j].id && l.inputs>0 && l.inputs<=1048576 &&
          l.outputs>0 && l.outputs<=1048576 && ni>0 && no>0,"public layer header"); 
        for(U k=0;k<ni;++k)l.maps.push_back(read_map(r,l.inputs));
        std::vector<bool> used(l.outputs,false);
        for(U k=0;k<no;++k){U nq=r.bounded(DEGREE,"query cap");require(nq>0,"empty query block");
            std::array<bool,DEGREE> exps{};std::vector<Query> q;
            for(U v=0;v<nq;++v){Query a{r.word(),r.word(),0};
                require(a.destination<l.outputs && a.exponent<DEGREE && !used[a.destination] &&
                    !exps[a.exponent],"query partition"); used[a.destination]=true;exps[a.exponent]=true;q.push_back(a);}
            l.queries.push_back(std::move(q));}
        require(std::all_of(used.begin(),used.end(),[](bool v){return v;}),"incomplete output partition");
        all[l.id]=std::move(l);r.end_at(frames[j].end);});
    return all;
}
inline Program parse_program(const std::vector<unsigned char>& bytes) {
    const auto frames=layer_frames4(bytes,true);Program all;
    pcn_data089::each("AQCF_OWNER_PARSE_PROGRAM",AFFINES,[&](std::size_t j){Reader r(bytes,frames[j].begin);Layer z;auto& l=z.layout;l.id=r.word();l.inputs=r.word();l.outputs=r.word();
        U ni=r.bounded(DEGREE,"private input CT cap"),no=r.bounded(DEGREE,"private output CT cap");
        require(l.id<AFFINES && l.id==frames[j].id && l.inputs>0 && l.inputs<=1048576 &&
          l.outputs>0 && l.outputs<=1048576 && ni>0 && no>0,"private layer header");
        for(U k=0;k<ni;++k)l.maps.push_back(read_map(r,l.inputs));
        std::vector<bool> used(l.outputs,false);U total_edges=0;
        for(U k=0;k<no;++k){Output o;U ne=r.bounded(ni,"edge count"),nq=r.bounded(DEGREE,"private query count");
            total_edges+=ne;require(total_edges<=32768 && nq>0,"edge/query resource cap");
            std::vector<bool> input_seen(ni,false);U norm=0;
            for(U v=0;v<ne;++v){Edge e;e.input=r.word();U nnz=r.bounded(DEGREE,"coefficient count");
                require(e.input<ni && !input_seen[e.input] && nnz>0,"private edge");input_seen[e.input]=true;
                std::array<bool,DEGREE> exps{};e.coefficients.reserve(nnz);
                for(U w=0;w<nnz;++w){Coefficient a{r.word(),r.signed_word()};
                    U m=magnitude(a.value);require(a.exponent<DEGREE && !exps[a.exponent] && a.value!=0 && m<P/2,
                      "private coefficient");exps[a.exponent]=true;
                    require(m<=(U(1)<<30)-norm,"private output PT norm cap");norm+=m;e.coefficients.push_back(a);}
                o.edges.push_back(std::move(e));}
            std::array<bool,DEGREE> exps{};
            for(U v=0;v<nq;++v){Query a{r.word(),r.word(),r.signed_word()};
                require(a.destination<l.outputs && a.exponent<DEGREE && !used[a.destination] &&
                  !exps[a.exponent] && magnitude(a.bias)<P/2,"private query/bias");
                used[a.destination]=true;exps[a.exponent]=true;o.queries.push_back(a);}
            l.queries.push_back(o.queries);z.output.push_back(std::move(o));}
        require(std::all_of(used.begin(),used.end(),[](bool v){return v;}),"private query partition");all[l.id]=std::move(z);r.end_at(frames[j].end);});
    return all;
}
inline void match_program(const Program& p,const Layouts& ls) {
    pcn_data089::each("AQCF_OWNER_MATCH",AFFINES,[&](std::size_t id){const auto& a=p[id].layout;const auto& b=ls[id];
        require(a.id==b.id && a.inputs==b.inputs && a.outputs==b.outputs &&
          a.maps.size()==b.maps.size() && a.queries.size()==b.queries.size(),"program/public geometry mismatch");
        for(std::size_t i=0;i<a.maps.size();++i){require(a.maps[i].size()==b.maps[i].size(),"map size mismatch");
            for(std::size_t j=0;j<a.maps[i].size();++j){auto x=a.maps[i][j],y=b.maps[i][j];
                require(x.source==y.source && x.exponent==y.exponent && x.multiplier==y.multiplier,"signed map mismatch");}}
        for(std::size_t i=0;i<a.queries.size();++i){require(a.queries[i].size()==b.queries[i].size(),"query size mismatch");
            for(std::size_t j=0;j<a.queries[i].size();++j){auto x=a.queries[i][j],y=b.queries[i][j];
                require(x.destination==y.destination && x.exponent==y.exponent,"query map mismatch");}}
    });
}
struct Census { U input_ct=0,output_ct=0,pm=0,ct_add=0; };
inline Census census(const Program& p) { Census c;
    for(const auto& l:p){c.input_ct+=l.layout.maps.size();c.output_ct+=l.output.size();
        for(const auto& o:l.output){c.pm+=o.edges.size();c.ct_add+=o.edges.empty()?0:o.edges.size()-1;}}
    return c;
}
inline std::vector<U> gather(const Layout& l,std::size_t index,const std::vector<U>& own,U modulus) {
    require(own.size()==l.inputs && index<l.maps.size(),"gather public shape");
    std::vector<U> out(DEGREE);const auto& map=l.maps[index];
    pcn_context108::ranges("AQCF_GATHER_COEFFICIENTS",DEGREE,[&](std::size_t lo,std::size_t hi){
      auto it=std::lower_bound(map.begin(),map.end(),lo,[](const Term& t,std::size_t e){return t.exponent<e;});
      for(auto j=lo;j<hi;++j){W sum=0;for(;it!=map.end()&&it->exponent==j;++it){
        require(own[it->source]<modulus,"canonical owned gather input");sum+=W(own[it->source])*it->multiplier;}
        out[j]=aqcf2::residue(sum,modulus);}
    });return out;
}
inline std::vector<I> read_i64_asset(const std::filesystem::path& path,const std::string& hash,std::size_t count) {
    auto bytes=bound_bytes(path,hash,count*8);require(bytes.size()==count*8,"exact owned tensor size");
    std::vector<I> x(count);pcn_data089::each("AQCF_OWNER_I64",count,[&](std::size_t j){Reader r(bytes,8*j);x[j]=r.signed_word();r.end_at(8*(j+1));});return x;
}
} // namespace aqcf3
