"""Serialize the exact integer coefficient program and public client layout."""
import struct
import numpy as np
N=8192
P=1152921504606830593

def need(x,m):
    if not x: raise ValueError(m)

def words(f,*x):f.write(struct.pack('<'+'Q'*len(x),*x))

def validate_write(program,pf,lf):
    ni,no=program['input_integer_count'],program['output_integer_count']
    ins,outs=program['inputs'],program['outputs']
    need(0<ni<=1048576 and 0<no<=1048576,'native model tensor cap')
    need(0<len(ins)<=N and 0<len(outs)<=N,'ciphertext count bounds')
    for f in (pf,lf):words(f,program['id'],ni,no,len(ins),len(outs))
    seen=np.zeros(no,dtype=np.uint8);maxnorm=pm=adds=0
    for m in ins:
        need(len(m)>0 and m.shape[1]==3,'nonempty maps')
        need(bool(((m[:,0]>=0)&(m[:,0]<ni)&(m[:,1]>=0)&(m[:,1]<N)&(m[:,2]==1)).all()),'map domain')
        need(len(np.unique(m[:,1]))==len(m),'gather L1=1')
        for f in(pf,lf):words(f,len(m));f.write(m.astype('<i8').tobytes())
    for edges,q in outs:
        words(pf,len(edges),len(q));words(lf,len(q));norm=0;used=set()
        for i,a in edges:
            need(0<=i<len(ins) and i not in used,'unique valid input edge');used.add(i)
            need(bool(((a[:,0]>=0)&(a[:,0]<N)&(a[:,1]!=0)&(np.abs(a[:,1])<P//2)).all()),'coefficient domain')
            need(len(np.unique(a[:,0]))==len(a),'coefficient uniqueness')
            norm+=sum(abs(int(x)) for x in a[:,1]);words(pf,i,len(a));pf.write(a.astype('<i8').tobytes())
        need(norm<=1<<30,'old AQCF all-edge plaintext L1 cap retained')
        maxnorm=max(maxnorm,norm);pm+=len(edges);adds+=max(0,len(edges)-1)
        need(bool(((q[:,0]>=0)&(q[:,0]<no)&(q[:,1]>=0)&(q[:,1]<N)&(np.abs(q[:,2])<P//2)).all()),'query/bias domain')
        need(len(np.unique(q[:,0]))==len(q) and len(np.unique(q[:,1]))==len(q) and bool((seen[q[:,0]]==0).all()),'unique output positions')
        seen[q[:,0]]=1;pf.write(q.astype('<i8').tobytes());lf.write(q[:,:2].astype('<i8').tobytes())
    need(bool((seen==1).all()),'full output coverage')
    return dict(input_CT=len(ins),output_CT=len(outs),PM=pm,CT_Add=adds,Rot=0,max_output_PT_L1=maxnorm)
