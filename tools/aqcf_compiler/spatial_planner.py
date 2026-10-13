"""Public-shape ordinary dense packing; compact and legacy spaces are shared.

No weights, arrays, samples, model or HE imports. All arithmetic is Python int.
The min-PM and min-CT names apply only to the finite uniform-tile/class family
enumerated here, not global latency, noise, or all conceivable HE algorithms.
No difference-collision weight constraint is used or assumed.
"""
from functools import lru_cache

N = 8192
T = 1152921504606830593
LAYOUTS = ('compact_query_safe', 'safe_rectangle')
COUNT_KEYS = ('PM', 'CT_Add', 'PT_bias_Add', 'input_CT', 'output_CT', 'Rot')


def require(ok, message):
    if not ok:
        raise ValueError(message)


def positive(value, name):
    require(type(value) is int and value > 0, name + ': positive Python int')
    return value


def ceil_div(a, b):
    return (a + b - 1) // b


def tile_geometry(hi, wi, oy, ox, th, tw, kernel, stride, padding,
                  layout='compact_query_safe', degree=N, input_sampling_stride=1):
    """True cropped input halo and exact spatial addresses for one output tile.

    Input local exponent: y*R+x. Kernel: (k-1-ky)*R+(k-1-kx).
    Query: (s*(oy+dy)-pad-cy0+k-1)*R+s*(ox+dx)-pad-cx0+k-1.
    Bounds are in encoded coordinates; K1/s2 uses public sampling first.
    """
    for name, value in (('hi',hi),('wi',wi),('th',th),('tw',tw),('degree',degree)):
        positive(value, name)
    require(type(kernel) is int and kernel in (1,2,3,4,7)
            and type(stride) is int and stride in (1,2)
            and type(padding) is int and padding in (0,1,2,3), 'CNN geometry domain')
    require(type(oy) is type(ox) is int and oy >= 0 and ox >= 0, 'output origin')
    require(layout in LAYOUTS and degree >= 2 and degree & (degree-1) == 0,
            'declared layout and power-of-two ring degree')
    require(type(input_sampling_stride) is int and input_sampling_stride in (1,2), 'public sampling stride')
    ho, wo = (hi+2*padding-kernel)//stride+1, (wi+2*padding-kernel)//stride+1
    require(ho > 0 and wo > 0 and oy+th <= ho and ox+tw <= wo, 'tile within complete output')
    y0, x0 = stride*oy-padding, stride*ox-padding
    y1, x1 = stride*(oy+th-1)-padding+kernel-1, stride*(ox+tw-1)-padding+kernel-1
    cy0, cx0, cy1, cx1 = max(0,y0), max(0,x0), min(hi-1,y1), min(wi-1,x1)
    ih, iw = cy1-cy0+1, cx1-cx0+1
    require(ih > 0 and iw > 0, 'all-bias tile is outside this packing family')
    left, right = cx0-x0, x1-cx1
    require(left >= 0 and right >= 0, 'nonnegative omitted padding')
    r = max(kernel, iw+max(left,right)) if layout == LAYOUTS[0] else iw+kernel-1
    imax, tmax = (ih-1)*r+iw-1, (kernel-1)*(r+1)
    qmin = (y0-cy0+kernel-1)*r+x0-cx0+kernel-1
    qmax = qmin+stride*(th-1)*r+stride*(tw-1)
    qxmin, qxmax = x0-cx0+kernel-1, x0-cx0+kernel-1+stride*(tw-1)
    # An overflowing horizontal term has remainder below the first query x.
    require(r >= kernel and r >= iw and 0 <= qxmin <= qxmax < r
            and iw+kernel-2-r < qxmin, 'horizontal carry/query or tap uniqueness not proved')
    ell = 1+max(qmax, imax+tmax-qmin, tmax, imax) if layout == LAYOUTS[0] else (ih+kernel-1)*r
    require(0 <= qmin <= qmax < ell and imax < ell and tmax < ell
            and imax+tmax-ell < qmin, 'non-target dense channel-plane separation not proved')
    return dict(layout=layout, degree=degree, output_origin=[oy,ox], output_tile=[th,tw],
                encoded_input_shape=[hi,wi], encoded_output_shape=[ho,wo],
                kernel=kernel, stride=stride, padding=padding,
                actual_input_bounds=[cy0,cy1,cx0,cx1], actual_input_shape=[ih,iw],
                required_input_bounds=[y0,y1,x0,x1], omitted_padding_columns=[left,right],
                row_stride=r, L=ell, input_spatial_max=imax, tap_spatial_max=tmax,
                query_spatial_min=qmin, query_spatial_max=qmax,
                input_sampling_stride=input_sampling_stride,
                horizontal_alias_query_separation=qxmin-(iw+kernel-2-r),
                channel_alias_query_separation=qmin-(imax+tmax-ell),
                arbitrary_dense_weight_support=True)


def placement_certificate(g, j, f):
    positive(j,'J'); positive(f,'F')
    n, ell = g['degree'], g['L']
    imax, tmax = g['input_spatial_max'], g['tap_spatial_max']
    qmin, qmax = g['query_spatial_min'], g['query_spatial_max']
    require(0 <= qmin <= qmax < ell and max(imax,tmax) < ell
            and imax+tmax-ell < qmin, 'local dense plane certificate')
    ix, wx = (j-1)*ell+imax, (f*j-1)*ell+tmax
    first, last = (j-1)*ell+qmin, (f*j-1)*ell+qmax
    raw = ix+wx
    require(0 <= ix < n and 0 <= wx < n and 0 <= first <= last < n,
            'input/PT/query exponent outside ring')
    require(raw < 2*n and (raw < n or raw-n < first), 'negacyclic wrap may reach query')
    return dict(input_degree_max=ix, plaintext_degree_max=wx,
                query_degree_min=first, query_degree_max=last, product_degree_max=raw,
                wrapped_degree_max=raw-n if raw >= n else None,
                wrap_query_separation=first-(raw-n) if raw >= n else None,
                channel_plane_difference_nonzero_cannot_hit_query=True,
                input_PT_coefficients_unique=True, nominal_J_kept_for_tail=True,
                nominal_F_kept_for_tail=True)


@lru_cache(maxsize=32768)
def _options(ci, co, n, ell, imax, tmax, qmin, qmax):
    max_j = min(ci, (n-1-imax)//ell+1)
    cap = min(n-1-qmax+ell, n-1-tmax+ell, n+qmin+ell-imax-tmax-1)
    rows = []
    for j in range(1,max_j+1):
        f = cap//(j*ell)
        if f < 1:
            continue
        u, v = ceil_div(ci,j), ceil_div(co,f)
        rows.append((j,f,u,v))
    return tuple(rows)


def placement_options(g, ci, co):
    """All legal J, each with count-dominant largest legal F.

    For a fixed J, 1..Fmax are admitted. Smaller F cannot improve PM/CT counts,
    but may change norms/noise; they are not declared noise/latency dominated.
    """
    require(type(ci) is type(co) is int and 1 <= ci <= 400 and 1 <= co <= 400, 'channel resource domain')
    rows = []
    for j,f,u,v in _options(ci,co,g['degree'],g['L'],g['input_spatial_max'],
                             g['tap_spatial_max'],g['query_spatial_min'],g['query_spatial_max']):
        counts = dict(PM=u*v,CT_Add=v*(u-1),PT_bias_Add=v,input_CT=u,output_CT=v,Rot=0)
        rows.append(dict(J=j,F=f,U=u,V=v,counts=counts,
                         input_plus_output_CT=u+v,admitted_F_interval=[1,f],
                         certificate=placement_certificate(g,j,f)))
    return rows


def _placement_key(p, objective):
    c=p['counts'];ct=c['input_CT']+c['output_CT']
    lead=(c['PM'],ct,c['output_CT']) if objective=='minimum_PM' else (ct,c['PM'],c['output_CT'])
    return (*lead,p['J'],p['F'])


@lru_cache(maxsize=32768)
def _chosen(ci,co,n,ell,imax,tmax,qmin,qmax,objective):
    rows=_options(ci,co,n,ell,imax,tmax,qmin,qmax)
    if not rows:return None
    def key(row):
        j,f,u,v=row
        lead=(u*v,u+v,v) if objective=='minimum_PM' else (u+v,u*v,v)
        return (*lead,j,f)
    return min(rows,key=key)


def tiling_plan(ci,co,hi,wi,kernel,stride,padding,th,tw,layout='compact_query_safe',
                objective='minimum_PM',degree=N,input_sampling_stride=1):
    require(objective in ('minimum_PM','minimum_CT'),'declared count objective')
    ho,wo=(hi+2*padding-kernel)//stride+1,(wi+2*padding-kernel)//stride+1
    positive(th,'tile height');positive(tw,'tile width')
    require(th<=ho and tw<=wo,'nominal output tile domain')
    total={k:0 for k in COUNT_KEYS};tiles=[];mapped=0
    for oy in range(0,ho,th):
        for ox in range(0,wo,tw):
            g=tile_geometry(hi,wi,oy,ox,min(th,ho-oy),min(tw,wo-ox),
                            kernel,stride,padding,layout,degree,input_sampling_stride)
            chosen=_chosen(ci,co,degree,g['L'],g['input_spatial_max'],g['tap_spatial_max'],
                           g['query_spatial_min'],g['query_spatial_max'],objective)
            if chosen is None:
                return None
            j,f,u,v=chosen
            p=dict(J=j,F=f,U=u,V=v,
                   counts=dict(PM=u*v,CT_Add=v*(u-1),PT_bias_Add=v,input_CT=u,output_CT=v,Rot=0),
                   input_plus_output_CT=u+v,admitted_F_interval=[1,f],
                   certificate=placement_certificate(g,j,f))
            for key in COUNT_KEYS:total[key]+=p['counts'][key]
            mapped+=ci*g['actual_input_shape'][0]*g['actual_input_shape'][1]
            tiles.append(dict(geometry=g,placement=p))
    total['input_plus_output_CT']=total['input_CT']+total['output_CT']
    return dict(layout=layout,objective=objective,nominal_output_tile=[th,tw],
                output_tile_area=th*tw,tile_count=len(tiles),counts=total,tiles=tiles,
                input_mapped_values_including_repeated_halos=mapped,
                actual_executed_counts=None,noise_qualified=False,latency_qualified=False)


def _plan_key(p, objective):
    c=p['counts'];ct=c['input_plus_output_CT']
    lead=(c['PM'],ct,c['output_CT']) if objective=='minimum_PM' else (ct,c['PM'],c['output_CT'])
    return (*lead,-p['output_tile_area'],p['layout'],p['nominal_output_tile'])


def layer_plans(ci,co,hi,wi,ho,wo,kernel,stride,padding,degree=N):
    """Finite uniform output-tile and J/F family; shared by ON/OFF dense layers.

    K1/s2/pad0 uses exact public subsampling before packing. g input bounds then
    address sampled coordinates: original_y/x = encoded_y/x * sampling_stride.
    Legacy safe full-image rectangles stay available, not silently discarded.
    """
    require(type(ci) is type(co) is int and 1<=ci<=400 and 1<=co<=400,'channel domain')
    for name,val in (('hi',hi),('wi',wi),('ho',ho),('wo',wo)):positive(val,name)
    require(kernel in (1,2,3,4,7) and type(kernel) is int and type(stride) is int and stride in (1,2)
            and type(padding) is int and padding in (0,1,2,3),'full38 convolution domain')
    require(ho==(hi+2*padding-kernel)//stride+1 and wo==(wi+2*padding-kernel)//stride+1,'exact output geometry')
    require(hi<=32 and wi<=32 and not (kernel==1 and padding!=0),'current full38 spatial scope; no all-bias-only border')
    sampling=2 if kernel==1 and stride==2 and padding==0 else 1
    eh,ew,es=(ho,wo,1) if sampling==2 else (hi,wi,stride)
    best_pm=best_ct=largest=None; considered=0
    legacy=None;full_compact=None
    for layout in LAYOUTS:
        for th in range(1,ho+1):
            for tw in range(1,wo+1):
                pm=tiling_plan(ci,co,eh,ew,kernel,es,padding,th,tw,layout,'minimum_PM',degree,sampling)
                if pm is None:continue
                ct=tiling_plan(ci,co,eh,ew,kernel,es,padding,th,tw,layout,'minimum_CT',degree,sampling)
                require(ct is not None,'same feasible family across count objectives')
                considered+=1
                if best_pm is None or _plan_key(pm,'minimum_PM')<_plan_key(best_pm,'minimum_PM'):best_pm=pm
                if best_ct is None or _plan_key(ct,'minimum_CT')<_plan_key(best_ct,'minimum_CT'):best_ct=ct
                if largest is None or (-pm['output_tile_area'],*_plan_key(pm,'minimum_PM'))<(-largest['output_tile_area'],*_plan_key(largest,'minimum_PM')):largest=pm
                if th==ho and tw==wo:
                    if layout=='safe_rectangle':legacy=dict(minimum_PM=pm,minimum_CT=ct)
                    else:full_compact=dict(minimum_PM=pm,minimum_CT=ct)
    require(best_pm is not None and best_ct is not None,'no certified ordinary dense plan')
    return dict(schema='aqcf.common_dense_shared_spatial.v1',degree=degree,ci=ci,co=co,
                original_input_shape=[hi,wi],output_shape=[ho,wo],kernel=kernel,
                original_stride=stride,padding=padding,input_sampling_stride=sampling,
                encoded_geometry=[eh,ew,kernel,es,padding],
                minimum_PM=best_pm,minimum_CT=best_ct,maximum_spatial_tile=largest,
                legacy_safe_rectangle_full_image=legacy,compact_full_image=full_compact,
                feasible_uniform_tile_layouts=considered,
                all_dense_weights_free=True,requires_no_difference_address_codes=True,
                optimum_scope='finite declared uniform output tiles and one dense class per tile; count objectives only',
                integer_range_requires_actual_input_and_PT_L1=True,
                no_claims=['global HE layout optimality','wall-time optimum','noise qualification','novel algorithm','classification accuracy'])


def output_range_bound(input_maxima, edge_input_indices, edge_l1, bias_maxabs, modulus=T):
    """Python-int full-ring sufficient bound, not an accuracy or noise bound.

    Caller derives exact sparse PT L1 and mapped current-input maxima. This
    gate never narrows arithmetic or retries a different public plan silently.
    """
    require(type(modulus) is int and modulus>2 and modulus%2==1,'odd plaintext modulus')
    require(type(bias_maxabs) is int and 0<=bias_maxabs<=modulus//2,'centered aligned-Q32 bias maximum')
    require(len(edge_input_indices)==len(edge_l1) and len(set(edge_input_indices))==len(edge_input_indices),'distinct edge/input roster')
    require(all(type(x) is int and 0<=x<=modulus//2 for x in input_maxima),'actual input maxima')
    bound=bias_maxabs
    for idx,l1 in zip(edge_input_indices,edge_l1):
        require(type(idx) is int and 0<=idx<len(input_maxima) and type(l1) is int and l1>0,'edge index and exact positive PT L1')
        term=input_maxima[idx]*l1
        require(term<=modulus//2-bound,'whole negative ring cannot be certified without wrap')
        bound+=term
    return bound
