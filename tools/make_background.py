# Fond d'Impulse (assets/background.jpg) tire de la maquette de l'utilisateur (design/maquette_impulse.png) :
# on garde la plaque brossee, les cadres, les etiquettes, les graduations et le logo IMPULSE ; on efface
# ce que le plugin redessine (pied de page NINE HANDS avec ses traits, icone power, colonnes des faders).
# Les zones effacees sont recouvertes par du metal brosse pris sur les memes lignes (stries horizontales).
import numpy as np
from PIL import Image
src = Image.open('design/maquette_impulse.png').convert('RGBA')
a = np.asarray(src)
ys, xs = np.where(a[..., 3] > 128)
plug = src.crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))
assert plug.size == (1876, 737), plug.size
bg = Image.new('RGBA', plug.size, (13, 13, 13, 255))
bg.alpha_composite(plug)
p = np.asarray(bg.convert('RGB')).copy()

def copy_cols(dst_x0, dst_x1, src_x0, src_x1, y0, y1, nb=4, feather=6):
    """remplit les colonnes dst avec la texture des colonnes src (memes lignes), puis recale la luminosite de
    chaque ligne sur celle des pixels voisins a gauche et a droite (interpolee), avec un fondu sur les bords"""
    w = src_x1 - src_x0
    orig = p.astype(np.float32)
    patch = np.zeros((y1 - y0, dst_x1 - dst_x0, 3), np.float32)
    tile = orig[y0:y1, src_x0:src_x1].copy()
    # retire la variation de luminosite d'une colonne a l'autre du motif (sinon bandes verticales en le repetant)
    colm = tile.mean(axis=0, keepdims=True)
    tile = tile - colm + colm.mean(axis=1, keepdims=True)
    for x in range(dst_x0, dst_x1):
        k = (x - dst_x0) % (2 * w)
        patch[:, x - dst_x0] = tile[:, k if k < w else 2 * w - 1 - k]      # repetition en miroir : pas de couture
    # basse frequence horizontale du motif (moyenne glissante) : on ne garde que les stries
    kw = 41
    c = np.cumsum(np.pad(patch, ((0, 0), (kw, kw), (0, 0)), mode='reflect'), axis=1)
    pm = (c[:, 2 * kw:] - c[:, :-2 * kw])[:, :patch.shape[1]] / (2 * kw)
    L = np.median(orig[y0:y1, dst_x0 - nb:dst_x0], axis=1)          # voisins gauche / droite
    R = np.median(orig[y0:y1, dst_x1:dst_x1 + nb], axis=1)
    # lissage vertical (evite de reproduire un trait de graduation voisin)
    k = 9
    def smooth(v):
        out = v.copy()
        for i in range(len(v)):
            out[i] = np.median(v[max(0, i - k):i + k + 1], axis=0)
        return out
    L, R = smooth(L), smooth(R)
    t = np.linspace(0, 1, dst_x1 - dst_x0)[None, :, None]
    target = L[:, None, :] * (1 - t) + R[:, None, :] * t
    patch = patch - pm + target
    # fondu sur les bords
    H, W = patch.shape[:2]
    fx = np.minimum(np.arange(W) + 1, np.arange(W)[::-1] + 1) / float(feather)
    fy = np.minimum(np.arange(H) + 1, np.arange(H)[::-1] + 1) / float(feather)
    m = np.clip(np.minimum(fy[:, None], fx[None, :]), 0, 1)[..., None]
    region = orig[y0:y1, dst_x0:dst_x1]
    p[y0:y1, dst_x0:dst_x1] = np.clip(region * (1 - m) + patch * m, 0, 255).astype(np.uint8)

# pied de page « —— NINE HANDS | DSP —— » -> metal brosse pris sous la carte A (memes lignes)
copy_cols(712, 1222, 60, 570, 661, 727)
# icone power (redessinee par le plugin, couleur d'accent / etat)
copy_cols(36, 98, 340, 402, 22, 84)
# colonnes des faders A et B (fente, remplissage, bouton) -> marge propre de la tranche
copy_cols(682, 717, 612, 632, 318, 640)
copy_cols(1162, 1197, 1092, 1112, 318, 640)
# boutons des faders de la maquette (plus larges que la fente) : A a 0 dB (y 400), B vers 6 dB (y 428)
copy_cols(675, 727, 612, 632, 376, 426)
copy_cols(1155, 1207, 1092, 1112, 404, 454)
# puis on remet les graduations effacees en recopiant une graduation intacte (lignes et tirets)
def copy_rows(dst_y, src_y, x0, x1, h=3):
    p[dst_y - h:dst_y + h + 1, x0:x1] = p[src_y - h:src_y + h + 1, x0:x1]
copy_rows(400, 371, 675, 727)          # graduation 0 de A (son prolongement jusqu'a 690 est dessine par le plugin)
copy_rows(431, 461, 1155, 1207)        # graduation 6 de B
# colonne centrale (potards, etiquettes) : redessinee en vectoriel par le plugin, nette a toutes les tailles
copy_cols(806, 1070, 1003, 1063, 96, 661, nb=4, feather=4)
# jonction colonne centrale / pied de page : ramene la luminosite de chaque ligne au niveau du pied de page
def level_rows(x0, x1, y0, y1, ref_y0, ref_y1, feather=6):
    f = p.astype(np.float32)
    target = np.median(f[ref_y0:ref_y1, x0:x1].mean(axis=1), axis=0)
    for y in range(y0, y1):
        row = f[y, x0:x1]
        new = row - row.mean(axis=0) + target
        w = np.ones(x1 - x0, np.float32)
        ramp = np.linspace(0, 1, feather + 2)[1:-1]
        w[:feather] = ramp; w[-feather:] = ramp[::-1]
        p[y, x0:x1] = np.clip(row * (1 - w[:, None]) + new * w[:, None], 0, 255).astype(np.uint8)
level_rows(806, 1070, 640, 668, 670, 700)

# details fins redessines en vectoriel par le plugin (nets a toutes les tailles) :
copy_cols(128, 352, 420, 644, 17, 90, nb=4, feather=10)                      # logo IMPULSE
copy_cols(612, 784, 612, 632, 318, 642)                    # graduations et chiffres du fader A
copy_cols(1092, 1264, 1092, 1112, 318, 642)                # graduations et chiffres du fader B
# disquette et roue crantee : le plugin dessine le bouton complet

# homothetie : 3500 px de large, hauteur proportionnelle (reechantillonnage Lanczos + accentuation legere)
from PIL import ImageFilter
W2 = 3500
H2 = round(W2 * p.shape[0] / p.shape[1])
big = Image.fromarray(p).resize((W2, H2), Image.LANCZOS).filter(ImageFilter.UnsharpMask(radius=1.6, percent=60, threshold=2))
big.save('assets/background.jpg', quality=92)
print('fond', big.size)

print('ok', p.shape)
