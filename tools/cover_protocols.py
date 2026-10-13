"""Dense, two-party AQCF mechanism diagrams for the academic cover.

This module draws only the lower panel on a 1600 x 1380 design canvas.
The supplied image uses the supersampling factor in cover_drawing.
The two numerical examples are independent illustrations, not one execution.
"""

import math
import re
from PIL import ImageDraw
import cover_drawing as base

P = '#452267'
T = '#146d78'
C = '#b75d50'
INK = '#241933'
MUTED = '#7b7085'
LINE = '#dcd3e7'
PL = '#f5f0fa'
TL = '#eff8f8'
RL = '#faece7'
WHITE = '#ffffff'


def text(im, x, y, value, size=17, color=INK, style='sans', anchor='lm'):
    if re.search(r'(?:u[AB]|s[SC])',value):
        d=ImageDraw.Draw(im)
        tokens=re.split(r'(u[AB]|s[SC])',value)
        def width(token):
            if re.fullmatch(r'u[AB]|s[SC]',token):
                return d.textlength(token[0],font=base.font(size,style))/base.S+d.textlength(token[1],font=base.font(size*.64,style))/base.S+2
            return d.textlength(token,font=base.font(size,style))/base.S
        total=sum(width(token) for token in tokens)
        xx=x-total/2 if anchor=='mm' else x-total if anchor=='rm' else x
        for token in tokens:
            if re.fullmatch(r'u[AB]|s[SC]',token):
                base.draw_text(im,xx,y,token[0],size,color,style,'lm')
                ux=d.textlength(token[0],font=base.font(size,style))/base.S
                base.draw_text(im,xx+ux,y+size*.22,token[1],size*.64,color,style,'lm')
            else:base.draw_text(im,xx,y,token,size,color,style,'lm')
            xx+=width(token)
        return
    base.draw_text(im, x, y, value, size, color, style, anchor)


def rect(im, box, fill, outline=None, width=1, radius=3):
    base.rect(im, box, fill, outline, width, radius)


def line(im, points, color=LINE, width=1):
    base.line(im, points, color, width)


def arrow(im, a, b, color=P, width=1.5, head=6):
    base.arrow(im, a, b, color, width, head)


def dot(im, x, y, color, radius=3.4):
    ImageDraw.Draw(im).ellipse(base.pos((x-radius, y-radius, x+radius, y+radius)), fill=color)


def flow(im, points, color, t, begin, end, width=1.5, head=6):
    """A stationary dependency/message line with one travelling activity dot."""
    line(im, points, color, width)
    arrow(im, points[-2], points[-1], color, width, head)
    if not begin <= t < end:
        return
    progress = (t-begin)/(end-begin)
    lengths = [math.dist(a, b) for a, b in zip(points, points[1:])]
    remaining = progress * sum(lengths)
    for a, b, length in zip(points, points[1:], lengths):
        if remaining <= length:
            q = remaining / max(length, 1e-9)
            dot(im, a[0]+q*(b[0]-a[0]), a[1]+q*(b[1]-a[1]), color)
            return
        remaining -= length


def bitstrip(im, x, y, high, low, color, active=False):
    """A compact 37-bit field plus its two literal low bits."""
    width = 205
    fill = '#eadff4' if color == P else '#dceff0'
    rect(im, (x, y, x+width, y+31), fill, color if active else LINE, 1.7 if active else .8, 2)
    for k in range(1, 13):
        xx = x+k*width/13
        line(im, [(xx, y+3), (xx, y+8)], color, .55)
        line(im, [(xx, y+23), (xx, y+28)], color, .55)
    text(im, x+width/2, y+16, str(high), 20, color, 'math', 'mm')
    for j, bit in enumerate(f'{low:02b}'):
        xx = x+width+5+j*25
        rect(im, (xx, y, xx+22, y+31), RL, C, 1.15, 2)
        text(im, xx+11, y+16, bit, 19, C, 'math', 'mm')


def token(im, x, y, value, color=P, fill=PL, width=55, height=32):
    rect(im, (x, y, x+width, y+height), fill, color, 1, 3)
    text(im, x+width/2, y+height/2, value, 18, color, 'math', 'mm')


def draw_lower(im, t):
    """Draw both complete mechanisms; t is seconds in a 30-second loop."""
    t %= 30
    # Shared editorial rule, with two mechanism regions rather than floating cards.
    line(im, [(60, 871), (1540, 871)], P, 1.4)
    text(im, 60, 892, '02  Retained-phase interface', 24, P, 'bold')
    text(im, 815, 892, '03  Joint Boolean–arithmetic carry', 24, P, 'bold')
    line(im, [(799, 913), (799, 1265)], LINE, 1)

    # LEFT: party-local normalized phases. Only c1 crosses this lane.
    rect(im, (60, 916, 397, 1129), PL, None, radius=0)
    rect(im, (416, 916, 790, 1129), TL, None, radius=0)
    text(im, 76, 931, 'SERVER · local state', 15, P, 'bold')
    text(im, 434, 931, 'CLIENT · local state', 15, T, 'bold')
    text(im, 76, 955, 'Retain R = −d₀', 18, P, 'math')
    text(im, 694, 955, 'receive c₁', 15, T, 'sans', 'rm')
    # This is the ciphertext component, not the retained phase or key.
    flow(im, [(295, 955), (557, 955)], P, t, 16.0, 17.0)
    rect(im, (390, 942, 457, 966), WHITE, LINE, .7, 2)
    text(im, 423.5, 954, 'c₁', 19, P, 'math', 'mm')

    text(im, 76, 983, 'A = 55 = 4·13 + 3', 19, P, 'math')
    bitstrip(im, 77, 997, 13, 3, P, 17 <= t < 18)
    text(im, 77, 1045, 'corr = 2   →   subtract 4corr = 8', 15, C, 'math')
    flow(im, [(360, 1013), (377, 1013), (377, 1093), (360, 1093)], C, t, 17.2, 18.5)
    text(im, 76, 1070, 'A′ = 47 = 4·11 + 3', 19, P, 'math')
    bitstrip(im, 77, 1084, 11, 3, P, 18 <= t < 20)

    text(im, 436, 983, 'B = 76 = 4·19 + 0', 19, T, 'math')
    bitstrip(im, 436, 997, 19, 0, T, 17 <= t < 19)
    text(im, 440, 1051, '37 high bits', 14, MUTED)
    text(im, 680, 1051, '2 low bits', 14, C, 'sans', 'mm')
    line(im, [(439, 1034), (640, 1034)], T, 1)
    line(im, [(648, 1034), (694, 1034)], C, 1)
    text(im, 438, 1080, 'Client low bits: t = 0 (00₂)', 18, C, 'math')
    text(im, 438, 1110, 'Correction preserves the low two bits.', 14, MUTED)

    # Protected relation: arrows stop at the shared-operation band and never
    # depict either phase being opened to the other party.
    flow(im, [(208, 1120), (208, 1139), (290, 1139)], P, t, 19.0, 20.0)
    flow(im, [(597, 1120), (597, 1139), (528, 1139)], T, t, 19.0, 20.0)
    rect(im, (290, 1127, 528, 1153), WHITE, LINE, 1, 3)
    text(im, 409, 1140, 'PROTECTED QUARTER RELATION', 12, MUTED, 'bold', 'mm')
    text(im, 76, 1175, 'ω(0, 3) = −1', 22, C, 'math')
    text(im, 345, 1175, 'z ≡ 19 − 11 − 1 = 7', 22, P, 'math')
    text(im, 76, 1203, 'ω is the quarter correction, not the floor carry.', 14, MUTED)
    token(im, 79, 1224, 'sS', P, PL, 64, 33)
    token(im, 174, 1224, 'sC', T, TL, 64, 33)
    text(im, 150, 1240, '+', 21, MUTED, 'math', 'mm')
    text(im, 253, 1240, 'sS + sC ≡ 7 (mod 2³⁷)', 21, P, 'math')
    text(im, 766, 1240, 'shared output', 14, MUTED, 'sans', 'rm')

    # RIGHT: setup is local and keys never appear on the communication arrows.
    rect(im, (815, 916, 1147, 1155), PL, None, radius=0)
    rect(im, (1164, 916, 1540, 1155), TL, None, radius=0)
    text(im, 832, 932, 'SENDER', 15, P, 'bold')
    text(im, 1182, 932, 'RECEIVER', 15, T, 'bold')
    text(im, 1525, 932, 'fresh bank · w = 25', 14, MUTED, 'sans', 'rm')
    text(im, 832, 960, 'k₀ = 6       k₁ = 9', 19, P, 'math')
    text(im, 1182, 960, 'r = 0       kᵣ = 6', 19, T, 'math')
    text(im, 832, 986, 'α = (6 + 9) mod 2 = 1', 18, P, 'math')
    text(im, 832, 1008, 'D = α + k₀ + k₁ = 16', 16, MUTED, 'math')
    flow(im, [(1058, 1008), (1282, 1008)], P, t, 21.0, 22.0)
    rect(im, (1121, 996, 1221, 1020), WHITE, LINE, .8, 2)
    text(im, 1171, 1008, 'D/2 = 8', 17, P, 'math', 'mm')

    # The preceding lookup is a protocol boundary, not a transmission of alpha.
    line(im, [(831, 1027), (1523, 1027)], LINE, .8)
    text(im, 832, 1044, 'PROGRAM α BEFORE CARRY SHARING', 11.8, P, 'bold')
    text(im, 1182, 1044, 'PRECEDING LOOKUP · local output', 12, T, 'bold')
    token(im, 834, 1060, 'α = 1', P, PL, 91, 32)
    text(im, 945, 1076, 'local output mask', 14, MUTED)
    token(im, 1183, 1060, 'lookup output β = 0', T, TL, 232, 32)
    flow(im, [(1281, 1108), (1058, 1108)], T, t, 23.0, 24.0)
    rect(im, (1058, 1096, 1315, 1120), WHITE, LINE, .8, 2)
    text(im, 1186, 1108, 'Existing prefix correction · low bit e = 0', 12, T, 'sans', 'mm')
    text(im, 832, 1138, 'K = kₑ = 6', 18, P, 'math')
    text(im, 1182, 1138, 'Q = kᵣ = 6  (β = 0)', 18, T, 'math')

    text(im, 829, 1177, 'uA = α + 2K = 13', 21, P, 'math')
    text(im, 1180, 1177, 'uB = β − 2Q = −12', 21, T, 'math')
    if 24 <= t < 26:
        line(im, [(830, 1193), (1078, 1193)], P, 2.3)
        line(im, [(1181, 1193), (1453, 1193)], T, 2.3)
    text(im, 829, 1214, 'Boolean:  1 xor 0 = 1', 19, P, 'math')
    text(im, 1180, 1214, 'Arithmetic:  13 − 12 = 1', 19, T, 'math')
    text(im, 829, 1240, 'Example: d = 1, β = d xor α; signed display mod 2²⁵.', 13.1, MUTED)
    # A separate outlined symbol makes the next carry visibly distinct.
    rect(im, (1336, 1231, 1535, 1260), WHITE, LINE, .8, 2)
    text(im, 1435.5, 1245, 'next d′ · separate Boolean shares', 11.5, MUTED, 'sans', 'mm')
    if 26 <= t < 28:
        rect(im, (1336, 1231, 1535, 1260), None, C, 1.5, 2)

    line(im, [(60, 1270), (1540, 1270)], LINE, .9)
    text(im, 800, 1287,
         'Independent numerical illustrations · protected values shown for explanation · no phases or keys are opened',
         14, MUTED, 'sans', 'mm')
    return im

