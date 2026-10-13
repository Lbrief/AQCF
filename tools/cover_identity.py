"""Bodoni wordmark and a quiet matrix-patterned masthead for the AQCF cover."""
from functools import lru_cache
from PIL import Image, ImageDraw
import cover_drawing as b


@lru_cache(None)
def header_image():
    width, height = 1600, 190
    im = Image.new('RGB', (width * b.S, height * b.S))
    d = ImageDraw.Draw(im)
    # Pearl-lavender field; the clear centre keeps the author line readable.
    for x in range(width * b.S):
        u = x / (width * b.S - 1)
        edge = abs(2 * u - 1) ** 1.25
        color = tuple(round(a + (c - a) * edge)
                      for a, c in zip((247, 244, 250), (230, 222, 241)))
        d.line((x, 0, x, height * b.S), fill=color)

    # Two low-contrast coefficient fields recede towards the wordmark.
    for side in (0, 1):
        for row in range(4):
            for col in range(8):
                x = 55 + col * 39 if side == 0 else 1545 - (col + 1) * 39
                y = 15 + row * 39
                strength = (7 - col) / 7
                shade = tuple(round(a + (c - a) * strength)
                              for a, c in zip((239, 233, 246), (214, 204, 229)))
                area = (x * b.S, y * b.S, (x + 32) * b.S, (y + 32) * b.S)
                if (row + col * 2 + side) % 5 == 0:
                    d.rectangle(area, fill=shade)
                else:
                    d.rectangle(area, outline=shade, width=b.S)

    # Use the user's chosen Bodoni Black letterforms, with optical centring.
    font = b.font(133, 'brand')
    word = 'aqcf'
    left, top, right, bottom = d.textbbox((0, 0), word, font=font)
    x = (width * b.S - (right - left)) / 2 - left
    y = 21 * b.S - top
    d.text((x, y), word, font=font, fill='#2d193f')

    names = [name.strip() for name in b.AUTHORS.split('·')]
    size = 24
    author_font = b.font(size, 'sans')
    widths = [d.textlength(name, font=author_font) / b.S for name in names]
    gap = 47
    x = (width - sum(widths) - gap * (len(names) - 1)) / 2
    for i, name in enumerate(names):
        b.draw_text(im, x, 159, name, size, '#51455e', 'sans', 'lm')
        x += widths[i]
        if i < len(names) - 1:
            cx = x + gap / 2
            d.ellipse(((cx - 1.4) * b.S, 157.6 * b.S,
                       (cx + 1.4) * b.S, 160.4 * b.S), fill='#a697b5')
        x += gap
    return im


def draw_header(im):
    im.paste(header_image(), (0, 0))
