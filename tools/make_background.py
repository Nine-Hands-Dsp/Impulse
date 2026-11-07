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


def octopus_frame(out, T):
    """Contour du boitier repris d'Octopus : jante arrondie (profils mesures sur le fond d'Octopus,
    design/octopus_bevel.json), coins arrondis, meme taille a l'ecran a la taille par defaut."""
    import json
    prof = json.load(open('design/octopus_bevel.json'))
    H, W = out.shape[:2]
    s = (W / 887.0) / (2498.0 / 993.0)        # px du fond Impulse par px du fond Octopus (meme taille a l'ecran)
    SIZE, EXPO = 0.70, 0.70                    # contour 30 % plus fin et 30 % moins expose qu'Octopus
    s = s * SIZE
    R = 50.0 * s                               # rayon des coins
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    px, py = xx + 0.5, yy + 0.5
    cx = np.clip(px, R, W - R); cy = np.clip(py, R, H - R)
    vx, vy = px - cx, py - cy
    ln = np.sqrt(vx * vx + vy * vy)
    inner = np.minimum(np.minimum(px, W - px), np.minimum(py, H - py))
    d = np.where(ln > 0, R - ln, inner)        # distance au bord, vers l'interieur
    nx = np.where(ln > 0, vx / np.maximum(ln, 1e-6), 0.0); ny = np.where(ln > 0, vy / np.maximum(ln, 1e-6), 0.0)
    edge = ln <= 0
    m = np.minimum(np.minimum(px, W - px), np.minimum(py, H - py))
    nx = np.where(edge & (m == px), -1.0, nx); nx = np.where(edge & (m == W - px), 1.0, nx)
    ny = np.where(edge & (m == py), -1.0, ny); ny = np.where(edge & (m == H - py), 1.0, ny)
    idx = np.clip(d / s, 0, None)
    def delta(name):
        v = np.array(prof[name], np.float32)
        surf = v[30:].mean()
        dv = v - surf
        dv[28:] = dv[28:] * np.linspace(1, 0, len(dv) - 28)   # raccord en douceur avec la surface
        return np.interp(idx, np.arange(len(dv)), dv, right=0.0)
    wl, wr = np.maximum(-nx, 0) ** 2, np.maximum(nx, 0) ** 2
    wt, wb = np.maximum(-ny, 0) ** 2, np.maximum(ny, 0) ** 2
    ws = np.maximum(wl + wr + wt + wb, 1e-6)
    D = EXPO * (wl * delta('left') + wr * delta('right') + wt * delta('top') + wb * delta('bottom')) / ws
    # sous la jante : surface de la texture (l'ancien filet et la marge noire disparaissent)
    b = np.clip((62.0 * SIZE - d) / (14.0 * SIZE), 0, 1)[..., None]
    base = out * (1 - b) + T * b
    res = base + D[..., None]
    a = np.clip(d, 0, 1)[..., None]            # coins : noir hors de la forme, bord lisse
    res = res * a
    return np.clip(res, 0, 255)

def apply_texture(img):
    """Texture metal brosse de l'utilisateur (design/texture_boitier.png) appliquee au boitier
    (tout sauf les zones creuses des formes d'onde) en gardant biseaux, rainures et ombres d'origine."""
    from scipy import ndimage as nd
    O = np.asarray(img).astype(np.float32)
    H, W = O.shape[:2]
    k = W / 1876.0
    # texture : agrandie a 3900 px, faisceau lumineux recentre sur la colonne centrale
    tex = Image.open('design/texture_boitier.png').convert('RGB').resize((3900, 3900), Image.LANCZOS)
    x0 = int(round(0.5435 * 3900 - W / 2)); y0 = 195
    T = np.asarray(tex.crop((x0, y0, x0 + W, y0 + H))).astype(np.float32)
    # faisceau central attenue : on retire 70 % de la surbrillance au-dessus du niveau des cotes
    Tg = nd.gaussian_filter(T.mean(axis=2), 40 * k)
    T = T - (0.70 * np.maximum(Tg - np.percentile(Tg, 5), 0.0))[..., None]
    # masque du boitier : tout sauf les panneaux A et B
    # seules les zones creuses sombres (forme d'onde + fader) gardent leur fond ; bord clair inclus, coins arrondis
    from PIL import ImageDraw
    mk = Image.new('L', (W, H), 255); dr = ImageDraw.Draw(mk)
    sx = W / 3500.0
    for (a0, b0, a1, b1) in ((64, 446, 1471, 1229), (2027, 446, 3436, 1229)):
        dr.rounded_rectangle((a0 * sx, b0 * sx, a1 * sx, b1 * sx), radius=18 * sx, fill=0)
    M = np.asarray(mk).astype(np.float32) / 255.0
    # calculs de lumiere en luminance (pas de derive de couleur) ; pixels colores (icones de la maquette) ignores
    lum = O.mean(axis=2)
    sat = (O.max(axis=2) - O.min(axis=2))
    Wt = M * np.clip(1.0 - (sat - 12.0) / 20.0, 0.0, 1.0)
    def mblur(a, sig):
        num = nd.gaussian_filter(a * Wt, sig)
        den = nd.gaussian_filter(Wt, sig)
        return num / np.maximum(den, 1e-4)
    Tl = T.mean(axis=2)
    sL, sS = 60 * k, 4 * k
    OL, TL, OS = mblur(lum, sL), mblur(Tl, sL), mblur(lum, sS)
    # zones de detail (biseaux, aretes, filets) : gardees telles quelles, ramenees a la lumiere de la texture
    E = nd.gaussian_filter(np.abs(lum - nd.gaussian_filter(lum, 15)), 4)
    K = np.clip((E - 6.0) / 6.0, 0, 1)
    # filets fins (1 a 3 px) : contraste local fort sur petite echelle
    F = np.abs(lum - nd.gaussian_filter(lum, 3 * k))
    K = np.maximum(K, np.clip((F - 10.0) / 8.0, 0, 1))
    # reflets lumineux de l'ancienne texture (bandeau du haut et bas de facade) : supprimes, pas des biseaux
    for (a0, b0, a1, b1) in ((124, 9, 1168, 86), (12, 664, 1864, 721)):
        K[int(b0 * k):int(b1 * k), int(a0 * k):int(a1 * k)] = 0.0
    K = nd.gaussian_filter(nd.maximum_filter(K, size=int(3 * k)), 1.5 * k)[..., None]
    detail = O + (TL - OL)[..., None]                 # biseau d'origine sur la lumiere de la nouvelle texture
    surface = T                                       # nouvelle texture seule : les sources lumineuses de l'ancienne sont supprimees
    new = K * detail + (1 - K) * surface
    m3 = M[..., None]
    out = m3 * new + (1 - m3) * O
    out = octopus_frame(out, T)
    return Image.fromarray(np.clip(out, 0, 255).astype(np.uint8))

# homothetie : 3500 px de large, hauteur proportionnelle (reechantillonnage Lanczos + accentuation legere)
from PIL import ImageFilter
W2 = 3500
H2 = round(W2 * p.shape[0] / p.shape[1])
big = Image.fromarray(p).resize((W2, H2), Image.LANCZOS).filter(ImageFilter.UnsharpMask(radius=1.6, percent=60, threshold=2))
big = apply_texture(big)
big.save('assets/background.jpg', quality=92)
print('fond', big.size)

print('ok', p.shape)
