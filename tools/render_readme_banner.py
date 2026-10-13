"""Render the AQCF README animation. Requires Pillow; no model or data access.

    python -m pip install Pillow
    python tools/render_readme_banner.py

The scenes follow manuscript Figure 1(b), Algorithm 1, and Figure 2.
They show symbolic protected views; animation time is not protocol latency.
"""
from __future__ import annotations

import argparse
import math
import os
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H, SCALE = 1440, 800, 2
BG = (7, 14, 28)
INK = (236, 246, 255)
MUTED = (153, 177, 201)
CYAN = (74, 225, 241)
PURPLE = (180, 154, 255)


def mix(a, b, t):
    return tuple(round(x * (1 - t) + y * t) for x, y in zip(a, b))


def font_path(kind):
    roots = []
    if os.environ.get('WINDIR'):
        roots.append(Path(os.environ['WINDIR']) / 'Fonts')
    roots += [Path('/usr/share/fonts/truetype/dejavu'), Path('/usr/share/fonts/truetype/liberation2')]
    names = {
        'hero': ['bahnschrift.ttf', 'DejaVuSans-Bold.ttf', 'LiberationSans-Bold.ttf'],
        'bold': ['segoeuib.ttf', 'DejaVuSans-Bold.ttf', 'LiberationSans-Bold.ttf'],
        'body': ['segoeui.ttf', 'DejaVuSans.ttf', 'LiberationSans-Regular.ttf'],
        'mono': ['consola.ttf', 'DejaVuSansMono.ttf', 'LiberationMono-Regular.ttf'],
    }[kind]
    for root in roots:
        for name in names:
            if (root / name).is_file():
                return str(root / name)
    raise RuntimeError('Install DejaVu Sans and DejaVu Sans Mono, or use standard Windows fonts.')


FONTS = {}


def font(size, kind='body'):
    key = (size, kind)
    if key not in FONTS:
        FONTS[key] = ImageFont.truetype(font_path(kind), size * SCALE)
    return FONTS[key]


class Canvas:
    def __init__(self, image):
        self.image = image
        self.d = ImageDraw.Draw(image)

    def rect(self, box, fill, radius=0, outline=None, width=1):
        coords = tuple(round(v * SCALE) for v in box)
        self.d.rounded_rectangle(coords, radius=round(radius * SCALE), fill=fill,
                                 outline=outline, width=width * SCALE)

    def line(self, points, fill, width=1):
        self.d.line([(round(x * SCALE), round(y * SCALE)) for x, y in points],
                    fill=fill, width=width * SCALE, joint='curve')

    def circle(self, x, y, r, fill, outline=None, width=1):
        self.d.ellipse(tuple(round(v * SCALE) for v in (x-r, y-r, x+r, y+r)),
                       fill=fill, outline=outline, width=width * SCALE)

    def text(self, x, y, text, size=20, fill=INK, kind='body', anchor='la'):
        self.d.text((round(x*SCALE), round(y*SCALE)), text, font=font(size, kind),
                    fill=fill, anchor=anchor)

    def centered(self, x, y, text, size=20, fill=INK, kind='body'):
        f = font(size, kind)
        width = self.d.textlength(text, font=f) / SCALE
        self.text(x-width/2, y, text, size, fill, kind)

    def arrow(self, points, fill, width=2):
        self.line(points, fill, width)
        x, y = points[-1]
        px, py = points[-2]
        a = math.atan2(y-py, x-px)
        self.line([(x-7*math.cos(a-.55), y-7*math.sin(a-.55)), (x, y),
                   (x-7*math.cos(a+.55), y-7*math.sin(a+.55))], fill, width)

    def packet(self, points, progress, color, radius=4):
        lengths = [math.hypot(b[0]-a[0], b[1]-a[1]) for a, b in zip(points, points[1:])]
        distance = (progress % 1) * sum(lengths)
        for a, b, length in zip(points, points[1:], lengths):
            if distance <= length:
                t = distance/length
                x, y = a[0]+(b[0]-a[0])*t, a[1]+(b[1]-a[1])*t
                self.circle(x, y, radius+4, mix(BG, color, .15))
                self.circle(x, y, radius, color)
                self.circle(x-1, y-1, max(1, radius/3), INK)
                break
            distance -= length


def poly(c, points, fill):
    c.d.polygon([(round(x*SCALE),round(y*SCALE)) for x,y in points], fill=fill)


def smooth(t):
    t=max(0,min(1,t))
    return t*t*(3-2*t)


def lerp(a,b,t):
    return a+(b-a)*t


def math_text(c,x,y,text,size,color=INK,center=False):
    import re
    tokens=re.split(r'(u_[AB])',text)
    widths=[]
    for token in tokens:
        if token in ('u_A','u_B'):
            widths.append(c.d.textlength('u',font=font(size))/SCALE+c.d.textlength(token[-1],font=font(round(size*.64)))/SCALE+2)
        else:
            widths.append(c.d.textlength(token,font=font(size))/SCALE)
    if center:x-=sum(widths)/2
    for token,w in zip(tokens,widths):
        if token in ('u_A','u_B'):
            c.text(x,y,'u',size,color)
            offset=c.d.textlength('u',font=font(size))/SCALE
            c.text(x+offset,y+size*.4,token[-1],round(size*.64),color)
        else:c.text(x,y,token,size,color)
        x+=w


def tile(c,x,y,w,h,label,color=CYAN,level=.7,size=22,depth=5):
    # Extruded coefficient tiles; the front face carries the algebraic identity.
    poly(c,[(x+w,y+3),(x+w+depth,y+depth),(x+w+depth,y+h+depth),(x+w,y+h)],mix(BG,color,.19*level))
    poly(c,[(x+3,y+h),(x+w,y+h),(x+w+depth,y+h+depth),(x+depth,y+h+depth)],mix(BG,color,.12*level))
    c.rect((x,y,x+w,y+h),mix((10,23,37),color,.14*level),8,mix((36,52,68),color,.5*level))
    c.line([(x+10,y+1),(x+w-10,y+1)],mix((44,71,86),color,.8*level),1)
    if label:
        if 'u_' in label:
            math_text(c,x+w/2,y+(h-size)/2-2,label,size,mix((53,71,86),INK,level),True)
        else:
            c.centered(x+w/2,y+(h-size)/2-2,label,size,mix((53,71,86),INK,level),'body')


def cap(c,x,y,text,color=CYAN):
    c.text(x,y,text,16,color,'bold')


def tag(c,x,y,text,color=CYAN,size=18):
    w=c.d.textlength(text,font=font(size,'body'))/SCALE+28
    c.rect((x,y,x+w,y+35),mix(BG,color,.09),8,mix((32,49,65),color,.28))
    c.text(x+14,y+5,text,size,color)
    return w


def sub(n):
    return str(n).translate(str.maketrans('0123456789','₀₁₂₃₄₅₆₇₈₉'))


def base_image():
    image=Image.new('RGB',(W*SCALE,H*SCALE),BG)
    light=Image.new('RGB',image.size,BG)
    d=ImageDraw.Draw(light)
    d.ellipse((200,-500,2200,1400),fill=(10,42,59))
    d.ellipse((2200,100,3450,1350),fill=(39,23,64))
    light=light.filter(ImageFilter.GaussianBlur(160*SCALE))
    image=Image.blend(image,light,.65)
    c=Canvas(image)
    # Quiet perspective construction grid underneath the coefficient workspace.
    for i in range(11):
        x=480+i*90
        c.line([(880+(x-880)*.25,195),(x,685)],(17,33,48),1)
    for y in (232,280,338,407,489,586,680):
        c.line([(473,y),(1377,y)],(16,32,46),1)
    c.rect((1,1,W-2,H-2),None,27,(35,54,70))
    # Four phase plates form the small brand mark.
    for i,col in enumerate((CYAN,PURPLE,CYAN,(139,206,212))):
        x=49+(i%2)*24;y=46+(i//2)*24
        poly(c,[(x,y+6),(x+17,y),(x+27,y+7),(x+10,y+13)],mix(BG,col,.6))
        c.line([(x,y+6),(x+10,y+13),(x+27,y+7)],col,1)
    c.text(119,26,'AQCF',75,INK,'hero')
    c.text(390,44,'EXACT PRIVATE INFERENCE',20,INK,'bold')
    c.text(391,76,'ALGORITHMS FOR THE CLIENT–CLOUD BOUNDARY',16,MUTED)
    c.rect((1098,44,1385,98),(11,25,40),27,(49,67,89))
    c.circle(1122,71,4,CYAN)
    c.centered(1245,56,'CLIENT  ↔  CLOUD',20,(201,221,237),'body')
    c.line([(48,137),(1391,137)],(36,57,72),1)
    c.text(51,154,'PAPER MECHANISMS / IN MOTION',15,MUTED,'mono')
    c.text(1081,154,'SYMBOLIC PROTECTED VIEWS',15,MUTED,'mono')
    c.line([(451,216),(451,662)],(39,58,75),1)
    return image


def left_panel(c,scene,color):
    keywords=['RETAINED-PHASE INTERFACE','MASK-PROGRAMMED JOINT CARRY','OUTPUT-DIRECTED TRACE MERGE']
    titles=[['Keep the','phase.'],['Program the','carry.'],['Merge after','contraction.']]
    subtitles=['Recover the exact integer.','Two compatible sharings.','Preserve every coefficient index.']
    descriptions=[['Apply the correction locally.','Leave the low two bits intact.'],
                  ['Prepare the mask first.','Reuse the incoming carry','at the selected boundary.'],
                  ['Contract channels before projection.','Cancel complementary coefficients.']]
    cap(c,53,223,keywords[scene],color)
    for j,line in enumerate(titles[scene]):
        c.text(48,265+j*67,line,54 if scene!=1 else 51,INK,'bold')
    c.text(53,416,subtitles[scene],23,color)
    for j,line in enumerate(descriptions[scene]):
        c.text(53,469+j*31,line,20,MUTED)
    if scene==0:
        tag(c,53,575,'LOW BITS PRESERVED',CYAN,18)
        c.text(53,624,'Figure 1(b) · local phases',17,(117,148,169))
    elif scene==1:
        tag(c,53,594,'ONE FRESH BANK · ONE USE',PURPLE,16)
        c.text(53,638,'Algorithm 1 · selected carry boundary',16,(117,148,169))
    else:
        c.text(53,548,'128 → 32',45,color,'bold')
        c.text(54,605,'KEY SWITCHES IN THE EXAMPLE LAYER',14,MUTED,'bold')
        c.text(54,635,'Figure 2 · C = 2, 256 → 64 channels',16,(117,148,169))


def phase_scene(c,p):
    q=smooth((p-.1)/.35)
    reveal=smooth((p-.46)/.22)
    left_panel(c,0,CYAN)
    # A and B remain local to different parties, as in Figure 1(b).
    cap(c,505,222,'SERVER-LOCAL PHASE',CYAN)
    cap(c,505,396,'CLIENT-LOCAL PHASE',PURPLE)
    c.text(508,251,'A′ =' if q>.65 else 'A =',29,INK,'body')
    c.text(508,425,'B =',29,INK,'body')
    c.text(1189,222,'LOW 2 BITS',16,(233,194,125),'bold')
    c.text(1189,396,'LOW 2 BITS',16,(233,194,125),'bold')
    highx=593; highw=525
    for y,col,label in ((248,CYAN,'4a' if q>.65 else '4ã'),(422,PURPLE,'4b')):
        tile(c,highx,y,highw,75,'',col,.9,28,7)
        for i in range(24):
            bx=highx+18+i*20
            c.line([(bx,y+19),(bx,y+55)],mix(BG,col,.14+.07*(i%4)),2)
        c.rect((highx+201,y+12,highx+319,y+63),mix((9,22,36),col,.08),9)
        c.centered(highx+260,y+22,label,31,INK,'body')
        c.text(1139,y+19,'+',27,MUTED)
    for y,label in ((248,'r'),(422,'t')):
        tile(c,1186,y,147,75,label,(236,194,126),.85,30,7)
        c.line([(1260,y+6),(1260,y+69)],(129,107,76),1)
        c.rect((1236,y+19,1284,y+57),(35,37,37),6)
        c.centered(1260,y+20,label,30,(251,219,154),'body')
    # The correction enters only the quotient part; the gold low-bit cell is fixed.
    cx=lerp(614,846,q); cy=lerp(346,278,q)
    if q<.98:
        c.arrow([(663,367),(853,367),(853,329)],(43,108,120),1)
        tile(c,cx,cy,155,39,'−4 corr',CYAN,.9,23,3)
    else:
        tag(c,654,344,'a ≡ ã − corr  (mod 2³⁷)',CYAN,20)
    c.line([(1200,335),(1320,335)],(214,173,107),2)
    c.centered(1260,345,'r unchanged',18,(236,194,126),'body')
    # Both local paths feed protected reconstruction; no phase is opened.
    c.arrow([(1338,287),(1360,287),(1360,522),(804,522),(804,536)],(76,93,108),1)
    c.arrow([(1338,463),(1360,463)],(76,93,108),1)
    c.rect((505,541,1110,639),mix(BG,CYAN,.07),14,(50,100,119))
    cap(c,529,554,'PROTECTED QUARTER RECONSTRUCTION',CYAN)
    c.text(529,585,'z ≡ b − a + ω(t,r)  (mod 2³⁷)',27,INK,'body')
    c.arrow([(1114,592),(1141,592),(1141,568),(1155,568)],(85,154,168),1)
    for r in range(2):
        for col in range(3):
            i=r*3+col
            tile(c,1160+col*64,550+r*44,54,34,'z'+sub(i),CYAN,.35+.6*reveal,19,4)
    c.text(1160,647,'shared integer',18,MUTED)
    c.text(507,655,'A′ = A − 4 corr     ·     39-bit local phases',18,MUTED,'body')


def carry_scene(c,p):
    left_panel(c,1,PURPLE)
    mask=smooth((p-.1)/.28)
    forwarded=smooth((p-.4)/.25)
    produced=smooth((p-.62)/.22)
    cap(c,510,219,'FRESH ROT BANK',PURPLE)
    tile(c,510,252,70,48,'k₀',PURPLE,.9,26)
    tile(c,592,252,70,48,'k₁',PURPLE,.9,26)
    c.arrow([(675,276),(742,276)],(115,97,150),2)
    tile(c,754,246,276,62,'α = (k₀+k₁) mod 2',PURPLE,.9,23)
    c.text(1110,225,'PREPARE BEFORE LOOKUP',15,MUTED,'bold')
    c.text(1109,255,'local mask dependency',19,MUTED)
    c.arrow([(890,316),(890,340),(637,340),(637,367)],(98,83,131),1)
    # The two panels are dependency stages, not parties or transmitted secrets.
    c.rect((510,366,800,471),(18,24,44),15,(94,78,134))
    cap(c,531,381,'PRECEDING LOOKUP',PURPLE)
    c.text(532,414,'program its output mask',21,INK)
    mx=lerp(859,674,mask);my=lerp(267,443,mask)
    if mask<.98:
        tile(c,mx,my,59,38,'α',PURPLE,.95,24,3)
    else:
        tile(c,538,448,79,43,'α',PURPLE,.95,26,3)
        tile(c,639,448,137,43,'β = d xor α',CYAN,.9,19,3)
    c.arrow([(808,417),(985,417)],(99,88,143),2)
    if mask>.7:
        c.packet([(817,417),(978,417)],min(.999,forwarded),PURPLE,5)
    c.text(818,370,'incoming carry d',18,(187,175,224))
    c.text(825,442,'Boolean shares (α,β)',16,MUTED)
    c.rect((992,366,1355,471),(20,25,44),15,(104,88,145))
    cap(c,1016,381,'NEXT PREFIX LOOKUP',PURPLE)
    c.text(1016,414,'advance + derive arithmetic shares',19,INK)
    c.arrow([(1092,474),(1092,512),(790,512),(790,536)],(87,117,127),1)
    c.arrow([(1272,474),(1272,536)],(156,126,78),1)
    tile(c,524,542,239,71,'u_A = α + 2K',CYAN,.35+.6*produced,23,5)
    tile(c,777,542,239,71,'u_B = β − 2Q',CYAN,.35+.6*produced,23,5)
    tile(c,1068,542,287,71,'next Boolean carry',(237,193,117),.4+.5*produced,22,5)
    math_text(c,529,635,'u_A + u_B ≡ d  (mod 2ʷ)',28,INK)
    c.text(1068,630,'distinct outgoing sharing',17,MUTED)
    math_text(c,530,674,'u_A mod 2 = α   ·   u_B mod 2 = β',18,CYAN)


def contraction_scene(c,p):
    left_panel(c,2,(160,231,204))
    cap(c,508,222,'SAME INPUT · TWO PRIVATE SHORT WEIGHT PACKS',CYAN)
    c.text(508,343,'Enc(Pᵢ)',26,INK)
    for i in range(4):
        tile(c,510+i*49,386,42,46,'x'+sub(i),CYAN,.85,22,6)
    c.rect((503,379,708,442),None,11,(44,129,152),1)
    c.text(508,467,'128 input ciphertexts',19,MUTED)
    c.text(508,496,'reuse the same pack',19,CYAN)
    branches=[[(714,410),(745,410),(745,313),(789,313)],
              [(714,410),(745,410),(745,513),(789,513)]]
    for row,(y,color) in enumerate(((288,CYAN),(488,PURPLE))):
        c.arrow(branches[row],(67,103,127),1)
        c.packet(branches[row],p*.9,color,4)
        tile(c,794,y,92,47,'X⁻¹',color,.9,26,4)
        c.text(905,y+5,'×',32,MUTED)
        cap(c,945,y-29,'PRIVATE W'+sub(row)+',ᵢ',color)
        letters=('b','c')[row]
        for j,value in enumerate((letters+'₁',letters+'₀','0','0')):
            tile(c,944+j*48,y,41,47,value,color,.8 if j<2 else .3,21,4)
        c.arrow([(1145,y+24),(1195,y+24)],(64,104,126),1)
        c.circle(1216,y+24,23,(13,35,47),(86,153,164),1)
        c.centered(1216,y+6,'Σᵢ',23,INK)
        c.arrow([(1244,y+24),(1278,y+24)],(68,126,143),1)
        tile(c,1287,y,72,47,'Z'+sub(row),color,.75,24,4)
    c.arrow([(1324,340),(1324,385)],(62,120,135),1)
    c.arrow([(1324,482),(1324,443)],(92,88,137),1)
    c.circle(1324,414,27,(17,40,49),(117,204,184),1)
    c.centered(1324,396,'M',29,(171,236,208),'body')
    c.text(993,400,'contract, then merge',23,(177,204,218))
    c.rect((504,588,1363,642),mix(BG,CYAN,.055),12,(50,85,103))
    c.centered(933,600,'Zᵣ = Σᵢ Wᵣ,ᵢ X⁻¹ Pᵢ',28,INK,'body')
    c.text(508,659,'256 × 56 × 56  →  64 × 56 × 56',22,(177,204,218))
    c.text(1072,659,'32 protected output CT',18,MUTED)


def trace_scene(c,p):
    left_panel(c,2,(160,231,204))
    q=smooth((p-.1)/.76)
    cap(c,508,222,'16 CONSECUTIVE COEFFICIENTS · C = 2',CYAN)
    c.text(507,251,'V = A + U',26,INK)
    c.text(827,251,'σ₁(T),  T = A − U',24,INK)
    c.text(1161,251,'M = V + σ₁(T)',24,INK)
    origins=(506,827,1150)
    step=53;cell=45
    active=min(15,int(q*16))
    for i in range(16):
        row,col=divmod(i,4)
        xx=col*step;yy=302+row*step
        selected=i<=active
        local=smooth(q*16-i)
        color=CYAN if i%2==0 else PURPLE
        # v and t show the two summands; the signed t term removes its complement.
        tile(c,origins[0]+xx,yy,cell,cell,'v'+sub(i),color,.55+.4*(i==active),21,4)
        tile(c,origins[1]+xx,yy,cell,cell,('−' if i%2 else '')+'t'+sub(i),color,.55+.4*(i==active),20,4)
        out='2'+('a' if i%2==0 else 'u')+sub(i)
        tile(c,origins[2]+xx,yy,cell,cell,out,color,.93 if selected else .19,18,4)
        if i==active:
            for origin in origins:
                c.rect((origin+xx-3,yy-3,origin+xx+cell+3,yy+cell+3),None,10,color,1)
            # Lift the active result into its original coefficient slot.
            if local<.96:
                c.rect((origins[2]+xx,yy,origins[2]+xx+cell,yy+cell),(8,20,32),8)
                tile(c,origins[2]+xx,yy-26*(1-local),cell,cell,out,color,.9,18,8)
    c.text(757,376,'+',35,MUTED)
    c.text(1090,376,'=',35,MUTED)
    c.text(507,522,'A = Z₀    U = XZ₁',21,MUTED)
    c.text(827,522,'σ₁: X → −X',21,(214,194,250))
    c.text(1152,522,'trace factor 2 kept',18,CYAN)
    # Exact symbolic cancellation corresponding to Figure 2(c).
    i=active;idx=sub(i)
    if i%2==0:
        eq=f'(a{idx} + u{idx}) + (a{idx} − u{idx}) = 2a{idx}'
    else:
        eq=f'(a{idx} + u{idx}) − (a{idx} − u{idx}) = 2u{idx}'
    c.rect((504,568,1363,628),mix(BG,CYAN,.055),13,(53,86,104))
    c.centered(933,580,eq,28,INK,'body')
    # Layer-specific cost, not a claim of whole-model latency improvement.
    c.text(508,653,'256 × 56 × 56  →  64 × 56 × 56',23,(177,204,218))
    c.text(1059,653,'8,192 products unchanged',18,MUTED)


def scene_image(base,scene,p):
    image=base.copy();c=Canvas(image)
    if scene<2:
        (phase_scene,carry_scene)[scene](c,p)
    elif p<.32:
        contraction_scene(c,min(1,p/.26))
        if p>.26:
            clean=base.copy();left_panel(Canvas(clean),2,(160,231,204))
            nxt=base.copy();trace_scene(Canvas(nxt),0)
            if p<.29:
                image=Image.blend(image,clean,smooth((p-.26)/.03))
            else:
                image=Image.blend(clean,nxt,smooth((p-.29)/.03))
    else:
        trace_scene(c,(p-.32)/.68)
    return image


def progress_rail(image,scene,p):
    c=Canvas(image)
    labels=('RETAINED PHASES','JOINT CARRY','OUTPUT-DIRECTED MERGE')
    colors=(CYAN,PURPLE,(160,231,204))
    for i in range(3):
        x=53+i*460
        active=i==scene
        c.line([(x,734),(x+414,734)],(35,54,72),2)
        if active:
            c.line([(x,734),(x+414*p,734)],colors[i],3)
        c.text(x,754,f'0{i+1}',19,colors[i] if active else (78,99,119),'mono')
        c.text(x+39,754,labels[i],18,INK if active else (92,118,139),'bold')
    return image


def frame(base,t):
    stage=(t%1)*3
    scene=min(2,int(stage));p=stage-scene
    image=scene_image(base,scene,p)
    if p>.93:
        next_scene=(scene+1)%3
        if p<.965:
            image=Image.blend(image,base,smooth((p-.93)/.035))
        else:
            image=Image.blend(base,scene_image(base,next_scene,0),smooth((p-.965)/.035))
    progress_rail(image,scene,p)
    return image.resize((W,H),Image.Resampling.LANCZOS)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=Path(__file__).resolve().parents[1]/'docs'/'media')
    parser.add_argument('--frames',type=int,default=300)
    parser.add_argument('--preview-only',action='store_true')
    args=parser.parse_args()
    if args.frames<30 or args.frames%3:
        parser.error('--frames must be divisible by 3 and at least 30')
    args.output.mkdir(parents=True,exist_ok=True)
    base=base_image()
    poster=frame(base,.25)
    poster.save(args.output/'aqcf-overview.png',optimize=True)
    if args.preview_only:
        for i in range(3):
            frame(base,(i+.72)/3).save(args.output/f'preview-scene-{i+1}.png')
        frame(base,(2+.18)/3).save(args.output/'preview-contraction.png')
        print('Preview scenes rendered.');return
    # One palette across all scenes keeps typography and cross-fades stable.
    sample=Image.new('RGB',(W,H*3))
    for i in range(3):
        sample.paste(frame(base,(i+.68)/3),(0,i*H))
    palette=sample.quantize(colors=144,method=Image.Quantize.MEDIANCUT)
    frames=[frame(base,i/args.frames).quantize(palette=palette,dither=Image.Dither.NONE) for i in range(args.frames)]
    frames[0].save(args.output/'aqcf-overview.gif',save_all=True,append_images=frames[1:],
                   duration=100,loop=0,disposal=1,optimize=True)
    print(f'Rendered {W} x {H}, {args.frames} frames, {args.frames/10:g} seconds.')
    for name in ('aqcf-overview.gif','aqcf-overview.png'):
        target=args.output/name
        print(f'{name}: {target.stat().st_size:,} bytes')


if __name__=='__main__':
    main()
