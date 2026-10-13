"""Shared drawing primitives for the AQCF academic cover."""
from functools import lru_cache
from pathlib import Path
import math
import os
import shutil
from PIL import ImageDraw, ImageFont

S=2
WHITE='#ffffff'; INK='#241933'; PURPLE='#452267'; MUTED='#7b7085'
LINE='#e0d8e8'; LAVENDER='#ede2f6'; TEAL='#146d78'; MINT='#e0f1f1'
CORAL='#b75d50'; ROSE='#faece7'
AUTHORS='Keliang Xing   ·   Ru Xue   ·   Hejuan Du   ·   Guangxiu Pan   ·   Jianglu Wen'

def ffmpeg_executable():
    executable=shutil.which('ffmpeg')
    if executable:
        return executable
    try:
        import imageio_ffmpeg
        return imageio_ffmpeg.get_ffmpeg_exe()
    except ImportError as exc:
        raise RuntimeError('Install FFmpeg or run: python -m pip install imageio-ffmpeg') from exc

@lru_cache(None)
def font(size,style='sans'):
    preferred={'sans':'segoeui.ttf','bold':'seguisb.ttf','light':'segoeuil.ttf','serif':'georgia.ttf','math':'cambria.ttc','italic':'georgiai.ttf','mono':'consola.ttf'}
    fallback={'sans':'DejaVuSans.ttf','bold':'DejaVuSans-Bold.ttf','light':'DejaVuSans.ttf','serif':'DejaVuSerif.ttf','math':'DejaVuSerif.ttf','italic':'DejaVuSerif-Italic.ttf','mono':'DejaVuSansMono.ttf'}
    roots=[Path(os.environ.get('WINDIR','C:/Windows'))/'Fonts',Path('/usr/share/fonts/truetype/dejavu'),Path('/usr/local/share/fonts'),Path('/Library/Fonts')]
    if os.environ.get('AQCF_FONT_DIR'):
        roots.insert(0,Path(os.environ['AQCF_FONT_DIR']))
    for root in roots:
        for name in (preferred[style],fallback[style]):
            path=root/name
            if path.is_file():return ImageFont.truetype(str(path),round(size*S))
    raise RuntimeError('Install DejaVu fonts, or point AQCF_FONT_DIR to a font directory.')

def pos(p):return tuple(round(x*S) for x in p)

def draw_text(im,x,y,s,size=25,color=INK,style='sans',anchor='lm'):
    if '⁻' in s:style='math'
    ImageDraw.Draw(im).text(pos((x,y)),s,font=font(size,style),fill=color,anchor=anchor)

def line(im,pts,color=LINE,width=1):
    ImageDraw.Draw(im).line([pos(p) for p in pts],fill=color,width=round(width*S),joint='curve')

def rect(im,box,fill,outline=None,width=1,radius=0):
    d=ImageDraw.Draw(im)
    if radius:d.rounded_rectangle(pos(box),radius*S,fill=fill,outline=outline,width=round(width*S))
    else:d.rectangle(pos(box),fill=fill,outline=outline,width=round(width*S))

def bracket(im,x,y,width,height,color=INK):
    line(im,[(x+7,y),(x,y),(x,y+height),(x+7,y+height)],color,1.6)
    line(im,[(x+width-7,y),(x+width,y),(x+width,y+height),(x+width-7,y+height)],color,1.6)

def arrow(im,start,end,color=PURPLE,width=1.7,head=7):
    line(im,[start,end],color,width)
    a=math.atan2(end[1]-start[1],end[0]-start[0])
    p=[end,(end[0]-head*math.cos(a-.48),end[1]-head*math.sin(a-.48)),(end[0]-head*math.cos(a+.48),end[1]-head*math.sin(a+.48))]
    ImageDraw.Draw(im).polygon([pos(q) for q in p],fill=color)
