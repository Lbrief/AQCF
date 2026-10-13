"""Render AQCF's paper mechanisms as moving coefficient objects.

Requires Pillow. Geometry, lighting and motion are generated locally.
The coefficients are symbolic protected views, not opened plaintext values.
The scenes follow Figure 1(b), Algorithm 1 and Figure 2 of the AQCF paper.
"""
from __future__ import annotations

import argparse
from functools import lru_cache
import math
import os
from pathlib import Path
import random

from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H, SCALE = 1440, 810, 2
PAPER = (237, 233, 223)
INK = (38, 40, 40)
MUTED = (104, 103, 96)
STONE = (84, 91, 91)
PALE = (182, 185, 175)
RED = (188, 69, 43)
BLUE = (107, 130, 143)
WHITE = (246, 242, 232)


def blend(a, b, t):
    return tuple(round(x+(y-x)*t) for x, y in zip(a, b))


def smooth(t):
    t = min(1, max(0, t))
    return t*t*(3-2*t)


def ramp(p, a, b):
    return smooth((p-a)/(b-a))


def sub(n):
    return str(n).translate(str.maketrans('0123456789', '₀₁₂₃₄₅₆₇₈₉'))


@lru_cache(None)
def font(size, style='body'):
    roots = [Path(os.environ.get('WINDIR', 'C:/Windows'))/'Fonts',
             Path('/usr/share/fonts/truetype/dejavu'),
             Path('/usr/share/fonts/truetype/liberation2')]
    options = {
        'body': ['segoeui.ttf', 'DejaVuSans.ttf', 'LiberationSans-Regular.ttf'],
        'light': ['segoeuil.ttf', 'DejaVuSans.ttf', 'LiberationSans-Regular.ttf'],
        'bold': ['seguisb.ttf', 'segoeuib.ttf', 'DejaVuSans-Bold.ttf', 'LiberationSans-Bold.ttf'],
        'math': ['georgia.ttf', 'DejaVuSerif.ttf', 'LiberationSerif-Regular.ttf'],
        'italic': ['georgiai.ttf', 'DejaVuSerif.ttf', 'LiberationSerif-Italic.ttf'],
        'mono': ['consola.ttf', 'DejaVuSansMono.ttf', 'LiberationMono-Regular.ttf'],
    }[style]
    for root in roots:
        for name in options:
            if (root/name).exists():
                return ImageFont.truetype(str(root/name), round(size*SCALE))
    raise RuntimeError('Install the DejaVu font family.')


def text(im, xy, value, size=22, color=INK, style='body', anchor='la'):
    ImageDraw.Draw(im).text(tuple(round(v*SCALE) for v in xy), value,
                           font=font(size, style), fill=color, anchor=anchor)


def centered(im, xy, value, size=22, color=INK, style='body'):
    d = ImageDraw.Draw(im)
    length = d.textlength(value, font=font(size, style))/SCALE
    text(im, (xy[0]-length/2, xy[1]), value, size, color, style)


def line(im, points, color, width=1):
    ImageDraw.Draw(im).line([(round(x*SCALE), round(y*SCALE)) for x, y in points],
                            fill=color, width=max(1, round(width*SCALE)))


def poly(im, points, fill):
    ImageDraw.Draw(im).polygon([(round(x*SCALE), round(y*SCALE)) for x, y in points], fill=fill)


def math_text(im, xy, value, size=28, color=INK):
    # Letter subscripts do not exist as a complete Unicode alphabet.
    import re
    x, y = xy
    for token in re.split('(u_[AB])', value):
        if token in ('u_A', 'u_B'):
            text(im, (x,y), 'u', size, color, 'math')
            x += ImageDraw.Draw(im).textlength('u', font=font(size,'math'))/SCALE
            text(im, (x,y+size*.42), token[-1], size*.62, color)
            x += ImageDraw.Draw(im).textlength(token[-1],font=font(size*.62))/SCALE+2
        else:
            text(im,(x,y),token,size,color,'body')
            x += ImageDraw.Draw(im).textlength(token,font=font(size))/SCALE


@lru_cache(None)
def background():
    # A quiet warm studio surface, without glow, grids or interface decoration.
    small = Image.new('RGB',(360,203))
    pixels = small.load()
    rng = random.Random(13)
    for y in range(203):
        for x in range(360):
            vignette = min(1, math.hypot((x-174)/285,(y-88)/260))
            shade = -.045*vignette + .012*math.exp(-((x-210)**2+(y-75)**2)/23000)
            noise = rng.uniform(-.0015,.0015)
            pixels[x,y] = tuple(round(c*(1+shade+noise)) for c in PAPER)
    im = small.resize((W*SCALE,H*SCALE),Image.Resampling.BICUBIC)
    return im


def header(im, scene):
    text(im,(61,39),'AQCF',61,INK,'light')
    text(im,(64,112),'Exact private inference',20,MUTED)
    titles = [('Retained phase', 'Local correction. The low two bits stay intact.'),
              ('Joint carry', 'One incoming carry, two compatible sharings.'),
              ('Output-directed merge', 'Contract channels, then cancel complementary terms.')]
    text(im,(484,49),titles[scene][0],34,INK,'light')
    text(im,(487,101),titles[scene][1],20,MUTED)
    text(im,(1300,56),f'0{scene+1}',35,RED,'math')
    text(im,(1360,69),'/ 03',17,MUTED,'mono')
    line(im,[(64,755),(1375,755)],(187,184,174),1)
    text(im,(64,771),'Symbolic protected views',16,MUTED)
    text(im,(1136,773),'CLIENT–CLOUD INFERENCE',14,MUTED,'mono')


class Stage:
    """Orthographic geometry with consistent lighting and soft contact shadows."""
    def __init__(self, image, origin=(845,415), scale=47, yaw=.26, pitch=.53):
        self.image, self.origin, self.scale = image, origin, scale
        self.cy,self.sy,self.cp,self.sp = math.cos(yaw),math.sin(yaw),math.cos(pitch),math.sin(pitch)
        self.objects = []

    def project(self, v):
        x,y,z = v
        return (self.origin[0]+self.scale*(self.cy*x-self.sy*y),
                self.origin[1]+self.scale*(self.sp*(self.sy*x+self.cy*y)-self.cp*z))

    def depth(self, v):
        x,y,z=v
        return self.cp*(self.sy*x+self.cy*y)+self.sp*z

    def box(self,x,y,z,dx,dy,dz,color=STONE,label=None,size=21,ink=WHITE):
        self.objects.append((x,y,z,dx,dy,dz,color,label,size,ink))

    def draw(self):
        shadows=Image.new('RGBA',self.image.size,(0,0,0,0))
        sd=ImageDraw.Draw(shadows)
        for x,y,z,dx,dy,dz,col,label,size,ink in self.objects:
            # Farther objects cast wider, lighter shadows on the studio plane.
            pts=[self.project((xx+.12*(z+dz),yy+.18*(z+dz),0))
                 for xx,yy in [(x,y),(x+dx,y),(x+dx,y+dy),(x,y+dy)]]
            sd.polygon([(round(a*SCALE),round(b*SCALE)) for a,b in pts],fill=(53,46,36,32))
        shadows=shadows.filter(ImageFilter.GaussianBlur(10*SCALE))
        self.image.paste(shadows,(0,0),shadows)
        faces=[]
        for box in self.objects:
            x,y,z,dx,dy,dz,col,label,size,ink=box
            vertices=[(x,y,z),(x+dx,y,z),(x+dx,y+dy,z),(x,y+dy,z),
                      (x,y,z+dz),(x+dx,y,z+dz),(x+dx,y+dy,z+dz),(x,y+dy,z+dz)]
            for ids,shade in [((1,2,6,5),.55),((2,3,7,6),.73),((4,5,6,7),1.0)]:
                vv=[vertices[j] for j in ids]
                depth=sum(self.depth(v) for v in vv)/4
                color=tuple(round(c*shade) for c in col)
                faces.append((depth,[self.project(v) for v in vv],color,ids==(4,5,6,7),box))
        for depth,pts,color,is_top,box in sorted(faces,key=lambda f:f[0]):
            poly(self.image,pts,color)
            if is_top:
                # Light grazes the edge; fine bevels replace glowing outlines.
                line(self.image,[pts[0],pts[1]],blend(color,WHITE,.22),.8)
                x,y,z,dx,dy,dz,col,label,size,ink=box
                if label:
                    self.surface_label(label,(x,y,z+dz+.005),(dx,dy),size,ink)

    def surface_label(self, value, at, dims, size, color):
        # Warp the engraved symbol onto its actual top plane.
        x,y,z=at;dx,dy=dims
        pad=.08
        pts=[self.project(v) for v in [(x+pad,y+pad,z),(x+dx-pad,y+pad,z),
                                      (x+pad,y+dy-pad,z)]]
        left=math.floor(min(p[0] for p in pts)-2)
        top=math.floor(min(p[1] for p in pts)-2)
        fourth=(pts[1][0]+pts[2][0]-pts[0][0],pts[1][1]+pts[2][1]-pts[0][1])
        right=math.ceil(max([p[0] for p in pts]+[fourth[0]])+2)
        bottom=math.ceil(max([p[1] for p in pts]+[fourth[1]])+2)
        tw=max(4,round((dx-.16)*self.scale*SCALE))
        th=max(4,round((dy-.16)*self.scale*SCALE))
        label=Image.new('RGBA',(tw,th),(0,0,0,0))
        dd=ImageDraw.Draw(label)
        f=font(size)
        while dd.textlength(value,font=f)>tw*.9 and size>10:
            size-=1;f=font(size)
        dd.text((tw/2,th/2),value,font=f,fill=color,anchor='mm')
        ax=(pts[1][0]-pts[0][0])*SCALE/tw
        ay=(pts[1][1]-pts[0][1])*SCALE/tw
        bx=(pts[2][0]-pts[0][0])*SCALE/th
        by=(pts[2][1]-pts[0][1])*SCALE/th
        ox=(pts[0][0]-left)*SCALE;oy=(pts[0][1]-top)*SCALE
        det=ax*by-ay*bx
        coeff=(by/det,-bx/det,(bx*oy-by*ox)/det,
               -ay/det,ax/det,(ay*ox-ax*oy)/det)
        warped=label.transform(((right-left)*SCALE,(bottom-top)*SCALE),
                               Image.Transform.AFFINE,coeff,Image.Resampling.BICUBIC)
        self.image.paste(warped,(left*SCALE,top*SCALE),warped)

    def note(self, xyz, value, offset=(0,-35), size=20, color=MUTED):
        xx,yy=self.project(xyz)
        target=(xx+offset[0],yy+offset[1])
        line(self.image,[(xx,yy),target],blend(PAPER,color,.45),1)
        if 'u_' in value:
            math_text(self.image,(target[0]+8,target[1]-size),value,size,color)
        else:
            text(self.image,(target[0]+8,target[1]-size),value,size,color)


def phase_scene(im,p):
    stage=Stage(im,origin=(873,432),scale=46,yaw=.18,pitch=.63)
    q=ramp(p,.12,.5)
    # The correction affects the high part only; the two low-bit cells never move.
    for row,y in enumerate((-2.7,1.65)):
        col=STONE if row==0 else BLUE
        stage.box(-6.4,y,.35,9.0,1.65,.78,col,'4a' if row==0 and q>.94 else ('4ã' if row==0 else '4b'),34)
        for j in range(2):
            stage.box(3.05+j*.77,y,.35,.66,1.65,.78,RED)
    if q<.98:
        # A thin local-correction plate settles into A, never into r.
        z=1.3+1.7*(1-q)
        stage.box(-6.4,-2.7,z,9.0,1.65,.17,blend(RED,STONE,q*.75),'−4 corr',27)
    stage.draw()
    for row,y in enumerate((-2.7,1.65)):
        for j in range(36):
            if j%3==0:
                xx=-6.22+j*.245
                a=stage.project((xx,y+.06,1.139));b=stage.project((xx,y+.32,1.139))
                line(im,[a,b],(139,147,143),.65)
    stage.note((-5.9,-2.7,.35),'A′  /  server local' if q>.94 else 'A  /  server local',(-210,-14),19,INK)
    stage.note((-5.9,3.3,.35),'B  /  client local',(-66,68),19,INK)
    stage.note((3.7,-2.7,1.2),'r  /  fixed low 2 bits',(47,-48),20,RED)
    stage.note((3.7,3.3,1.2),'t  /  low 2 bits',(35,56),20,RED)
    text(im,(65,272),'Local correction',24,INK,'bold')
    text(im,(65,315),'A′ = A − 4 corr',27,INK,'math')
    text(im,(65,365),'37-bit high part',19,MUTED)
    text(im,(65,395),'+ 2 retained low bits',19,MUTED)
    text(im,(65,606),'PROTECTED QUARTER RECONSTRUCTION',13,MUTED,'mono')
    text(im,(65,639),'z ≡ b − a + ω(t,r)  (mod 2³⁷)',35,INK)
    text(im,(931,667),'Shared integer before the floor operation',18,MUTED)


def carry_scene(im,p):
    # Dependency stages live along one diagonal rail, not a client/server route.
    beta=ramp(p,.28,.44);arith=ramp(p,.54,.73)
    stage=Stage(im,origin=(895,410),scale=55,yaw=-.17,pitch=.62)
    # Boolean shares keep their cap while growing an even arithmetic component.
    for row,(x,label,col) in enumerate(((-3.85,'α',STONE),(.35,'β',BLUE))):
        appear=1 if row==0 else beta
        if appear<.015:continue
        basez=.45+1.35*arith+(1-beta)*1.3*(row==1)
        if arith>.015:
            stage.box(x,-.65,.35,2.65,2.65,1.42*arith,col,'',22)
        stage.box(x,-.65,basez,2.65,2.65,.16,WHITE,label,43,RED)
        stage.box(x,1.91,basez-.015,2.65,.09,.18,RED)
        # An inset footing establishes the odd/even boundary.
        stage.box(x-.10,-.75,.15,2.85,2.85,.16,PALE)
    # Separate outgoing carry: low-profile block, set farther back.
    if arith>.06:
        stage.box(4.7,-4.0,.26,1.1,1.1,.26+arith*.26,PALE,'d′',25,INK)
    stage.draw()
    # Mask preparation appears first; lookup output later, arithmetic last.
    text(im,(65,221),'PREPARE',13,MUTED,'mono')
    text(im,(65,254),'α = (k₀ + k₁) mod 2',26,INK)
    text(im,(65,298),'Fresh ROT keys. One use.',18,MUTED)
    if beta>.05:
        text(im,(65,367),'PRECEDING LOOKUP',13,MUTED,'mono')
        text(im,(65,400),'β = d xor α',29,INK)
    if arith>.05:
        stage.note((-2.52,2,.48),'u_A = α + 2K',(-114,64),22,INK)
        stage.note((1.67,2,.48),'u_B = β − 2Q',(2,61),22,INK)
        stage.note((5.25,-3.45,.8),'next Boolean carry',(24,-35),18,MUTED)
    else:
        stage.note((-2.52,-.65,.9),'prepared mask',(-84,-87),20,INK)
        if beta>.05:stage.note((1.67,2,.8),'masked incoming carry',(12,87),20,INK)
    text(im,(65,606),'NEXT LOOKUP / ARITHMETIC SHARES',13,MUTED,'mono')
    math_text(im,(65,638),'u_A + u_B ≡ d  (mod 2ʷ)',35,INK)
    text(im,(932,629),'The low bits remain α and β.',20,RED)
    text(im,(932,666),'The outgoing Boolean carry is distinct.',18,MUTED)


def contraction_scene(im,p):
    stage=Stage(im,origin=(811,420),scale=46,yaw=.25,pitch=.65)
    align=ramp(p,.08,.42)
    contracted=ramp(p,.48,.83)
    # The input is one pack; both branches use it. Short weights are reversed.
    for j in range(4):
        stage.box(-6+j*.94,-.3,.35,.84,1.3,.55,STONE,'x'+sub(j),23)
    for row,y in enumerate((-3.6,2.7)):
        col=STONE if row==0 else BLUE
        stage.box(-.5,y,.45,1.28,1.4,.32,PALE,'X⁻¹',27,INK)
        for j,v in enumerate((('b₁','b₀','0','0'),('c₁','c₀','0','0'))[row]):
            stage.box(1.35+j*.91+(1-align)*.6,y,.45+(1-align)*.4,.81,1.4,.52,col if j<2 else PALE,v,24,WHITE if j<2 else MUTED)
        stage.box(6.05,y,.45,1.12,1.4,.12+.6*contracted,blend(PALE,RED,contracted),'Z'+sub(row),26)
    # Small restrained arrows connect actual operator dependencies.
    def arrow(a,b):
        x0,y0=stage.project(a);x1,y1=stage.project(b)
        line(im,[(x0,y0),(x1,y1)],(148,144,134),1.3)
        a=math.atan2(y1-y0,x1-x0)
        line(im,[(x1-6*math.cos(a-.5),y1-6*math.sin(a-.5)),(x1,y1),
                 (x1-6*math.cos(a+.5),y1-6*math.sin(a+.5))],(148,144,134),1.3)
    arrow((-1.95,.3,.6),(-.7,-2.9,.6));arrow((-1.95,.3,.6),(-.7,3.4,.6))
    arrow((5.1,-2.9,.6),(5.95,-2.9,.6));arrow((5.1,3.4,.6),(5.95,3.4,.6))
    stage.draw()
    stage.note((-4.5,-.3,1),'same Enc(Pᵢ)',(-54,-53),21,INK)
    stage.note((3,-3.6,1),'private short weights',(-60,-43),19,MUTED)
    text(im,(65,230),'BEFORE THE MERGE',13,MUTED,'mono')
    text(im,(65,265),'One input pack.',27,INK,'light')
    text(im,(65,305),'Two contractions.',27,INK,'light')
    text(im,(65,605),'CHANNEL CONTRACTION',13,MUTED,'mono')
    text(im,(65,640),'Zᵣ = Σᵢ Wᵣ,ᵢ X⁻¹ Pᵢ',34,INK)
    text(im,(905,636),'256 × 56 × 56  →  64 × 56 × 56',24,INK)
    text(im,(906,675),'Native ResNet-50 example  /  C = 2',18,MUTED)


def trace_scene(im,p):
    complete=ramp(p,.65,.79)
    stage=Stage(im,origin=(868-93*complete,425-5*complete),scale=63+9*complete,yaw=.17,pitch=.63)
    q=ramp(p,.03,.63)
    # Two signed coefficient surfaces approach; a completed cell is their sum.
    # Row-major indices correspond to 16 consecutive coefficients, not a tensor reshape.
    for i in range(16):
        row,col=divmod(i,8)
        x=-5.5+col*1.29;y=-1.45+row*2.0
        k=ramp(q*16,i,i+1)
        done=q*16>=i+1
        color=STONE if i%2==0 else BLUE
        if not done:
            gap=2.55*(1-k)
            stage.box(x,y,.38+gap,1.13,1.55,.27,STONE,'v'+sub(i),22)
            stage.box(x,y,.22,1.13,1.55,.26,BLUE,('−' if i%2 else '')+'t'+sub(i),21)
        else:
            stage.box(x,y,.23,1.13,1.55,.55,color,'2'+('a' if i%2==0 else 'u')+sub(i),20)
        if 0<k<1:
            # A restrained rust rim marks only the pair currently cancelling.
            stage.box(x-.025,y-.025,.14,1.18,1.6,.08,RED)
    stage.draw()
    active=min(15,int(q*16))
    idx=sub(active)
    left=im.copy()
    text(left,(65,215),'COEFFICIENT CANCELLATION',13,MUTED,'mono')
    text(left,(65,253),'V = A + U',26,INK,'math')
    text(left,(65,299),'T = A − U',26,INK,'math')
    text(left,(65,358),'A = Z₀    U = XZ₁',20,MUTED)
    text(left,(65,391),'σ₁(X) = −X',21,RED)
    text(left,(65,606),'EXACT OUTPUT TRACE',13,MUTED,'mono')
    text(left,(65,640),'M = V + σ₁(T)',34,INK,'math')
    im.paste(Image.blend(left,im,complete))
    # Labels for the two surfaces, not decorations or implied plaintext values.
    if q<.97:
        stage.note((4.095,1.32,3.2),'V',(72,-44),24,INK)
        stage.note((4.095,2.1,.49),'σ₁(T)',(37,42),23,INK)
    sign='+' if active%2==0 else '−'
    term='a' if active%2==0 else 'u'
    if q<1:
        text(im,(484,573),f'(a{idx} + u{idx}) {sign} (a{idx} − u{idx}) = 2{term}{idx}',25,INK)
    else:
        centered(im,(786,573),'M = 2[a₀, u₁, a₂, u₃, …, a₁₄, u₁₅]',32,INK)
    text(im,(942,630),'128 → 32',42,RED,'light')
    text(im,(945,684),'key switches / selected layer',18,MUTED)
    text(im,(485,690),'16 coefficients · trace factor 2 retained',19,MUTED)


def scene(scene,p):
    im=background().copy()
    header(im,scene)
    if scene==0:phase_scene(im,p)
    elif scene==1:carry_scene(im,p)
    else:
        if p<.29:contraction_scene(im,p/.29)
        elif p<.34:
            # A paper wipe clears geometry before the trace close-up.
            q=(p-.29)/.05
            if q<.5:
                old=im.copy();contraction_scene(old,1)
                im=Image.blend(old,im,smooth(q*2))
            else:
                nxt=im.copy();trace_scene(nxt,0)
                im=Image.blend(im,nxt,smooth((q-.5)*2))
        else:trace_scene(im,(p-.34)/.66)
    return im


def frame(base,t):
    stage=(t%1)*3
    i=min(2,int(stage));p=stage-i
    im=scene(i,p)
    # A brief exposure through the warm studio surface, never overlapping text.
    if p>.955:
        q=(p-.955)/.045
        clean=background()
        if q<.5:im=Image.blend(im,clean,smooth(q*2))
        else:im=Image.blend(clean,scene((i+1)%3,0),smooth((q-.5)*2))
    return im.resize((W,H),Image.Resampling.LANCZOS)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=Path(__file__).resolve().parents[1]/'docs'/'media')
    parser.add_argument('--frames',type=int,default=600)
    parser.add_argument('--preview-only',action='store_true')
    args=parser.parse_args()
    if args.frames<30 or args.frames%3:parser.error('--frames must be a multiple of 3 and at least 30')
    args.output.mkdir(parents=True,exist_ok=True)
    # A completed trace makes the strongest static cover.
    frame(None,(2+.94)/3).save(args.output/'aqcf-overview.png',optimize=True)
    if args.preview_only:
        for i in range(3):
            frame(None,(i+.72)/3).save(args.output/f'preview-scene-{i+1}.png')
        frame(None,(2+.19)/3).save(args.output/'preview-contraction.png')
        print('Four mechanism previews rendered.');return
    samples=Image.new('RGB',(W,H*4))
    for i,t in enumerate((.24,.57,.74,.97)):
        samples.paste(frame(None,t),(0,i*H))
    palette=samples.quantize(colors=224,method=Image.Quantize.MEDIANCUT)
    images=[]
    for i in range(args.frames):
        images.append(frame(None,i/args.frames).quantize(palette=palette,dither=Image.Dither.NONE))
        if i%60==0:print(f'Rendered {i}/{args.frames}',flush=True)
    # 50ms = 20fps; 600 frames produce a readable 30-second loop.
    images[0].save(args.output/'aqcf-overview.gif',save_all=True,append_images=images[1:],
                   duration=50,loop=0,disposal=1,optimize=True)
    print(f'Rendered {W} x {H}, {args.frames} frames, {args.frames*.05:g} seconds.')
    for name in ('aqcf-overview.gif','aqcf-overview.png'):
        print(f'{name}: {(args.output/name).stat().st_size:,} bytes')


if __name__=='__main__':
    main()
