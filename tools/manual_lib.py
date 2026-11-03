# Studio Wiga - moteur commun des notices PDF bilingues (anglais puis francais), style maison.
# Utilise par tools/manual.py (contenu de la notice). Images et polices dans tools/manual_assets/.
# Blocs de contenu (listes de tuples) :
#   ("h1", txt) ("h2", txt) ("p", txt) ("small", txt) ("bullets", [txt...]) ("pb",) ("cond", mm) ("space", mm)
#   ("table", rows, widths_mm, opts)      opts : {"header": True, "small": False, "total": largeur_mm}  (None = reste de la largeur)
#   ("img", nom, largeur_mm, legende)      image centree, legende optionnelle (garde ensemble)
#   ("imgs", [noms], largeur_max_mm, legende)  images cote a cote
#   ("side", nom, largeur_mm, [blocs])     image a gauche, blocs a droite
#   ("keep", [blocs])                      blocs gardes sur la meme page
import os
from PIL import Image
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_CENTER
from reportlab.platypus import (BaseDocTemplate, PageTemplate, NextPageTemplate, Frame, Paragraph, Spacer, Image as RLImage,
                                Table, TableStyle, PageBreak, CondPageBreak, KeepTogether)
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.fonts import addMapping

TOOLS = os.path.dirname(os.path.abspath(__file__))
ASSETS = os.path.join(TOOLS, "manual_assets")
FONTS = os.path.join(ASSETS, "fonts")
for n, f in (("PopL", "Poppins-Light"), ("Pop", "Poppins-Regular"), ("PopM", "Poppins-Medium"), ("PopB", "Poppins-Bold"),
             ("PopLI", "Poppins-LightItalic")):
    pdfmetrics.registerFont(TTFont(n, os.path.join(FONTS, f + ".ttf")))
addMapping("PopL", 0, 0, "PopL"); addMapping("PopL", 1, 0, "PopB"); addMapping("PopL", 0, 1, "PopLI"); addMapping("PopL", 1, 1, "PopB")

DARK = colors.HexColor("#1B1F2A"); GREY = colors.HexColor("#6B7080"); LIGHT = colors.HexColor("#F2F3F7")
W = A4[0] - 36 * mm


def build(out, product, version, accent, cover_name, texts, content, footer=None):
    """texts[lang] = dict(lang=..., sub=[lignes], ver=...) ; content(lang) -> liste de blocs."""
    ACC = colors.HexColor(accent)
    st = {
        "h1": ParagraphStyle("h1", fontName="PopB", fontSize=17, leading=21, textColor=ACC, spaceAfter=6, spaceBefore=8, keepWithNext=1),
        "h2": ParagraphStyle("h2", fontName="PopM", fontSize=11.5, leading=15, textColor=DARK, spaceAfter=3, spaceBefore=7, keepWithNext=1),
        "p": ParagraphStyle("p", fontName="PopL", fontSize=9.3, leading=13.4, textColor=DARK, spaceAfter=5),
        "bul": ParagraphStyle("bul", fontName="PopL", fontSize=9.3, leading=13.4, textColor=DARK, spaceAfter=3, leftIndent=10, bulletIndent=0),
        "small": ParagraphStyle("small", fontName="PopL", fontSize=7.8, leading=10.5, textColor=GREY, alignment=TA_CENTER, spaceAfter=4),
        "cell": ParagraphStyle("cell", fontName="PopL", fontSize=8.2, leading=10.8, textColor=DARK),
        "scell": ParagraphStyle("scell", fontName="PopL", fontSize=7.6, leading=9.8, textColor=DARK),
        "hcell": ParagraphStyle("hcell", fontName="PopM", fontSize=8.2, leading=10.8, textColor=DARK),
        "cover": ParagraphStyle("cover", fontName="PopL", fontSize=38, leading=46, textColor=colors.white, alignment=TA_CENTER),
        "coversub": ParagraphStyle("coversub", fontName="PopL", fontSize=11, leading=15, textColor=colors.HexColor("#BFC2CC"), alignment=TA_CENTER),
        "coverlang": ParagraphStyle("coverlang", fontName="PopM", fontSize=8.5, leading=11, textColor=ACC),
    }

    def P(t, s="p"): return Paragraph(t, st[s])

    import tempfile
    tmpdir = tempfile.mkdtemp(prefix="wiga_manual_")

    def img(name, width):
        path = os.path.join(ASSETS, name)
        im = Image.open(path)
        if im.mode in ("RGB", "P", "L") and os.path.getsize(path) > 150000:     # photos : JPEG q92 (PDF plus leger)
            jp = os.path.join(tmpdir, os.path.splitext(name)[0] + ".jpg")
            if not os.path.exists(jp): im.convert("RGB").save(jp, quality=92)
            path = jp
        return RLImage(path, width=width, height=width * im.height / im.width)

    def table(rows, widths, opts):
        opts = opts or {}
        header = opts.get("header", True)
        cs = "scell" if opts.get("small") else "cell"
        free = (opts["total"] * mm if "total" in opts else W) - sum(w * mm for w in widths if w is not None)
        nn = max(1, sum(1 for w in widths if w is None))
        cw = [free / nn if w is None else w * mm for w in widths]
        data = [[P(c, "hcell" if (header and r == 0) else cs) for c in row] for r, row in enumerate(rows)]
        t = Table(data, colWidths=cw, repeatRows=1 if header else 0)
        style = [("VALIGN", (0, 0), (-1, -1), "TOP"), ("LINEBELOW", (0, 0), (-1, -1), 0.3, colors.HexColor("#D9DBE3")),
                 ("TOPPADDING", (0, 0), (-1, -1), 2.5), ("BOTTOMPADDING", (0, 0), (-1, -1), 2.5),
                 ("LEFTPADDING", (0, 0), (-1, -1), 3), ("RIGHTPADDING", (0, 0), (-1, -1), 3)]
        if header:
            style += [("BACKGROUND", (0, 0), (-1, 0), LIGHT), ("LINEBELOW", (0, 0), (-1, 0), 0.8, ACC)]
        else:
            style += [("FONTNAME", (0, 0), (0, -1), "PopM")]
        t.setStyle(TableStyle(style))
        return t

    def flow(blocks):
        f = []
        for b in blocks:
            k = b[0]
            if k in ("h1", "h2", "p"): f.append(P(b[1], k))
            elif k == "small": f.append(P(b[1], "small"))
            elif k == "bullets": f += [Paragraph(x, st["bul"], bulletText="•") for x in b[1]]
            elif k == "pb": f.append(PageBreak())
            elif k == "cond": f.append(CondPageBreak(b[1] * mm))
            elif k == "space": f.append(Spacer(1, b[1] * mm))
            elif k == "table": f.append(table(b[1], b[2], b[3] if len(b) > 3 else None))
            elif k == "img":
                g = [Spacer(1, 1.5 * mm), img(b[1], b[2] * mm)]
                if len(b) > 3 and b[3]: g.append(P(b[3], "small"))
                f.append(KeepTogether(g))
            elif k == "imgs":
                n = len(b[1]); gw = (W - (n - 1) * 4 * mm) / n
                row = Table([[img(x, min(gw, b[2] * mm)) for x in b[1]]], colWidths=[gw] * n)
                row.setStyle(TableStyle([("ALIGN", (0, 0), (-1, -1), "CENTER"), ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
                                         ("LEFTPADDING", (0, 0), (-1, -1), 0), ("RIGHTPADDING", (0, 0), (-1, -1), 0)]))
                g = [Spacer(1, 1.5 * mm), row]
                if len(b) > 3 and b[3]: g.append(P(b[3], "small"))
                f.append(KeepTogether(g))
            elif k == "side":
                iw = b[2] * mm
                t = Table([[img(b[1], iw), flow(b[3])]], colWidths=[iw + 5 * mm, W - iw - 5 * mm])
                t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (-1, -1), 0),
                                       ("RIGHTPADDING", (0, 0), (-1, -1), 0), ("TOPPADDING", (0, 0), (-1, -1), 2),
                                       ("BOTTOMPADDING", (0, 0), (-1, -1), 4)]))
                f.append(t)
            elif k == "keep": f.append(KeepTogether(flow(b[1])))
            else: raise ValueError(k)
        return f

    def section(lang):
        t = texts[lang]
        f = [NextPageTemplate("p"), Spacer(1, 2 * mm), P(t["lang"], "coverlang"), Spacer(1, 4 * mm)]
        cov = Image.open(os.path.join(ASSETS, "cover.png"))
        ch = 150 * mm
        cw = ch * cov.width / cov.height
        if cw > W * 0.9: cw = W * 0.9; ch = cw * cov.height / cov.width
        f.append(img("cover.png", cw))
        f += [Spacer(1, 6 * mm), P(cover_name, "cover"), Spacer(1, 2 * mm)]
        f += [P(x, "coversub") for x in t["sub"]]
        f += [Spacer(1, 1 * mm), P(t["ver"], "coversub"), Spacer(1, 8 * mm), img("logo_studio_wiga_blanc.png", 50 * mm), PageBreak()]
        f += flow(content(lang))
        f.append(PageBreak())
        return f

    foot = footer or (product + " (Studio Wiga) · v" + version)

    def deco(c, doc):
        c.saveState()
        c.setFillColor(ACC); c.rect(0, A4[1] - 2.2 * mm, A4[0], 2.2 * mm, stroke=0, fill=1)
        c.setFont("PopM", 7.5); c.setFillColor(GREY)
        c.drawString(18 * mm, 10 * mm, foot)
        c.drawRightString(A4[0] - 18 * mm, 10 * mm, str(doc.page))
        c.restoreState()

    def cover_bg(c, doc):
        c.saveState(); c.setFillColor(colors.black); c.rect(0, 0, A4[0], A4[1], stroke=0, fill=1)
        c.setFillColor(ACC); c.rect(0, A4[1] - 3 * mm, A4[0], 3 * mm, stroke=0, fill=1); c.restoreState()

    doc = BaseDocTemplate(out, pagesize=A4, leftMargin=18 * mm, rightMargin=18 * mm, topMargin=16 * mm, bottomMargin=18 * mm,
                          title=product + " - Manual / Notice v" + version, author="Studio Wiga")
    doc.addPageTemplates([PageTemplate(id="cover", frames=[Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="fc")], onPage=cover_bg),
                          PageTemplate(id="p", frames=[Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="f")], onPage=deco)])
    en = section("en"); fr = section("fr")
    en[-1:] = [NextPageTemplate("cover"), PageBreak()]
    doc.build(en + fr[:-1])
    print(out)
