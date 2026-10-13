"""Exact layout emitter. No trained assets, encryption, socket or global cache.

Weights are supplied by the owner-side caller as W[o][c][ky][kx]; the owner
adapter must separately bind and reconcile every coefficient with the frozen
framework export. This module does not claim that reconciliation is already done.
"""
def build(aid,h,w,ci,co,g,weights,bias,spatial,n=8192):
    r,b,j,f=(g[k] for k in ('r','b','j','f'))
    assert r==w+1 and b&(b-1)==0 and f*j*b<=n and n%b==0
    assert (h-1)*r+w-1+2*r+2<b
    assert len(weights)==co and len(bias)==co
    size=h*w;maps=[];out=[]
    for first in range(0,ci,j):
        maps.append([(c*size+y*w+x,(j-1-(c-first))*b+y*r+x,1)
                     for c in range(first,min(ci,first+j)) for y in range(h) for x in range(w)])
    # Reversed channel planes require canonical (exponent,source) order.
    # Sorting changes serialization only, not a coefficient or source.
    maps=[sorted(row,key=lambda t:(t[1],t[0])) for row in maps]
    taps=[(a,z) for a in range(3) for z in range(3)] if spatial else [(0,0)]
    shift=r+1 if spatial else 0
    for first in range(0,co,f):
        edges=[]
        for group,chan in enumerate(range(0,ci,j)):
            coeff=[]
            for o in range(first,min(co,first+f)):
                assert len(weights[o])==ci
                for c in range(chan,min(ci,chan+j)):
                    for ky,kx in taps:
                        v=int(weights[o][c][ky][kx])
                        if v:coeff.append((((o-first)*j+c-chan)*b+((2-ky)*r+2-kx if spatial else 0),v))
            merged={}
            for exponent,value in coeff:merged[exponent]=merged.get(exponent,0)+value
            coeff=[(exponent,value) for exponent,value in merged.items() if value]
            assert len({e for e,v in coeff})==len(coeff) and all(0<=e<n for e,v in coeff)
            if coeff:edges.append((group,sorted(coeff)))
        qs=[(o*size+y*w+x,((o-first)*j+j-1)*b+y*r+x+shift,int(bias[o]))
            for o in range(first,min(co,first+f)) for y in range(h) for x in range(w)]
        assert len({e for d,e,v in qs})==len(qs)
        assert sum(abs(v) for _,coeff in edges for _,v in coeff)<=1<<30
        out.append((edges,qs))
    assert sorted(d for _,qs in out for d,e,v in qs)==list(range(co*size))
    assert all(len({e for s,e,m in row})==len(row) and all(0<=s<ci*size and 0<=e<n and m==1 for s,e,m in row) for row in maps)
    return dict(id=aid,inputs=ci*size,outputs=co*size,maps=maps,out=out)

def clear_polynomial_reference(layer,x,n):
    """Tiny PUBLIC only; deliberately independent convolution reference below."""
    maps=[{e:x[s]*m for s,e,m in row} for row in layer['maps']];result=[None]*layer['outputs']
    for edges,qs in layer['out']:
        accum={}
        for i,coeff in edges:
            for ie,a in maps[i].items():
                for ke,v in coeff:
                    degree=ie+ke;pos=degree%n
                    accum[pos]=accum.get(pos,0)+a*v*(-1 if degree//n%2 else 1)
        for dest,exponent,bias in qs:
            assert result[dest] is None;result[dest]=accum.get(exponent,0)+bias
    assert all(v is not None for v in result);return result

def direct_reference(x,h,w,ci,co,weights,bias,spatial):
    result=[];k=3 if spatial else 1;pad=1 if spatial else 0
    for o in range(co):
        for y in range(h):
            for z in range(w):
                value=bias[o]
                for c in range(ci):
                    for ky in range(k):
                        for kx in range(k):
                            yy,xx=y-pad+ky,z-pad+kx
                            if 0<=yy<h and 0<=xx<w:value+=x[(c*h+yy)*w+xx]*weights[o][c][ky][kx]
                result.append(value)
    return result
