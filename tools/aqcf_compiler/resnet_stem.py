"""Exact ResNet-50 first-layer polyphase compilation."""
from pathlib import Path
from . import polynomial_io as io
N=8192; HI=WI=230; HO=WO=112; CI=3; CO=64; K=7; STRIDE=115
TILE=(N-1-117)//STRIDE-5
assert TILE==65

def read_first(path,model):
 with Path(path).open('rb') as f:
  assert f.read(8)==(b'AQCFGPS1' if model else b'AQCFCL01')
  head=io.words(f,4);assert head[0:2]==(1,N) and head[3]==72
  ident,ni,no,nm,nq=io.words(f,5);assert (ident,ni,no)==(0,CI*HI*WI,CO*HO*WO)
  maps=[io.rows(f,io.words(f,1)[0],3) for _ in range(nm)];outs=[]
  for _ in range(nq):
   if model:
    ne,n=io.words(f,2);edges=[]
    for _ in range(ne):
     index,count=io.words(f,2);edges.append((index,io.rows(f,count,2)))
    outs.append((edges,io.rows(f,n,3)))
   else:outs.append(io.rows(f,io.words(f,1)[0],2))
  return head,dict(id=ident,inputs=ni,outputs=no,maps=maps,out=outs),f.tell()

def expected(weights,y,x):
 return {ch*HI*WI+(2*y+ky)*WI+2*x+kx:weights[ch*49+ky*7+kx]
         for ch in range(CI) for ky in range(K) for kx in range(K) if weights[ch*49+ky*7+kx]}

def certify(l,source_weights=None,source_bias=None):
 maps=[]
 for m in l['maps']:
  d={}
  for src,e,mul in m:
   assert e not in d and mul==1;d[e]=(src,mul)
  maps.append(d)
 W=[None]*CO if source_weights is None else source_weights
 bias=[None]*CO if source_bias is None else source_bias
 seen=bytearray(CO*HO*WO);pm=0;maxnorm=0
 for idx,(edges,queries) in enumerate(l['out']):
  pm+=len(edges);norm=sum(abs(v) for _,cs in edges for e,v in cs)
  assert norm<=1<<30;maxnorm=max(maxnorm,norm)
  for dst,qe,bb in queries:
   o,t=divmod(dst,HO*WO);y,x=divmod(t,WO);got=io.qrow(edges,qe,maps)
   if W[o] is None:
    W[o]=[got.get(ch*HI*WI+(2*y+ky)*WI+2*x+kx,0)
          for ch in range(CI) for ky in range(K) for kx in range(K)]
    bias[o]=bb
   assert bb==bias[o] and got==expected(W[o],y,x),(o,y,x,'symbolic row mismatch')
   assert not seen[dst];seen[dst]=1
 assert all(seen)
 return W,bias,dict(checked_outputs=len(seen),PM=pm,max_output_PT_L1=maxnorm,all_linear_rows_exact=True)

def make_first(W,bias):
 maps=[];outs=[]
 for oy in range(0,HO,TILE):
  th=min(TILE,HO-oy);base=len(maps)
  for ch in range(CI):
   for a in range(2):
    for b in range(2):
     m=[(ch*HI*WI+(2*(oy+y)+a)*WI+2*x+b,y*STRIDE+x,1)
        for y in range(th+3) for x in range(WO+3)]
     assert max(s for s,e,v in m)<CI*HI*WI
     assert 2*(oy+th+2)+a<HI and 2*(WO+2)+b<WI
     assert max(e for s,e,v in m)+3*STRIDE+3<N
     maps.append(m)
  for o in range(CO):
   edges=[]
   for ch in range(CI):
    for a in range(2):
     for b in range(2):
      cs=sorted(((3-ky//2)*STRIDE+3-kx//2,W[o][ch*49+ky*7+kx])
                for ky in range(a,K,2) for kx in range(b,K,2) if W[o][ch*49+ky*7+kx])
      if cs:edges.append((base+ch*4+a*2+b,cs))
   q=[(o*HO*WO+(oy+y)*WO+x,(y+3)*STRIDE+x+3,bias[o]) for y in range(th) for x in range(WO)]
   outs.append((edges,q))
 assert len(maps)==24 and len(outs)==128
 return dict(id=0,inputs=CI*HI*WI,outputs=CO*HO*WO,maps=maps,out=outs)
