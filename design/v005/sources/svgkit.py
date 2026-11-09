# Textes (Inter, convertis en tracés) et icônes vectorielles d'Impulse.
# Chaque élément est un SVG autonome : la couleur est dans une seule règle CSS (.c / .s) facile à changer.
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
import math

FONTS = {w: TTFont(f'/home/claude/Impulse/assets/Inter-{w}.otf') for w in ('Medium', 'SemiBold', 'Bold')}


def text_path(text, size, weight='SemiBold', x=0.0, y=0.0, anchor='start', tracking=0.0):
    """Tracé SVG d'un texte (ligne de base y). Retourne (d, largeur)."""
    f = FONTS[weight]
    gs = f.getGlyphSet(); cmap = f.getBestCmap(); upm = f['head'].unitsPerEm; hmtx = f['hmtx']
    s = size / upm
    names = [cmap.get(ord(ch), '.notdef') for ch in text]
    width = sum(hmtx[n][0] for n in names) * s + tracking * size * max(0, len(names) - 1)
    x0 = x - (width / 2 if anchor == 'middle' else width if anchor == 'end' else 0)
    pen = SVGPathPen(gs)
    cx = x0
    for n in names:
        gs[n].draw(TransformPen(pen, (s, 0, 0, -s, cx, y)))
        cx += hmtx[n][0] * s + tracking * size
    return pen.getCommands(), width


def cap_height(size, weight='SemiBold'):
    f = FONTS[weight]
    return f['OS/2'].sCapHeight * size / f['head'].unitsPerEm


def svg_doc(w, h, body, color='#E8E8E8', stroke=False, extra_style=''):
    style = f'.c{{fill:{color};}} .s{{fill:none;stroke:{color};stroke-linecap:round;stroke-linejoin:round;}} {extra_style}'
    return (f'<?xml version="1.0" encoding="UTF-8"?>\n<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w:g} {h:g}" '
            f'width="{w:g}" height="{h:g}">\n<style>{style}</style>\n{body}\n</svg>\n')


# ---------------------------------------------------------------- icônes (centrées sur (0,0), unité = px du rendu 3600)
def icon_power(r=33, sw=10):
    a0, a1 = math.radians(-50), math.radians(230)
    def pt(a): return (r * math.sin(a), -r * math.cos(a))
    # arc ouvert en haut (comme sur la maquette)
    p0 = pt(math.radians(40)); p1 = pt(math.radians(320))
    d = f'M{p0[0]:.2f},{p0[1]:.2f} A{r},{r} 0 1 1 {p1[0]:.2f},{p1[1]:.2f} M0,{-r - 4:.2f} L0,{-r * 0.05:.2f}'
    return f'<path class="s" stroke-width="{sw}" d="{d}"/>'


def icon_list_search(u=1.0, sw=5.0):
    d = (f'M{-27*u},{-17*u} H{19*u} M{-27*u},{0} H{9*u} M{-27*u},{17*u} H{1*u}')
    c = f'<circle class="s" stroke-width="{sw}" cx="{19*u}" cy="{10*u}" r="{9*u}"/>'
    h = f'<path class="s" stroke-width="{sw*1.15}" d="M{25.5*u},{16.5*u} L{33*u},{24*u}"/>'
    return f'<path class="s" stroke-width="{sw}" d="{d}"/>{c}{h}'


def icon_tri(direction=1, u=1.0, sw=5.0):
    s = direction
    d = f'M{-14*s*u},{-19*u} L{-14*s*u},{19*u} L{17*s*u},0 Z'
    return f'<path class="s" stroke-width="{sw}" d="{d}"/>'


def icon_save(u=1.0, sw=5.0):
    # disquette (coin coupé, volet) ouverte en bas à droite pour le « + »
    body = (f'M{4*u},{21*u} H{-20*u} V{-21*u} H{11*u} L{20*u},{-12*u} V{3*u}')
    shutter = f'M{-11*u},{-21*u} V{-9*u} H{6*u} V{-21*u}'
    plus = f'M{17*u},{9*u} V{27*u} M{8*u},{18*u} H{26*u}'
    return (f'<path class="s" stroke-width="{sw}" d="{body}"/><path class="s" stroke-width="{sw}" d="{shutter}"/>'
            f'<path class="s" stroke-width="{sw*1.1}" d="{plus}"/>')


def icon_gear(u=1.0, sw=5.0, teeth=8):
    pts = []
    for i in range(teeth * 2):
        a0 = 2 * math.pi * i / (teeth * 2) - 0.14; a1 = 2 * math.pi * (i + 1) / (teeth * 2) - 0.14
        rr = (22 if i % 2 == 0 else 16) * u
        pts += [(rr * math.sin(a0), -rr * math.cos(a0)), (rr * math.sin(a1), -rr * math.cos(a1))]
    d = 'M' + ' L'.join(f'{x:.2f},{y:.2f}' for x, y in pts) + ' Z'
    return f'<path class="s" stroke-width="{sw}" d="{d}"/><circle class="s" stroke-width="{sw}" cx="0" cy="0" r="{7*u}"/>'


def icon_phase(u=1.0, sw=5.0):
    return (f'<circle class="s" stroke-width="{sw}" cx="0" cy="0" r="{15*u}"/>'
            f'<path class="s" stroke-width="{sw}" d="M{-18*u},{19*u} L{18*u},{-19*u}"/>')


def icon_mute(size=40):
    ch = cap_height(size, 'SemiBold')
    d, _ = text_path('M', size, 'SemiBold', 0, ch / 2, 'middle')
    return f'<path class="c" d="{d}"/>'


def icon_updown(u=1.0):
    return (f'<path class="c" d="M{-26*u},{8*u} L{-12*u},{-9*u} L{2*u},{8*u} Z"/>'
            f'<path class="c" d="M{4*u},{-8*u} L{32*u},{-8*u} L{18*u},{9*u} Z"/>')


def icon_chevron(u=1.0, sw=5.5):
    return f'<path class="s" stroke-width="{sw}" d="M{-14*u},{-7*u} L0,{7*u} L{14*u},{-7*u}"/>'


def icon_infinity(u=1.0, sw=3.0):
    return (f'<ellipse class="s" stroke-width="{sw}" cx="{-7.5*u}" cy="0" rx="{7.5*u}" ry="{6*u}"/>'
            f'<ellipse class="s" stroke-width="{sw}" cx="{7.5*u}" cy="0" rx="{7.5*u}" ry="{6*u}"/>')


def icon_svg_file(body, half, color='#E8E8E8'):
    """SVG autonome d'une icône centrée, boîte carrée de côté 2*half."""
    return svg_doc(2 * half, 2 * half, f'<g transform="translate({half},{half})">{body}</g>', color)


def text_svg_file(text, size, weight='SemiBold', color='#E8E8E8', pad=4):
    ch = cap_height(size, weight)
    d, w = text_path(text, size, weight, pad, pad + ch)
    # descendantes : marge basse proportionnelle
    h = ch + pad * 2 + size * 0.22
    return svg_doc(w + pad * 2, h, f'<path class="c" d="{d}"/>', color)
