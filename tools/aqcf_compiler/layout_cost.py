"""Fixed public placement objective used by the ResNet-50 compiler."""
import functools

def cost(counts):return counts['PM']+15*counts['input_CT']+33*counts['output_CT']

def install_cost(planner):
 @functools.lru_cache(maxsize=32768)
 def chosen(ci,co,n,ell,imax,tmax,qmin,qmax,objective):
  rows=planner._options(ci,co,n,ell,imax,tmax,qmin,qmax)
  return min(rows,key=lambda q:(q[2]*q[3]+15*q[2]+33*q[3],q[2]+q[3],q[2]*q[3],q[0],q[1])) if rows else None
 planner._chosen=chosen

def choose(planner,ci,co,h,w,ho,wo,k,s,pad,cache):
 key=(ci,co,h,w,ho,wo,k,s,pad)
 if key in cache:return cache[key]
 assert ho==(h+2*pad-k)//s+1 and wo==(w+2*pad-k)//s+1
 sample=2 if k==1 and s==2 else 1;eh,ew,es=(ho,wo,1) if sample==2 else (h,w,s)
 best=None
 for layout in planner.LAYOUTS:
  for th in sorted({ho}|{x for x in (1,2,4,8,16,32) if x<=ho}):
   for tw in sorted({wo}|{x for x in (1,2,4,8,16,32) if x<=wo}):
    p=planner.tiling_plan(ci,co,eh,ew,k,es,pad,th,tw,layout,'minimum_CT',8192,sample)
    if p is not None and (best is None or (cost(p['counts']),p['counts']['input_plus_output_CT'],-p['output_tile_area'])<(cost(best['counts']),best['counts']['input_plus_output_CT'],-best['output_tile_area'])):best=p
 assert best is not None
 best['objective']='public_cost_PM_plus_15input_plus_33output';cache[key]=best;return best
