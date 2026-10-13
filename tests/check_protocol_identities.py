"""Arithmetic regression checks for joint carry, quotient and output trace merge. No HE execution or network."""
from pathlib import Path
from collections import Counter
from itertools import product
import json
import random

root = Path(__file__).resolve().parents[1]
rng = random.Random(16012)
counts = Counter()

def tap(w, k0, k1, r, d):
    M = 1 << (w-1)
    alpha = (k0+k1) & 1
    D = (alpha+k0+k1) % M
    beta = d ^ alpha
    e = beta ^ r
    kr = (k0, k1)[r]
    Q = kr if beta == 0 else (D-kr) % M
    ua = (alpha+2*(k0,k1)[e]) % (2*M)
    ub = (beta-2*Q) % (2*M)
    assert (ua+ub) % (2*M) == d and ua % 2 == alpha and ub % 2 == beta
    return alpha, beta, D, e, kr, Q, ua, ub

# Enumerate complete toy views, not only arithmetic-share marginals.
for w in range(3,7):
    M = 1 << (w-1)
    sender, receiver = [], []
    for d in (0,1):
        sv, rv = Counter(), Counter()
        for k0,k1,r in product(range(M),range(M),range(2)):
            a,b,D,e,kr,Q,ua,ub = tap(w,k0,k1,r,d)
            sv[(k0,k1,a,D,e,ua)] += 1
            rv[(r,kr,D,b,e,ub)] += 1
            counts['joint_toy_cases'] += 1
        sender.append(sv); receiver.append(rv)
    assert sender[0] == sender[1] and receiver[0] == receiver[1]

# Check the full digit message path under every small input/key/choice combination.
# Selected pad is tested at both bit values; unselected table entries cancel out
# of correctness but their privacy is an untested backend assumption.
for m in (2,3):
    B = 1 << m
    for k0,k1,r,d,x,y,R,mu in product(range(B),range(B),range(2),range(2),range(B),range(B),range(B),range(2)):
        a,b,D,e,kr,Q,ua,ub = tap(m+1,k0,k1,r,d)
        V=(b-2*Q)%(2*B); W=y+V
        delta=(W%B)^R; q=(W//B)%2
        U=x+((a+2*(k0,k1)[e])%(2*B))
        for pad in (0,1):
            T=int((U%B)+(R^delta)>=B)^((U//B)%2)^mu^pad
            got=T^pad^q
            assert (mu^got)==(x+y+d)//B
            counts['digit_transcript_cases'] += 1

for w in (25,26,37):
    M=1<<(w-1)
    for _ in range(10000):
        tap(w,rng.randrange(M),rng.randrange(M),rng.randrange(2),rng.randrange(2))
        counts['implemented_width_cases'] += 1

# Quarter low-bit initialization and initial lookup.
for r,t in product(range(4),repeat=2):
    if (t-r)%4 == 2:
        continue
    omega=1 if (t,r)==(3,0) else -1 if (t,r)==(0,3) else 0
    c0=omega+int(r!=0)-int(t==3)
    us=0 if r==0 else 2 if r==3 else 3
    uc=(t+1)%4
    assert c0 in (0,1) and (us+uc)//4==c0
    for a,b in product(range(2),repeat=2):
        assert (4*a+us+4*b+uc)//8==(a+b+c0)//2
        counts['quarter_leaf_cases'] += 1

def center(v,w):
    v%=1<<w
    return v-(1<<w) if v>=1<<(w-1) else v

for w,f in ((6,2),(8,3)):
    for a,b,c0 in product(range(1<<w),range(1<<w),range(2)):
        cf=((a% (1<<f))+(b%(1<<f))+c0)>>f
        ch=((a%(1<<(w-1)))+(b%(1<<(w-1)))+c0)>>(w-1)
        h=(a>>(w-1))+(b>>(w-1))+ch
        k=(h+1)//2
        got=(a>>f)+(b>>f)+cf-k*(1<<(w-f))
        expected=center(a+b+c0,w)//(1<<f)
        assert got==expected
        counts['signed_floor_cases'] += 1

for w in (5,7):
    for a,b in product(range(1<<w),repeat=2):
        v=center(a+b,w)
        if not -(1<<(w-2)) <= v < (1<<(w-2)):
            continue
        A=a>>(w-2); B=b>>(w-2)
        c=((a% (1<<(w-2)))+(b% (1<<(w-2))))>>(w-2)
        assert A+B+c in (0,3,4,7)
        q=(A+B+2)//4
        assert a+b-q*(1<<w)==v
        counts['terminal_widening_cases'] += 1

for C in (2,8,32,128):
    quarter_mod=1<<46
    for _ in range(10000):
        A=rng.randrange(quarter_mod); z=rng.randrange(1<<37); eps=rng.choice((-1,0,1))
        B=(A+4*C*z+eps)%quarter_mod
        norm=(B//C-A//C-4*z)%(1<<39)
        assert norm in (0,1,(1<<39)-1)
        counts['channel_division_cases'] += 1

def shift(v,s):
    n=len(v); out=[0]*n
    for i,a in enumerate(v):
        q,j=divmod(i+s,n); out[j]+=a*(-1 if q%2 else 1)
    return out

def automorphism(v,k):
    n=len(v); out=[0]*n
    for i,a in enumerate(v):
        q,j=divmod(i*k,n); out[j]+=a*(-1 if q%2 else 1)
    return out

def merge(leaves,s,N):
    if len(leaves)==1: return leaves[0]
    a=merge(leaves[::2],2*s,N); b=shift(merge(leaves[1::2],2*s,N),s)
    rot=automorphism([x-y for x,y in zip(a,b)],1+N//s)
    return [x+y+z for x,y,z in zip(a,b,rot)]

for N,C in ((16,2),(16,4),(16,8),(32,8)):
    for _ in range(100):
        leaves=[[rng.randint(-100,100) for _ in range(N)] for _ in range(C)]
        expected=[0]*N
        for r,v in enumerate(leaves):
            projected=[C*a if i%C==0 else 0 for i,a in enumerate(v)]
            expected=[a+b for a,b in zip(expected,shift(projected,r))]
        assert merge(leaves,1,N)==expected
        counts['trace_merge_cases'] += 1

assert 5820416*(-49+20+26+1-1-1-33)//8 == -26919424
assert 3186176*32//8+5519360*8//8 == 18264064
assert 26920000-26919424 == 576 and 18265088-18264064 == 1024
report={'status':'PASS','scope':'Local algebra identities and nominal cost arithmetic only; no native inference, security proof, or new performance data.','case_counts':dict(counts),'network_calls':0,'inference_runs':0}
# Printed JSON is suitable for archiving by the caller.
print(json.dumps(report))
