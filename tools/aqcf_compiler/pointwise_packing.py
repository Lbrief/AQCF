"""Exact owner-side pointwise repacking; no inference or networking."""
from pathlib import Path
import hashlib,json,struct,array,itertools
N=8192

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def words(f,*v):f.write(struct.pack('<'+'Q'*len(v),*v))

def take(f,n):
    b=f.read(8*n);assert len(b)==8*n
    return struct.unpack('<'+'Q'*n,b)

def rows(f,n,cols):
    b=f.read(n*cols*8);assert len(b)==n*cols*8
    return list(struct.iter_unpack('<'+'q'*cols,b))

def emit(f,rs):
    a=array.array('q',itertools.chain.from_iterable(rs));assert a.itemsize==8
    import sys
    assert sys.byteorder=='little';f.write(a.tobytes())

def read_program(path):
    layers=[]
    with Path(path).open('rb') as f:
        assert f.read(8)==b'AQCFGPS1'
        head=take(f,4);assert head[0:2]==(1,N) and head[3]==26
        for aid in range(26):
            ident,ni,no,nm,nq=take(f,5);assert ident==aid
            maps=[rows(f,take(f,1)[0],3) for _ in range(nm)];outputs=[]
            for _ in range(nq):
                ne,n=take(f,2);edges=[]
                for _ in range(ne):
                    index,count=take(f,2);edges.append((index,rows(f,count,2)))
                outputs.append((edges,rows(f,n,3)))
            layers.append(dict(id=aid,inputs=ni,outputs=no,maps=maps,out=outputs))
        assert not f.read(1)
    return head,layers

def extract_pointwise(l,ci,co,s):
    maps=[]
    for a in l['maps']:
        m={}
        for source,exponent,mul in a:
            assert mul==1 and exponent not in m;m[exponent]=source
        maps.append(m)
    weights=[None]*co;bias=[None]*co;sites={0,s//2,s-1};seen=set()
    for edges,qs in l['out']:
        for dest,qe,bb in qs:
            channel,site=divmod(dest,s)
            if site not in sites:continue
            row=[0]*ci;used=set()
            for index,coeff in edges:
                m=maps[index]
                for exp,value in coeff:
                    a=(qe-exp)%N;source=m.get(a)
                    if source is None:continue
                    ch,position=divmod(source,s)
                    assert position==site and ch not in used
                    used.add(ch);row[ch]=value*(-1 if a+exp>=N else 1)
            if weights[channel] is None:
                weights[channel]=row;bias[channel]=bb
            else:assert weights[channel]==row and bias[channel]==bb
            assert (channel,site) not in seen;seen.add((channel,site))
    assert len(seen)==co*len(sites)
    return weights,bias

def repack(l,p,ci,co,s):
    weights,bias=extract_pointwise(l,ci,co,s)
    b,j,f=[p[k] for k in ('b','J','F')];assert f*j*b<=N and N%b==0
    maps=[];outs=[]
    for start in range(0,s,b):
        valid=min(b,s-start);offset=len(maps)
        for c in range(0,ci,j):
            maps.append([(ch*s+start+t,(ch-c)*b+t,1) for ch in range(c,min(ci,c+j)) for t in range(valid)])
        for o in range(0,co,f):
            edges=[]
            for group,c in enumerate(range(0,ci,j)):
                coeff=[(((out-o)*j+j-1-(ch-c))*b,weights[out][ch])
                       for out in range(o,min(co,o+f)) for ch in range(c,min(ci,c+j)) if weights[out][ch]!=0]
                if coeff:edges.append((offset+group,sorted(coeff)))
            qs=[(out*s+start+t,((out-o)*j+j-1)*b+t,bias[out]) for out in range(o,min(co,o+f)) for t in range(valid)]
            outs.append((edges,qs))
    assert len(maps)==p['input_CT'] and len(outs)==p['output_CT']
    assert sum(len(e) for e,q in outs)==p['PM'],'zero edge: preserve evidence for separate admission'
    return dict(l,maps=maps,out=outs)

def write_program(head,layers,program,layout):
    counts=dict(input_CT=0,output_CT=0,PM=0);norms=[]
    with Path(program).open('xb') as pf,Path(layout).open('xb') as lf:
        pf.write(b'AQCFGPS1');lf.write(b'AQCFCL01')
        for f in (pf,lf):words(f,*head)
        for l in layers:
            maps,outs=l['maps'],l['out'];seen=bytearray(l['outputs']);largest=0
            for f in (pf,lf):words(f,l['id'],l['inputs'],l['outputs'],len(maps),len(outs))
            for m in maps:
                assert m and len(set(t[1] for t in m))==len(m)
                assert all(0<=a<l['inputs'] and 0<=e<N and v==1 for a,e,v in m)
                for f in (pf,lf):words(f,len(m));emit(f,m)
            for edges,q in outs:
                words(pf,len(edges),len(q));words(lf,len(q));norm=0
                assert len(set(i for i,a in edges))==len(edges)
                for i,a in edges:
                    assert 0<=i<len(maps) and len(set(e for e,v in a))==len(a)
                    assert all(0<=e<N and v!=0 for e,v in a)
                    norm+=sum(abs(v) for e,v in a);words(pf,i,len(a));emit(pf,a)
                assert norm<=1<<30,'unchanged deterministic plaintext L1 premise'
                largest=max(largest,norm)
                assert len(set(e for d,e,bb in q))==len(q)
                for d,e,bb in q:
                    assert 0<=d<l['outputs'] and 0<=e<N and not seen[d];seen[d]=1
                emit(pf,q);emit(lf,[(d,e) for d,e,bb in q]);counts['PM']+=len(edges)
            assert all(seen);norms.append(largest)
            counts['input_CT']+=len(maps);counts['output_CT']+=len(outs)
    return dict(totals=counts,max_output_PT_L1_by_layer=norms)
