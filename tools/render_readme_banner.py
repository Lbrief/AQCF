"""Render a full AQCF algorithm plate with three animated mechanisms."""
from pathlib import Path
import math
import json
import subprocess
import argparse
from PIL import Image, ImageDraw
import cover_drawing as b
from cover_protocols import draw_lower, flow
from cover_identity import header_image

ROOT=Path(__file__).resolve().parents[1]/'docs/media'
W,H,FPS,DURATION=1600,1380,20,30
P=b.PURPLE;T=b.TEAL;C=b.CORAL;INK=b.INK;MUTED=b.MUTED;LINE=b.LINE
WM=[[2,1,-1,0],[1,3,0,1],[0,-1,2,1],[1,0,1,2]]
XM=[[1,3,2,0],[2,4,1,3],[0,1,2,1],[1,2,0,4]]
YM=[[sum(WM[i][k]*XM[k][j] for k in range(4)) for j in range(4)] for i in range(4)]
P0=[XM[i][j] for j in range(4) for i in (0,1)]
P1=[XM[i][j] for j in range(4) for i in (2,3)]
def conv(a,c):
    out=[0]*(len(a)+len(c)-1)
    for i,x in enumerate(a):
        for j,y in enumerate(c):out[i+j]+=x*y
    return out
Z0=[x+y for x,y in zip(conv(P0,[1,2]),conv(P1,[0,-1]))]
Z1=[x+y for x,y in zip(conv(P0,[3,1]),conv(P1,[1,0]))]
A=Z0+[0];U=[0]+Z1
V=[a+u for a,u in zip(A,U)];DIFF=[a-u for a,u in zip(A,U)]
SIG=[-a if i%2 else a for i,a in zip(range(-1,9),DIFF)]
MERGED=[a+v for a,v in zip(V,SIG)]
assert MERGED == [0]+[2*YM[i][j] for j in range(4) for i in (0,1)]+[0]

def txt(im,x,y,s,size=22,color=INK,style='sans',anchor='lm'):
    b.draw_text(im,x,y,s,size,color,style,anchor)
def box(im,area,fill='#fff',stroke=LINE,width=1,radius=3):b.rect(im,area,fill,stroke,width,radius)
def line(im,pts,color=LINE,width=1):b.line(im,pts,color,width)
def arrow(im,a,c,color=P,width=1.5):b.arrow(im,a,c,color,width)

def matrix4(im,x,y,vals,hot_rows=(),hot_cols=(),accent=P,cell=33):
    for r in range(4):
        for c in range(4):
            hot=r in hot_rows or c in hot_cols
            fill=b.LAVENDER if accent==P else b.MINT
            box(im,(x+c*cell+1,y+r*cell+1,x+(c+1)*cell-1,y+(r+1)*cell-1),fill if hot else '#fcfafc',accent if hot else LINE,.65,1)
            txt(im,x+(c+.5)*cell,y+(r+.5)*cell,str(vals[r][c]).replace('-','−'),20,accent if hot else INK,'math','mm')
    b.bracket(im,x-6,y-3,4*cell+12,4*cell+6,accent)

def vector(im,x,y,values,cell=36,height=30,colors=None,hot=-1,font=18):
    for j,v in enumerate(values):
        col=colors[j] if colors else (b.WHITE,INK)
        box(im,(x+j*cell,y,x+(j+1)*cell-3,y+height),col[0],P if j==hot else LINE,1.6 if j==hot else .7,1)
        txt(im,x+(j+.5)*cell-1.5,y+height/2,str(v).replace('-','−'),font,col[1],'math','mm')

def block(im,area,label,size=20,active=False,color=P):
    box(im,area,b.LAVENDER if active and color==P else b.MINT if active else '#fff',color,1.4 if active else .8,4)
    txt(im,(area[0]+area[2])/2,(area[1]+area[3])/2,label,size,color,'math','mm')

def frame(t):
    im=Image.new('RGB',(W*b.S,H*b.S),'white')
    box(im,(60,191,1540,242),P,P,1,0)
    txt(im,800,216,'Exact private inference  ·  algorithm overview',28,'white','bold','mm')
    # A single enclosing technical plate, matching the supplied academic format.
    box(im,(60,263,1540,852),'white',P,1.1,0)
    box(im,(61,264,1539,308),'#f2ecf7',None,0,0)
    txt(im,81,287,'01  Shape-directed encrypted linear evaluation',27,P,'bold')
    txt(im,1519,287,'4 × 4 example  ·  C = 2',19,MUTED,'sans','rm')

    # Full matrices are retained as context during all later protocol stages.
    c=min(15,int(t/.36));rr,cc=divmod(c,4)
    active=t<5.76
    txt(im,162,331,'W',25,P,'italic','mm')
    txt(im,375,331,'H',25,T,'italic','mm')
    txt(im,588,331,'Y = WH',25,P,'italic','mm')
    matrix4(im,96,350,WM,(rr,) if active else (),accent=P)
    matrix4(im,309,350,XM,hot_cols=(cc,) if active else (),accent=T)
    matrix4(im,522,350,YM,(rr,) if active else (0,1),accent=P)
    if active:
        box(im,(522+cc*33+1,350+rr*33+1,522+(cc+1)*33-1,350+(rr+1)*33-1),None,P,2,1)
    txt(im,270,416,'×',31,MUTED,'math','mm');txt(im,481,416,'=',31,MUTED,'math','mm')
    txt(im,162,498,'Private weights',16,MUTED,'sans','mm')
    txt(im,375,498,'Client input',16,MUTED,'sans','mm')
    txt(im,588,498,'Two output groups',16,MUTED,'sans','mm')
    flow(im,[(444,450),(465,450),(465,511),(724,511),(724,390),(747,390)],T,t,5.8,6.5)
    txt(im,941,330,'Channel-interleaved coefficient packs',20,T,'bold','mm')
    txt(im,754,371,'P₀',22,T,'math');txt(im,754,426,'P₁',22,T,'math')
    packhot=min(7,int((t-6.5)/.4)) if 6.5<=t<9.7 else -1
    cols=[(b.MINT,T) if i%2==0 else (b.LAVENDER,P) for i in range(8)]
    vector(im,799,355,P0,36,32,cols,packhot,20)
    vector(im,799,410,P1,36,32,cols,packhot,20)
    txt(im,941,469,'Pᵢ = Σⱼ (h₂ᵢ,ⱼ + h₂ᵢ₊₁,ⱼ X) X²ʲ',21,T,'math','mm')
    line(im,[(1124,324),(1124,495)],LINE)
    txt(im,1328,333,'Reversed short weight packs',21,P,'bold','mm')
    txt(im,1309,369,'input group 0       input group 1',15,MUTED,'sans','mm')
    txt(im,1160,410,'row 0',17,MUTED);txt(im,1160,460,'row 1',17,MUTED)
    block(im,(1223,389,1340,426),'1 + 2X',22,9.7<=t<11)
    block(im,(1362,389,1479,426),'−X',22,9.7<=t<11)
    block(im,(1223,439,1340,476),'3 + X',22,11<=t<12.3)
    block(im,(1362,439,1479,476),'1',22,11<=t<12.3)
    line(im,[(80,521),(1520,521)],LINE)

    # Ciphertext dependency graph, including shifts before channel contraction.
    txt(im,88,547,'Cloud evaluation · first output group',22,P,'bold')
    txt(im,220,591,'Encrypted input packs',17,T,'bold','mm')
    vector(im,102,609,['p₀','p₁','p₂','p₃','…'],39,31,[(b.MINT,T)]*5,font=17)
    vector(im,102,653,['q₀','q₁','q₂','q₃','…'],39,31,[(b.MINT,T)]*5,font=17)
    txt(im,219,708,'Enc(P₀), Enc(P₁)',20,T,'math','mm')
    flow(im,[(312,647),(337,647),(337,593),(377,593)],T,t,9.7,10.4)
    flow(im,[(312,647),(337,647),(337,723),(377,723)],T,t,10.6,11.3)
    block(im,(377,574,438,614),'X⁻¹',21,9.7<=t<10.5)
    block(im,(377,704,438,744),'X⁻¹',21,10.6<=t<11.4)
    flow(im,[(438,594),(477,594)],P,t,10.4,10.8)
    flow(im,[(438,724),(477,724)],P,t,11.3,11.7)
    box(im,(477,568,847,620),'#fbf8fd',P,1,4)
    box(im,(477,698,847,750),'#fbf8fd',P,1,4)
    txt(im,662,590,'Z₀ = X⁻¹[(1 + 2X)P₀ − XP₁]',23,P,'math','mm')
    txt(im,662,722,'Z₁ = X⁻¹[(3 + X)P₀ + P₁]',23,P,'math','mm')
    txt(im,480,644,'even coefficients',16,MUTED)
    vector(im,646,631,[4,9,3,2],46,29,[(b.LAVENDER,P)]*4,font=20)
    txt(im,480,774,'even coefficients',16,MUTED)
    vector(im,646,761,[8,17,5,13],46,29,[(b.MINT,T)]*4,font=20)
    flow(im,[(847,594),(865,594),(865,594),(901,594)],P,t,11.8,12.3)
    flow(im,[(847,724),(870,724),(870,641),(901,641)],T,t,12.2,12.8)
    txt(im,869,681,'× X',18,T,'math','mm')
    # Compact coefficient arrays express cancellation rather than a table.
    box(im,(903,541,1519,803),'#fcfbfd',P,1,4)
    txt(im,1212,561,'M = A + U + σ(A − U)     ·     σ(X) = −X',20,P,'math','mm')
    powers=['X⁻¹','1','X','X²','X³','X⁴','X⁵','X⁶','X⁷','X⁸']
    for j,q in enumerate(powers):txt(im,1066+j*43+19,584,q,13,MUTED,'math','mm')
    for label,vals,y in [('A = Z₀',A,599),('U = XZ₁',U,636),('V = A + U',V,673),('σ(A − U)',SIG,710),('M',MERGED,760)]:
        txt(im,919,y+14,label,19,P if label=='M' else INK,'math')
        colors=[]
        for j in range(10):
            degree=j-1
            if label=='A = Z₀' and degree in (0,2,4,6):colors.append((b.LAVENDER,P))
            elif label=='U = XZ₁' and degree in (1,3,5,7):colors.append((b.MINT,T))
            elif label=='M' and j in range(1,9):colors.append((b.LAVENDER,P) if degree%2==0 else (b.MINT,T))
            elif label=='σ(A − U)' and degree%2:colors.append((b.ROSE,C))
            else:colors.append((b.WHITE,INK))
        hot=min(9,int((t-13)/.43)) if 13<=t<17.3 else -1
        vector(im,1066,y,vals,43,28,colors,hot,18)
    line(im,[(916,751),(1506,751)],P,1)
    txt(im,219,778,'Both paths keep their original shift.',16,MUTED,'sans','mm')
    # The tiny algebra example is not the native scheduling cost example.
    txt(im,82,830,'Native layer: 256 → 64 channels · 56 × 56 · C = 2',17,MUTED)
    txt(im,1519,830,'Key switches 128 → 32     |     CT–PT products 8,192 unchanged',18,P,'sans','rm')
    draw_lower(im,t)
    # Diagram legend and a current-step ribbon provide context at all moments.
    txt(im,64,1320,'■  even target coefficients',17,P)
    txt(im,372,1320,'■  odd target coefficients',17,T)
    txt(im,672,1320,'■  sign / low-bit correction',17,C)
    txt(im,1536,1320,'Numbers illustrate algebra; the original integer map and scale are preserved.',16,MUTED,'sans','rm')
    if t<5.8:caption=f'Y[{rr+1},{cc+1}] = '+ ' + '.join(f'({WM[rr][k]} × {XM[k][cc]})' for k in range(4)) + f' = {YM[rr][cc]}'
    elif t<9.7:caption='Pack two input channels per group; reverse the private short weights.'
    elif t<13:caption='Shift both ciphertext paths, multiply their short weight packs, then sum the channel groups.'
    elif t<17.3:
        j=min(9,int((t-13)/.43));caption=f'Merge coefficient {j-1}:   {V[j]} + ({SIG[j]}) = {MERGED[j]}    ·    Original trace factor 2 is retained.'
    elif t<21:caption='Retain the server phase locally; apply the affine correction without changing the low two bits.'
    elif t<26:caption='Program the next prefix mask first, then derive Boolean and arithmetic shares of the incoming carry.'
    else:caption='Exact matrix outputs, compatible local phases, and one incoming carry in two shared representations.'
    line(im,[(60,1341),(1540,1341)],LINE)
    txt(im,800,1360,caption,19,P,'sans','mm')
    result = im.resize((W,H),Image.Resampling.LANCZOS)
    result.paste(header_image().resize((W,190),Image.Resampling.LANCZOS), (0,0))
    return result

def render():
    ffmpeg=b.ffmpeg_executable()
    out=ROOT/'aqcf-overview-master.mkv'
    cmd=[str(ffmpeg),'-hide_banner','-loglevel','warning','-y','-f','rawvideo','-pix_fmt','rgb24','-s',f'{W}x{H}','-r',str(FPS),'-i','pipe:0','-an','-c:v','ffv1','-level','3',str(out)]
    proc=subprocess.Popen(cmd,stdin=subprocess.PIPE)
    for i in range(FPS*DURATION):
        proc.stdin.write(frame(i/FPS).tobytes())
        if i%(FPS*5)==0:print(f'Rendered {i/FPS:.0f}/{DURATION} seconds',flush=True)
    proc.stdin.close()
    if proc.wait():raise RuntimeError('Encoding failed')
    def run(args):subprocess.run([str(ffmpeg),'-hide_banner','-loglevel','warning','-y',*args],check=True)
    run(['-i',str(out),'-filter_complex','fps=15,scale=1440:1242:flags=lanczos,split[a][b];[a]palettegen=max_colors=192:stats_mode=full[p];[b][p]paletteuse=dither=none:diff_mode=rectangle','-loop','0',str(ROOT/'aqcf-overview.gif')])
    run(['-i',str(out),'-c:v','libx264','-crf','17','-preset','medium','-pix_fmt','yuv420p','-movflags','+faststart',str(ROOT/'aqcf-overview.mp4')])

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--stills',action='store_true');p.add_argument('--output',type=Path,default=ROOT);args=p.parse_args()
    ROOT=args.output.resolve();ROOT.mkdir(parents=True,exist_ok=True)
    frame(29).save(ROOT/'aqcf-overview.png')
    (ROOT/'aqcf-cover-algebra.json').write_text(json.dumps({'status':'PASS','W':WM,'H':XM,'Y':YM,'P0':P0,'P1':P1,'A':A,'U':U,'V':V,'T':DIFF,'sigma_T':SIG,'M':MERGED,'scope':'Illustrative first output group; no new inference experiment'},indent=2),encoding='utf-8')
    if not args.stills:render()
    print('Done.',flush=True)
