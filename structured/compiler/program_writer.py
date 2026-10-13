"""Polynomial program serialization and depthwise group compilation."""
import math,struct
import numpy as np
N=8192
P=1152921504606830593
SCALE=4096

def need(x,m):
    if not x: raise ValueError(m)

def words(f,*x):f.write(struct.pack('<'+'Q'*len(x),*x))

def choose(planner,ci,co,h,w,ho,wo,k,s,pad,cache):
    key=(ci,co,h,w,ho,wo,k,s,pad)
    if key in cache:return cache[key]
    need(1<=ci<=1280 and 1<=co<=1280,'admitted MobileNet public channel domain')
    need(ho==(h+2*pad-k)//s+1 and wo==(w+2*pad-k)//s+1,'convolution geometry')
    sample=2 if k==1 and s==2 else 1
    eh,ew,es=(ho,wo,1) if sample==2 else (h,w,s)
    best=None
    for layout in planner.LAYOUTS:
      for th in sorted({ho}|{x for x in (1,2,4,8,16,32) if x<=ho}):
       for tw in sorted({wo}|{x for x in (1,2,4,8,16,32) if x<=wo}):
        p=planner.tiling_plan(ci,co,eh,ew,k,es,pad,th,tw,layout,'minimum_PM',N,sample)
        if p is not None and (best is None or planner._plan_key(p,'minimum_PM')<planner._plan_key(best,'minimum_PM')):best=p
    need(best is not None,'no certified dense placement')
    cache[key]=best;return best

def depth_group(planner,c,h,w,ho,wo,k,s,pad,cache):
    options=[]
    for g in (1,2,4,8,16,32,64,128):
        if g>c or c%g:continue
        p=choose(planner,g,g,h,w,ho,wo,k,s,pad,cache)
        # Public diagonal channel support, not weight values or sparsity search.
        pm=0
        for tile in p['tiles']:
            a=tile['placement'];j,f,u,v=[a[x] for x in ('J','F','U','V')]
            pm+=sum(max(ic*j,oc*f)<min((ic+1)*j,(oc+1)*f,g) for ic in range(u) for oc in range(v))
        groups=c//g;counts=p['counts']
        options.append(((groups*pm,groups*(counts['input_CT']+counts['output_CT']),groups*counts['output_CT'],g),g))
    return min(options)[1]

def compile_affine(helper,planner,index,node,w,b,input_scale,cache):
    shape=node['input_shape_nchw']
    if node['op']=='FC':shape=[1,math.prod(shape[1:]),1,1];w=w[:,:,None,None]
    _,ci,h,iw=shape;outshape=node['shape_nchw'];co=outshape[1]
    ho,wo=outshape[2:] if len(outshape)==4 else (1,1)
    k,s,pad=node['K'],node['stride'],node['padding']
    stub=dict(shape=[ho,wo,co],k=k,stride=s,pad=pad)
    # Helper emits bias*SCALE. Adjust only the bias scaling for Q24 IR input.
    bias_scaled=b*(1<<(input_scale-12))
    if node['op']!='DEPTHWISE':
        return helper.dense_program(np,planner,index,stub,shape,w,bias_scaled,cache)
    need(ci==co==node['groups'] and w.shape==(co,1,k,k),'native depthwise shape')
    g=depth_group(planner,co,h,iw,ho,wo,k,s,pad,cache)
    ins=[];outs=[];plan_rows=[]
    for first in range(0,co,g):
        weights=np.zeros((g,g,k,k),dtype=np.int64)
        for j in range(g):weights[j,j]=w[first+j,0]
        small=dict(stub,shape=[ho,wo,g])
        part,p=helper.dense_program(np,planner,index,small,[1,g,h,iw],weights,bias_scaled[first:first+g],cache)
        offset=len(ins)
        for m in part['inputs']:
            m[:,0]+=first*h*iw;ins.append(m)
        for edges,q in part['outputs']:
            q[:,0]+=first*ho*wo
            outs.append(([(offset+i,a) for i,a in edges],q))
        plan_rows.append(p)
    counts={key:sum(p['counts'][key] for p in plan_rows) for key in planner.COUNT_KEYS}
    return dict(id=index,input_integer_count=ci*h*iw,output_integer_count=co*ho*wo,inputs=ins,outputs=outs),dict(
        family='ordinary_depthwise_grouped_coefficient',group=g,groups=co//g,counts=counts,geometry=plan_rows[0])

def validate_write(program,pf,lf):
    ni,no=program['input_integer_count'],program['output_integer_count']
    ins,outs=program['inputs'],program['outputs']
    need(0<ni<=409600 and 0<no<=409600,'unchanged AQCF parser tensor cap')
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
