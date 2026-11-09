# Captures de la notice (a lancer depuis le dossier contenant m_ui.png, m_col.png, m_pick.png rendus par ImpulseTool a l'echelle 2)
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import numpy as np, sys
A = sys.argv[1] if len(sys.argv) > 1 else '/home/claude/Impulse/tools/manual_assets/'
ui = Image.open('m_ui.png').convert('RGB'); W, H = ui.size
# repere du plugin v005 (3600 x 1440, la fenetre montre tout le rendu)
k = W / 3600.0
font = ImageFont.truetype('/home/claude/Impulse/assets/Inter-Bold.otf', 26)
an = ui.copy(); d = ImageDraw.Draw(an)
pts = {1:(175,62),2:(2300,40),3:(3268,46),4:(3422,46),5:(48,300),6:(1078,300),7:(1309,300),8:(300,640),9:(1203,560),
       10:(1355,560),11:(1255,1180),12:(1915,375),13:(1915,720),14:(1915,1062),15:(1490,122)}
for n, (x, y) in pts.items():
    cx, cy, r = x * k, y * k, 17
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill='white', outline='black', width=3)
    d.text((cx, cy + 1), str(n), fill='black', font=font, anchor='mm')
an.save(A + 'annotated.png')
Image.open('m_col.png').convert('RGB').save(A + 'colour.png')
Image.open('m_pick.png').convert('RGB').crop((610, 0, 1170, 470)).save(A + 'picker.png')
# ---- couverture « minimale » : facade detouree (alpha du fond v005), reflet sur sol noir, halo d'accent tres doux
# cover.png : la facade occupe exactement (PAD, PAD) -> (PAD + W, PAD + H) ; le reste est le reflet et le halo (fond noir).
alpha = Image.open('/home/claude/Impulse/assets/v5/bg.png').getchannel('A').resize((W, H), Image.LANCZOS)
PAD = int(W * 0.16); cw, ch = W + 2 * PAD, H + 2 * PAD
bg = Image.new('RGB', (cw, ch), (0, 0, 0))
halo = Image.new('L', (cw, ch), 0); halo.paste(alpha, (PAD, PAD)); halo = halo.filter(ImageFilter.GaussianBlur(PAD * 0.33))
glow = Image.new('RGB', (cw, ch), (42, 164, 220)); bg = Image.composite(glow, bg, halo.point(lambda v: int(v * 0.30)))
ref = ui.transpose(Image.FLIP_TOP_BOTTOM); ra = alpha.transpose(Image.FLIP_TOP_BOTTOM)
fade = np.linspace(0.22, 0, int(H * 0.45))
m = np.zeros((H, W)); m[:len(fade)] = fade[:, None]
ra = Image.fromarray((np.asarray(ra).astype(float) * m).astype('uint8'))
bg.paste(ref, (PAD, PAD + H + 4), ra)
bg.paste(ui, (PAD, PAD), alpha)
bg.save(A + 'cover.png')
open(A + 'cover_geom.txt', 'w').write('%d %d %d\n' % (PAD, W, H))
