# Alu brosse d'Impulse = celui d'Ampire (reference) : meme texture, meme cadrage, meme eclairage d'ensemble.
# Rapport pixel a pixel (plaque Ampire / plaque Impulse) applique sur l'alu seulement : ombres et reliefs d'Impulse conserves.
# Bandes des faders : la bande B (teinte de reference) est reprise telle quelle sur la bande A.
import numpy as np, sys
from PIL import Image
from scipy.ndimage import gaussian_filter, distance_transform_edt
W, H = 3600, 1440
Y, X = np.mgrid[0:H, 0:W].astype(np.float32) + 0.5
def sd_rrect(x0, y0, x1, y1, r):
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2; hx, hy = (x1 - x0) / 2 - r, (y1 - y0) / 2 - r
    qx, qy = np.abs(X - cx) - hx, np.abs(Y - cy) - hy
    return np.hypot(np.maximum(qx, 0), np.maximum(qy, 0)) + np.minimum(np.maximum(qx, qy), 0) - r

imp = np.asarray(Image.open(sys.argv[1]).convert('RGBA')).astype(np.float32)
ampbg = np.asarray(Image.open('/home/claude/ampire/assets/bg.png').convert('RGBA').resize((W, H), Image.LANCZOS)).astype(np.float32)
TEX = np.asarray(Image.open('/home/claude/imp5/src/texture_alu_brosse_noir_satine_4000x4000.png').convert('RGB')).astype(np.float32)

# plaque « nue » de chaque plugin
imp_plate = TEX[1280:1280 + H, 200:200 + W]                       # Impulse : cadrage 1:1 (x 1.15 d'eclairage, commun)
lightgrad = 1.10 - 0.16 * (X / W) * 0.6 - 0.16 * (Y / H) * 0.4     # Ampire : eclairage d'ensemble haut-gauche
amp_tex = TEX[1000:1000 + H, 200:200 + W] * lightgrad[..., None]
FLIP = len(sys.argv) > 3 and sys.argv[3] == 'flip'    # miroir haut / bas de la texture globale (partie sombre en haut)
# gain d'Ampire (ombrage du plat) mesure sur des zones de plaque nue d'Ampire, rapporte a celui d'Impulse
def med_ratio(img, base, boxes):
    r = []
    for x0, y0, x1, y1 in boxes:
        r.append(np.median(img[y0:y1, x0:x1, :3].mean(2) / np.maximum(base[y0:y1, x0:x1].mean(2), 1)))
    return float(np.median(r))
g_amp = med_ratio(ampbg, amp_tex, [(300, 1300, 900, 1400), (1300, 1290, 2000, 1400), (2400, 1300, 3000, 1400), (1000, 60, 1400, 120)])
g_imp = med_ratio(imp, imp_plate, [(800, 200, 1000, 280), (1300, 1370, 1550, 1420), (2050, 1370, 2300, 1420), (1650, 200, 1700, 330)])
print('gain plat Ampire', g_amp, 'Impulse', g_imp)
tgt = amp_tex[::-1] if FLIP else amp_tex
ratio = (tgt * g_amp) / np.maximum(imp_plate * g_imp, 2.0)
ratio = np.clip(ratio, 0.25, 4.0)

# masque de l'alu d'Impulse (hors bevel, ecrans, boutons, potards, glissieres, logos)
din = distance_transform_edt(np.pad(imp[..., 3] > 127, 1))[1:-1, 1:-1]
m = (din > 34).astype(np.float32)
ex = []
CARDS = [(58, 537, 1506, 1348), (2094, 537, 3542, 1348)]
for c in CARDS: ex.append(sd_rrect(*c, 22) < 2)
for b in [(58, 312.5, 1044, 432.5), (2094, 312.5, 3080, 432.5), (2271, 58, 3234, 178)]: ex.append(sd_rrect(*b, 13) < 2)
for cx, cy in [(1078, 312.5), (1232, 312.5), (1386, 312.5), (3114, 312.5), (3268, 312.5), (3422, 312.5), (3268, 58), (3422, 58)]:
    ex.append(sd_rrect(cx, cy, cx + 120, cy + 120, 14) < 2)
for cy in (466.5, 810, 1153.5): ex.append(np.hypot(X - 1800, Y - cy) < 112)
for e in ex: m[e] = 0
L = imp[..., :3].mean(2)
protect = np.clip((130 - L) / 30, 0, 1)                          # textes et logos blancs intacts
w = m * protect
out = imp.copy()
out[..., :3] = imp[..., :3] * (1 - w[..., None]) + imp[..., :3] * ratio * w[..., None]

# bandes des faders : celle de B (reference, un peu plus sombre) reprise sur A (meme dessin, decalee de 942 px)
SA, SB = (1152, 537, 1506, 1348), (2094, 537, 2448, 1348)
dx = SB[0] - SA[0]
inner = sd_rrect(*CARDS[0], 22) < -14                               # sans les bords arrondis de la carte A
band = np.zeros((H, W), bool); band[SA[1]:SA[3], SA[0] + 34:SA[2]] = True      # sans la paroi gauche de la carte B (ombre du renfoncement)
band &= inner
src = np.roll(out, -dx, axis=1)                                    # pixel B a la meme place relative
kA = gaussian_filter(band.astype(np.float32), 2.0)
out[..., :3] = out[..., :3] * (1 - kA[..., None]) + src[..., :3] * kA[..., None]
Image.fromarray(np.clip(out, 0, 255).astype(np.uint8), 'RGBA').save(sys.argv[2])
print('ok')
