"""Render the AQCF README animation. Requires Pillow; no model or data access.

    python -m pip install Pillow
    python tools/render_readme_banner.py

The animation is a conceptual algorithm view, not a protocol execution trace.
The coefficient counts and animation timing are illustrative.
"""
from __future__ import annotations

import argparse
import math
import os
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H, SCALE = 1440, 720, 2
BG = (7, 14, 28)
INK = (236, 246, 255)
MUTED = (153, 177, 201)
CYAN = (74, 225, 241)
PURPLE = (180, 154, 255)
GREEN = (176, 239, 134)
ACCENTS = (CYAN, PURPLE, GREEN)
PANELS = (48, 506, 964)


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


def base_image():
    image = Image.new('RGB', (W*SCALE, H*SCALE), BG)
    glow = Image.new('RGB', image.size, BG)
    d = ImageDraw.Draw(glow)
    d.ellipse((-300, -300, 1350, 680), fill=(12, 46, 60))
    d.ellipse((1900, -200, 3350, 1100), fill=(30, 23, 60))
    glow = glow.filter(ImageFilter.GaussianBlur(150*SCALE))
    image = Image.blend(image, glow, .62)
    c = Canvas(image)
    for x in range(24, W, 36):
        for y in range(22, H, 36):
            c.circle(x, y, .65, (32, 49, 65))
    c.rect((1, 1, W-2, H-2), None, 28, (36, 61, 78))
    c.text(48, 27, 'AQCF', 83, INK, 'hero')
    c.text(51, 120, 'Exact private inference in the cloud', 27, (190, 213, 232))
    c.line([(326, 42), (326, 110)], (50, 78, 96), 1)
    c.text(348, 49, 'CO-DESIGNING', 16, CYAN, 'bold')
    c.text(348, 75, 'ENCRYPTED COMPUTATION', 20, INK, 'bold')
    c.text(348, 101, 'AND SECURE NONLINEAR PROTOCOLS', 16, MUTED, 'body')
    # Client and cloud ownership: two separate private inputs.
    c.rect((1002, 42, 1392, 133), (11, 26, 44), 17, (42, 65, 90))
    c.text(1022, 55, 'CLIENT', 17, CYAN, 'bold')
    c.text(1022, 87, 'private input', 20, INK)
    c.text(1220, 55, 'CLOUD', 17, PURPLE, 'bold')
    c.text(1220, 87, 'private model', 20, INK)
    c.arrow([(1150, 76), (1197, 76)], MUTED, 1)
    c.arrow([(1197, 95), (1150, 95)], MUTED, 1)
    c.text(49, 180, 'THREE ALGORITHMIC MECHANISMS', 16, MUTED, 'bold')
    c.text(1057, 180, 'SHARED VALUES STAY PROTECTED', 15, MUTED, 'body')
    return image


def panel(c, index, t):
    x = PANELS[index]
    color = ACCENTS[index]
    intensity = .34 + .25 * (1 + math.cos(2*math.pi*(t-index/3))) / 2
    c.rect((x, 218, x+428, 615), (11, 23, 40), 19, mix((26, 45, 63), color, intensity*.6))
    c.line([(x+22, 219), (x+406, 219)], mix((26, 45, 63), color, intensity), 2)
    c.rect((x+22, 242, x+60, 276), mix(BG, color, .16), 8)
    c.centered(x+41, 247, f'0{index+1}', 18, color, 'mono')
    c.text(x+73, 244, ['Retain the phase', 'Advance the carry', 'Merge the traces'][index], 27, INK, 'bold')
    c.text(x+24, 288, ['RETAINED-PHASE QUOTIENT', 'MASK-PROGRAMMED JOINT CARRY', 'OUTPUT-DIRECTED TRACE MERGE'][index], 15, color, 'bold')
    c.line([(x+24, 544), (x+404, 544)], (31, 49, 67), 1)
    descriptions = [
        ('Keep one component local.', 'Recover the exact shared quotient.'),
        ('Program the lookup mask.', 'Derive shares while the prefix advances.'),
        ('Combine indexed coefficient traces.', 'Project only the required output groups.'),
    ]
    for j, line in enumerate(descriptions[index]):
        c.text(x+24, 561+j*24, line, 18, MUTED)


def coefficient_strip(c, x, y, color, t, count=6, step=15, height=24):
    for i in range(count):
        light = .27 + .5 * (1+math.cos(2*math.pi*(t-i/count)))/2
        c.rect((x+i*step, y, x+i*step+step-4, y+height), mix((12, 27, 43), color, light), 3)


def retained(c, t):
    x = PANELS[0]
    # Retained c0 phase remains in a cloud-local block; c1 crosses the channel.
    c.rect((x+24, 330, x+219, 403), (10, 32, 46), 12, (38, 92, 108))
    c.text(x+36, 338, 'c0', 20, CYAN, 'mono')
    coefficient_strip(c, x+80, 346, CYAN, t, count=6, height=16)
    c.text(x+36, 371, 'retain as local phase', 16, (165, 202, 216))
    c.text(x+35, 431, 'c1', 20, CYAN, 'mono')
    coefficient_strip(c, x+78, 435, CYAN, t-.18, count=6, height=20)
    c.text(x+78, 461, 'send component', 16, MUTED)
    a = [(x+179, 445), (x+248, 445), (x+248, 426), (x+299, 426)]
    b = [(x+219, 367), (x+248, 367), (x+248, 395), (x+299, 395)]
    c.arrow(a, (57, 124, 145), 2)
    c.arrow(b, (57, 124, 145), 2)
    c.packet(a, t*2, CYAN)
    c.packet(b, t*2+.4, CYAN)
    c.rect((x+299, 365, x+401, 459), (15, 46, 60), 13, (62, 174, 189))
    c.centered(x+350, 379, 'EXACT', 16, CYAN, 'bold')
    c.centered(x+350, 403, 'quotient', 20, INK, 'bold')
    c.centered(x+350, 436, 'shares', 14, MUTED)
    c.text(x+36, 507, 'local phase + protected low-bit correction', 16, MUTED)


def joint(c, t):
    x = PANELS[1]
    c.text(x+24, 329, 'programmed mask', 16, MUTED)
    coefficient_strip(c, x+220, 332, PURPLE, t, count=8, step=18, height=13)
    points = [(x+47, 412), (x+376, 412)]
    c.arrow(points, (89, 81, 128), 2)
    c.packet(points, t*2, PURPLE, 4)
    for i in range(4):
        px = x+65+i*89
        if i == 3:
            c.line([(px, 351), (px, 386)], (112, 95, 157))
            c.circle(px, 359, 2, PURPLE)
        light = .28+.25*(1+math.cos(2*math.pi*(t*2-i/4)))/2
        c.rect((px-26, 388, px+26, 436), mix((17, 25, 44), PURPLE, light*.55), 9,
               mix((67, 61, 100), PURPLE, light))
        c.centered(px, 398, f'd{i}', 21, INK, 'mono')
    c.text(x+286, 365, 'next carry', 14, PURPLE)
    # Only the selected joint stage is drawn with an arithmetic tap.
    # Those shares encode its incoming carry, not its next Boolean carry.
    px=x+332
    path=[(px, 437), (px, 480)]
    c.arrow(path, (119, 101, 160), 2)
    c.packet(path, t*2, PURPLE, 3)
    c.rect((px-25, 487, px+25, 511), (43, 37, 70), 5, (132, 112, 176))
    c.line([(px, 490), (px, 508)], (132, 112, 176), 1)
    c.text(x+27, 461, 'incoming carry', 16, MUTED)
    c.text(x+27, 484, 'arithmetic shares', 16, PURPLE)
    c.line([(x+176, 499), (x+303, 499)], (74, 66, 103))
    c.text(x+24, 514, 'Boolean prefix + selected arithmetic tap', 15, MUTED)


TRACE_COLORS = [(176,239,134), (116,209,177), (74,225,241), (112,188,250),
                (166,165,247), (202,166,243), (234,191,157), (219,226,148)]


def trace(c, t):
    x = PANELS[2]
    c.text(x+24, 329, 'indexed traces', 16, MUTED)
    c.text(x+289, 329, 'output slots', 16, MUTED)
    levels = [[(x+53, 370+i*18) for i in range(8)],
              [(x+139, 379+i*36) for i in range(4)],
              [(x+220, 397+i*72) for i in range(2)], [(x+297, 433)]]
    for level in range(3):
        for i, a in enumerate(levels[level]):
            b = levels[level+1][i//2]
            c.line([a, (a[0]+18, a[1]), b], (57, 83, 75), 1)
    # Bit-reversed leaf order follows the recursive even/odd partition.
    order = (0, 4, 2, 6, 1, 5, 3, 7)
    for i, a in enumerate(levels[0]):
        index = order[i]
        color = TRACE_COLORS[index]
        c.text(x+20, a[1]-9, str(index), 13, MUTED, 'mono')
        c.rect((a[0]-10, a[1]-5, a[0]+6, a[1]+5), mix(BG, color, .75), 2)
        output_y = 370+index*18
        route = [a, levels[1][i//2], levels[2][i//4], levels[3][0], (x+347, 433), (x+347, output_y)]
        c.packet(route, (t*2+i/8), color, 3)
        # Each original index has its own output position: no averaging or discarded output.
        c.rect((x+341, output_y-6, x+364, output_y+6), mix(BG, color, .58), 3)
        c.text(x+376, output_y-9, str(index), 13, color, 'mono')
    for level in (1,2,3):
        for px, py in levels[level]:
            c.circle(px, py, 6, (23, 48, 44), (133, 183, 125), 1)
    c.text(x+99, 514, 'merge', 15, MUTED)
    c.text(x+274, 514, 'project', 15, MUTED)


def frame(base, t):
    image=base.copy()
    c=Canvas(image)
    for i in range(3):
        panel(c,i,t)
    retained(c,t)
    joint(c,t)
    trace(c,t)
    # Small continuous status rail, without fabricated performance measurements.
    c.circle(59, 662, 5, CYAN)
    c.text(76, 648, 'EXACT INTEGER OUTPUTS', 18, INK, 'bold')
    c.line([(360, 662), (457, 662)], (41, 67, 85), 1)
    c.packet([(360, 662), (457, 662)], t, CYAN, 3)
    c.text(478, 648, 'HE AFFINE LAYERS + TWO-PARTY NONLINEAR OPERATIONS', 17, MUTED)
    c.text(1254, 650, 'AQCF / CORE', 16, (112, 139, 160), 'mono')
    return image.resize((W,H),Image.Resampling.LANCZOS)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path(__file__).resolve().parents[1]/'docs'/'media')
    parser.add_argument('--frames', type=int, default=100)
    args=parser.parse_args()
    if args.frames < 12:
        parser.error('--frames must be at least 12')
    args.output.mkdir(parents=True,exist_ok=True)
    base=base_image()
    poster=frame(base,0)
    poster.save(args.output/'aqcf-overview.png', optimize=True)
    palette=poster.quantize(colors=112,method=Image.Quantize.MEDIANCUT)
    frames=[frame(base,i/args.frames).quantize(palette=palette,dither=Image.Dither.NONE) for i in range(args.frames)]
    frames[0].save(args.output/'aqcf-overview.gif', save_all=True, append_images=frames[1:],
                   duration=100, loop=0, disposal=1, optimize=True)
    print(f'Rendered {W} x {H}, {args.frames} frames, {args.frames/10:g} seconds.')
    for name in ('aqcf-overview.gif','aqcf-overview.png'):
        p=args.output/name
        print(f'{name}: {p.stat().st_size:,} bytes')


if __name__=='__main__':
    main()
