"""Coefficient compilation with the original collision-certified placement."""
N=8192
P=1152921504606830593
SCALE=4096

def need(ok, message):
    if not ok: raise ValueError(message)

def dense_program(np, planner, index, node, inshape, weight, bias, cache):
    _, ci, h, w = inshape
    ho, wo, co = node['shape']
    k, s, pad = node['k'], node['stride'], node['pad']
    need(weight.shape == (co, ci, k, k), 'private convolution shape')
    plan = select_plan(planner, ci, co, h, w, ho, wo, k, s, pad, cache)
    inputs, outputs = [], []
    for tile in plan['tiles']:
        g, a = tile['geometry'], tile['placement']
        j, f, u, v = (a[z] for z in ('J','F','U','V'))
        r, ell = g['row_stride'], g['L']
        oy, ox = g['output_origin']; th, tw = g['output_tile']
        cy0, cy1, cx0, cx1 = g['actual_input_bounds']
        ey, ex = np.meshgrid(np.arange(cy0, cy1+1), np.arange(cx0, cx1+1), indexing='ij')
        iy, ix = (ey*s-pad, ex*s-pad) if k == 1 else (ey, ex)
        need(bool(((iy >= 0)&(iy < h)&(ix >= 0)&(ix < w)).all()), 'cropped mapped coordinates')
        spatial = ((ey-cy0)*r+ex-cx0).ravel()
        srcsp = (iy*w+ix).ravel()
        off = len(inputs)
        for ic in range(u):
            channels = np.arange(ic*j, min((ic+1)*j,ci))
            source = (channels[:,None]*h*w+srcsp).ravel()
            exp = ((j-1-np.arange(len(channels)))[:,None]*ell+spatial).ravel()
            maps = np.column_stack((source, exp, np.ones(len(source),dtype=np.int64)))
            inputs.append(maps[np.lexsort((maps[:,0],maps[:,1]))].astype('<i8'))
        ky, kx = np.meshgrid(np.arange(k),np.arange(k),indexing='ij')
        taps = ((k-1-ky)*r+k-1-kx).ravel()
        qy, qx = np.meshgrid(np.arange(th),np.arange(tw),indexing='ij')
        qstep = 1 if k == 1 else s
        qsp = g['query_spatial_min']+qstep*(qy*r+qx).ravel()
        destsp = ((oy+qy)*wo+ox+qx).ravel()
        for oc in range(v):
            outch = np.arange(oc*f,min((oc+1)*f,co))
            edges = []
            for ic in range(u):
                inch = np.arange(ic*j,min((ic+1)*j,ci))
                coeff = weight[outch[:,None], inch[None,:]].reshape(-1)
                exp = (((np.arange(len(outch))[:,None]*j+np.arange(len(inch)))*ell)[:,:,None]+taps).ravel()
                nz = coeff != 0
                if bool(nz.any()):
                    pairs = np.column_stack((exp[nz],coeff[nz])).astype('<i8')
                    pairs = pairs[np.argsort(pairs[:,0],kind='stable')]
                    edges.append((off+ic,pairs))
            dest = (outch[:,None]*ho*wo+destsp).ravel()
            exp = ((np.arange(len(outch))*j+j-1)[:,None]*ell+qsp).ravel()
            b = np.repeat(bias[outch]*SCALE,th*tw)
            outputs.append((edges,np.column_stack((dest,exp,b)).astype('<i8')))
    return dict(id=index,input_integer_count=ci*h*w,output_integer_count=co*ho*wo,
                inputs=inputs,outputs=outputs), plan

def bn_program(np, planner, index, node, weight, bias):
    h,w,c = node['shape']; ell=h*w
    g=planner.tile_geometry(h,w,0,0,h,w,1,1,0)
    legal=[]
    for j in range(1,min(c,N//ell)+1):
        try: cert=planner.placement_certificate(g,j,j)
        except ValueError: continue
        legal.append((j,cert))
    need(legal,'BN diagonal certificate'); j,cert=max(legal,key=lambda z:z[0])
    inputs=[];outputs=[];sp=np.arange(ell,dtype=np.int64)
    for start in range(0,c,j):
        ch=np.arange(start,min(start+j,c));local=np.arange(len(ch))
        src=(ch[:,None]*ell+sp).ravel()
        iexp=((j-1-local)[:,None]*ell+sp).ravel()
        maps=np.column_stack((src,iexp,np.ones(len(src),dtype=np.int64)))
        inputs.append(maps[np.argsort(maps[:,1],kind='stable')].astype('<i8'))
        coeff=weight[ch];exp=local*(j+1)*ell;nz=coeff!=0
        edges=[] if not bool(nz.any()) else [(len(inputs)-1,np.column_stack((exp[nz],coeff[nz])).astype('<i8'))]
        qexp=((local*j+j-1)[:,None]*ell+sp).ravel()
        q=np.column_stack((src,qexp,np.repeat(bias[ch]*SCALE,ell))).astype('<i8')
        outputs.append((edges,q))
    counts=dict(input_CT=len(inputs),output_CT=len(outputs),PM=len(outputs),CT_Add=0,Rot=0,PT_bias_Add=len(outputs))
    return dict(id=index,input_integer_count=c*ell,output_integer_count=c*ell,inputs=inputs,outputs=outputs),dict(
        family='ordinary_coefficient_private_channel_diagonal',J=j,F=j,L=ell,certificate=cert,counts=counts,
        diagonal_exponent='i*(J+1)*L',query_exponent='(i*J+J-1)*L+spatial')



def select_plan(planner,ci,co,h,w,ho,wo,k,s,pad,cache):
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
