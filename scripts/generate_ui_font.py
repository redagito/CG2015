"""Regenerate the bundled ASCII UI atlas (development only; requires Pillow)."""
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parents[1]
font = ImageFont.truetype(str(root / 'data/ui/fonts/Rajdhani-SemiBold.ttf'), 128)
atlas = Image.new('L', (2560, 960), 0)
draw = ImageDraw.Draw(atlas)
ascent, descent = font.getmetrics()
glyphs = {}
for index, code in enumerate(range(32, 127)):
    char = chr(code)
    left, top, right, bottom = font.getbbox(char, anchor='ls')
    x, y = (index % 16) * 160 + 8, (index // 16) * 160 + 8
    draw.text((x - left, y - top), char, font=font, fill=255, anchor='ls')
    glyphs[str(code)] = dict(x=x, y=y, width=right-left, height=bottom-top,
                             offsetX=left, offsetY=top, advance=font.getlength(char))
atlas.save(root / 'data/ui/font.png')
(root / 'data/ui/font.json').write_text(json.dumps(dict(size=128, ascent=ascent,
    descent=descent, width=atlas.width, height=atlas.height, glyphs=glyphs), indent=2)+'\n')
