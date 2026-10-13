// Included inside BfvProgram. One instance is one request; never serialized.
bool preparing18=false, prepared18=false;
bool correction341_enabled=false;U correction341_pw_calls=0,correction341_bn_calls=0,correction341_pw_rows=0,correction341_bn_rows=0;
std::function<std::vector<U>(unsigned,const std::vector<U>&)> correction28;
std::function<void()> initialize28;
std::function<bool(unsigned)> selected28;
bool selected213(unsigned id)const{return aqcf_input213::enabled()&&selected28&&selected28(id);}
std::function<U(unsigned)> count28;
std::function<U(unsigned,U)> size28;
std::function<std::vector<U>(unsigned,const std::vector<U>&)> produce28;
std::function<void(unsigned,std::vector<U>&,std::vector<std::vector<unsigned char>>&)> decode28;
U prepared_count18=0, consumed_count18=0, correction_bytes18=0, queued_bytes18=0;
std::array<bool,AFFINES> consumed18{};
std::array<std::vector<U>,AFFINES> masks18, phases18;
std::array<U,2> request18{};
static constexpr U queue_cap18=U(1)<<30;
void flush18(){
    if(!preparing18){s.flush();return;}
    s.io.io->flush();
    if(s.party==1)s.io.io_rev->flush(); // No second thread owns either server object.
}
sci::NetIO* output18(){return preparing18&&s.party==1?s.io.io_rev:s.io.io;}
template<class F> void work18(U n,const char* label,F f){
    require(n>=4,"four nonempty online18 workers");
    std::array<std::exception_ptr,4> errors;std::vector<aqcf_owner4::Task> th;
    for(unsigned k=0;k<4;++k)th.emplace_back(k,[&,k]{try{
        cpu_set_t mask;CPU_ZERO(&mask);CPU_SET(aqcf_native4::cpu(k),&mask);
        require(!sched_setaffinity(0,sizeof(mask),&mask),"online18 worker affinity");
        aqcf_native4::Outer audit(k,label);
        U lo=n*k/4,hi=n*(k+1)/4;f(lo,hi,k);audit.jobs=hi-lo;
    }catch(...){errors[k]=std::current_exception();}});
    for(auto&t:th)t.join();for(auto&e:errors)if(e)std::rethrow_exception(e);
}
void wipe18(std::vector<U>& v){
    if(v.size()>=4)work18(v.size(),"LINEAR18_WIPE",[&](U lo,U hi,unsigned){OPENSSL_cleanse(v.data()+lo,(hi-lo)*sizeof(U));});
    else if(!v.empty())OPENSSL_cleanse(v.data(),v.size()*sizeof(U));
    std::vector<U>().swap(v);
}
~BfvProgram(){
    // Destruction is cleanup, never a protocol retry. Avoid throwing/thread creation.
    for(auto&v:masks18)if(!v.empty())OPENSSL_cleanse(v.data(),v.size()*sizeof(U));
    for(auto&v:phases18)if(!v.empty())OPENSSL_cleanse(v.data(),v.size()*sizeof(U));
    for(auto& o:owners19){if(!o.weight.empty())OPENSSL_cleanse(o.weight.data(),o.weight.size()*sizeof(I));if(!o.bias.empty())OPENSSL_cleanse(o.bias.data(),o.bias.size()*sizeof(I));}
}
// Only the reverse-channel IO owner calls this. Native NetIO's zero-byte EOF
// loop is not used here: loss of the protected stream is fatal, never retried.
void reverse_read18(void* dst,U n){
    auto* io=s.io.io_rev;
    if(io->last_call!=LastCall::Recv){++io->num_rounds;io->last_call=LastCall::Recv;}
    require(!io->has_sent,"reverse channel flushed before unique reader ownership");
    U at=0;while(at<n){auto got=std::fread(static_cast<unsigned char*>(dst)+at,1,n-at,io->stream);
        require(got>0,"online18 protected frame EOF/error");at+=got;}
}
void prepare_all18(){
    require(!preparing18&&!prepared18&&frame_counter_started4,"online18 only after genuine first affine frame");
    if(s.party==2)require(service_started,"all correlation cost inside online timer");
    initialize19(); if(initialize28)initialize28(); // Additional setup is online, before assigning the reverse reader.
    U bounded=0;for(unsigned id=1;id<AFFINES;++id)for(U group=0;group<return_count19(id);++group){
        U n=return_size19(id,group);require(n<=queue_cap18-bounded,"prospective protected queue bound");bounded+=n;}
    require(bounded>0&&bounded<=queue_cap18,"bounded complete protected queue");
    s.flush();s.io.io_rev->has_sent=false;
    if(s.party==2){require(RAND_bytes(reinterpret_cast<unsigned char*>(request18.data()),sizeof(request18))==1,"fresh request nonce");
        number(request18[0]);number(request18[1]);}
    else{request18[0]=number();request18[1]=number();}
    preparing18=true;auto begin=Steady::now();const bool old_defer=defer_raw_phase;defer_raw_phase=true;
    std::array<std::vector<std::vector<unsigned char>>,AFFINES> frames;
    std::exception_ptr reader_error;U read_bytes=0;std::thread reader;
    if(s.party==2)reader=std::thread([&]{trace037::event("REVERSE_IO_ENTER");try{
        for(unsigned id=1;id<AFFINES;++id){U got_id,no;reverse_read18(&got_id,8);reverse_read18(&no,8);read_bytes+=16;
            require(got_id==id&&no==return_count19(id),"prepared return affine/roster identity");frames[id].resize(no);
            for(U j=0;j<no;++j){U n;reverse_read18(&n,8);read_bytes+=8;require(n==return_size19(id,j),"prepared return exact size");
                require(n<=queue_cap18-queued_bytes18,"bounded actual queue");frames[id][j].resize(n);reverse_read18(frames[id][j].data(),n);
                queued_bytes18+=n;read_bytes+=n;}
        }
    }catch(...){reader_error=std::current_exception();::shutdown(s.io.io->consocket,SHUT_RDWR);::shutdown(s.io.io_rev->consocket,SHUT_RDWR);}
        trace037::event("REVERSE_IO_EXIT",-1,-1,nullptr,read_bytes);});
    try{
        for(unsigned id=1;id<AFFINES;++id){
            std::vector<U> own(layouts[id].inputs,0);
            if(s.party==2&&!(selected191(id)||selected213(id)))work18(own.size(),"LINEAR18_FRESH_MASK",[&](U lo,U hi,unsigned){
                auto rng=seal::Blake2xbPRNGFactory().create();rng->generate((hi-lo)*sizeof(U),reinterpret_cast<seal::seal_byte*>(own.data()+lo));
                for(U j=lo;j<hi;++j)own[j]&=modulus-1;});
            auto phase=direct_affine(id,own);
            if(s.party==2){if(!(selected191(id)||selected213(id)))masks18[id]=std::move(own);else require(masks18[id].size()==own.size(),"191 or213 actual programmed input mask");}else phases18[id]=std::move(phase);
            ++prepared_count18;
            std::cout<<"LINEAR18_PREPARE "<<s.party<<" "<<id<<"\n"<<std::flush;
        }
    }catch(...){::shutdown(s.io.io->consocket,SHUT_RDWR);::shutdown(s.io.io_rev->consocket,SHUT_RDWR);if(reader.joinable())reader.join();preparing18=false;defer_raw_phase=old_defer;throw;}
    if(reader.joinable())reader.join();if(reader_error)std::rethrow_exception(reader_error);
    if(s.party==2){
        require(queued_bytes18==bounded,"complete bounded protected queue");received_bytes+=read_bytes;
        for(unsigned id=1;id<AFFINES;++id){phases18[id].resize(layouts[id].outputs);if(selected28&&selected28(id))decode28(id,phases18[id],frames[id]);else if(auto g=aqcf_bn19::find(id))decode_bn19(*g,phases18[id],frames[id]);else receive_return_pipeline(layouts[id],phases18[id],&frames[id]);}
    }
    preparing18=false;defer_raw_phase=old_defer;prepared18=true;s.flush();
    // Join-before-native-OT: one public completion handshake, inside online time.
    if(s.party==2){number(0x1839);require(number()==0x1839,"prepare completion barrier");}
    else{require(number()==0x1839,"prepare completion barrier");number(0x1839);}
    metrics.seconds["linear18_online_all_preparation_wall"]+=elapsed(begin);
    std::cout<<"LINEAR18_PREPARED "<<s.party<<" "<<prepared_count18<<" "<<queued_bytes18<<" "<<elapsed(begin)<<"\n"<<std::flush;
}
// Original owner coefficients, signs, gather maps, query scatter and public
// stride/spatial/short-weight transforms. No bias, no changed HE producer.
std::vector<U> clear_owner18(unsigned id,const std::vector<U>& v){
    require(s.party==1&&program&&v.size()==layouts[id].inputs,"owner-only linear correction");
    if(correction341_enabled){
        auto begin341=Steady::now();
        auto work341=[&](U n,const char* label,auto f){work18(n,label,f);};
        if(auto g=aqcf_bn19::find(id)){
            auto out=correction341::batchnorm(*g,owners19[id],v,work341);
            ++correction341_bn_calls;correction341_bn_rows+=out.size();
            metrics.seconds["correction341_BN_exact_local_wall"]+=elapsed(begin341);return out;
        }
        if(selected28&&selected28(id)){
            require(bool(correction28),"341 bound original dense owner");
            auto out=correction28(id,v);++correction341_pw_calls;correction341_pw_rows+=out.size();
            metrics.seconds["correction341_pointwise_exact_local_wall"]+=elapsed(begin341);return out;
        }
    }
    const auto&l=layouts[id];const auto&owner=program->at(id);
    const auto&cd=*context->first_context_data();
    std::vector<std::vector<U>> input(l.maps.size());
    work18(input.size(),"LINEAR18_CLEAR_INPUT",[&](U lo,U hi,unsigned){
        for(U i=lo;i<hi;++i){auto a=gather(l,i,v,modulus);input[i].resize(data_limbs*DEGREE);
            for(unsigned r=0;r<data_limbs;++r){std::copy(a.begin(),a.end(),input[i].begin()+r*DEGREE);
                seal::util::ntt_negacyclic_harvey(input[i].data()+r*DEGREE,cd.small_ntt_tables()[r]);}}
    });
    std::vector<U> out(l.outputs);
    const __uint128_t Q=__uint128_t(primes[0])*primes[1];
    const U inv=joint_crt_wire::powmod(primes[0],primes[1]-2,primes[1]);
    work18(owner.output.size(),"LINEAR18_CLEAR_OWNER",[&](U lo,U hi,unsigned){
        for(U oi=lo;oi<hi;++oi){const auto&o=owner.output[oi];std::vector<__uint128_t> sum(data_limbs*DEGREE,0);
            for(const auto&e:o.edges){std::vector<U> w;
                const auto wr=r50_stride(id,oi);const auto sr=r50_spatial(id,oi);
                if(wr.stride)w=stride_weight::spectrum(*context,e,wr.stride,wr.support,r50_public_tile_spacing(id,oi));
                else if(sr.ell)w=spatial_action::spectrum(*context,e,sr.ell,sr.row,sr.k,sr.channels,sr.unit);
                else w=tile_weight::spectrum(*context,e,r50_public_tile_spacing(id,oi));
                require(w.size()==data_limbs*DEGREE&&e.input<input.size(),"original owner spectrum shape");
                for(U j=0;j<w.size();++j)sum[j]+=__uint128_t(w[j])*input[e.input][j];
            }
            std::vector<U> a(data_limbs*DEGREE);
            for(unsigned r=0;r<data_limbs;++r){for(U j=0;j<DEGREE;++j)a[r*DEGREE+j]=U(sum[r*DEGREE+j]%primes[r]);
                seal::util::inverse_ntt_negacyclic_harvey(a.data()+r*DEGREE,cd.small_ntt_tables()[r]);}
            for(const auto&q:o.queries){U x=a[q.exponent],y=a[DEGREE+q.exponent];U d=(y+primes[1]-x)%primes[1];
                __uint128_t z=x+__uint128_t(primes[0])*U(__uint128_t(d)*inv%primes[1]);
                __int128_t signed_z=z>Q/2?__int128_t(z)-__int128_t(Q):__int128_t(z);
                out[q.destination]=U(signed_z)&(modulus-1); // Exact Ring37; original bias already in prepared phase.
            }
        }
    });
    return out;
}
std::vector<U> affine(int id,const std::vector<U>& own,int producer,int input_tensor,int output_tensor){
    require(id>=0&&id<int(AFFINES)&&!consumed18[id]&&own.size()==layouts[id].inputs,"single-use affine/input version");
    const auto&bind=linear18_roster::rows[id];
    require(producer==bind.node&&input_tensor==bind.input&&output_tensor==bind.output,"source-bound SSA tensor identity");
    pcn_data089::each("LINEAR18_INPUT_CHECK",own.size(),[&](std::size_t j){require(own[j]<modulus,"canonical original share");});
    consumed18[id]=true;
    if(id==0){auto out=direct_affine(id,own);prepare_all18();return out;}
    require(prepared18&&!preparing18&&requested[id]&&prepared_count18==AFFINES-1,"online preparation ready once");
    auto began=Steady::now();std::vector<U> correction191;std::vector<U> delta(own.size());std::array<std::vector<unsigned char>,4> packed;
    if(s.party==2){require(masks18[id].size()==own.size(),"single-use client mask present");
        work18(own.size(),"LINEAR18_CORRECTION_PACK",[&](U lo,U hi,unsigned k){CoefficientWriter w;
            for(U j=lo;j<hi;++j)w.append((own[j]-masks18[id][j])&(modulus-1),37);packed[k]=w.finish();});
        number(id);number(producer);number(input_tensor);number(request18[0]);number(request18[1]);
        for(unsigned k=0;k<4;++k){U n=packed[k].size();s.io.io->send_data(packed[k].data(),s.dim(n));sent_bytes+=n;correction_bytes18+=n;}s.flush();wipe18(masks18[id]);
    }else{
        require(number()==U(id)&&number()==U(producer)&&number()==U(input_tensor)&&number()==request18[0]&&number()==request18[1],"correction request/affine/SSA identity");
        for(unsigned k=0;k<4;++k){U n=((own.size()*(k+1)/4-own.size()*k/4)*37+7)/8;packed[k].resize(n);s.io.io->recv_data(packed[k].data(),s.dim(n));received_bytes+=n;correction_bytes18+=n;}
        work18(own.size(),"LINEAR18_CORRECTION_UNPACK",[&](U lo,U hi,unsigned k){CoefficientReader r(packed[k]);
            for(U j=lo;j<hi;++j)delta[j]=(own[j]+r.take(37,modulus))&(modulus-1);r.end();});
        auto correction=clear_owner18(id,delta);require(correction.size()==phases18[id].size(),"complete correction scatter");
        if(selected191(id))correction191=std::move(correction);else{
        work18(correction.size(),"LINEAR18_PHASE_SHIFT",[&](U lo,U hi,unsigned){for(U j=lo;j<hi;++j)phases18[id][j]=(phases18[id][j]-4*correction[j])&(4*modulus-1);});
        }
        wipe18(correction);wipe18(delta);
    }
    if(selected191(id)){require(bool(complete191),"191 bound late caller");if(s.party==2)correction191.resize(layouts[id].outputs,0);phases18[id]=complete191(id,correction191);wipe18(correction191);}
    auto out=std::move(phases18[id]);require(out.size()==layouts[id].outputs,"single-use cached phase");++consumed_count18;
    if(defer_raw_phase)raw_phase_rows+=out.size();else{auto t=Steady::now();recover_four(out);quarter_rows+=out.size();quarter_correction_s+=elapsed(t);metrics.seconds["quarter_2pc_recovery_all_costs"]+=elapsed(t);}
    metrics.seconds["linear18_correction_and_consumption_wall"]+=elapsed(began);
    std::cout<<"LINEAR18_CONSUME "<<s.party<<" "<<id<<" "<<consumed_count18<<" "<<elapsed(began)<<"\n"<<std::flush;
    return out;
}
