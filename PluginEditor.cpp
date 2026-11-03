#include "PluginEditor.h"
#include <BinaryData.h>
#include "WigaWindowSize.h"
#include "ColourPicker.h"

// ============================================================================
// IMPULSE - interface (format rack large, d'apres la maquette fournie)
// Tout est dessine en vectoriel dans un repere de 887 x 338 points (taille « 60 % »),
// puis mis a l'echelle : net a toutes les tailles et a tous les zooms Windows / Retina.
// ============================================================================
namespace
{
    constexpr int kW = 887, kH = 338;   // symetrique autour de x = 443.5 (colonne centrale)
    const float kScales[3] = { 1.0f, 1.25f, 5.0f / 3.0f };   // 60 % (defaut) / 75 % / 100 %

    namespace col
    {
        const juce::Colour outer   (0xff161616), header (0xff232323), card (0xff3b3b3a), cardLo (0xff353534),
                           nameBox (0xff131313), btnHi (0xff414141), btnLo (0xff363636), btnEdge (0xff4a4a4a),
                           center  (0xff272727), strip (0xff2f2f2f), frame (0xff1c1c1c), waveTop (0xff0f0f0f),
                           waveBot (0xff1b1b1b), grid (0xff262626), label (0xff8a8a8a), text (0xffd6d6d6),
                           dim     (0xff6c6c6c), knobTrack (0xff141414), thumb (0xffababab),
                           letter  (0xff1d1d1d), err (0xffe0705a);
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
        auto f = fSemi (h);
        f.setFeatureSetting ({ juce::FontFeatureTag ("calt"), juce::FontFeatureSetting::featureDisabled });
        return f;
    }

    // ---- geometrie (points du repere 887 x 338)
    const juce::Rectangle<float> rHeader (10, 5, 867, 39);
    const juce::Rectangle<float> rCenter (379.5f, 44, 128, 289);
    const juce::Rectangle<float> rCard[2]  = { { 8, 44, 384, 76 }, { 495, 44, 384, 76 } };
    const juce::Rectangle<float> rLower[2] = { { 20, 120, 360, 210 }, { 507, 120, 360, 210 } };
    const juce::Rectangle<float> rWave[2]  = { { 24, 120, 266, 206 }, { 597, 120, 266, 206 } };
    const juce::Rectangle<float> rStrip[2] = { { 290, 120, 86, 206 }, { 511, 120, 86, 206 } };
    constexpr float kCardDx = 487.0f, kStripDx = 218.0f;
    const juce::Rectangle<float> rName (21, 56, 211, 51), rFolder (239, 57, 51, 50), rPrev (296, 57, 40, 50), rNext (341, 57, 40, 50);
    const juce::Rectangle<float> rMute (303, 134, 26, 22), rPhase (341, 134, 26, 22);
    constexpr float kFaderX = 334.5f, kFaderTop = 170.0f, kFaderBot = 305.5f, kTickLabelX = 354.0f;
    constexpr float kKnobX = 443.5f, kKnobR = 24.0f;
    const float kKnobY[3] = { 94.25f, 184.75f, 274.25f }, kLabelY[3] = { 56.5f, 146.5f, 237.5f };
    const char* kFaderTicks[10] = { "12", "6", "0", "6", "12", "18", "36", "45", "60", "" };
    const juce::Rectangle<float> rPreset (568, 10, 226, 30), rSave (802, 10, 30, 30), rGear (840, 10, 30, 30);
    const juce::Rectangle<float> rStatus (300, 13, 250, 22);

    juce::Rectangle<int> toInt (juce::Rectangle<float> r) { return r.getSmallestIntegerContainer(); }

    void drawButtonBg (juce::Graphics& g, juce::Rectangle<float> r, bool pressed, bool hover)
    {
        const float hb = hover && ! pressed ? 0.08f : 0.0f;
        juce::ColourGradient gr ((pressed ? col::btnLo : col::btnHi).brighter (hb), r.getX(), r.getY(),
                                 (pressed ? col::btnHi : col::btnLo).brighter (hb), r.getX(), r.getBottom(), false);
        g.setGradientFill (gr);
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawLine (r.getX() + 3, r.getY() + 0.5f, r.getRight() - 3, r.getY() + 0.5f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
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
        const auto c = getLocalBounds().toFloat().getCentre();
        const float a0 = juce::degreesToRadians (-132.0f), a1 = juce::degreesToRadians (132.0f);
        const float n = norm(), ang = a0 + n * (a1 - a0);
        const float r = kKnobR;
        // rail sombre, creuse
        juce::Path track; track.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a1, true);
        g.setColour (col::knobTrack);
        g.strokePath (track, juce::PathStrokeType (5.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white.withAlpha (0.035f));
        juce::Path rim; rim.addCentredArc (c.x, c.y, r + 3.2f, r + 3.2f, 0.0f, a0, a1, true);
        g.strokePath (rim, juce::PathStrokeType (0.8f));
        // valeur (accent) : depuis le debut, ou depuis le centre pour MIX
        const float from = bipolar ? 0.0f : a0;
        if (std::abs (ang - from) > 0.01f)
        {
            juce::Path v; v.addCentredArc (c.x, c.y, r, r, 0.0f, juce::jmin (from, ang), juce::jmax (from, ang), true);
            g.setColour (col::accent);
            g.strokePath (v, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        // repere : point blanc sur le rail
        const juce::Point<float> dot (c.x + r * std::sin (ang), c.y - r * std::cos (ang));
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (dot.x - 2.6f, dot.y - 2.1f, 5.2f, 5.2f);
        g.setColour (juce::Colours::white);
        g.fillEllipse (dot.x - 2.2f, dot.y - 2.2f, 4.4f, 4.4f);
        // valeur au centre
        g.setColour (juce::Colours::white);
        // MIX : valeur + cote dominant (« 50A », « 30B », « 0 » au centre) ; autres potards : valeur seule
        const int v = (int) std::lround (value());
        const juce::String txt = bipolar && v != 0 ? juce::String (std::abs (v)) + (v < 0 ? "A" : "B") : juce::String (v);
        g.setFont (fBold (txt.length() >= 4 ? 17.5f : 21.0f));
        g.drawText (txt, juce::Rectangle<float> (c.x - 26, c.y - 13, 52, 26), juce::Justification::centred);
        if (LearnBlink::shown (proc, learnIdx)) LearnBlink::draw (g, getLocalBounds().toFloat(), 6.0f);
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
        const float sens = e.mods.isShiftDown() ? 900.0f : 180.0f;
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
        const float yT = kFaderTop - top - 3.0f, yB = kFaderBot - top + 3.0f;
        g.setColour (juce::Colour (0xff1a1a1a));
        g.fillRoundedRectangle (cx - 3.5f, yT, 7.0f, yB - yT, 3.5f);
        const float y = yForNorm (norm());
        g.setColour (col::accent);
        g.fillRoundedRectangle (cx - 2.0f, y, 4.0f, yB - 1.5f - y, 2.0f);
        // bouton du fader : disque gris satine + ombre
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (cx - 8.0f, y - 6.6f, 16.0f, 16.0f);
        juce::ColourGradient gr (juce::Colour (0xffc4c4c4), cx - 4, y - 7, juce::Colour (0xff959595), cx + 4, y + 7, false);
        g.setGradientFill (gr);
        g.fillEllipse (cx - 7.5f, y - 7.5f, 15.0f, 15.0f);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawEllipse (cx - 7.0f, y - 7.0f, 14.0f, 14.0f, 0.6f);
        if (LearnBlink::shown (proc, learnIdx)) LearnBlink::draw (g, getLocalBounds().toFloat(), 4.0f);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { showContextMenu (*this, learnIdx); return; }
        dragging = true; startNorm = norm();
        attachment.beginGesture();
        // clic hors du bouton : le fader saute a la position cliquee
        if (std::abs ((float) e.y - yForNorm (startNorm)) > 9.0f)
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
            juce::Path p; p.addCentredArc (c.x, c.y + 0.5f, 7.0f, 7.0f, 0.0f, juce::degreesToRadians (35.0f), juce::degreesToRadians (325.0f), true);
            g.setColour (ic);
            g.strokePath (p, juce::PathStrokeType (1.9f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.drawLine (c.x, c.y - 8.0f, c.x, c.y - 0.5f, 1.9f);
        }
        else
        {
            if (on) { g.setColour (col::accent); g.fillRoundedRectangle (r, 3.0f); }
            else drawButtonBg (g, r, false, isMouseOver());
            const juce::Colour ic = on ? juce::Colour (0xff151515) : col::label;
            g.setColour (ic);
            if (kind == mute)
            {
                g.setFont (fSemi (15.0f));
                g.drawText ("M", r, juce::Justification::centred);
            }
            else
            {
                const auto c = r.getCentre();
                g.drawEllipse (c.x - 6.0f, c.y - 6.0f, 12.0f, 12.0f, 1.8f);
                g.drawLine (c.x - 6.5f, c.y + 7.0f, c.x + 6.5f, c.y - 7.0f, 1.8f);
            }
        }
        if (LearnBlink::shown (proc, learnIdx)) LearnBlink::draw (g, r, 3.0f);
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
        const auto r = getLocalBounds().toFloat();
        drawButtonBg (g, r, pressed, isMouseOver());
        const auto c = r.getCentre() + juce::Point<float> (0.0f, pressed ? 0.5f : 0.0f);
        juce::Path p;
        if (kind == folder)
        {
            p.startNewSubPath (c.x - 11, c.y - 8);
            p.lineTo (c.x - 4, c.y - 8); p.lineTo (c.x - 2, c.y - 5.5f); p.lineTo (c.x + 10, c.y - 5.5f);
            p.lineTo (c.x + 10, c.y + 8); p.lineTo (c.x - 11, c.y + 8); p.closeSubPath();
            g.setColour (col::accent.darker (0.25f));
            g.fillPath (p);
            juce::Path front;
            front.startNewSubPath (c.x - 9, c.y - 2.5f); front.lineTo (c.x + 13, c.y - 2.5f);
            front.lineTo (c.x + 9.5f, c.y + 8); front.lineTo (c.x - 12, c.y + 8); front.closeSubPath();
            g.setColour (col::accent);
            g.fillPath (front);
            return;
        }
        if (kind == prev || kind == next)
        {
            const float s = kind == next ? 1.0f : -1.0f;
            p.addTriangle (c.x - 6.5f * s, c.y - 11.0f, c.x - 6.5f * s, c.y + 11.0f, c.x + 7.5f * s, c.y);
            g.setColour (col::accent);
            g.fillPath (p, juce::AffineTransform());
            g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            if (proc != nullptr && LearnBlink::shown (*proc, learnIdx)) LearnBlink::draw (g, r, 3.0f);
            return;
        }
        g.setColour (juce::Colour (0xffc9c9c9));
        if (kind == save)
        {
            // disquette + « + »
            juce::Path d;
            d.startNewSubPath (c.x - 7, c.y - 7); d.lineTo (c.x + 4, c.y - 7); d.lineTo (c.x + 7, c.y - 4);
            d.lineTo (c.x + 7, c.y + 1); d.startNewSubPath (c.x + 1, c.y + 7); d.lineTo (c.x - 7, c.y + 7); d.closeSubPath();
            juce::Path body; body.addRoundedRectangle (c.x - 7.5f, c.y - 7.5f, 13.5f, 15.0f, 1.5f);
            g.strokePath (body, juce::PathStrokeType (1.6f));
            g.fillRect (c.x - 4.0f, c.y - 7.5f, 6.0f, 4.5f);
            g.fillRoundedRectangle (c.x - 4.5f, c.y + 1.5f, 7.0f, 4.5f, 1.0f);
            g.setColour (col::header);
            g.fillEllipse (c.x + 1.0f, c.y + 1.0f, 10.0f, 10.0f);
            g.setColour (juce::Colour (0xffc9c9c9));
            g.drawLine (c.x + 6.0f, c.y + 2.5f, c.x + 6.0f, c.y + 9.5f, 1.7f);
            g.drawLine (c.x + 2.5f, c.y + 6.0f, c.x + 9.5f, c.y + 6.0f, 1.7f);
            return;
        }
        // roue dentee
        juce::Path gp;
        const int teeth = 8;
        for (int i = 0; i < teeth * 2; ++i)
        {
            const float a0 = juce::MathConstants<float>::twoPi * (float) i / (teeth * 2) - 0.12f;
            const float a1 = juce::MathConstants<float>::twoPi * (float) (i + 1) / (teeth * 2) - 0.12f;
            const float rr = (i % 2 == 0) ? 8.6f : 6.4f;
            if (i == 0) gp.startNewSubPath (c.x + rr * std::sin (a0), c.y - rr * std::cos (a0));
            else gp.lineTo (c.x + rr * std::sin (a0), c.y - rr * std::cos (a0));
            gp.lineTo (c.x + rr * std::sin (a1), c.y - rr * std::cos (a1));
        }
        gp.closeSubPath();
        gp.addEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        gp.setUsingNonZeroWinding (false);
        g.fillPath (gp);
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
        addAndMakeVisible (power); power.setBounds (toInt ({ 18, 12, 24, 24 }));
        addAndMakeVisible (save);  save.setBounds (toInt (rSave));
        addAndMakeVisible (gear);  gear.setBounds (toInt (rGear));
        save.onClick = [this] { presetFileMenu(); };
        gear.onClick = [this] { showGearMenu (gear); };

        Knob* knobs[3] = { &mix, &over, &vari };
        const char* names[3] = { "MIX", "OVERLOAD", "VARIPHI" };
        for (int i = 0; i < 3; ++i)
        {
            auto* k = knobs[i];
            addAndMakeVisible (*k);
            k->setBounds (toInt ({ kKnobX - 34.0f, kKnobY[i] - 34.0f, 68.0f, 68.0f }));
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
            sl.folder->setBounds (toInt (rFolder.translated (dx, 0)));
            sl.prev->setBounds (toInt (rPrev.translated (dx, 0)));
            sl.next->setBounds (toInt (rNext.translated (dx, 0)));
            sl.folder->onClick = [this, s] { chooseFile (s); };
            sl.prev->onClick = [this, s] { proc.stepIR (s, -1); };
            sl.next->onClick = [this, s] { proc.stepIR (s, +1); };

            sl.mute  = std::make_unique<ToggleBtn> (proc, s == 0 ? "muteA" : "muteB", s == 0 ? ImpulseProcessor::lMuteA : ImpulseProcessor::lMuteB, ToggleBtn::mute);
            sl.phase = std::make_unique<ToggleBtn> (proc, s == 0 ? "phaseA" : "phaseB", s == 0 ? ImpulseProcessor::lPhaseA : ImpulseProcessor::lPhaseB, ToggleBtn::phase);
            addAndMakeVisible (*sl.mute);  sl.mute->setBounds (toInt (rMute.translated (sx, 0)));
            addAndMakeVisible (*sl.phase); sl.phase->setBounds (toInt (rPhase.translated (sx, 0)));
            sl.mute->onValue  = [this, s] { askRepaint (toInt (rWave[s])); };
            sl.phase->onValue = [this, s] { showStatus (juce::String ("PHASE ") + (s == 0 ? "A " : "B ") + (slot[s].phase->norm() > 0.5f ? "inverted" : "normal")); };

            sl.fader = std::make_unique<Fader> (proc, s == 0 ? "gainA" : "gainB", s == 0 ? ImpulseProcessor::lGainA : ImpulseProcessor::lGainB);
            sl.fader->top = kFaderTop - 12.0f;
            addAndMakeVisible (*sl.fader);
            sl.fader->setBounds (toInt ({ kFaderX + sx - 10.0f, kFaderTop - 12.0f, 20.0f, kFaderBot - kFaderTop + 24.0f }));
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
            g.setColour (proc.learnTarget.load() >= 0 ? col::accent : juce::Colour (0xffcfcfcf));
            g.setFont (fSemi (12.0f));
            g.drawText (st, rStatus, juce::Justification::centred);
        }

        // preset
        {
            const auto r = rPreset;
            const float ax = r.getX() + 14.0f, cy = r.getCentreY();
            juce::Path up, dn;
            up.addTriangle (ax - 5.5f, cy - 1.0f, ax + 5.5f, cy - 1.0f, ax, cy - 7.5f);
            dn.addTriangle (ax + 7.0f - 5.5f, cy + 1.0f, ax + 7.0f + 5.5f, cy + 1.0f, ax + 7.0f, cy + 7.5f);
            dn.applyTransform (juce::AffineTransform::translation (-7.0f + 11.0f, -8.0f + 6.0f));
            up.applyTransform (juce::AffineTransform::translation (0.0f, 4.0f));
            g.setColour (col::accent);
            g.fillPath (up); g.fillPath (dn);
            g.setColour (col::text);
            g.setFont (fMedium (13.0f));
            juce::String name = proc.getPresetName();
            name = name.isEmpty() ? juce::String ("(Custom)") : name + (proc.isPresetDirty() ? " *" : "");
            g.drawText (name, juce::Rectangle<float> (r.getX() + 40, r.getY(), r.getWidth() - 66, r.getHeight()), juce::Justification::centredLeft, true);
            juce::Path chev;
            const float vx = r.getRight() - 15.0f;
            chev.startNewSubPath (vx - 5.5f, cy - 2.5f); chev.lineTo (vx, cy + 3.0f); chev.lineTo (vx + 5.5f, cy - 2.5f);
            g.setColour (col::accent);
            g.strokePath (chev, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // slots : nom + forme d'onde
        for (int s = 0; s < 2; ++s)
        {
            const auto& inf = info[s];
            const auto nb = rName.translated (s * kCardDx, 0);
            // badge A / B
            const juce::Rectangle<float> badge (nb.getX() + 17, nb.getCentreY() - 12.5f, 25, 25);
            g.setColour (col::accent);
            g.fillRoundedRectangle (badge, 2.0f);
            g.setColour (juce::Colour (0xff151515));
            g.setFont (fBold (19.0f));
            g.drawText (s == 0 ? "A" : "B", badge.translated (0, 0.5f), juce::Justification::centred);
            const auto tr = juce::Rectangle<float> (badge.getRight() + 9, nb.getY(), nb.getRight() - badge.getRight() - 15, nb.getHeight());
            const bool showErr = inf.error.isNotEmpty() && juce::Time::getMillisecondCounter() < errUntil[s];
            g.setFont (fName (12.5f));
            if (showErr) { g.setColour (col::err); g.drawFittedText (inf.error, tr.toNearestInt(), juce::Justification::centredLeft, 2, 0.8f); }
            else if (inf.loaded)
            {
                g.setColour (col::text);
                g.drawFittedText (inf.name, tr.toNearestInt(), juce::Justification::centredLeft, 2, 0.8f);
            }
            else { g.setColour (col::dim); g.drawText ("Drop an IR or click to load", tr, juce::Justification::centredLeft, true); }
            if (dropHover == s) { g.setColour (col::accent.withAlpha (0.8f)); g.drawRoundedRectangle (nb.reduced (1.0f), 3.0f, 1.5f); }

            // forme d'onde
            const auto wr = rWave[s];
            if (inf.loaded && ! inf.peakMax.empty())
            {
                const bool muted = slot[s].mute->norm() > 0.5f;
                g.saveState();
                g.reduceClipRegion (wr.toNearestInt());
                const float cy = wr.getCentreY(), amp = wr.getHeight() * 0.5f - 1.0f;
                const int cols = (int) wr.getWidth() * 2;
                const int P = (int) inf.peakMax.size();
                juce::Path wave;
                float lastY = cy;
                for (int x = 0; x < cols; ++x)
                {
                    const int a = x * P / cols, b = juce::jmax (a + 1, (x + 1) * P / cols);
                    float mn = 1.0f, mx = -1.0f;
                    for (int i = a; i < juce::jmin (b, P); ++i) { mn = juce::jmin (mn, inf.peakMin[(size_t) i]); mx = juce::jmax (mx, inf.peakMax[(size_t) i]); }
                    const float px = wr.getX() + 0.5f + (float) x * 0.5f;
                    const float y0 = cy - mx * amp, y1 = cy - mn * amp;
                    if (x == 0) wave.startNewSubPath (px, lastY);
                    // relie la colonne precedente puis trace l'etendue min / max de cette colonne
                    wave.lineTo (px, std::abs (lastY - y0) < std::abs (lastY - y1) ? y0 : y1);
                    wave.lineTo (px, std::abs (lastY - y0) < std::abs (lastY - y1) ? y1 : y0);
                    lastY = std::abs (lastY - y0) < std::abs (lastY - y1) ? y1 : y0;
                }
                g.setColour (col::accent.withAlpha (muted ? 0.3f : 1.0f));
                g.strokePath (wave, juce::PathStrokeType (1.15f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
                g.restoreState();
                g.setColour (juce::Colour (0xffa3a3a3));
                g.setFont (fSemi (11.0f));
                g.drawText (formatInfo (inf), juce::Rectangle<float> (wr.getX() + 6, wr.getY() + 8, wr.getWidth() - 14, 16), juce::Justification::centredRight, true);
            }
            if (dropHover == s) { g.setColour (col::accent.withAlpha (0.6f)); g.drawRect (wr.reduced (1.0f), 1.5f); }
        }
    }

    // fond statique : panneaux, etiquettes, graduations, lettres A / B
    void drawStatic (juce::Graphics& g)
    {
        g.fillAll (col::outer);
        // en-tete
        g.setColour (col::header);
        g.fillRoundedRectangle (rHeader, 3.0f);
        g.setColour (col::nameBox);
        g.fillRoundedRectangle (rPreset, 3.0f);
        drawButtonBg (g, rSave, false, false);
        drawButtonBg (g, rGear, false, false);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawLine (48.0f, 10.0f, 48.0f, 39.0f, 1.0f);
        if (title.isValid())
        {
            // logo IMPULSE a la place du titre texte
            const float h = 24.37f, w = h * (float) title.getWidth() / (float) title.getHeight();
            g.setOpacity (0.92f);
            g.drawImage (title, juce::Rectangle<float> (60.0f, 24.5f - h * 0.5f, w, h));
            g.setOpacity (1.0f);
        }

        // colonne centrale
        juce::ColourGradient cg (col::center.brighter (0.03f), 0, rCenter.getY(), col::center.darker (0.08f), 0, rCenter.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (rCenter, 4.0f);
        g.setFont (fSemi (14.0f));
        const char* names[3] = { "MIX", "OVERLOAD", "VARIPHI" };
        for (int i = 0; i < 3; ++i)
        {
            g.setColour (col::label);
            g.drawText (names[i], juce::Rectangle<float> (kKnobX - 60, kLabelY[i] - 9, 120, 18), juce::Justification::centred);
        }
        g.setFont (fSemi (12.0f));
        g.drawText ("ir a", juce::Rectangle<float> (kKnobX - 38, 114, 30, 16), juce::Justification::centred);
        g.drawText ("ir b", juce::Rectangle<float> (kKnobX + 8, 114, 30, 16), juce::Justification::centred);
        g.drawText ("%", juce::Rectangle<float> (kKnobX - 15, 204.75f, 30, 16), juce::Justification::centred);
        g.drawText ("%", juce::Rectangle<float> (kKnobX - 15, 293.75f, 30, 16), juce::Justification::centred);
        if (logo.isValid())
        {
            const float h = 6.65f, w = h * (float) logo.getWidth() / (float) logo.getHeight();
            g.setOpacity (0.55f);
            g.drawImage (logo, juce::Rectangle<float> (kKnobX - w * 0.5f, 323.5f - h * 0.5f, w, h));   // bas de la colonne centrale
            g.setOpacity (1.0f);
        }

        for (int s = 0; s < 2; ++s)
        {
            // cadre bas (forme d'onde + tranche) avec ombre
            const auto lo = rLower[s];
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRect (lo.translated (0, 3.0f));
            g.setColour (col::frame);
            g.fillRect (lo);
            const auto wr = rWave[s];
            juce::ColourGradient wg (col::waveTop, 0, wr.getY(), col::waveBot, 0, wr.getBottom(), false);
            g.setGradientFill (wg);
            g.fillRect (wr);
            // grande lettre en filigrane
            g.saveState();
            g.reduceClipRegion (wr.toNearestInt());
            g.setColour (col::letter);
            g.setFont (fBold (210.0f));
            g.drawText (s == 0 ? "A" : "B", juce::Rectangle<float> (wr.getX(), wr.getY() + 46, wr.getWidth(), 210), juce::Justification::centredTop);
            g.restoreState();
            g.setColour (col::grid);
            const float q = wr.getHeight() / 4.0f;
            g.fillRect (wr.getX(), wr.getY() + q, wr.getWidth(), 1.0f);
            g.fillRect (wr.getX(), wr.getY() + 3 * q, wr.getWidth(), 1.0f);

            // tranche : graduations du fader
            const auto sr = rStrip[s];
            g.setColour (col::strip);
            g.fillRect (sr);
            const float sx = s * kStripDx;
            for (int t = 0; t < 10; ++t)
            {
                const float y = kFaderTop + (kFaderBot - kFaderTop) * t / 9.0f;
                const bool zero = t == 2;
                g.setColour (zero ? juce::Colour (0xff595959) : juce::Colour (0xff3e3e3e));
                g.fillRect (303.0f + sx, y - 0.5f, 26.0f, 1.0f);
                if (zero) g.fillRect (329.0f + sx, y - 0.5f, 12.0f, 1.0f);
                g.setColour (juce::Colour (0xff4a4a4a));
                g.fillRect (kTickLabelX + sx - 13.0f, y - 0.5f, 4.0f, 1.0f);
                g.fillRect (kTickLabelX + sx + 9.0f, y - 0.5f, 4.0f, 1.0f);
                g.setColour (juce::Colour (0xff727272));
                if (t == 9)
                {
                    // symbole infini
                    juce::Path inf;
                    const float cx = kTickLabelX + sx, cy = y;
                    inf.addEllipse (cx - 5.5f, cy - 2.3f, 5.5f, 4.6f);
                    inf.addEllipse (cx, cy - 2.3f, 5.5f, 4.6f);
                    g.strokePath (inf, juce::PathStrokeType (1.0f));
                }
                else
                {
                    g.setFont (fMedium (9.5f));
                    g.drawText (kFaderTicks[t], juce::Rectangle<float> (kTickLabelX + sx - 9, y - 6, 18, 12), juce::Justification::centred);
                }
            }

            // carte du slot (au-dessus de la colonne centrale)
            const auto cr = rCard[s];
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillRoundedRectangle (cr.translated (0, 2.0f), 4.0f);
            juce::ColourGradient cgr (col::card, 0, cr.getY(), col::cardLo, 0, cr.getBottom(), false);
            g.setGradientFill (cgr);
            g.fillRoundedRectangle (cr, 4.0f);
            g.setColour (juce::Colours::white.withAlpha (0.045f));
            g.drawLine (cr.getX() + 4, cr.getY() + 0.5f, cr.getRight() - 4, cr.getY() + 0.5f, 1.0f);
            const auto nb = rName.translated (s * kCardDx, 0);
            g.setColour (col::nameBox);
            g.fillRoundedRectangle (nb, 3.0f);
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.drawRoundedRectangle (nb.reduced (0.5f), 3.0f, 1.0f);
        }
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
        m.addItem (999, "Impulse IR loader v" + juce::String (JucePlugin_VersionString) + " - STUDIO WIGA", false);

        juce::Component::SafePointer<ImpulseContent> sp (this);
        juce::Component::SafePointer<juce::Component> tp (target);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
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
        m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (toInt (rPreset))),
                         [sp, names] (int r) { if (sp != nullptr) sp->handlePresetChoice (r, names); });
    }

    // disquette : enregistrer / exporter / importer
    void presetFileMenu()
    {
        juce::PopupMenu m;
        addPresetFileItems (m);
        juce::Component::SafePointer<ImpulseContent> sp (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&save),
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
            const float px = (float) ed.getWidth() / (float) kW * displayScale;
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
    juce::Image logo = juce::ImageCache::getFromMemory (BinaryData::logo_studio_wiga_blanc_png, BinaryData::logo_studio_wiga_blanc_pngSize);
    juce::Image title = juce::ImageCache::getFromMemory (BinaryData::logo_impulse_png, BinaryData::logo_impulse_pngSize);

    // anti-gel v2
    juce::Image bgCache;
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
    setResizeLimits (kW, kH, (int) std::lround (kW * 2.2f), (int) std::lround (kH * 2.2f));
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
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
    const float s = (float) getWidth() / (float) kW;
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
    const float s = (float) getWidth() / (float) kW;
    content->setTransform (juce::AffineTransform::scale (s));
    content->setBounds (0, 0, kW, kH);
    content->editorResized();            // jamais de setSize ici (boucle avec l'hote a 125 / 150 %)
}

int ImpulseEditor::getScaleIndex() const
{
    const float s = (float) getWidth() / (float) kW;
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
    const int w = (int) std::lround (kW * kScales[idx]);
    setSize (w, (int) std::lround ((double) w * kH / kW));
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
