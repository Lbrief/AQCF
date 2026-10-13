"""Integer program IO and independent symbolic polynomial evaluation."""
from pathlib import Path
import hashlib,struct,array,itertools,sys
N=8192

def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb') as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()

def words(f,n):
 b=f.read(8*n);assert len(b)==8*n
 return struct.unpack('<'+'Q'*n,b)

def rows(f,n,c):
 b=f.read(8*n*c);assert len(b)==8*n*c
 return list(struct.iter_unpack('<'+'q'*c,b))

def qrow(edges,qe,maps):
 out={}
 for i,coeff in edges:
  mp=maps[i]
  for ke,value in coeff:
   a=qe-ke;sign=1 if a>=0 else -1
   term=mp.get(a%N)
   if term is not None:
    source,mul=term;out[source]=out.get(source,0)+value*mul*sign
 return {i:v for i,v in out.items() if v}

def put_words(f,*v):f.write(struct.pack('<'+'Q'*len(v),*v))

def emit(f,rs):
 a=array.array('q',itertools.chain.from_iterable(rs));assert a.itemsize==8 and sys.byteorder=='little';f.write(a.tobytes())

def write_first(f,head,l,model):
 f.write(b'AQCFGPS1' if model else b'AQCFCL01');put_words(f,*head)
 put_words(f,l['id'],l['inputs'],l['outputs'],len(l['maps']),len(l['out']))
 for m in l['maps']:put_words(f,len(m));emit(f,m)
 for edges,q in l['out']:
  if model:
   put_words(f,len(edges),len(q))
   for i,c in edges:put_words(f,i,len(c));emit(f,c)
   emit(f,q)
  else:put_words(f,len(q));emit(f,[(d,e) for d,e,bb in q])

def tail_sha(p,at):
 h=hashlib.sha256()
 with Path(p).open('rb') as f:
  f.seek(at)
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
