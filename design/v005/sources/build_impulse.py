# Impulse v005 : rendu réaliste 3600 x 1440 (fond transparent) d'après Impulse_Design_Mockup_v005.svg
#   lumière en haut à gauche (45°), ombres portées, occlusion ambiante, alu brossé noir satiné.
# Sorties dans out/ : rendu complet (aperçu avec contenus), fond statique, éléments séparés, SVG.
import os, io, math, subprocess, wave
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
from scipy import ndimage as nd
import svgkit as K

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, 'src'); OUT = os.path.join(HERE, 'out')
for d in ('', 'elements', 'knob', 'svg', 'svg/icons', 'svg/texts', 'svg/logos'):
    os.makedirs(os.path.join(OUT, d), exist_ok=True)

W, H = 3600, 1440
ACC = np.array([79, 195, 247], np.float32)          # accent (bleu Impulse)
L = np.array([-1.0, -1.0, 1.25]); L = L / np.linalg.norm(L)   # lumière : haut gauche, 45°
FLAT = L[2]

# ---------------------------------------------------------------- outils
def sd_rrect(X, Y, x0, y0, x1, y1, r):
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    hx, hy = (x1 - x0) / 2 - r, (y1 - y0) / 2 - r
    qx, qy = np.abs(X - cx) - hx, np.abs(Y - cy) - hy
    return np.hypot(np.maximum(qx, 0), np.maximum(qy, 0)) + np.minimum(np.maximum(qx, qy), 0) - r

def sd_circle(X, Y, cx, cy, r):
    return np.hypot(X - cx, Y - cy) - r

def cov(sd):
    return np.clip(0.5 - sd, 0, 1).astype(np.float32)

def grads(sd):
    gy, gx = np.gradient(sd)
    n = np.hypot(gx, gy) + 1e-6
    return gx / n, gy / n

Y, X = np.mgrid[0:H, 0:W].astype(np.float32) + 0.5

def shade_from_normal(nx, ny, nz):
    n = np.sqrt(nx * nx + ny * ny + nz * nz) + 1e-6
    return (nx * L[0] + ny * L[1] + nz * L[2]) / n

def blur(a, s):
    return nd.gaussian_filter(a, s)

def shift(a, dx, dy):
    return nd.shift(a, (dy, dx), order=1, mode='constant')

def over(img, alpha, col):
    """img (H,W,3) <- col (H,W,3 ou 3) par-dessus avec alpha (H,W)."""
    a = alpha[..., None]
    img *= (1 - a); img += a * col

# ---------------------------------------------------------------- texture alu brossé
TEX = np.asarray(Image.open(os.path.join(SRC, 'texture_alu_brosse_noir_satine_4000x4000.png')).convert('RGB'), np.float32)
TX0, TY0 = 200, 1280                                   # cadrage de la plaque dans la texture (même échelle 1:1)
PLATE_T = TEX[TY0:TY0 + H, TX0:TX0 + W].copy()

def tex_center_patch(w, h):
    """morceau du centre de la texture, à la même échelle (zones 0A2100)"""
    cx, cy = TEX.shape[1] // 2, TEX.shape[0] // 2
    return TEX[cy - h // 2: cy - h // 2 + h, cx - w // 2: cx - w // 2 + w].copy()

# ---------------------------------------------------------------- géométrie de la maquette (px du rendu)
OUTER = (0, 0, 3600, 1440, 43.54); BEVEL = 24.0
CARDS = [(58, 537, 1506, 1348), (2094, 537, 3542, 1348)]; CARD_R = 22.01
STRIPS = [(1152, 537, 1506, 1348, 'right'), (2094, 537, 2448, 1348, 'left')]
NAMES = [(58, 312.5, 1044, 432.5), (2094, 312.5, 3080, 432.5)]
PRESET = (2271, 58, 3234, 178)
SCREEN_R = 13.05
BTN120 = [(1078, 312.5), (1232, 312.5), (1386, 312.5), (3114, 312.5), (3268, 312.5), (3422, 312.5), (3268, 58), (3422, 58)]
BTN100 = [(1203, 571), (1355, 571), (2145, 571), (2297, 571)]
KNOBS = [(1800, 466.5), (1800, 810), (1800, 1153.5)]; KNOB_R = 85.0; GAUGE_R0, GAUGE_R1 = 95.5, 107.5
SLOTS = [(1314, 704, 1344, 1280), (2256, 704, 2286, 1280)]; SLOT_STROKE = 8
THUMBS = [(1329, 852.32), (2271, 852.32)]; THUMB_R = 25.03; THUMB_RING = 8.0
POWER = (118, 121.76)
LOGO_IMP = (273, 56, 698, 179); LOGO_NH = (1600, 1362, 1999, 1396)
FADER_TOP, FADER_BOT = 740.5, 1243.5      # graduations : 12 6 0 6 12 18 36 45 60 inf (0 dB sous le bouton de la maquette)

# ================================================================ 1. plaque + bevel rond 24 px (FF0049) + fond noir texturé
sd_o = sd_rrect(X, Y, *OUTER)
alpha = cov(sd_o)
d_in = -sd_o
gx, gy = grads(sd_o)
t = np.clip(d_in / BEVEL, 0, 1)
u = 1 - t
slope = np.where(d_in < BEVEL, u / np.sqrt(np.maximum(1 - u * u, 0.02)), 0.0)   # quart de rond
slope = np.minimum(slope, 6.0)
sh = shade_from_normal(slope * gx, slope * gy, np.ones_like(slope))
k = 0.35 + 0.80 * np.clip(sh, 0, None) / FLAT
zoneB = (d_in < BEVEL).astype(np.float32)
Hh = L + np.array([0, 0, 1.0]); Hh = Hh / np.linalg.norm(Hh)
nrm = np.sqrt((slope * gx) ** 2 + (slope * gy) ** 2 + 1)
ndh = ((slope * gx) * Hh[0] + (slope * gy) * Hh[1] + Hh[2]) / nrm
spec = (np.clip(ndh, 0, 1) ** 30 * 95 + np.clip(sh - FLAT, 0, None) * 110) * zoneB      # reflet du bevel (haut / gauche)
img = PLATE_T * k[..., None] + spec[..., None]
# arête intérieure du bevel : fin liseré sombre (jonction avec la plaque)
edge = np.exp(-((d_in - BEVEL) / 1.2) ** 2) * 0.35
img *= (1 - edge)[..., None]

# ================================================================ 2. écrans encastrés (A5A7AA) + zones 0A2100 + bevel plat 5 px (003177)
def plastic(h, w, top=(22, 23, 25), bot=(14, 15, 16), seed=1):
    g = np.linspace(0, 1, h)[:, None, None]
    col = np.array(top, np.float32) * (1 - g) + np.array(bot, np.float32) * g
    rng = np.random.default_rng(seed)
    grain = blur(rng.normal(0, 1, (h, w)).astype(np.float32), 0.6) * 1.6
    out = np.repeat(col, w, axis=1) + grain[..., None]
    # léger reflet diagonal (lumière en haut à gauche)
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    band = np.exp(-(((xx / w) + (yy / h) * 0.6 - 0.35) / 0.18) ** 2) * 6.0
    return out + band[..., None]

def recess(img, mask, depth=1.0, s=9, off=7, lip=True):
    """ombre intérieure (haut / gauche) + lèvre claire (bas / droite) d'une zone encastrée"""
    outside = 1 - mask
    sh_in = blur(shift(outside, off, off), s) * mask * 0.85 * depth
    img *= (1 - np.clip(sh_in, 0, 0.9))[..., None]
    occ = blur(outside, 2.2) * mask * 0.45 * depth                 # occlusion au fond des bords
    img *= (1 - occ)[..., None]
    if lip:
        li = blur(shift(outside, -3, -3), 2.0) * mask * 0.10 * depth
        img += (li * 255)[..., None] * 0.35

def flat_bevel_inside(img, sd, width, base, strength=1.0):
    """bevel plat (chanfrein à 45°) dans la bande 0..width à l'intérieur d'une forme encastrée"""
    d = -sd
    zone = ((d >= 0) & (d < width)).astype(np.float32) * np.clip(d + 0.5, 0, 1)
    gx, gy = grads(sd)
    shv = shade_from_normal(-gx, -gy, np.ones_like(gx))      # paroi qui descend vers l'intérieur
    kk = 0.25 + 1.0 * np.clip(shv, 0, None) / FLAT
    col = base * kk[..., None]
    over(img, zone * strength, col)

for (x0, y0, x1, y1), strip in zip(CARDS, STRIPS):
    sd = sd_rrect(X, Y, x0, y0, x1, y1, CARD_R)
    m = cov(sd)
    # écran plastique
    pl = np.zeros_like(img)
    pl[int(y0):int(y1), int(x0):int(x1)] = plastic(int(y1) - int(y0), int(x1) - int(x0), seed=int(x0))
    over(img, m, pl)
    # bande de la glissière : alu brossé, centre de la texture, même échelle
    sx0, sy0, sx1, sy1, side = strip
    sm = m.copy()
    sm[:, :int(sx0)] = 0; sm[:, int(sx1):] = 0
    patch = tex_center_patch(int(sx1 - sx0), int(sy1 - sy0)) * 0.78
    tp = np.zeros_like(img); tp[int(sy0):int(sy1), int(sx0):int(sx1)] = patch
    over(img, sm, tp)
    # séparation écran / bande (petit filet sombre + reflet)
    xs = int(sx0) if side == 'right' else int(sx1)
    sep = np.exp(-((X - xs) / 1.6) ** 2) * m
    img *= (1 - 0.55 * sep)[..., None]
    recess(img, m, 1.0, s=10, off=8)
    flat_bevel_inside(img, sd, 5.0, PLATE_T)

def screen_box(img, box, r=SCREEN_R, depth=0.9, seed=3):
    x0, y0, x1, y1 = box
    sd = sd_rrect(X, Y, x0, y0, x1, y1, r)
    m = cov(sd)
    pl = np.zeros_like(img)
    ys, ye, xs_, xe = int(math.floor(y0)), int(math.ceil(y1)), int(math.floor(x0)), int(math.ceil(x1))
    pl[ys:ye, xs_:xe] = plastic(ye - ys, xe - xs_, seed=seed)
    over(img, m, pl)
    recess(img, m, depth, s=7, off=5)
    flat_bevel_inside(img, sd, 3.0, PLATE_T, 0.9)
    return m

for i, b in enumerate(NAMES): screen_box(img, b, seed=10 + i)
screen_box(img, PRESET, seed=20)

# ================================================================ 3. écrans des jauges des potards (000DFF) : anneaux encastrés
for (cx, cy) in KNOBS:
    sd = np.maximum(sd_circle(X, Y, cx, cy, GAUGE_R1), -sd_circle(X, Y, cx, cy, GAUGE_R0))
    m = cov(sd)
    pl = np.zeros_like(img); pl[:] = np.array([12, 13, 14], np.float32)
    over(img, m, pl)
    recess(img, m, 1.0, s=4, off=3)

# ================================================================ 4. glissières (008CFF) : rainure encastrée, bord 8 px (0A2100)
for (x0, y0, x1, y1) in SLOTS:
    sdb = sd_rrect(X, Y, x0 - SLOT_STROKE / 2, y0 - SLOT_STROKE / 2, x1 + SLOT_STROKE / 2, y1 + SLOT_STROKE / 2, 15 + SLOT_STROKE / 2)
    mb = cov(sdb)
    img *= (1 - 0.35 * mb)[..., None]                       # cuvette autour de la fente
    recess(img, mb, 0.6, s=3, off=2, lip=True)
    sd = sd_rrect(X, Y, x0, y0, x1, y1, 15)
    m = cov(sd)
    pl = np.zeros_like(img); pl[:] = np.array([5, 5, 6], np.float32)
    over(img, m, pl)
    recess(img, m, 1.0, s=5, off=4)

# ================================================================ 5. graduations des faders (imprimées sur la bande)
TICK_LABELS = ['12', '6', '0', '6', '12', '18', '36', '45', '60', '']
def tick_y(i): return FADER_TOP + (FADER_BOT - FADER_TOP) * i / 9.0

# ================================================================ 6. boutons poussoirs plastique (45FF00) : relief, ombre portée, occlusion
def raised_shadow(img, m, off=9, s=12, k1=0.75, ao_s=3.0, ao_k=0.55):
    sdw = blur(shift(m, off, off), s) * k1
    ao = blur(m, ao_s) * ao_k
    a = np.clip(np.maximum(sdw, ao) * (1 - m), 0, 0.92)
    img *= (1 - a)[..., None]

def button_body(sd, pressed=False):
    """couleur d'un bouton plastique (réf. fm1 : gris anthracite, contour noir, bord arrondi)"""
    d = -sd
    m = cov(sd)
    gx, gy = grads(sd)
    rim = 4.0; bev = 9.0
    # contour noir (jupe), puis bord arrondi de la touche, puis face
    t = np.clip((d - rim) / bev, 0, 1); u = 1 - t
    slope = np.where((d >= rim) & (d < rim + bev), u / np.sqrt(np.maximum(1 - u * u, 0.03)), 0.0)
    slope = np.minimum(slope, 4.0) * 0.55
    shv = shade_from_normal(slope * gx, slope * gy, np.ones_like(slope))
    base = 44.0 if not pressed else 36.0
    # face : léger dégradé vertical (plus claire en haut, comme fm1)
    yy = Y
    col = np.full(sd.shape + (3,), base, np.float32)
    col *= (0.55 + 0.6 * np.clip(shv, 0, None) / FLAT)[..., None]
    col = np.where((d < rim)[..., None], np.array([10, 10, 11], np.float32), col)
    return m, col

def make_button(img, x, y, size, r=13.05, pressed=False, shadow=True):
    sd = sd_rrect(X, Y, x, y, x + size, y + size, r)
    m, col = button_body(sd, pressed)
    if shadow: raised_shadow(img, m, off=6 if not pressed else 3, s=9 if not pressed else 5)
    # dégradé vertical de la face
    g = np.clip((Y - y) / size, 0, 1)
    col = col * (1.08 - 0.20 * g)[..., None]
    # reflet fin du haut de la face
    d = -sd
    hl = np.exp(-((Y - (y + 13.5)) / 1.4) ** 2) * (d > 13) * np.clip((size / 2 - np.abs(X - (x + size / 2)) - 14) / 10, 0, 1) * (0.14 if not pressed else 0.05)
    col = col + (hl * 255)[..., None]
    over(img, m, col)
    return m

for (x, y) in BTN120: make_button(img, x, y, 120)
for (x, y) in BTN100: make_button(img, x, y, 100)

# ================================================================ 7. potards (FF00FF) : centre fixe + couronne mobile + reflet statique
KS = 2 * KNOB_R / 800.0
def load_layer(name, size):
    im = Image.open(os.path.join(SRC, name)).convert('RGBA')
    return im.resize((size, size), Image.LANCZOS)
KPX = int(round(2 * KNOB_R))
k_center = load_layer('knob_01_black_fine_serrated_center_fixed.png', KPX)
k_crown = load_layer('knob_01_black_fine_serrated_crown_rotating.png', KPX)
k_refl = load_layer('knob_01_black_fine_serrated_reflect_static.png', KPX)
for nme, im in (('knob_center_fixed', k_center), ('knob_crown_rotating', k_crown), ('knob_reflect_static', k_refl)):
    im.save(os.path.join(OUT, 'knob', f'{nme}_{KPX}px.png'))

def paste_rgba(img, layer, x0, y0, extra_alpha=1.0):
    a = np.asarray(layer, np.float32) / 255.0
    h, w = a.shape[:2]
    reg = img[y0:y0 + h, x0:x0 + w]
    al = a[..., 3:4] * extra_alpha
    reg *= (1 - al); reg += al * a[..., :3] * 255

def knob_shadow(img, cx, cy):
    m = cov(sd_circle(X, Y, cx, cy, KNOB_R))
    raised_shadow(img, m, off=10, s=11, k1=0.85, ao_s=3.5, ao_k=0.6)

def knob(img, cx, cy, crown_angle=0.0):
    x0, y0 = int(round(cx - KNOB_R)), int(round(cy - KNOB_R))
    paste_rgba(img, k_center, x0, y0)
    paste_rgba(img, k_crown.rotate(-crown_angle, Image.BICUBIC), x0, y0)
    paste_rgba(img, k_refl, x0, y0)

# ================================================================ 8. bouton de fader : disque blanc + bevel plat 8 px (48D39D)
def thumb_rgba():
    s = 2 * int(THUMB_R + THUMB_RING + 30)
    yy, xx = np.mgrid[0:s, 0:s].astype(np.float32) + 0.5
    c = s / 2
    sd = sd_circle(xx, yy, c, c, THUMB_R + THUMB_RING)
    m = cov(sd)
    gx, gy = grads(sd)
    d = -sd
    ring = (d < THUMB_RING).astype(np.float32)
    shv = shade_from_normal(gx * 1.0, gy * 1.0, np.ones_like(gx))          # bevel plat, montant vers le centre
    kring = 0.35 + 0.95 * np.clip(shv, 0, None) / FLAT
    # face : métal clair (blanc satiné), dégradé radial décalé vers la lumière
    rr = np.hypot(xx - (c - 8), yy - (c - 9)) / (THUMB_R * 1.6)
    face = (238 - 95 * np.clip(rr, 0, 1))[..., None] * np.ones(3, np.float32)
    col = np.where(ring[..., None] > 0, (np.array([150, 150, 152], np.float32) * kring[..., None]), face)
    hl = np.exp(-(((xx - (c - 9)) ** 2 + (yy - (c - 11)) ** 2) / (2 * 6.0 ** 2))) * 40 * (d >= THUMB_RING)
    col = np.clip(col + hl[..., None], 0, 255)
    edge = np.exp(-(d / 1.0) ** 2) * 0.6
    col *= (1 - edge)[..., None]
    sh = np.clip(blur(shift(m, 7, 7), 7) * 0.8 * (1 - m) + blur(m, 2.5) * 0.5 * (1 - m), 0, 0.9)
    rgba = np.zeros((s, s, 4), np.float32)
    rgba[..., :3] = col * m[..., None]
    rgba[..., 3] = np.maximum(m, sh)
    rgba[..., :3] = np.where(m[..., None] > 0, rgba[..., :3] / np.maximum(rgba[..., 3:4], 1e-6), 0)
    return Image.fromarray(np.clip(np.dstack([rgba[..., :3], rgba[..., 3:] * 255]), 0, 255).astype(np.uint8), 'RGBA')
THUMB = thumb_rgba()
THUMB.save(os.path.join(OUT, 'elements', 'fader_thumb.png'))

# ================================================================ 9. textes et icônes (SVG -> calque)
def svg_layer(body):
    doc = f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">{body}</svg>'
    png = subprocess.run(['rsvg-convert', '-w', str(W), '-h', str(H)], input=doc.encode(), capture_output=True, check=True).stdout
    return np.asarray(Image.open(io.BytesIO(png)).convert('RGBA'), np.float32) / 255.0

def put_layer(img, lay, emboss=True):
    a = lay[..., 3]
    if emboss:   # impression : petite ombre portée (lumière haut gauche)
        sh = blur(shift(a, 2, 2), 1.4) * 0.55
        img *= (1 - sh)[..., None]
    over(img, a, lay[..., :3] * 255)

def G(x, y, inner, color, cls_color=None):
    inner = inner.replace('class="s"', 'fill="none" stroke="%s" stroke-linecap="round" stroke-linejoin="round"' % color)
    inner = inner.replace('class="c"', 'fill="%s"' % color)
    return '<g transform="translate(%.2f,%.2f)">%s</g>' % (x, y, inner)

WHITE = '#E8E8E8'; LABEL = '#E6E6E6'; DIM = '#CFCFCF'; ACCH = '#4FC3F7'

def static_overlay():
    parts = []
    # power (accent)
    parts.append(G(POWER[0], POWER[1], K.icon_power(33, 10), ACCH))
    # icônes des boutons
    for i, (x, y) in enumerate(BTN120[:6]):
        cx, cy = x + 60, y + 60
        ic = [K.icon_list_search(1.25, 5.2), K.icon_tri(-1, 1.15, 5.2), K.icon_tri(1, 1.15, 5.2)][i % 3]
        parts.append(G(cx, cy, ic, WHITE))
    parts.append(G(3268 + 60, 58 + 60, K.icon_save(1.2, 5.2), WHITE))
    parts.append(G(3422 + 60, 58 + 60, K.icon_gear(1.25, 5.2), WHITE))
    for i, (x, y) in enumerate(BTN100):
        cx, cy = x + 50, y + 50
        parts.append(G(cx, cy, K.icon_mute(44) if i % 2 == 0 else K.icon_phase(1.15, 5.0), WHITE))
    # étiquettes de la colonne centrale
    def lab(t, x, base, size, w='Medium', col=LABEL):
        d, _ = K.text_path(t, size, w, x, base, 'middle', 0.02)
        return f'<path fill="{col}" d="{d}"/>'
    parts.append(lab('MIX', 1800, 338, 44))
    parts.append(lab('OVERLOAD', 1800, 682, 44))
    parts.append(lab('VARIPHI', 1800, 1026, 44))
    parts.append(lab('IR A', 1650, 580, 32))
    parts.append(lab('IR B', 1950, 580, 32))
    parts.append(lab('%', 1800, 962, 36))
    parts.append(lab('%', 1800, 1306, 36))
    # graduations des faders
    for (sx, _y) in THUMBS:
        for i, t in enumerate(TICK_LABELS):
            y = tick_y(i)
            long_ = i == 2
            parts.append(f'<rect x="{sx - 158:.1f}" y="{y - 1.6:.1f}" width="{128 if not long_ else 140:.1f}" height="3.2" rx="1.6" fill="{"#7A7A7A" if long_ else "#5A5A5A"}"/>')
            parts.append(f'<rect x="{sx + 34:.1f}" y="{y - 1.6:.1f}" width="14" height="3.2" rx="1.6" fill="#5A5A5A"/>')
            parts.append(f'<rect x="{sx + 128:.1f}" y="{y - 1.6:.1f}" width="10" height="3.2" rx="1.6" fill="#5A5A5A"/>')
            if t:
                ch = K.cap_height(31, 'Medium')
                d, _ = K.text_path(t, 31, 'Medium', sx + 88, y + ch / 2, 'middle')
                parts.append(f'<path fill="{DIM}" d="{d}"/>')
            else:
                parts.append(G(sx + 88, y, K.icon_infinity(1.55, 3.6), DIM))
    return ''.join(parts)

def logo_rgba(svg, box, crop_content=True):
    x0, y0, x1, y1 = box
    png = subprocess.run(['rsvg-convert', '-w', '4000', svg], capture_output=True, check=True).stdout
    im = Image.open(io.BytesIO(png)).convert('RGBA')
    im = im.crop(im.getchannel('A').point(lambda v: 255 if v > 8 else 0).getbbox())
    return im.resize((int(round(x1 - x0)), int(round(y1 - y0))), Image.LANCZOS)

def put_logo(img, svg, box):
    lg = logo_rgba(svg, box)
    a = np.zeros((H, W), np.float32); x0, y0 = int(round(box[0])), int(round(box[1]))
    al = np.asarray(lg, np.float32)[..., 3] / 255.0
    a[y0:y0 + al.shape[0], x0:x0 + al.shape[1]] = al
    img *= (1 - blur(shift(a, 2, 2), 1.4) * 0.5)[..., None]
    over(img, a * 0.97, np.array([247, 247, 245], np.float32))

put_logo(img, os.path.join(SRC, 'Impulse_LogoPP_v003_White.svg'), LOGO_IMP)
put_logo(img, os.path.join(SRC, 'NineHands_LogoPP_v003_FullWhite.svg'), LOGO_NH)
STATIC = static_overlay()
put_layer(img, svg_layer(STATIC))

# ---- fond statique (sans potards mobiles, sans boutons de fader, sans contenus d'écran)
bg = img.copy()
for (cx, cy) in KNOBS: knob_shadow(bg, cx, cy)
def save_rgba(arr, a, path):
    out = np.dstack([np.clip(arr, 0, 255), np.clip(a * 255, 0, 255)]).astype(np.uint8)
    Image.fromarray(out, 'RGBA').save(path, optimize=True)
save_rgba(bg, alpha, os.path.join(OUT, 'Impulse_background_3600x1440.png'))

# ================================================================ 10. aperçu complet (contenus comme imp100)
full = img.copy()
def read_ir(path):
    with wave.open(path) as w:
        n = w.getnframes(); ch = w.getnchannels(); sw = w.getsampwidth(); rate = w.getframerate()
        raw = w.readframes(n)
    if sw == 3:
        b = np.frombuffer(raw, np.uint8).reshape(-1, 3)
        v = (b[:, 0].astype(np.int32) | (b[:, 1].astype(np.int32) << 8) | (b[:, 2].astype(np.int32) << 16))
        v = np.where(v >= 1 << 23, v - (1 << 24), v).astype(np.float32) / (1 << 23)
    elif sw == 2: v = np.frombuffer(raw, np.int16).astype(np.float32) / 32768
    else: v = np.frombuffer(raw, np.int32).astype(np.float32) / 2 ** 31
    v = v.reshape(-1, ch).mean(1)
    last = np.nonzero(np.abs(v) > np.abs(v).max() * 3e-5)[0].max()
    return v[:last + 1], rate, sw * 8, ch

IRDIR = '/home/claude/work/irs'
irs = [read_ir(os.path.join(IRDIR, 'Cab 4x12 V30 SM57 A.wav')), read_ir(os.path.join(IRDIR, 'Cab 4x12 V30 R121 B.wav'))]
names = ['Cab 4x12 V30 SM57 A.wav', 'Cab 4x12 V30 R121 B.wav']
WAVE_AREAS = [(58, 537, 1152, 1348), (2448, 537, 3542, 1348)]

dyn = []
for s, ((wx0, wy0, wx1, wy1), (v, rate, bits, ch)) in enumerate(zip(WAVE_AREAS, irs)):
    # grille + grande lettre
    for f in (0.2476, 0.405, 0.595, 0.745):
        y = wy0 + (wy1 - wy0) * f
        dyn.append(f'<rect x="{wx0 + 10}" y="{y - 1.5:.1f}" width="{wx1 - wx0 - 20}" height="3" fill="#232425"/>')
    d, _ = K.text_path('AB'[s], 470, 'SemiBold', (wx0 + wx1) / 2, wy1 - 20, 'middle')
    dyn.append(f'<path fill="#FFFFFF" fill-opacity="0.045" d="{d}"/>')
    # infos
    info = f'{len(v) / rate * 1000:.0f}ms | {rate / 1000:g}kHz | {bits}-bit | {"mono" if ch == 1 else "stereo"}'
    d, _ = K.text_path(info, 35, 'Medium', wx1 - 34, wy0 + 64, 'end')
    dyn.append(f'<path fill="#D4D4D4" d="{d}"/>')
    # forme d'onde
    P = int(wx1 - wx0 - 40); mid = wy0 + (wy1 - wy0) * 0.50; amp = (wy1 - wy0) * 0.43
    pk = np.abs(v).max(); idx = np.linspace(0, len(v), P + 1).astype(int)
    xs_ = np.linspace(wx0 + 20, wx1 - 20, P)
    top = [v[idx[i]:max(idx[i + 1], idx[i] + 1)].max() / pk for i in range(P)]
    bot = [v[idx[i]:max(idx[i + 1], idx[i] + 1)].min() / pk for i in range(P)]
    pts = ' '.join(f'{x:.1f},{mid - t * amp:.1f}' for x, t in zip(xs_, top)) + ' ' + ' '.join(f'{x:.1f},{mid - b * amp:.1f}' for x, b in zip(xs_[::-1], bot[::-1]))
    dyn.append(f'<polygon points="{pts}" fill="{ACCH}" fill-opacity="0.95" stroke="{ACCH}" stroke-width="3"/>')
    dyn.append(f'<rect x="{wx0 + 20}" y="{mid - 2.2:.1f}" width="{wx1 - wx0 - 40}" height="4.4" fill="{ACCH}"/>')
    # nom de l'IR + pastille
    nx0, ny0, nx1, ny1 = NAMES[s]
    dyn.append(f'<rect x="{nx0 + 26}" y="{(ny0 + ny1) / 2 - 38:.1f}" width="84" height="76" rx="9" fill="{ACCH}"/>')
    ch_ = K.cap_height(50, 'Bold')
    d, _ = K.text_path('AB'[s], 50, 'Bold', nx0 + 68, (ny0 + ny1) / 2 + ch_ / 2, 'middle')
    dyn.append(f'<path fill="#0D1A22" d="{d}"/>')
    ch_ = K.cap_height(48, 'Medium')
    d, _ = K.text_path(names[s], 48, 'Medium', nx0 + 146, (ny0 + ny1) / 2 + ch_ / 2)
    dyn.append(f'<path fill="#ECECEC" d="{d}"/>')
# preset
px0, py0, px1, py1 = PRESET
dyn.append(G(px0 + 72, (py0 + py1) / 2, K.icon_updown(1.15), ACCH))
ch_ = K.cap_height(48, 'Medium')
d, _ = K.text_path('(Custom)', 48, 'Medium', px0 + 150, (py0 + py1) / 2 + ch_ / 2)
dyn.append(f'<path fill="#ECECEC" d="{d}"/>')
dyn.append(G(px1 - 62, (py0 + py1) / 2, K.icon_chevron(1.35, 6.0), ACCH))
# arcs des jauges
def arc(cx, cy, r, a0, a1):
    p0 = (cx + r * math.sin(a0), cy - r * math.cos(a0)); p1 = (cx + r * math.sin(a1), cy - r * math.cos(a1))
    large = 1 if abs(a1 - a0) > math.pi else 0; sweep = 1 if a1 > a0 else 0
    return f'M{p0[0]:.2f},{p0[1]:.2f} A{r},{r} 0 {large} {sweep} {p1[0]:.2f},{p1[1]:.2f}'
A0, A1 = math.radians(-135), math.radians(135)
vals = [(-0.20, True, '20A'), (0.25, False, '25'), (0.15, False, '15')]
for (cx, cy), (v, bip, txt) in zip(KNOBS, vals):
    rr = (GAUGE_R0 + GAUGE_R1) / 2
    a_from = 0.0 if bip else A0
    a_to = (v * A1) if bip else A0 + v * (A1 - A0)
    dyn.append(f'<path d="{arc(cx, cy, rr, min(a_from, a_to), max(a_from, a_to))}" fill="none" stroke="{ACCH}" stroke-width="8" stroke-linecap="round"/>')
    dot = (cx + rr * math.sin(a_to), cy - rr * math.cos(a_to))
    dyn.append(f'<circle cx="{dot[0]:.2f}" cy="{dot[1]:.2f}" r="7" fill="#FFFFFF"/>')
# glissières : remplissage accent sous le bouton
for (x0, y0, x1, y1), (tx, ty) in zip(SLOTS, THUMBS):
    dyn.append(f'<rect x="{(x0 + x1) / 2 - 6:.1f}" y="{ty:.1f}" width="12" height="{y1 - 10 - ty:.1f}" rx="6" fill="{ACCH}"/>')
DYN = ''.join(dyn)
put_layer(full, svg_layer(DYN), emboss=False)

# potards + valeurs
for (cx, cy), (v, bip, txt) in zip(KNOBS, vals):
    knob_shadow(full, cx, cy)
    ang = (v * 135) if bip else (-135 + v * 270)
    knob(full, cx, cy, ang)
vlay = []
for (cx, cy), (v, bip, txt) in zip(KNOBS, vals):
    sz = 60 if len(txt) < 3 else 54
    ch_ = K.cap_height(sz, 'Bold')
    d, _ = K.text_path(txt, sz, 'Bold', cx, cy + ch_ / 2, 'middle')
    vlay.append(f'<path fill="#FFFFFF" d="{d}"/>')
vl = svg_layer(''.join(vlay))
full *= (1 - blur(shift(vl[..., 3], 2, 3), 2.0) * 0.7)[..., None]
over(full, vl[..., 3], vl[..., :3] * 255)
# boutons de fader
for (tx, ty) in THUMBS:
    s = THUMB.size[0]
    paste_rgba(full, THUMB, int(round(tx - s / 2)), int(round(ty - s / 2)))

save_rgba(full, alpha, os.path.join(OUT, 'Impulse_render_3600x1440.png'))

# ================================================================ 11. éléments séparés
def elem(fn, w, h, name):
    global X, Y
    Xs, Ys = X, Y
    Yl, Xl = np.mgrid[0:h, 0:w].astype(np.float32) + 0.5
    X, Y = Xl, Yl
    canvas = np.zeros((h, w, 3), np.float32)
    m = fn(canvas)
    sh = np.zeros((h, w), np.float32)
    X, Y = Xs, Ys
    return canvas, m

def export_button(size, pressed, name):
    pad = 30
    w = h = size + 2 * pad
    Yl, Xl = np.mgrid[0:h, 0:w].astype(np.float32) + 0.5
    global X, Y
    Xs, Ys = X, Y; X, Y = Xl, Yl
    sd = sd_rrect(X, Y, pad, pad, pad + size, pad + size, 13.05)
    m, col = button_body(sd, pressed)
    g = np.clip((Y - pad) / size, 0, 1); col = col * (1.08 - 0.20 * g)[..., None]
    d = -sd
    hl = np.exp(-((Y - (pad + 13.5)) / 1.4) ** 2) * (d > 13) * np.clip((size / 2 - np.abs(X - (pad + size / 2)) - 14) / 10, 0, 1) * (0.14 if not pressed else 0.05)
    col = col + (hl * 255)[..., None]
    off, s_ = (6, 9) if not pressed else (3, 5)
    shd = np.clip(np.maximum(blur(shift(m, off, off), s_) * 0.75, blur(m, 3.0) * 0.55) * (1 - m), 0, 0.92)
    a = np.maximum(m, shd)
    rgb = np.where(m[..., None] > 0, col, 0)
    rgb = np.where(a[..., None] > 0, rgb * m[..., None] / np.maximum(a[..., None], 1e-6), 0)
    X, Y = Xs, Ys
    Image.fromarray(np.dstack([np.clip(rgb, 0, 255), a * 255]).astype(np.uint8), 'RGBA').save(os.path.join(OUT, 'elements', name))
export_button(120, False, 'button_120.png'); export_button(120, True, 'button_120_pressed.png')
export_button(100, False, 'button_100.png'); export_button(100, True, 'button_100_pressed.png')

# couronne : sprite d'animation (128 images, -135° .. +135°), empilées verticalement
NF = 128
spr = Image.new('RGBA', (KPX, KPX * NF))
for i in range(NF):
    a = -135 + 270 * i / (NF - 1)
    spr.paste(k_crown.rotate(-a, Image.BICUBIC), (0, i * KPX))
spr.save(os.path.join(OUT, 'knob', f'knob_crown_sprite_{NF}frames_{KPX}px.png'), optimize=True)

# ================================================================ 12. SVG : icônes, textes, calques complets
icons = {
    'power': (K.icon_power(33, 10), 50, ACCH), 'list_search': (K.icon_list_search(1.25, 5.2), 50, WHITE),
    'prev': (K.icon_tri(-1, 1.15, 5.2), 32, WHITE), 'next': (K.icon_tri(1, 1.15, 5.2), 32, WHITE),
    'save': (K.icon_save(1.2, 5.2), 40, WHITE), 'gear': (K.icon_gear(1.25, 5.2), 34, WHITE),
    'mute_M': (K.icon_mute(44), 26, WHITE), 'phase': (K.icon_phase(1.15, 5.0), 28, WHITE),
    'preset_updown': (K.icon_updown(1.15), 40, ACCH), 'chevron': (K.icon_chevron(1.35, 6.0), 24, ACCH),
    'infinity': (K.icon_infinity(1.55, 3.6), 26, DIM),
}
for n, (body, half, colr) in icons.items():
    with open(os.path.join(OUT, 'svg', 'icons', f'{n}.svg'), 'w') as f: f.write(K.icon_svg_file(body, half, colr))
texts = [('MIX', 44, 'Medium'), ('OVERLOAD', 44, 'Medium'), ('VARIPHI', 44, 'Medium'), ('IR A', 32, 'Medium'), ('IR B', 32, 'Medium'),
         ('%', 36, 'Medium')] + [(t, 31, 'Medium') for t in sorted(set(TICK_LABELS) - {''}, key=lambda s: int(s))]
for t, sz, w_ in texts:
    nm = {'%': 'percent'}.get(t, t.replace(' ', '_'))
    with open(os.path.join(OUT, 'svg', 'texts', f'label_{nm}.svg'), 'w') as f: f.write(K.text_svg_file(t, sz, w_, LABEL if sz > 31 else DIM))
for t in ('A', 'B'):
    ch_ = K.cap_height(50, 'Bold'); d, _ = K.text_path(t, 50, 'Bold', 42, 38 + ch_ / 2, 'middle')
    body = f'<rect class="bg" x="0" y="0" width="84" height="76" rx="9"/><path class="c" d="{d}"/>'
    with open(os.path.join(OUT, 'svg', 'icons', f'badge_{t}.svg'), 'w') as f:
        f.write(K.svg_doc(84, 76, body, '#0D1A22', extra_style=f'.bg{{fill:{ACCH};}}'))
for src_, dst in (('Impulse_LogoPP_v003_White.svg', 'logo_impulse.svg'), ('NineHands_LogoPP_v003_FullWhite.svg', 'logo_ninehands_dsp.svg')):
    with open(os.path.join(SRC, src_)) as fi, open(os.path.join(OUT, 'svg', 'logos', dst), 'w') as fo: fo.write(fi.read())
hdr = f'<?xml version="1.0" encoding="UTF-8"?>\n<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">\n'
with open(os.path.join(OUT, 'svg', 'Impulse_static_overlay_3600x1440.svg'), 'w') as f:
    f.write(hdr + '<!-- icônes, étiquettes et graduations, positionnées au pixel près sur le rendu -->\n' + STATIC + '\n</svg>\n')
with open(os.path.join(OUT, 'svg', 'Impulse_preview_contents_3600x1440.svg'), 'w') as f:
    f.write(hdr + '<!-- contenus d\'aperçu : formes d\'onde, noms, preset, jauges, valeurs -->\n' + DYN + ''.join(vlay) + '\n</svg>\n')
print('ok')
