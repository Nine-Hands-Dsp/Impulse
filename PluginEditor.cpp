#include "PluginEditor.h"
#include <BinaryData.h>
#include "WigaWindowSize.h"
#include "ColourPicker.h"

// ============================================================================
// IMPULSE - interface « metal brosse » (d'apres la maquette fournie, nov. 2026)
// Tout est dessine dans le repere de la maquette (1876 x 737 points), puis mis a l'echelle de la fenetre :
// net a toutes les tailles et a tous les zooms Windows / Retina. Fenetre par defaut (« 60 % ») : 747 x 298 (sans fond perdu) (meme largeur qu'Ampire).
// ============================================================================
namespace
{
    constexpr int kW = 1876, kH = 782;          // repere de dessin : maquette 1876 x 737 + 45 de bandeau (meme hauteur qu'Ampire)
    constexpr float kTop = 45.0f;               // corps descendu de 45 (bandeau du haut agrandi)
    // partie visible : le boitier seul, sans le fond perdu (marge noire facon Ampire : 16.1 a gauche / droite,
    // 13.2 en haut, 33 en bas, en unites du repere)
    constexpr float kVisX = 16.1f, kVisY = 13.2f, kVisW = 1876.0f - 2.0f * 16.1f, kVisH = 782.0f - 13.2f - 33.0f;
    constexpr int kWin = 747, kWinH = 298;      // taille de la fenetre « 60 % » (meme largeur qu'Ampire)
    const float kScales[3] = { 1.0f, 1.25f, 5.0f / 3.0f };   // 60 % (defaut) / 75 % / 100 %

    namespace col
    {
        const juce::Colour outer   (0xff0d0d0d), nameBox (0xff181818), nameBoxLo (0xff111111),
                           btnHi   (0xff4a4a4a), btnLo (0xff262626), btnEdge (0xff0b0b0b),
                           waveTop (0xff0e0e0e), waveBot (0xff161616), grid (0xff272727),
                           label   (0xffd2d2d2), text (0xffececec), info (0xffd4d4d4), dim (0xff8a8a8a),
                           knobTrack (0xff141414), letter (0xff1c1c1c), tick (0xff4a4a4a), tickTxt (0xffcfcfcf),
                           icon    (0xffe4e4e4), err (0xffe0705a);
        // couleur d'accent (nuancier) : reprise du processeur au debut de chaque paint (une couleur par instance)
        juce::Colour accent (ImpulseSettings::kDefaultAccent);
    }

    // ---- polices (Inter, OFL) incluses dans le plugin
    struct Fonts
    {
        juce::Typeface::Ptr semi, bold, medium;
        Fonts()
        {
            semi   = juce::Typeface::createSystemTypefaceFor (BinaryData::InterSemiBold_otf, BinaryData::InterSemiBold_otfSize);
            bold   = juce::Typeface::createSystemTypefaceFor (BinaryData::InterBold_otf,     BinaryData::InterBold_otfSize);
            medium = juce::Typeface::createSystemTypefaceFor (BinaryData::InterMedium_otf,   BinaryData::InterMedium_otfSize);
        }
        static const Fonts& get() { static Fonts f; return f; }
    };
    juce::Font fSemi (float h)   { return juce::Font (juce::FontOptions (Fonts::get().semi).withHeight (h)); }
    juce::Font fBold (float h)   { return juce::Font (juce::FontOptions (Fonts::get().bold).withHeight (h)); }
    juce::Font fMedium (float h) { return juce::Font (juce::FontOptions (Fonts::get().medium).withHeight (h)); }
    // noms de fichiers : sans les alternatives contextuelles d'Inter (sinon « 4x12 » s'affiche « 4×12 »)
    juce::Font fName (float h)
    {
        auto f = fMedium (h);
        f.setFeatureSetting ({ juce::FontFeatureTag ("calt"), juce::FontFeatureSetting::featureDisabled });
        return f;
    }

    // ---- geometrie (pixels de la maquette 1876 x 737, symetrique autour de x = 938)
    const juce::Rectangle<float> rFrame (0, 0, (float) kW, (float) kH);
    const juce::Rectangle<float> rCard[2]  = { { 12, 93 + kTop, 790, 567 }, { 1074, 93 + kTop, 790, 567 } };
    const juce::Rectangle<float> rLower[2] = { { 37, 243 + kTop, 748, 412 }, { 1090, 243 + kTop, 748, 412 } };
    const juce::Rectangle<float> rWave[2]  = { { 37, 243 + kTop, 573, 412 }, { 1265, 243 + kTop, 573, 412 } };
    const juce::Rectangle<float> rStrip[2] = { { 610, 243 + kTop, 175, 412 }, { 1090, 243 + kTop, 175, 412 } };
    const juce::Rectangle<float> rWaveIn[2] = { { 41, 247 + kTop, 568, 404 }, { 1266, 247 + kTop, 568, 404 } };   // interieur (dans le cadre de la maquette)
    constexpr float kCardDx = 1053.0f, kStripDx = 480.0f;
    // boutons dossier / precedent / suivant a la taille de ceux d'Ampire (28 px a l'ecran a la taille par defaut)
    // colonnes alignees sur les boutons d'Ampire quand Impulse est empile dessous (memes x, meme pas) ;
    // le slot A reprend la meme place dans son panneau
    const juce::Rectangle<float> rName (37, 133 + kTop, 501, 69), rFolder (549.7f, 133 + kTop, 69.6f, 69), rPrev (627.5f, 133 + kTop, 69.6f, 69), rNext (705.3f, 133 + kTop, 69.6f, 69);
    const juce::Rectangle<float> rMute (634, 263 + kTop, 54, 47), rPhase (711, 263 + kTop, 54, 47);
    constexpr float kLetterSize = 338.0f, kLetterDy = 146.0f, kLetterDx[2] = { 5.5f, 15.0f }, kLetterSx[2] = { 1.10f, 1.06f };
    constexpr float kFaderX = 700.0f, kFaderTop = 341.0f + kTop, kFaderBot = 615.0f + kTop, kTickLabelX = 743.0f;
    // colonne centrale (mesures de la maquette) : potards de rayon 45 dans un anneau sombre de rayon 57, arc a 51.5
    constexpr float kKnobX = 936.5f, kKnobR = 51.5f, kKnobBodyR = 45.0f, kKnobRingR = 57.0f;
    const float kKnobY[3] = { 201.5f + kTop, 378.5f + kTop, 555.0f + kTop }, kLabelY[3] = { 124.5f + kTop, 308.0f + kTop, 480.0f + kTop };
    const char* kFaderTicks[10] = { "12", "6", "0", "6", "12", "18", "36", "45", "60", "" };
    const juce::Rectangle<float> rPreset (1175, 46.2f, 490, 69), rSave (1680.5f, 46.2f, 69.6f, 69), rGear (1758.3f, 46.2f, 69.6f, 69);   // sous le bord du haut facon Ampire
    const juce::Rectangle<float> rStatus (360, 58.7f, 790, 44);
    const juce::Rectangle<float> rTitle (158.6f, 51.0f, 203.0f, 59.6f), rPower (40, 54.7f, 52, 52);   // bandeau : tout centre a y = 80.7 (a egale distance du filet du haut et du bas)
    const juce::Rectangle<float> rBrand (938 - 108, 675.5f + kTop - 10.0f, 216, 20);   // au-dessus du bas sombre facon Ampire

    juce::Rectangle<int> toInt (juce::Rectangle<float> r) { return r.getSmallestIntegerContainer(); }

    // bouton en relief (metal sombre) : degrade, bord sombre, liseré clair en haut, ombre portee
    void drawButtonBg (juce::Graphics& g, juce::Rectangle<float> r, bool pressed, bool hover, float radius = 8.0f)
    {
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (r.translated (0, 3.0f).expanded (1.0f), radius + 1.0f);
        const float hb = hover && ! pressed ? 0.07f : 0.0f;
        juce::ColourGradient gr ((pressed ? col::btnLo : col::btnHi).brighter (hb), r.getX(), r.getY(),
                                 (pressed ? col::btnHi.darker (0.2f) : col::btnLo).brighter (hb), r.getX(), r.getBottom(), false);
        g.setGradientFill (gr);
        g.fillRoundedRectangle (r, radius);
        g.setColour (juce::Colours::white.withAlpha (pressed ? 0.04f : 0.14f));
        g.drawLine (r.getX() + radius, r.getY() + 1.5f, r.getRight() - radius, r.getY() + 1.5f, 1.5f);
        g.setColour (col::btnEdge.withAlpha (0.95f));
        g.drawRoundedRectangle (r.reduced (1.0f), radius, 2.2f);
    }

    // bouton facon Ampire (releve sur son fond) : contour noir, face plate, filet blanc en haut, ombre douce dessous
    void drawAmpireBtn (juce::Graphics& g, juce::Rectangle<float> r, bool pressed, bool hover)
    {
        const float u = r.getHeight() / 94.0f, rad = 11.0f * u;
        for (int i = 6; i >= 1; --i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.07f));
            g.fillRoundedRectangle (r.translated (0, (float) i * 1.6f * u).expanded (0.5f * (float) i * u), rad);
        }
        g.setColour (juce::Colour (0xff030303));
        g.fillRoundedRectangle (r, rad);
        auto f = r.reduced (4.5f * u);
        if (pressed) f = f.translated (0, 1.0f * u);
        const float lift = hover && ! pressed ? 0.05f : 0.0f;
        juce::ColourGradient face (juce::Colour (0xff2f2f2f).brighter (lift), 0, f.getY(), juce::Colour (0xff2a2a2a).brighter (lift), 0, f.getBottom(), false);
        face.addColour (0.45, juce::Colour (0xff343434).brighter (lift));
        if (pressed) face = juce::ColourGradient (juce::Colour (0xff232323), 0, f.getY(), juce::Colour (0xff272727), 0, f.getBottom(), false);
        g.setGradientFill (face);
        g.fillRoundedRectangle (f, rad - 4.0f * u);
        // filet clair du haut (s'efface aux coins)
        juce::ColourGradient hl (juce::Colours::white.withAlpha (0.0f), f.getX(), 0, juce::Colours::white.withAlpha (0.0f), f.getRight(), 0, false);
        const float a = pressed ? 0.35f : 0.95f;
        hl.addColour (0.10, juce::Colours::white.withAlpha (a)); hl.addColour (0.90, juce::Colours::white.withAlpha (a));
        g.setGradientFill (hl);
        g.fillRect (f.getX() + 3.0f * u, f.getY() + 0.5f * u, f.getWidth() - 6.0f * u, 3.0f * u);
        // bord gauche legerement eclaire, bord droit un peu plus sombre
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillRect (f.getX() + 0.5f * u, f.getY() + 6.0f * u, 4.0f * u, f.getHeight() - 12.0f * u);
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.fillRect (f.getRight() - 4.0f * u, f.getY() + 6.0f * u, 3.5f * u, f.getHeight() - 12.0f * u);
    }

    // cadre creuse (nom de l'IR, menu des presets)
    void drawInset (juce::Graphics& g, juce::Rectangle<float> r, float radius = 8.0f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillRoundedRectangle (r.translated (0, 1.5f).expanded (1.0f), radius + 1.0f);
        juce::ColourGradient gr (col::nameBox, 0, r.getY(), col::nameBoxLo, 0, r.getBottom(), false);
        g.setGradientFill (gr);
        g.fillRoundedRectangle (r, radius);
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.drawRoundedRectangle (r.reduced (1.0f), radius, 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawLine (r.getX() + radius, r.getY() + 3.5f, r.getRight() - radius, r.getY() + 3.5f, 3.0f);
    }

    juce::String formatInfo (const IRInfo& i)
    {
        juce::String s;
        s << (i.lengthMs >= 1000.0 ? juce::String (i.lengthMs / 1000.0, 2) + "s" : juce::String ((int) std::lround (i.lengthMs)) + "ms");
        juce::String k = juce::String (i.fileRate / 1000.0, 1);
        if (k.endsWith (".0")) k = k.dropLastCharacters (2);
        s << " | " << k << "kHz | " << i.bits << "-bit" << (i.isFloat ? " float" : "") << " | " << (i.channels > 1 ? "stereo" : "mono");
        if (i.fromEmbedded) s << " | saved copy";
        return s;
    }
}

static void showContextMenu (juce::Component& c, int learnIdx);
static void showGearMenu (juce::Component& c);
static void addSizeItems (juce::PopupMenu& m, juce::Component& c);
static bool handleSizeChoice (int r, juce::Component* c);
static bool handleLearnChoice (int r, ImpulseProcessor* p);
static juce::String assignText (ImpulseProcessor& p, int i);

// ============================================================================ commandes de base
struct LearnBlink
{
    // contour accent clignotant pendant le MIDI Learn de cette commande
    static bool shown (ImpulseProcessor& p, int idx)
    {
        return idx >= 0 && p.learnTarget.load() == idx && (juce::Time::getMillisecondCounter() / 250) % 2 == 0;
    }
    static void draw (juce::Graphics& g, juce::Rectangle<float> r, float radius)
    {
        g.setColour (col::accent);
        g.drawRoundedRectangle (r.reduced (1.0f), radius, 1.6f);
    }
};

class ParamControl : public juce::Component
{
public:
    ParamControl (ImpulseProcessor& p, const char* paramId, int learn)
        : proc (p), param (*p.apvts.getParameter (paramId)), learnIdx (learn),
          attachment (param, [this] (float) { repaint(); if (onValue) onValue(); }, nullptr)
    {
        attachment.sendInitialUpdate();
    }
    void useAccent() const { col::accent = juce::Colour (proc.accentColour.load()); }
    float norm() const { return param.getValue(); }
    float value() const { return param.convertFrom0to1 (param.getValue()); }
    void setNorm (float n) { attachment.setValueAsPartOfGesture (param.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, n))); }
    std::function<void()> onValue;
    ImpulseProcessor& proc;
    juce::RangedAudioParameter& param;
    int learnIdx;
    juce::ParameterAttachment attachment;
};

// ---- potard (MIX / OVERLOAD / VARIPHI)
class Knob : public ParamControl
{
public:
    Knob (ImpulseProcessor& p, const char* id, int learn, bool bipolarIn) : ParamControl (p, id, learn), bipolar (bipolarIn) {}

    void paint (juce::Graphics& g) override
    {
        useAccent();
        // le composant deborde vers le bas pour l'ombre portee : le centre est a 66 points du haut
        const juce::Point<float> c ((float) getWidth() * 0.5f, 66.0f);
        const float a0 = juce::degreesToRadians (-135.0f), a1 = juce::degreesToRadians (135.0f);
        const float n = norm(), ang = a0 + n * (a1 - a0);
        const float r = kKnobR, br = kKnobBodyR, rr = kKnobRingR;

        // ombre portee longue sous le potard (lumiere en haut)
        for (int i = 10; i >= 1; --i)
        {
            const float o = (float) i * 2.2f;
            g.setColour (juce::Colours::black.withAlpha (0.055f));
            g.fillEllipse (c.x - rr + o * 0.25f, c.y - rr + o, rr * 2.0f, rr * 2.0f);
        }
        // anneau sombre creuse
        juce::ColourGradient ring (juce::Colour (0xff1d1d1d), c.x, c.y - rr, juce::Colour (0xff0b0b0b), c.x, c.y + rr, false);
        g.setGradientFill (ring);
        g.fillEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f, 1.2f);

        // valeur (accent) : depuis le debut, ou depuis le centre pour MIX
        const float from = bipolar ? 0.0f : a0;
        if (std::abs (ang - from) > 0.01f)
        {
            juce::Path v; v.addCentredArc (c.x, c.y, r, r, 0.0f, juce::jmin (from, ang), juce::jmax (from, ang), true);
            g.setColour (col::accent);
            g.strokePath (v, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        }

        // corps : metal sombre tourne (degrade radial, reflets coniques, cercles fins)
        // valeurs relevees sur la maquette : corps presque uniforme (~#303030), leger reflet en haut a gauche
        juce::ColourGradient body (juce::Colour (0xff373737), c.x, c.y, juce::Colour (0xff323232), c.x + br, c.y, true);
        g.setGradientFill (body);
        g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
        {
            constexpr int wedges = 90;
            for (int i = 0; i < wedges; ++i)
            {
                const float t0 = juce::MathConstants<float>::twoPi * (float) i / wedges;
                const float t1 = juce::MathConstants<float>::twoPi * (float) (i + 1) / wedges;
                const float tm = 0.5f * (t0 + t1);
                const float k = std::cos (2.0f * (tm + 0.6f));             // deux reflets opposes
                juce::Path w;
                w.startNewSubPath (c);
                w.lineTo (c.x + br * std::sin (t0), c.y - br * std::cos (t0));
                w.lineTo (c.x + br * std::sin (t1), c.y - br * std::cos (t1));
                w.closeSubPath();
                g.setColour ((k > 0 ? juce::Colours::white : juce::Colours::black).withAlpha (0.022f * std::abs (k)));
                g.fillPath (w);
            }
            for (int i = 0; i < 18; ++i)
            {
                const float rc = br * (0.12f + 0.84f * (float) i / 17.0f);
                g.setColour ((i % 2 == 0 ? juce::Colours::white : juce::Colours::black).withAlpha (0.025f));
                g.drawEllipse (c.x - rc, c.y - rc, rc * 2.0f, rc * 2.0f, 1.3f);
            }
            // reflet doux en haut a gauche
            juce::ColourGradient hl (juce::Colours::white.withAlpha (0.07f), c.x - br * 0.62f, c.y - br * 0.62f,
                                     juce::Colours::white.withAlpha (0.0f), c.x - br * 0.62f + br * 0.55f, c.y - br * 0.62f + br * 0.55f, true);
            g.setGradientFill (hl);
            g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
        }
        // bord : liseré clair en haut, sombre en bas
        juce::ColourGradient edge (juce::Colours::white.withAlpha (0.62f), c.x, c.y - br,
                                   juce::Colours::white.withAlpha (0.0f), c.x, c.y - br * 0.55f, false);
        g.setGradientFill (edge);
        g.drawEllipse (c.x - br + 1.0f, c.y - br + 1.0f, br * 2.0f - 2.0f, br * 2.0f - 2.0f, 2.0f);
        juce::ColourGradient low (juce::Colours::white.withAlpha (0.0f), c.x, c.y, juce::Colours::white.withAlpha (0.10f), c.x, c.y + br, false);
        g.setGradientFill (low);
        g.drawEllipse (c.x - br + 1.5f, c.y - br + 1.5f, br * 2.0f - 3.0f, br * 2.0f - 3.0f, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));      // liseré discret tout autour
        g.drawEllipse (c.x - br + 1.5f, c.y - br + 1.5f, br * 2.0f - 3.0f, br * 2.0f - 3.0f, 2.5f);
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.drawEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f, 1.0f);

        // repere : point blanc sur l'arc
        const juce::Point<float> dot (c.x + r * std::sin (ang), c.y - r * std::cos (ang));
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (dot.x - 5.5f, dot.y - 4.5f, 11.0f, 11.0f);
        g.setColour (juce::Colours::white);
        g.fillEllipse (dot.x - 5.0f, dot.y - 5.0f, 10.0f, 10.0f);

        // valeur au centre (MIX : valeur + cote dominant « 50A », « 30B », « 0 » au centre)
        const int v = (int) std::lround (value());
        const juce::String txt = bipolar && v != 0 ? juce::String (std::abs (v)) + (v < 0 ? "A" : "B") : juce::String (v);
        g.setFont (fSemi (txt.length() >= 4 ? 33.0f : 40.0f));
        const juce::Rectangle<float> tr (c.x - 45, c.y - 24, 90, 46);
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.drawText (txt, tr.translated (0.0f, 2.0f), juce::Justification::centred);
        g.setColour (juce::Colours::white);
        g.drawText (txt, tr, juce::Justification::centred);
        if (LearnBlink::shown (proc, learnIdx)) LearnBlink::draw (g, juce::Rectangle<float> (c.x - 64.0f, c.y - 64.0f, 128.0f, 128.0f), 14.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { showContextMenu (*this, learnIdx); return; }
        dragging = true; startNorm = toVirtual (norm());
        attachment.beginGesture();
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging) return;
        const float sens = e.mods.isShiftDown() ? 1900.0f : 380.0f;
        setNorm (fromVirtual (startNorm - (float) e.getDistanceFromDragStartY() / sens + (float) e.getDistanceFromDragStartX() / (sens * 2.0f)));
    }
    void mouseUp (const juce::MouseEvent&) override { if (dragging) attachment.endGesture(); dragging = false; }
    // double-clic n'importe ou sur le potard (valeur comprise) : retour au centre / a la valeur par defaut
    void mouseDoubleClick (const juce::MouseEvent&) override { attachment.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue())); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        const float n = norm(), next = fromVirtual (toVirtual (n) + w.deltaY * 0.06f);
        // molette : arret sur le centre quand on passe d'un cote a l'autre
        const float target = bipolar && n != 0.5f && (next - 0.5f) * (n - 0.5f) < 0.0f ? 0.5f : next;
        attachment.beginGesture(); setNorm (target); attachment.endGesture();
    }
private:
    // MIX : butee au centre. La course « virtuelle » de la souris contient une zone morte autour du centre
    // (kDetent de chaque cote) : le potard reste sur 0 le temps de la traverser, puis repart sans perdre de course.
    static constexpr float kDetent = 0.035f;
    float toVirtual (float n) const
    {
        if (! bipolar) return n;
        const float x = n - 0.5f;
        if (x == 0.0f) return 0.5f;
        return 0.5f + (x > 0 ? 1.0f : -1.0f) * (kDetent + std::abs (x) * (0.5f - kDetent) / 0.5f);
    }
    float fromVirtual (float v) const
    {
        if (! bipolar) return v;
        const float x = v - 0.5f;
        if (std::abs (x) <= kDetent) return 0.5f;
        return 0.5f + (x > 0 ? 1.0f : -1.0f) * juce::jmin (0.5f, (std::abs (x) - kDetent) * 0.5f / (0.5f - kDetent));
    }
    bool bipolar, dragging = false;
    float startNorm = 0.0f;
};

// ---- fader de niveau A / B (+12 dB a -inf)
class Fader : public ParamControl
{
public:
    using ParamControl::ParamControl;
    // le composant couvre la colonne du fader ; y du repere local = y du repere global - top
    float top = 0.0f;
    float yForNorm (float n) const { return (kFaderBot - n * (kFaderBot - kFaderTop)) - top; }

    void paint (juce::Graphics& g) override
    {
        useAccent();
        const float cx = (float) getWidth() * 0.5f;
        const float yT = kFaderTop - top - 11.0f, yB = kFaderBot - top + 11.0f;
        // fente du fader
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillRoundedRectangle (cx - 8.0f, yT + 1.5f, 16.0f, yB - yT, 8.0f);
        g.setColour (juce::Colour (0xff080808));
        g.fillRoundedRectangle (cx - 7.5f, yT, 15.0f, yB - yT, 7.5f);
        const float y = yForNorm (norm());
        g.setColour (col::accent);
        g.fillRoundedRectangle (cx - 3.5f, y, 7.0f, yB - 4.0f - y, 3.5f);
        // bouton chrome + ombre
        const float tr = 17.0f;
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (cx - tr + 2.0f, y - tr + 4.0f, tr * 2.0f, tr * 2.0f);
        juce::ColourGradient gr (juce::Colour (0xfff2f2f2), cx - tr * 0.4f, y - tr * 0.5f, juce::Colour (0xff7d7d7d), cx + tr * 0.6f, y + tr, true);
        g.setGradientFill (gr);
        g.fillEllipse (cx - tr, y - tr, tr * 2.0f, tr * 2.0f);
        juce::ColourGradient cap (juce::Colour (0xffd8d8d8), cx, y - tr * 0.6f, juce::Colour (0xff9a9a9a), cx, y + tr * 0.6f, false);
        g.setGradientFill (cap);
        g.fillEllipse (cx - tr * 0.62f, y - tr * 0.62f, tr * 1.24f, tr * 1.24f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (cx - tr, y - tr, tr * 2.0f, tr * 2.0f, 1.2f);
        if (LearnBlink::shown (proc, learnIdx)) LearnBlink::draw (g, getLocalBounds().toFloat(), 8.0f);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { showContextMenu (*this, learnIdx); return; }
        dragging = true; startNorm = norm();
        attachment.beginGesture();
        // clic hors du bouton : le fader saute a la position cliquee
        if (std::abs ((float) e.y - yForNorm (startNorm)) > 18.0f)
        {
            startNorm = juce::jlimit (0.0f, 1.0f, (kFaderBot - top - (float) e.y) / (kFaderBot - kFaderTop));
            setNorm (startNorm);
        }
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging) return;
        const float travel = (kFaderBot - kFaderTop) * (e.mods.isShiftDown() ? 5.0f : 1.0f);
        setNorm (startNorm - (float) e.getDistanceFromDragStartY() / travel);
    }
    void mouseUp (const juce::MouseEvent&) override { if (dragging) attachment.endGesture(); dragging = false; }
    void mouseDoubleClick (const juce::MouseEvent&) override { attachment.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue())); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        attachment.beginGesture(); setNorm (norm() + w.deltaY * 0.04f); attachment.endGesture();
    }
private:
    bool dragging = false;
    float startNorm = 0.0f;
};

// ---- bouton a bascule (M, phase, power)
class ToggleBtn : public ParamControl
{
public:
    enum Kind { mute, phase, power };
    ToggleBtn (ImpulseProcessor& p, const char* id, int learn, Kind k) : ParamControl (p, id, learn), kind (k) {}
    void paint (juce::Graphics& g) override
    {
        useAccent();
        const auto r = getLocalBounds().toFloat();
        const bool on = norm() > 0.5f;
        if (kind == power)
        {
            const auto c = r.getCentre();
            const juce::Colour ic = on ? col::accent : col::dim;
            juce::Path p; p.addCentredArc (c.x, c.y + 2.0f, 16.0f, 16.0f, 0.0f, juce::degreesToRadians (38.0f), juce::degreesToRadians (322.0f), true);
            p.startNewSubPath (c.x, c.y - 17.0f); p.lineTo (c.x, c.y - 1.0f);
            const juce::PathStrokeType st (4.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.strokePath (p, st, juce::AffineTransform::translation (1.0f, 2.0f));
            g.setColour (ic);
            g.strokePath (p, st);
        }
        else
        {
            const auto b = r.reduced (3.0f);
            if (on)
            {
                g.setColour (juce::Colours::black.withAlpha (0.45f));
                g.fillRoundedRectangle (b.translated (0, 3.0f), 7.0f);
                g.setColour (col::accent);
                g.fillRoundedRectangle (b, 7.0f);
                g.setColour (col::btnEdge);
                g.drawRoundedRectangle (b.reduced (1.0f), 7.0f, 2.0f);
            }
            else drawButtonBg (g, b, false, isMouseOver(), 7.0f);
            g.setColour (on ? juce::Colour (0xff111111) : col::icon);
            if (kind == mute)
            {
                g.setFont (fSemi (27.0f));
                g.drawText ("M", b.translated (0, 0.5f), juce::Justification::centred);
            }
            else
            {
                const auto c = b.getCentre();
                g.drawEllipse (c.x - 11.0f, c.y - 11.0f, 22.0f, 22.0f, 3.2f);
                g.drawLine (c.x - 12.5f, c.y + 13.5f, c.x + 12.5f, c.y - 13.5f, 3.2f);
            }
        }
        if (LearnBlink::shown (proc, learnIdx)) LearnBlink::draw (g, r, 7.0f);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { showContextMenu (*this, learnIdx); return; }
        attachment.setValueAsCompleteGesture (norm() > 0.5f ? 0.0f : 1.0f);
    }
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }
private:
    Kind kind;
};

// ---- bouton a icone (dossier, precedent, suivant, enregistrer, reglages)
class IconBtn : public juce::Component
{
public:
    enum Kind { folder, prev, next, save, gear };
    IconBtn (Kind k, int learn = -1) : kind (k), learnIdx (learn) {}
    std::function<void()> onClick;
    ImpulseProcessor* proc = nullptr;
    int learnIdx;
    void useAccent() const { if (proc != nullptr) col::accent = juce::Colour (proc->accentColour.load()); }

    void paint (juce::Graphics& g) override
    {
        useAccent();
        const auto r = getLocalBounds().toFloat().reduced (3.0f);
        const bool ampireStyle = true;   // tous les boutons a icone facon Ampire
        if (ampireStyle) drawAmpireBtn (g, r, pressed, isMouseOver());
        else drawButtonBg (g, r, pressed, isMouseOver());
        const auto c = r.getCentre() + juce::Point<float> (0.0f, pressed ? 1.0f : 0.0f);
        const auto shadow = juce::AffineTransform::translation (1.0f, 2.0f);
        juce::Path p;
        // icones d'Ampire (traits fins blancs) : liste + loupe pour le dossier, triangles evides pour precedent / suivant.
        // Geometrie relevee sur le fond d'Ampire (en px de son fond, bouton de 92 px), mise a l'echelle du bouton.
        const float u = r.getHeight() / 94.0f;
        const juce::Colour ic (0xffebebeb);
        const juce::PathStrokeType thin (3.0f * u, juce::PathStrokeType::mitered, juce::PathStrokeType::butt);
        if (kind == folder)
        {
            juce::Path li;
            li.startNewSubPath (c.x - 21.0f * u, c.y - 14.5f * u); li.lineTo (c.x + 15.5f * u, c.y - 14.5f * u);
            li.startNewSubPath (c.x - 21.0f * u, c.y - 1.0f * u);  li.lineTo (c.x + 8.0f * u, c.y - 1.0f * u);
            li.startNewSubPath (c.x - 21.0f * u, c.y + 12.5f * u); li.lineTo (c.x + 1.5f * u, c.y + 12.5f * u);
            juce::Path lens; lens.addEllipse (c.x + 10.5f * u, c.y + 2.5f * u, 14.5f * u, 14.5f * u);
            juce::Path handle; handle.startNewSubPath (c.x + 22.5f * u, c.y + 14.5f * u); handle.lineTo (c.x + 29.0f * u, c.y + 21.0f * u);
            g.setColour (ic);
            g.strokePath (li, thin); g.strokePath (lens, thin);
            g.strokePath (handle, juce::PathStrokeType (3.6f * u, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
            return;
        }
        if (kind == prev || kind == next)
        {
            const float sd = kind == next ? 1.0f : -1.0f;
            p.addTriangle (c.x - 13.0f * sd * u, c.y - 16.0f * u, c.x - 13.0f * sd * u, c.y + 16.0f * u, c.x + 15.0f * sd * u, c.y);
            g.setColour (ic);
            g.strokePath (p, juce::PathStrokeType (3.0f * u, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
            if (proc != nullptr && LearnBlink::shown (*proc, learnIdx)) LearnBlink::draw (g, r, 8.0f);
            return;
        }
        if (kind == save)
        {
            // disquette au trait fin (style Ampire) + « + » dans une pastille
            g.setColour (ic);
            juce::Path body;
            body.startNewSubPath (c.x - 16.0f * u, c.y - 16.0f * u); body.lineTo (c.x + 9.0f * u, c.y - 16.0f * u);
            body.lineTo (c.x + 16.0f * u, c.y - 9.0f * u); body.lineTo (c.x + 16.0f * u, c.y + 1.0f * u);
            body.startNewSubPath (c.x + 1.0f * u, c.y + 16.0f * u); body.lineTo (c.x - 16.0f * u, c.y + 16.0f * u);
            body.lineTo (c.x - 16.0f * u, c.y - 16.0f * u);
            g.strokePath (body, thin);
            juce::Path shutter; shutter.addRectangle (c.x - 9.0f * u, c.y - 16.0f * u, 14.0f * u, 9.0f * u);
            g.strokePath (shutter, thin);
            juce::Path label; label.addRectangle (c.x - 10.0f * u, c.y + 2.0f * u, 10.0f * u, 14.0f * u);
            g.strokePath (label, thin);
            const juce::Point<float> pc (c.x + 14.0f * u, c.y + 14.0f * u);
            juce::Path plus;
            plus.startNewSubPath (pc.x, pc.y - 7.0f * u); plus.lineTo (pc.x, pc.y + 7.0f * u);
            plus.startNewSubPath (pc.x - 7.0f * u, pc.y); plus.lineTo (pc.x + 7.0f * u, pc.y);
            g.strokePath (plus, juce::PathStrokeType (3.2f * u, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
            return;
        }
        // roue dentee au trait fin (style Ampire)
        juce::Path gp;
        const int teeth = 8;
        for (int i = 0; i < teeth * 2; ++i)
        {
            const float a0 = juce::MathConstants<float>::twoPi * (float) i / (teeth * 2) - 0.14f;
            const float a1 = juce::MathConstants<float>::twoPi * (float) (i + 1) / (teeth * 2) - 0.14f;
            const float rr = ((i % 2 == 0) ? 17.0f : 12.5f) * u;
            if (i == 0) gp.startNewSubPath (c.x + rr * std::sin (a0), c.y - rr * std::cos (a0));
            else gp.lineTo (c.x + rr * std::sin (a0), c.y - rr * std::cos (a0));
            gp.lineTo (c.x + rr * std::sin (a1), c.y - rr * std::cos (a1));
        }
        gp.closeSubPath();
        g.setColour (ic);
        g.strokePath (gp, thin);
        g.drawEllipse (c.x - 5.5f * u, c.y - 5.5f * u, 11.0f * u, 11.0f * u, 3.0f * u);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { showContextMenu (*this, learnIdx); return; }
        pressed = true; repaint();
    }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! pressed) return;
        pressed = false; repaint();
        if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
    }
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }
private:
    Kind kind;
    bool pressed = false;
};

// ============================================================================ contenu de la fenetre
class ImpulseContent : public juce::Component,
                       public juce::FileDragAndDropTarget,
                       private juce::Timer
{
public:
    explicit ImpulseContent (ImpulseProcessor& p)
        : proc (p),
          power (p, "active", ImpulseProcessor::lActive, ToggleBtn::power),
          mix (p, "mix", ImpulseProcessor::lMix, true),
          over (p, "overload", ImpulseProcessor::lOverload, false),
          vari (p, "variphi", ImpulseProcessor::lVariphi, false),
          save (IconBtn::save), gear (IconBtn::gear)
    {
        setSize (kW, kH);
        setOpaque (true);

        save.proc = gear.proc = &proc;
        addAndMakeVisible (power); power.setBounds (toInt (rPower));
        addAndMakeVisible (save);  save.setBounds (toInt (rSave.expanded (3.0f)));
        addAndMakeVisible (gear);  gear.setBounds (toInt (rGear.expanded (3.0f)));
        save.onClick = [this] { presetFileMenu(); };
        gear.onClick = [this] { showGearMenu (gear); };

        Knob* knobs[3] = { &mix, &over, &vari };
        const char* names[3] = { "MIX", "OVERLOAD", "VARIPHI" };
        for (int i = 0; i < 3; ++i)
        {
            auto* k = knobs[i];
            addAndMakeVisible (*k);
            k->setBounds (toInt ({ kKnobX - 66.0f, kKnobY[i] - 66.0f, 132.0f, 154.0f }));
            k->onValue = [this, k, name = juce::String (names[i]), i]
            {
                const int v = (int) std::lround (k->value());
                if (i == 0)
                {
                    const int b = (v + 100) / 2;
                    showStatus ("MIX   A " + juce::String (100 - b) + " %  /  B " + juce::String (b) + " %");
                }
                else if (i == 2) showStatus (name + "   " + juce::String (v) + " %   (" + juce::String (v * 0.01, 2) + " ms)");
                else showStatus (name + "   " + juce::String (v) + " %");
            };
        }

        for (int s = 0; s < 2; ++s)
        {
            const float dx = s * kCardDx, sx = s * kStripDx;
            auto& sl = slot[s];
            sl.folder = std::make_unique<IconBtn> (IconBtn::folder);
            sl.prev   = std::make_unique<IconBtn> (IconBtn::prev, s == 0 ? ImpulseProcessor::lPrevA : ImpulseProcessor::lPrevB);
            sl.next   = std::make_unique<IconBtn> (IconBtn::next, s == 0 ? ImpulseProcessor::lNextA : ImpulseProcessor::lNextB);
            sl.prev->proc = sl.next->proc = sl.folder->proc = &proc;
            for (auto* b : { sl.folder.get(), sl.prev.get(), sl.next.get() }) addAndMakeVisible (*b);
            sl.folder->setBounds (toInt (rFolder.translated (dx, 0).expanded (3.0f)));
            sl.prev->setBounds (toInt (rPrev.translated (dx, 0).expanded (3.0f)));
            sl.next->setBounds (toInt (rNext.translated (dx, 0).expanded (3.0f)));
            sl.folder->onClick = [this, s] { chooseFile (s); };
            sl.prev->onClick = [this, s] { proc.stepIR (s, -1); };
            sl.next->onClick = [this, s] { proc.stepIR (s, +1); };

            sl.mute  = std::make_unique<ToggleBtn> (proc, s == 0 ? "muteA" : "muteB", s == 0 ? ImpulseProcessor::lMuteA : ImpulseProcessor::lMuteB, ToggleBtn::mute);
            sl.phase = std::make_unique<ToggleBtn> (proc, s == 0 ? "phaseA" : "phaseB", s == 0 ? ImpulseProcessor::lPhaseA : ImpulseProcessor::lPhaseB, ToggleBtn::phase);
            addAndMakeVisible (*sl.mute);  sl.mute->setBounds (toInt (rMute.translated (sx, 0).expanded (3.0f)));
            addAndMakeVisible (*sl.phase); sl.phase->setBounds (toInt (rPhase.translated (sx, 0).expanded (3.0f)));
            sl.mute->onValue  = [this, s] { askRepaint (toInt (rWave[s])); };
            sl.phase->onValue = [this, s] { showStatus (juce::String ("PHASE ") + (s == 0 ? "A " : "B ") + (slot[s].phase->norm() > 0.5f ? "inverted" : "normal")); };

            sl.fader = std::make_unique<Fader> (proc, s == 0 ? "gainA" : "gainB", s == 0 ? ImpulseProcessor::lGainA : ImpulseProcessor::lGainB);
            sl.fader->top = kFaderTop - 26.0f;
            addAndMakeVisible (*sl.fader);
            sl.fader->setBounds (toInt ({ kFaderX + sx - 22.0f, kFaderTop - 26.0f, 44.0f, kFaderBot - kFaderTop + 52.0f }));
            sl.fader->onValue = [this, s]
            {
                const float db = ImpulseProcessor::faderPosToDb (slot[s].fader->norm());
                showStatus (juce::String ("LEVEL ") + (s == 0 ? "A   " : "B   ") + (db <= -99.0f ? juce::String ("-inf") : (db > 0 ? "+" : "") + juce::String (db, 1)) + " dB");
            };
        }

        refreshInfos();
        startTimerHz (30);
    }

    ~ImpulseContent() override = default;

    // ---------------------------------------------------------------- dessin
    void paint (juce::Graphics& g) override
    {
        useAccent();
        // anti-gel v2 : on dessine le fond deja calcule par le timer (aucun calcul lourd ici)
        lastPaintMs = juce::Time::getMillisecondCounter();
        repaintAskedMs = 0;
        g.fillAll (col::outer);
        if (bgCache.isValid())
        {
            g.setImageResamplingQuality (juce::Graphics::mediumResamplingQuality);
            g.drawImageTransformed (bgCache, juce::AffineTransform::scale ((float) kW / (float) bgCache.getWidth(),
                                                                           (float) kH / (float) bgCache.getHeight()));
        }
        else drawStatic (g);    // premiere image seulement, avant que le timer ait prepare le cache

        // ligne d'etat (le logo IMPULSE est dans le fond)
        const auto st = statusText();
        if (st.isNotEmpty())
        {
            g.setColour (proc.learnTarget.load() >= 0 ? col::accent : juce::Colour (0xffd8d8d8));
            g.setFont (fSemi (24.0f));
            g.drawText (st, rStatus, juce::Justification::centred);
        }

        // preset
        {
            const auto r = rPreset;
            const float ax = r.getX() + 34.0f, cy = r.getCentreY();
            juce::Path up, dn;
            up.addTriangle (ax - 11.0f, cy + 4.0f, ax + 11.0f, cy + 4.0f, ax, cy - 9.0f);
            dn.addTriangle (ax + 13.0f, cy - 4.0f, ax + 35.0f, cy - 4.0f, ax + 24.0f, cy + 9.0f);
            g.setColour (col::accent);
            g.fillPath (up); g.fillPath (dn);
            g.setColour (col::text);
            g.setFont (fMedium (28.0f));
            juce::String name = proc.getPresetName();
            name = name.isEmpty() ? juce::String ("(Custom)") : name + (proc.isPresetDirty() ? " *" : "");
            g.drawText (name, juce::Rectangle<float> (r.getX() + 90, r.getY(), r.getWidth() - 150, r.getHeight()), juce::Justification::centredLeft, true);
            juce::Path chev;
            const float vx = r.getRight() - 33.0f;
            chev.startNewSubPath (vx - 11.0f, cy - 5.5f); chev.lineTo (vx, cy + 5.5f); chev.lineTo (vx + 11.0f, cy - 5.5f);
            g.setColour (col::accent);
            g.strokePath (chev, juce::PathStrokeType (3.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // slots : nom + forme d'onde
        for (int s = 0; s < 2; ++s)
        {
            const auto& inf = info[s];
            const auto nb = rName.translated (s * kCardDx, 0);
            // badge A / B
            const juce::Rectangle<float> badge (nb.getX() + 26, nb.getCentreY() - 26.0f, 58, 52);
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.fillRoundedRectangle (badge.translated (0, 2.0f), 6.0f);
            g.setColour (col::accent);
            g.fillRoundedRectangle (badge, 6.0f);
            g.setColour (juce::Colour (0xff111111));
            g.setFont (fBold (40.0f));
            g.drawText (s == 0 ? "A" : "B", badge.translated (0, 1.0f), juce::Justification::centred);
            const auto tr = juce::Rectangle<float> (badge.getRight() + 24, nb.getY(), nb.getRight() - badge.getRight() - 36, nb.getHeight());
            const bool showErr = inf.error.isNotEmpty() && juce::Time::getMillisecondCounter() < errUntil[s];
            g.setFont (fName (29.0f));
            if (showErr) { g.setColour (col::err); g.drawFittedText (inf.error, tr.toNearestInt(), juce::Justification::centredLeft, 2, 0.8f); }
            else if (inf.loaded)
            {
                g.setColour (col::text);
                g.drawFittedText (inf.name, tr.toNearestInt(), juce::Justification::centredLeft, 2, 0.8f);
            }
            else { g.setColour (col::dim); g.drawText ("Drop an IR or click to load", tr, juce::Justification::centredLeft, true); }
            if (dropHover == s) { g.setColour (col::accent.withAlpha (0.8f)); g.drawRoundedRectangle (nb.reduced (2.0f), 8.0f, 3.0f); }

            // forme d'onde
            const auto wr = rWaveIn[s];
            if (inf.loaded && ! inf.peakMax.empty())
            {
                const bool muted = slot[s].mute->norm() > 0.5f;
                g.saveState();
                g.reduceClipRegion (wr.toNearestInt());
                const float cy = wr.getCentreY(), amp = wr.getHeight() * 0.5f - 2.0f;
                const int cols = (int) wr.getWidth();
                const int P = (int) inf.peakMax.size();
                const float x0 = wr.getX() + 8.0f, pw = (wr.getWidth() - 8.0f) / (float) cols;
                juce::Path wave;
                float lastY = cy;
                for (int x = 0; x < cols; ++x)
                {
                    const int a = x * P / cols, b = juce::jmax (a + 1, (x + 1) * P / cols);
                    float mn = 1.0f, mx = -1.0f;
                    for (int i = a; i < juce::jmin (b, P); ++i) { mn = juce::jmin (mn, inf.peakMin[(size_t) i]); mx = juce::jmax (mx, inf.peakMax[(size_t) i]); }
                    const float px = x0 + (float) x * pw;
                    const float y0 = cy - mx * amp, y1 = cy - mn * amp;
                    if (x == 0) wave.startNewSubPath (px, lastY);
                    // relie la colonne precedente puis trace l'etendue min / max de cette colonne
                    wave.lineTo (px, std::abs (lastY - y0) < std::abs (lastY - y1) ? y0 : y1);
                    wave.lineTo (px, std::abs (lastY - y0) < std::abs (lastY - y1) ? y1 : y0);
                    lastY = std::abs (lastY - y0) < std::abs (lastY - y1) ? y1 : y0;
                }
                g.setColour (col::accent.withAlpha (muted ? 0.1f : 0.25f));
                g.strokePath (wave, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour (col::accent.withAlpha (muted ? 0.3f : 1.0f));
                g.strokePath (wave, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.restoreState();
                g.setColour (col::info);
                g.setFont (fMedium (24.0f));
                const auto ir = s == 0 ? juce::Rectangle<float> (wr.getX() + 12, wr.getY() + 16, wr.getWidth() - 32, 30)
                                       : juce::Rectangle<float> (wr.getX() + 12, wr.getY() + 16, wr.getWidth() - 32, 30);
                g.drawText (formatInfo (inf), ir, juce::Justification::centredRight, true);
            }
            if (dropHover == s) { g.setColour (col::accent.withAlpha (0.6f)); g.drawRoundedRectangle (rWave[s].reduced (3.0f), 8.0f, 3.0f); }
        }
    }

    // fond statique : la maquette de l'utilisateur (plaque brossee, cadres, etiquettes, graduations, logo IMPULSE),
    // puis ce que le plugin remplace : interieur des formes d'onde, cadres des noms et du preset, logo NINE HANDS | DSP
    void drawStatic (juce::Graphics& g)
    {
        g.fillAll (col::outer);
        if (background.isValid())
        {
            // fond 3500 px reduit par moities successives jusqu'a la taille affichee (pas d'aliasing)
            const int target = (int) std::ceil ((float) kW * g.getInternalContext().getPhysicalPixelScaleFactor());
            if (! bgScaled.isValid() || bgScaledFor != target)
            {
                juce::Image im = background;
                while (im.getWidth() >= target * 2)
                    im = im.rescaled (im.getWidth() / 2, im.getHeight() / 2, juce::Graphics::highResamplingQuality);
                bgScaled = im; bgScaledFor = target;
            }
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImage (bgScaled, rFrame);
        }

        drawInset (g, rPreset, 7.0f);
        for (int s = 0; s < 2; ++s)
        {
            drawInset (g, rName.translated (s * kCardDx, 0), 8.0f);

            // interieur de la forme d'onde (le cadre clair de la maquette reste visible autour)
            const auto wi = rWaveIn[s];
            g.saveState();
            juce::Path clip; clip.addRoundedRectangle (wi.getX(), wi.getY(), wi.getWidth(), wi.getHeight(), 8.0f, 8.0f,
                                                      s == 0, s == 1, s == 0, s == 1);
            g.reduceClipRegion (clip);
            juce::ColourGradient wg (col::waveTop, 0, wi.getY(), col::waveBot, 0, wi.getBottom(), false);
            g.setGradientFill (wg);
            g.fillRect (wi);
            g.setColour (col::letter);
            g.setFont (fSemi (kLetterSize));
            {
                // lettre de fond (position et proportions mesurees sur la maquette)
                const float lx = wi.getCentreX() + kLetterDx[s];
                juce::Graphics::ScopedSaveState ss (g);
                g.addTransform (juce::AffineTransform::scale (kLetterSx[s], 1.0f, lx, 0.0f));
                g.drawText (s == 0 ? "A" : "B", juce::Rectangle<float> (lx - 300.0f, wi.getY() + kLetterDy, 600.0f, kLetterSize), juce::Justification::centredTop);
            }
            g.setColour (col::grid);
            const auto wr = rWave[s];
            for (float f : { 0.2476f, 0.405f, 0.595f, 0.745f })
                g.fillRect (wi.getX(), wr.getY() + wr.getHeight() * f - 1.0f, wi.getWidth(), 2.0f);
            g.restoreState();

            // graduations du fader (vectorielles) : 12 6 0 6 12 18 36 45 60 inf, de kFaderTop a kFaderBot
            {
                static const char* labels[] = { "12", "6", "0", "6", "12", "18", "36", "45", "60", "" };
                const float sx = s * kStripDx;
                for (int i = 0; i < 10; ++i)
                {
                    const float y = kFaderTop + (kFaderBot - kFaderTop) * (float) i / 9.0f;
                    g.setColour (i == 2 ? juce::Colour (0xff7a7a7a) : juce::Colour (0xff585858));
                    g.fillRect (634.0f + sx, y - 1.0f, i == 2 ? 56.0f : 47.0f, 2.0f);
                    g.fillRect (kTickLabelX + sx - 28.0f, y - 1.0f, 9.0f, 2.0f);
                    g.fillRect (kTickLabelX + sx + 17.0f, y - 1.0f, 6.0f, 2.0f);
                    g.setColour (col::tickTxt);
                    if (i < 9)
                    {
                        g.setFont (fMedium (20.5f));
                        g.drawText (labels[i], juce::Rectangle<float> (kTickLabelX + sx - 20.0f, y - 11.0f, 40.0f, 22.0f), juce::Justification::centred);
                    }
                    else
                    {
                        // infini : deux boucles
                        g.drawEllipse (kTickLabelX + sx - 9.5f, y - 4.5f, 9.5f, 9.0f, 1.9f);
                        g.drawEllipse (kTickLabelX + sx + 0.0f, y - 4.5f, 9.5f, 9.0f, 1.9f);
                    }
                }
            }
        }

        // colonne centrale : etiquettes (vectorielles, mesurees sur la maquette)
        auto label = [&g] (const juce::String& t, float cx, float cy, float size)
        {
            g.setFont (fMedium (size));
            const juce::Rectangle<float> lr (cx - 120, cy - size * 0.8f, 240, size * 1.6f);
            g.setColour (juce::Colours::black.withAlpha (0.65f));
            g.drawText (t, lr.translated (0.0f, 2.0f), juce::Justification::centred);
            g.setColour (juce::Colour (0xffe6e6e6));
            g.drawText (t, lr, juce::Justification::centred);
        };
        // (taille JUCE = hauteur de ligne ~ 1.21 x la taille en points de la maquette)
        label ("MIX", 937.0f, kLabelY[0], 28.0f);
        label ("OVERLOAD", 936.0f, kLabelY[1] - 6.0f, 28.0f);
        label ("VARIPHI", 936.0f, kLabelY[2], 28.0f);
        label ("IR A", 868.0f, 250.0f + kTop, 23.0f);
        label ("IR B", 1005.0f, 250.0f + kTop, 23.0f);
        label ("%", 937.0f, 443.0f + kTop, 24.5f);
        label ("%", 936.5f, 626.0f + kTop, 24.5f);

        // filet vertical entre le power et le logo (redessine : le bandeau du fond a ete agrandi)
        {
            const float cy = rPower.getCentreY(), hh = 21.0f;
            g.setColour (juce::Colours::black.withAlpha (0.85f));
            g.fillRoundedRectangle (115.8f, cy - hh - 1.0f, 5.2f, hh * 2.0f + 2.0f, 1.5f);
            g.setColour (juce::Colour (0xffbdbdbd));
            g.fillRoundedRectangle (117.6f, cy - hh, 1.7f, hh * 2.0f, 0.85f);
        }
        // logo IMPULSE (vectoriel)
        if (title != nullptr)
            title->drawWithin (g, rTitle, juce::RectanglePlacement::centred, 1.0f);

        // logo NINE HANDS | DSP (vectoriel) en bas, a la place du texte entre deux traits
        if (logo != nullptr)
            logo->drawWithin (g, rBrand.withSizeKeepingCentre (rBrand.getWidth() * 0.92f, rBrand.getHeight() * 0.92f), juce::RectanglePlacement::centred, 0.88f);
    }

    // ---------------------------------------------------------------- souris / fichiers
    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto p = e.position;
        if (e.mods.isPopupMenu())
        {
            for (int s = 0; s < 2; ++s)
                if (rName.translated (s * kCardDx, 0).contains (p) || rWave[s].contains (p)) { showMainMenu (this, -1, s); return; }
            showMainMenu (this, -1, -1);
            return;
        }
        if (rPreset.contains (p))
        {
            if (p.x < rPreset.getX() + 30.0f) { proc.stepPreset (p.y < rPreset.getCentreY() ? -1 : +1); return; }
            presetMenu();
            return;
        }
        for (int s = 0; s < 2; ++s)
            if (rName.translated (s * kCardDx, 0).contains (p)) { chooseFile (s); return; }
    }

    bool isInterestedInFileDrag (const juce::StringArray& files) override
    {
        for (auto& f : files) if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac") || juce::File (f).isDirectory()) return true;
        return false;
    }
    void fileDragMove (const juce::StringArray&, int x, int y) override
    {
        const int s = slotAt ({ (float) x, (float) y });
        if (s != dropHover) { dropHover = s; repaint(); }
    }
    void fileDragExit (const juce::StringArray&) override { dropHover = -1; repaint(); }
    void filesDropped (const juce::StringArray& files, int x, int y) override
    {
        int s = slotAt ({ (float) x, (float) y });
        dropHover = -1;
        if (s < 0) s = x < kW / 2 ? 0 : 1;
        juce::Array<juce::File> irs;
        for (auto& f : files)
        {
            const juce::File file (f);
            if (file.isDirectory())
            {
                for (auto& c : file.findChildFiles (juce::File::findFiles, false))
                    if (ImpulseProcessor::isIRFile (c)) irs.add (c);
            }
            else if (ImpulseProcessor::isIRFile (file)) irs.add (file);
        }
        std::sort (irs.begin(), irs.end(), [] (const juce::File& a, const juce::File& b) { return a.getFileName().compareNatural (b.getFileName()) < 0; });
        // deux fichiers deposes ensemble : le premier dans A, le second dans B
        if (irs.size() >= 2 && files.size() >= 2) { proc.loadIR (0, irs[0]); proc.loadIR (1, irs[1]); }
        else if (! irs.isEmpty()) proc.loadIR (s, irs[0]);
        repaint();
    }

    void editorResized() { resizeSettle = 4; }
    void useAccent() const { col::accent = juce::Colour (proc.accentColour.load()); }

private:
    struct SlotUI
    {
        std::unique_ptr<IconBtn> folder, prev, next;
        std::unique_ptr<ToggleBtn> mute, phase;
        std::unique_ptr<Fader> fader;
    };

    int slotAt (juce::Point<float> p) const
    {
        for (int s = 0; s < 2; ++s)
            if (rCard[s].contains (p) || rLower[s].contains (p)) return s;
        return -1;
    }

    void chooseFile (int s)
    {
        chooser = std::make_unique<juce::FileChooser> (juce::String ("Load impulse response ") + (s == 0 ? "A" : "B"),
                                                       proc.startFolder (s), "*.wav;*.aif;*.aiff;*.flac");
        juce::Component::SafePointer<ImpulseContent> sp (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [sp, s] (const juce::FileChooser& fc)
                              {
                                  if (sp == nullptr) return;
                                  const auto f = fc.getResult();
                                  if (f.existsAsFile()) sp->proc.loadIR (s, f);
                              });
    }

public:
    // ---------------------------------------------------------------- menu clic droit (meme presentation que les autres pedales)
    // [MIDI Learn de la commande] [IR du slot] > Plugin size > Presets > Settings > MIDI > version
    void showMainMenu (juce::Component* target, int learnIdx, int slot, bool withPresets = true)
    {
        juce::PopupMenu m;
        const int lt = proc.learnTarget.load();
        if (learnIdx >= 0)
        {
            const juce::String name = ImpulseProcessor::learnDef (learnIdx).name;
            m.addItem (1000 + learnIdx, "MIDI Learn: " + name + " (" + assignText (proc, learnIdx) + ")", true, lt == learnIdx);
            m.addItem (1100 + learnIdx, "Clear MIDI: " + name, proc.ccMap[learnIdx].load() >= 0 || proc.noteMap[learnIdx].load() >= 0);
            if (lt >= 0) m.addItem (1200, "Cancel MIDI learn");
            m.addSeparator();
        }
        juce::String slotPath;
        if (slot >= 0)
        {
            const auto inf = proc.getInfo (slot);
            slotPath = inf.path;
            m.addSectionHeader (juce::String ("IR ") + (slot == 0 ? "A" : "B") + (inf.loaded ? ": " + inf.name : juce::String()));
            m.addItem (300, "Load IR...");
            m.addItem (301, "Previous IR in folder");
            m.addItem (302, "Next IR in folder");
            m.addItem (303, "Show in folder", inf.loaded && juce::File (inf.path).existsAsFile());
            m.addItem (304, "Clear", inf.loaded);
            if (inf.loaded)
                m.addItem (305, "Gain applied: " + juce::String (inf.normDb, 1) + " dB" + (inf.isLong ? "  (long IR / reverb)" : "  (cab IR)"), false);
            m.addSeparator();
        }

        juce::PopupMenu sizes;
        addSizeItems (sizes, *this);
        m.addSubMenu ("Plugin size", sizes);
        m.addSeparator();

        // Presets : dans le clic droit, pas dans la roue crantee (les presets ont leur menu en haut et la disquette)
        juce::StringArray names;
        if (withPresets)
        {
            juce::PopupMenu presets;
            names = addPresetItems (presets);
            m.addSubMenu ("Presets", presets);
            m.addSeparator();
        }

        auto& ap = proc.apvts;
        const bool norm = ap.getRawParameterValue ("normalize")->load() > 0.5f;
        const float low = ap.getRawParameterValue ("lowcut")->load(), high = ap.getRawParameterValue ("highcut")->load();
        juce::PopupMenu settings, lowM, highM, folderM;
        settings.addItem (2, "Colour...");
        {
            const auto def = ImpulseSettings::irFolder (JucePlugin_Name);
            folderM.addSectionHeader (def.isDirectory() ? def.getFullPathName() : juce::String ("No default folder (last folder used)"));
            folderM.addItem (40, "Choose default IR folder...");
            folderM.addItem (41, "Show default IR folder", def.isDirectory());
            folderM.addItem (42, "No default folder (use last folder)", def != juce::File(), def == juce::File());
            settings.addSubMenu ("IR folder", folderM);
        }
        settings.addSeparator();
        settings.addItem (1, "Normalize IRs (level match, TONE3000 law)", true, norm);
        for (int i = 0; i < 7; ++i) lowM.addItem (10 + i, i == 0 ? juce::String ("Off") : juce::String (kLows[i]) + " Hz", true, std::abs (low - (float) kLows[i]) < 1.0f);
        for (int i = 0; i < 7; ++i) highM.addItem (20 + i, i == 0 ? juce::String ("Off") : juce::String (kHighs[i] / 1000.0, 1).replace (".0", "") + " kHz", true, std::abs (high - (float) kHighs[i]) < 10.0f);
        settings.addSubMenu ("Low cut", lowM);
        settings.addSubMenu ("High cut", highM);
        settings.addSeparator();
        settings.addItem (43, "MIDI footswitches latching (127 = on / 0 = off)", true, ! proc.toggleMomentary.load());
        settings.addItem (44, "MIDI footswitches momentary (toggle each press)", true, proc.toggleMomentary.load());
        m.addSubMenu ("Settings", settings);

        juce::PopupMenu midi;
        for (int i = 0; i < ImpulseProcessor::numLearn; ++i)
            midi.addItem (1000 + i, juce::String ("Learn: ") + ImpulseProcessor::learnDef (i).name + " (" + assignText (proc, i) + ")", true, lt == i);
        midi.addSeparator();
        if (lt >= 0) midi.addItem (1200, "Cancel MIDI learn");
        midi.addItem (1201, "Clear all MIDI assignments");
        midi.addSeparator();
        midi.addItem (910, "CC n+32 = fine (14-bit) for knobs on CC 0-31", false);
        midi.addItem (911, "Buttons and IR / preset steps also learn a MIDI note", false);
        m.addSubMenu ("MIDI", midi);
        m.addSeparator();
        m.addItem (999, "Impulse IR loader v" + juce::String (JucePlugin_VersionString) + " - NINE HANDS | DSP", false);

        juce::Component::SafePointer<ImpulseContent> sp (this);
        juce::Component::SafePointer<juce::Component> tp (target);
        m.showMenuAsync (menuOptions (juce::Rectangle<int> (juce::Desktop::getMousePosition(), juce::Desktop::getMousePosition()).withSize (1, 1)),
                         [sp, tp, names, slot, slotPath] (int r)
        {
            if (sp == nullptr || r == 0) return;
            auto& c = *sp;
            auto& p = c.proc;
            auto set = [&p] (const char* id, float v)
            {
                if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
            };
            if (handleLearnChoice (r, &p)) return;
            if (handleSizeChoice (r, &c)) return;
            if (c.handlePresetChoice (r, names)) return;
            if (r == 300) c.chooseFile (slot);
            else if (r == 301) p.stepIR (slot, -1);
            else if (r == 302) p.stepIR (slot, +1);
            else if (r == 303) juce::File (slotPath).revealToUser();
            else if (r == 304) p.clearIR (slot);
            else if (r == 2) { if (auto* ed = c.findParentComponentOfClass<ImpulseEditor>()) ed->openColourPicker (tp.getComponent()); }
            else if (r == 1) set ("normalize", p.apvts.getRawParameterValue ("normalize")->load() > 0.5f ? 0.0f : 1.0f);
            else if (r >= 10 && r < 17) set ("lowcut", (float) kLows[r - 10]);
            else if (r >= 20 && r < 27) set ("highcut", (float) kHighs[r - 20]);
            else if (r == 40) { if (auto* ed = c.findParentComponentOfClass<ImpulseEditor>()) ed->chooseIRFolder(); }
            else if (r == 41) ImpulseSettings::irFolder (JucePlugin_Name).revealToUser();
            else if (r == 42) ImpulseSettings::setIrFolder (JucePlugin_Name, juce::File());
            else if (r == 43) p.toggleMomentary = false;
            else if (r == 44) p.toggleMomentary = true;
            c.repaint();
        });
    }

private:
    static constexpr int kLows[7]  = { 20, 40, 60, 80, 100, 120, 150 };
    static constexpr int kHighs[7] = { 20000, 12000, 10000, 8000, 7000, 6000, 5000 };

    // ---------------------------------------------------------------- presets (menu du haut, disquette, menu clic droit)
    juce::StringArray addPresetItems (juce::PopupMenu& m)
    {
        const auto names = proc.getPresetNames();
        const auto cur = proc.getPresetName();
        m.addItem (500, "Init (empty)");
        m.addSeparator();
        if (names.isEmpty()) m.addItem (590, "No preset saved yet", false);
        for (int i = 0; i < names.size() && i < 400; ++i) m.addItem (600 + i, names[i], true, names[i] == cur);
        m.addSeparator();
        addPresetFileItems (m);
        return names;
    }
    void addPresetFileItems (juce::PopupMenu& m)
    {
        const auto cur = proc.getPresetName();
        m.addItem (501, cur.isNotEmpty() ? "Save \"" + cur + "\"" : juce::String ("Save preset"), cur.isNotEmpty());
        m.addItem (502, "Save preset as...");
        m.addItem (503, cur.isNotEmpty() ? "Delete \"" + cur + "\"" : juce::String ("Delete preset"), cur.isNotEmpty());
        m.addSeparator();
        m.addItem (504, "Export preset to file...");
        m.addItem (505, "Import preset from file...");
        m.addItem (506, "Open presets folder");
    }
    bool handlePresetChoice (int r, const juce::StringArray& names)
    {
        const auto cur = proc.getPresetName();
        if (r == 500) proc.initPreset();
        else if (r == 501) { if (proc.savePreset (cur)) showStatus ("Preset saved: " + cur); }
        else if (r == 502) askPresetName();
        else if (r == 503) proc.deletePreset (cur);
        else if (r == 504) exportPreset();
        else if (r == 505) importPreset();
        else if (r == 506) { ImpulseProcessor::presetFolder().createDirectory(); ImpulseProcessor::presetFolder().startAsProcess(); }
        else if (r >= 600 && r - 600 < names.size()) proc.loadPreset (names[r - 600]);
        else return false;
        repaint();
        return true;
    }

    void presetMenu()
    {
        juce::PopupMenu m;
        const auto names = addPresetItems (m);
        juce::Component::SafePointer<ImpulseContent> sp (this);
        m.showMenuAsync (menuOptions (localAreaToGlobal (toInt (rPreset))),
                         [sp, names] (int r) { if (sp != nullptr) sp->handlePresetChoice (r, names); });
    }

    // menus a la taille normale, comme sur toutes les pedales : l'echelle vient de la fenetre du plugin
    // (pas du contenu reduit par setTransform), la position reste celle du clic / du bouton
    juce::PopupMenu::Options menuOptions (juce::Rectangle<int> screenArea)
    {
        juce::Component* ed = getParentComponent();
        return juce::PopupMenu::Options().withTargetComponent (ed != nullptr ? ed : this).withTargetScreenArea (screenArea);
    }

    // disquette : enregistrer / exporter / importer
    void presetFileMenu()
    {
        juce::PopupMenu m;
        addPresetFileItems (m);
        juce::Component::SafePointer<ImpulseContent> sp (this);
        m.showMenuAsync (menuOptions (save.getScreenBounds()),
                         [sp] (int r) { if (sp != nullptr) sp->handlePresetChoice (r, {}); });
    }

    void exportPreset()
    {
        const auto name = proc.getPresetName().isNotEmpty() ? proc.getPresetName() : juce::String ("Impulse preset");
        chooser = std::make_unique<juce::FileChooser> ("Export Impulse preset",
                      juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile (juce::File::createLegalFileName (name) + ".impulse"),
                      "*.impulse");
        juce::Component::SafePointer<ImpulseContent> sp (this);
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                | juce::FileBrowserComponent::warnAboutOverwriting,
                              [sp] (const juce::FileChooser& fc)
                              {
                                  if (sp == nullptr) return;
                                  auto f = fc.getResult();
                                  if (f == juce::File()) return;
                                  if (! f.hasFileExtension ("impulse")) f = f.withFileExtension ("impulse");
                                  sp->showStatus (sp->proc.exportPreset (f) ? "Preset exported: " + f.getFileName() : juce::String ("Export failed"));
                              });
    }

    void importPreset()
    {
        chooser = std::make_unique<juce::FileChooser> ("Import Impulse preset",
                      juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.impulse");
        juce::Component::SafePointer<ImpulseContent> sp (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [sp] (const juce::FileChooser& fc)
                              {
                                  if (sp == nullptr) return;
                                  const auto f = fc.getResult();
                                  if (! f.existsAsFile()) return;
                                  sp->showStatus (sp->proc.importPreset (f) ? "Preset imported: " + sp->proc.getPresetName() : juce::String ("Not an Impulse preset"));
                                  sp->repaint();
                              });
    }

    void askPresetName()
    {
        auto* aw = new juce::AlertWindow ("Save preset", "Preset name (IRs are saved inside the preset):", juce::MessageBoxIconType::NoIcon);
        aw->addTextEditor ("name", proc.getPresetName().isNotEmpty() ? proc.getPresetName() : juce::String ("My preset"));
        aw->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        juce::Component::SafePointer<ImpulseContent> sp (this);
        aw->enterModalState (true, juce::ModalCallbackFunction::create ([sp, aw] (int r)
        {
            if (r == 1 && sp != nullptr)
            {
                sp->proc.savePreset (aw->getTextEditorContents ("name"));
                sp->showStatus ("Preset saved: " + sp->proc.getPresetName());
            }
        }), true);
    }

    // ---------------------------------------------------------------- etat / timer
public:
    void showStatus (const juce::String& s) { statusMsg = s; statusUntil = juce::Time::getMillisecondCounter() + 1500; askRepaint (toInt (rStatus)); }
private:
    juce::String statusText() const
    {
        const int lt = proc.learnTarget.load();
        if (lt >= 0) return juce::String ("MIDI LEARN: move a control for ") + ImpulseProcessor::learnDef (lt).name;
        const auto now = juce::Time::getMillisecondCounter();
        if (learnMsg.isNotEmpty() && now < learnMsgUntil) return learnMsg;
        if (statusMsg.isNotEmpty() && now < statusUntil) return statusMsg;
        return {};
    }

    void refreshInfos()
    {
        for (int s = 0; s < 2; ++s)
        {
            const auto old = info[s].error;
            info[s] = proc.getInfo (s);
            if (info[s].error.isNotEmpty() && info[s].error != old) errUntil[s] = juce::Time::getMillisecondCounter() + 4000;
        }
        drawnInfoVersion = proc.getInfoVersion();
    }

    void askRepaint (juce::Rectangle<int> r)
    {
        repaint (r);
        if (repaintAskedMs == 0) repaintAskedMs = juce::Time::getMillisecondCounter();
    }

    void updateBackgroundCache (ImpulseEditor& ed, juce::uint32 now)
    {
        if (now - lastScaleCheck >= 250)
        {
            lastScaleCheck = now;
            const float sc = juce::Component::getApproximateScaleFactorForComponent (&ed);
            if (std::abs (sc - displayScale) > 0.001f) { displayScale = sc; resizeSettle = juce::jmax (resizeSettle, 1); }
        }
        if (resizeSettle > 0 && --resizeSettle == 0)
        {
            const float px = (float) ed.getWidth() / kVisW * displayScale;
            const int tw = juce::jlimit (64, kW * 4, (int) std::lround (kW * px));
            const int th = juce::jlimit (32, kH * 4, (int) std::lround (kH * px));
            if (! bgCache.isValid() || bgCache.getWidth() != tw || bgCache.getHeight() != th)
            {
                juce::Image img (juce::Image::RGB, tw, th, true);
                {
                    juce::Graphics g (img);
                    g.addTransform (juce::AffineTransform::scale ((float) tw / (float) kW, (float) th / (float) kH));
                    drawStatic (g);
                }
                bgCache = img;
                askRepaint (getLocalBounds());
            }
        }
    }

    void timerCallback() override
    {
        const auto now = juce::Time::getMillisecondCounter();
        if (auto* ed = findParentComponentOfClass<ImpulseEditor>())
        {
            ed->checkRenderer();
            updateBackgroundCache (*ed, now);
            if (repaintAskedMs != 0 && now - repaintAskedMs > 1500 && isShowing())
            {
                repaintAskedMs = 0;
                ed->resetRenderer();
                repaint();
            }
        }

        if (proc.getInfoVersion() != drawnInfoVersion)
        {
            refreshInfos();
            for (int s = 0; s < 2; ++s) { askRepaint (toInt (rCard[s])); askRepaint (toInt (rWave[s])); }
            askRepaint (toInt (rPreset));
        }
        for (int s = 0; s < 2; ++s)
            if (errUntil[s] != 0 && now > errUntil[s]) { errUntil[s] = 0; askRepaint (toInt (rName.translated (s * kCardDx, 0))); }

        // nom du preset (modifie / charge)
        const auto pv = proc.getPresetVersion();
        const auto pn = proc.getPresetName() + (proc.isPresetDirty() ? "*" : "");
        if (pv != drawnPresetVersion || pn != drawnPresetName) { drawnPresetVersion = pv; drawnPresetName = pn; askRepaint (toInt (rPreset)); }

        // MIDI Learn : confirmation 2 s, clignotement de la commande en attente
        const int learned = proc.lastLearned.exchange (-1);
        if (learned >= 0)
        {
            learnMsg = juce::String (ImpulseProcessor::learnDef (learned).name) + " = " + proc.learnAssignText (learned);
            learnMsgUntil = now + 2000;
        }
        const int lt = proc.learnTarget.load();
        const bool blink = lt >= 0 && (now / 250) % 2 == 0;
        if (lt != drawnLearn || blink != drawnBlink)
        {
            for (auto* c : getChildren()) c->repaint();
            drawnLearn = lt; drawnBlink = blink;
        }

        const auto st = statusText();
        if (st != drawnStatus) { drawnStatus = st; askRepaint (toInt (rStatus.expanded (4))); }
    }

    ImpulseProcessor& proc;
    ToggleBtn power;
    Knob mix, over, vari;
    IconBtn save, gear;
    SlotUI slot[2];
    std::unique_ptr<juce::FileChooser> chooser;
    IRInfo info[2];
    juce::uint32 errUntil[2] {};
    juce::uint32 drawnInfoVersion = 0, drawnPresetVersion = 0;
    juce::String drawnPresetName, drawnStatus, statusMsg, learnMsg;
    juce::uint32 statusUntil = 0, learnMsgUntil = 0;
    int drawnLearn = -1, dropHover = -1;
    bool drawnBlink = false;
    // logos vectoriels (SVG fournis) : nets a toutes les tailles
    std::unique_ptr<juce::Drawable> logo  = juce::Drawable::createFromImageData (BinaryData::nine_hands_logo_blanc_svg, BinaryData::nine_hands_logo_blanc_svgSize);
    std::unique_ptr<juce::Drawable> title = juce::Drawable::createFromImageData (BinaryData::impulse_logo_blanc_svg, BinaryData::impulse_logo_blanc_svgSize);
    juce::Image background = juce::ImageCache::getFromMemory (BinaryData::background_jpg, BinaryData::background_jpgSize);   // maquette (metal brosse)

    // anti-gel v2
    juce::Image bgCache;
    juce::Image bgScaled; int bgScaledFor = 0;
    int resizeSettle = 2;
    float displayScale = 1.0f;
    juce::uint32 lastScaleCheck = 0, lastPaintMs = 0, repaintAskedMs = 0;
};

// ============================================================================ editeur
ImpulseEditor::ImpulseEditor (ImpulseProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setOpaque (true);
    content = std::make_unique<ImpulseContent> (proc);
    addAndMakeVisible (*content);
    setResizable (true, true);
    // jamais plus petit que la taille par defaut (« 60 % »)
    setResizeLimits (kWin, kWinH, (int) std::lround (kWin * 2.2f), (int) std::lround (kWinH * 2.2f));
    getConstrainer()->setFixedAspectRatio ((double) kVisW / (double) kVisH);
    const int saved = juce::jlimit (-1, 2, proc.uiScaleIdx.load());
    if (saved >= 0) setScaleIndex (saved);
    else            applyScaleIndex (WigaWindowSize::defaultIndex (JucePlugin_Name, numSizes));
}

ImpulseEditor::~ImpulseEditor() = default;

// Anti-gel (Windows) : moteur de rendu logiciel au lieu de Direct2D
void ImpulseEditor::useSoftwareRenderer()
{
    auto* peer = getPeer();
    if (peer == nullptr) return;
   #if JUCE_WINDOWS
    int sw = peer->getAvailableRenderingEngines().indexOf ("Software Renderer");
    if (sw < 0) sw = 0;
    if (peer->getCurrentRenderingEngine() != sw)
        peer->setCurrentRenderingEngine (sw);
   #endif
    if ((void*) peer != lastPeer) { lastPeer = (void*) peer; repaint(); }
    lastRendererCheck = juce::Time::getMillisecondCounter();
}

void ImpulseEditor::checkRenderer()
{
    if ((void*) getPeer() != lastPeer || juce::Time::getMillisecondCounter() - lastRendererCheck >= 1000)
        useSoftwareRenderer();
}

void ImpulseEditor::resetRenderer()
{
    lastPeer = nullptr;
    useSoftwareRenderer();
    repaint();
}

void ImpulseEditor::paint (juce::Graphics& g) { g.fillAll (col::outer); }

// dossier des IR par defaut : le bouton dossier s'ouvre la quand le slot est vide,
// et les fleches d'un slot vide parcourent ce dossier
void ImpulseEditor::chooseIRFolder()
{
    const auto cur = ImpulseSettings::irFolder (JucePlugin_Name);
    folderChooser = std::make_unique<juce::FileChooser> ("Default IR folder", cur.isDirectory() ? cur : proc.lastFolder);
    juce::Component::SafePointer<ImpulseEditor> sp (this);
    folderChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                                [sp] (const juce::FileChooser& fc)
                                {
                                    const auto f = fc.getResult();
                                    if (f.isDirectory())
                                    {
                                        ImpulseSettings::setIrFolder (JucePlugin_Name, f);
                                        if (sp != nullptr) sp->content->showStatus ("IR folder: " + f.getFileName());
                                    }
                                });
}

// nuancier d'Octopus, ouvert dans la fenetre du plugin pres de 'near' (pas de fenetre flottante que l'hote pourrait cacher)
void ImpulseEditor::openColourPicker (juce::Component* near)
{
    const float s = (float) getWidth() / (float) kWin;
    auto picker = std::make_unique<WigaColourPicker> ("Impulse colour", juce::Colour (proc.accentColour.load()),
                                                      juce::jlimit (0.85f, 1.6f, s), JucePlugin_Name);
    juce::Component::SafePointer<ImpulseEditor> sp (this);
    auto apply = [sp] (juce::Colour c)
    {
        if (sp == nullptr) return;
        sp->proc.accentColour = c.getARGB();                          // en direct, sans acces disque
        sp->content->repaint();
    };
    picker->onChange = apply;
    picker->onCommit = [] (juce::Colour c) { ImpulseSettings::setAccent (JucePlugin_Name, c.getARGB()); };   // couleur des nouvelles fenetres
    picker->onDefault = [apply] { apply (juce::Colour (ImpulseSettings::kDefaultAccent)); };
    auto area = near != nullptr ? getLocalArea (near, near->getLocalBounds()) : getLocalBounds().withSizeKeepingCentre (10, 10);
    auto* layer = new WigaPickerLayer (std::move (picker), area, getLocalBounds());   // se detruit en se fermant
    addAndMakeVisible (layer);
    layer->toFront (true);
}
void ImpulseEditor::parentHierarchyChanged() { useSoftwareRenderer(); }
void ImpulseEditor::visibilityChanged() { useSoftwareRenderer(); if (isShowing()) repaint(); }

void ImpulseEditor::resized()
{
    const float s = (float) getWidth() / kVisW;
    content->setTransform (juce::AffineTransform::translation (-kVisX, -kVisY).scaled (s));
    content->setBounds (0, 0, kW, kH);
    content->editorResized();            // jamais de setSize ici (boucle avec l'hote a 125 / 150 %)
}

int ImpulseEditor::getScaleIndex() const
{
    const float s = (float) getWidth() / (float) kWin;
    for (int i = 0; i < numSizes; ++i) if (std::abs (s - kScales[i]) < 0.02f) return i;
    return -1;
}

void ImpulseEditor::setScaleIndex (int idx)
{
    idx = juce::jlimit (-1, numSizes - 1, idx);
    proc.uiScaleIdx = idx;
    applyScaleIndex (idx < 0 ? WigaWindowSize::defaultIndex (JucePlugin_Name, numSizes) : idx);
}

void ImpulseEditor::applyScaleIndex (int idx)
{
    idx = juce::jlimit (0, numSizes - 1, idx);
    const int w = (int) std::lround (kWin * kScales[idx]);
    setSize (w, (int) std::lround ((double) w * kVisH / kVisW));
}

// ============================================================================ menus clic droit
static ImpulseProcessor* procFor (juce::Component* c)
{
    if (c == nullptr) return nullptr;
    if (auto* ed = dynamic_cast<ImpulseEditor*> (c)) return &ed->getProc();
    if (auto* ed = c->findParentComponentOfClass<ImpulseEditor>()) return &ed->getProc();
    return nullptr;
}

// Plugin size : ids 101..103 = tailles de cette fenetre, 120 + i = taille des nouvelles fenetres
static void addSizeItems (juce::PopupMenu& m, juce::Component& c)
{
    auto* ed = c.findParentComponentOfClass<ImpulseEditor>();
    if (ed == nullptr) ed = dynamic_cast<ImpulseEditor*> (&c);
    const int cur = ed != nullptr ? ed->getScaleIndex() : -1;
    const int def = WigaWindowSize::defaultIndex (JucePlugin_Name, ImpulseEditor::numSizes);
    const char* names[ImpulseEditor::numSizes] = { "60 %", "75 %", "100 %" };
    juce::PopupMenu defSizes;
    for (int i = 0; i < ImpulseEditor::numSizes; ++i)
    {
        m.addItem (101 + i, juce::String (names[i]) + (i == def ? "  (default)" : ""), true, cur == i);
        defSizes.addItem (120 + i, names[i], true, i == def);
    }
    m.addSeparator();
    m.addSubMenu ("Default size (new windows)", defSizes);
}

static bool handleSizeChoice (int r, juce::Component* c)
{
    if (c == nullptr) return false;
    auto* ed = c->findParentComponentOfClass<ImpulseEditor>();
    if (ed == nullptr) return false;
    if (r >= 101 && r <= 103) { ed->setScaleIndex (r - 101); return true; }
    if (r >= 120 && r < 120 + ImpulseEditor::numSizes)
    {
        WigaWindowSize::setDefaultIndex (JucePlugin_Name, r - 120);
        ed->setScaleIndex (-1);
        return true;
    }
    return false;
}

static juce::String assignText (ImpulseProcessor& p, int i)
{
    const auto t = p.learnAssignText (i);
    return t == "none" ? juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x94")) : t;
}

static bool handleLearnChoice (int r, ImpulseProcessor* p)
{
    if (r < 1000 || r > 1299 || p == nullptr) return false;
    if (r < 1000 + ImpulseProcessor::numLearn) p->learnTarget.store (p->learnTarget.load() == r - 1000 ? -1 : r - 1000);
    else if (r >= 1100 && r < 1100 + ImpulseProcessor::numLearn) p->clearLearn (r - 1100);
    else if (r == 1200) p->learnTarget.store (-1);
    else if (r == 1201) p->resetMidiMap();
    return true;
}

// clic droit sur une commande (ou ailleurs) et roue dentee : le meme menu partout
static void showContextMenu (juce::Component& c, int learnIdx)
{
    auto* content = dynamic_cast<ImpulseContent*> (&c);
    if (content == nullptr) content = c.findParentComponentOfClass<ImpulseContent>();
    if (content != nullptr) content->showMainMenu (&c, learnIdx, -1);
}

static void showGearMenu (juce::Component& c)
{
    auto* content = c.findParentComponentOfClass<ImpulseContent>();
    if (content != nullptr) content->showMainMenu (&c, -1, -1, false);
}
