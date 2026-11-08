# Captures de la notice (a lancer depuis le dossier contenant m_ui.png, m_col.png, m_pick.png rendus par ImpulseTool a l'echelle 2)
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import numpy as np, sys
A = sys.argv[1] if len(sys.argv) > 1 else '/home/claude/Impulse/tools/manual_assets/'
ui = Image.open('m_ui.png').convert('RGB'); W, H = ui.size
# repere du plugin (1876 x 782) ; la fenetre montre le boitier seul (sans fond perdu) : x 16.1.., y 13.2..
VX, VY, VW = 16.1, 13.2, 1876.0 - 2 * 16.1
k = W / VW
font = ImageFont.truetype('/home/claude/Impulse/assets/Inter-Bold.otf', 26)
an = ui.copy(); d = ImageDraw.Draw(an)
pts = {1:(102,51),2:(1192,49),3:(1715,45),4:(1810,45),5:(42,163),6:(622,163),7:(785,163),8:(120,375),9:(628,301),
       10:(776,301),11:(668,645),12:(1006,195),13:(1006,375),14:(1006,550),15:(560,81)}
pts = {n: (x - VX, y - VY) for n, (x, y) in pts.items()}
for n, (x, y) in pts.items():
    cx, cy, r = x * k, y * k, 17
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill='white', outline='black', width=3)
    d.text((cx, cy + 1), str(n), fill='black', font=font, anchor='mm')
an.save(A + 'annotated.png')
Image.open('m_col.png').convert('RGB').save(A + 'colour.png')
Image.open('m_pick.png').convert('RGB').crop((600, 0, 1180, 480)).save(A + 'picker.png')
cw, ch = 1994, 1069; bg = Image.new('RGB', (cw, ch), (0, 0, 0))
x0, y0 = (cw - W) // 2, 110
halo = Image.new('L', (cw, ch), 0); ImageDraw.Draw(halo).rounded_rectangle((x0 - 10, y0 - 10, x0 + W + 10, y0 + H + 10), 30, fill=255)
halo = halo.filter(ImageFilter.GaussianBlur(70))
glow = Image.new('RGB', (cw, ch), (20, 90, 130)); bg = Image.composite(glow, bg, halo.point(lambda v: int(v * 0.55)))
bg.paste(ui, (x0, y0))
ref = ui.transpose(Image.FLIP_TOP_BOTTOM).crop((0, 0, W, ch - (y0 + H) - 6))
g = np.linspace(0.28, 0, ref.size[1])[:, None, None]
base = np.asarray(bg.crop((x0, y0 + H + 6, x0 + W, ch))).astype(float)
bg.paste(Image.fromarray((base * (1 - g) + np.asarray(ref).astype(float) * g).astype('uint8')), (x0, y0 + H + 6))
bg.save(A + 'cover.png')
