#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ColourPicker.h"

namespace
{
    constexpr double kMaxIrSeconds   = 10.0;   // longueur maxi chargee (comme TONE3000)
    constexpr double kShortIrSeconds = 1.0;    // < 1 s = IR de baffle (pad -18 dB), au-dela = reverb
    constexpr double kNormBaseRate   = 48000.0;
    constexpr double kMaxEmbedSeconds = 4.0;   // IR copiees dans le projet / preset si fichier introuvable
    constexpr float  kVariphiMaxMs   = 1.0f;   // VARIPHI : decalage de phase de B, 0 - 1 ms
    constexpr float  kBaseDelay      = 2.0f;   // A et B passent tous deux par un retard de base de 2 echantillons
    const float kFaderDb[10] = { -100.0f, -60.0f, -45.0f, -36.0f, -18.0f, -12.0f, -6.0f, 0.0f, 6.0f, 12.0f };

    float dbToGain (float db) { return db <= -99.0f ? 0.0f : std::pow (10.0f, db * 0.05f); }
    // log(cosh(u)) sans debordement (antiderivee de tanh)
    inline float logCosh (float u) { const float a = std::abs (u); return a + std::log1p (std::exp (-2.0f * a)) - 0.69314718f; }

    double besselI0 (double x)
    {
        double sum = 1.0, term = 1.0;
        const double q = x * x * 0.25;
        for (int k = 1; k < 40; ++k) { term *= q / ((double) k * k); sum += term; if (term < 1e-12 * sum) break; }
        return sum;
    }
}

// ============================================================================ fader
float ImpulseProcessor::faderPosToDb (float pos)
{
    const float f = juce::jlimit (0.0f, 9.0f, pos * 9.0f);
    if (f <= 0.0f) return -100.0f;
    if (f < 1.0f) return juce::jmax (-100.0f, -60.0f + 20.0f * std::log10 (f));   // dernier segment : gain lineaire vers -inf
    const int i = juce::jmin (8, (int) f);
    const float t = f - (float) i;
    return kFaderDb[i] + t * (kFaderDb[i + 1] - kFaderDb[i]);
}

float ImpulseProcessor::faderDbToPos (float db)
{
    if (db <= -99.0f) return 0.0f;
    if (db < -60.0f) return std::pow (10.0f, (db + 60.0f) / 20.0f) / 9.0f;
    for (int i = 1; i < 9; ++i)
        if (db <= kFaderDb[i + 1])
            return ((float) i + (db - kFaderDb[i]) / (kFaderDb[i + 1] - kFaderDb[i])) / 9.0f;
    return 1.0f;
}

float ImpulseProcessor::faderPosToGain (float pos)
{
    const float f = juce::jlimit (0.0f, 9.0f, pos * 9.0f);
    if (f < 1.0f) return f * dbToGain (-60.0f);
    return dbToGain (faderPosToDb (pos));
}

// ============================================================================ MIDI Learn
const ImpulseProcessor::LearnDef& ImpulseProcessor::learnDef (int i)
{
    // aucun CC par defaut : tout passe par le MIDI Learn (clic droit sur la commande)
    static const LearnDef defs[numLearn] = {
        { "mix",      "Mix A/B",        learnContinuous },
        { "overload", "Overload",       learnContinuous },
        { "variphi",  "Variphi",        learnContinuous },
        { "gainA",    "Level A",        learnContinuous },
        { "gainB",    "Level B",        learnContinuous },
        { "muteA",    "Mute A",         learnToggle },
        { "muteB",    "Mute B",         learnToggle },
        { "phaseA",   "Phase A",        learnToggle },
        { "phaseB",   "Phase B",        learnToggle },
        { "active",   "Power",          learnToggle },
        { "prevA",    "IR A previous",  learnTrigger },
        { "nextA",    "IR A next",      learnTrigger },
        { "prevB",    "IR B previous",  learnTrigger },
        { "nextB",    "IR B next",      learnTrigger },
        { "prevP",    "Preset previous",learnTrigger },
        { "nextP",    "Preset next",    learnTrigger },
    };
    return defs[juce::jlimit (0, numLearn - 1, i)];
}

void ImpulseProcessor::resetMidiMap()
{
    learnTarget = -1;
    for (int i = 0; i < numLearn; ++i)
    {
        ccMap[i].store (-1); noteMap[i].store (-1);
        ccMin[i] = 127; ccMax[i] = 0; ccHeld[i] = false;
    }
}

void ImpulseProcessor::clearLearn (int i)
{
    if (i < 0 || i >= numLearn) return;
    ccMap[i].store (-1); noteMap[i].store (-1);
    ccMin[i] = 127; ccMax[i] = 0;
    if (learnTarget.load() == i) learnTarget = -1;
}

juce::String ImpulseProcessor::learnAssignText (int i) const
{
    i = juce::jlimit (0, numLearn - 1, i);
    if (ccMap[i].load() >= 0)   return "CC " + juce::String (ccMap[i].load());
    if (noteMap[i].load() >= 0) return "Note " + juce::String (noteMap[i].load());
    return "none";
}

// ============================================================================ parametres
juce::AudioProcessorValueTreeState::ParameterLayout ImpulseProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "active", 1 }, "Power", true));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix A/B",
        NormalisableRange<float> (-100.0f, 100.0f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        {
            const int b = (int) std::lround ((v + 100.0f) * 0.5f);
            return "A " + String (100 - b) + " / B " + String (b);
        })));
    auto pct = AudioParameterFloatAttributes().withLabel ("%")
                  .withStringFromValueFunction ([] (float v, int) { return String ((int) std::lround (v)); });
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "overload", 1 }, "Overload", NormalisableRange<float> (0.0f, 100.0f), 0.0f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "variphi", 1 }, "Variphi", NormalisableRange<float> (0.0f, 100.0f), 0.0f, pct));

    auto fader = AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction ([] (float v, int)
    {
        const float db = faderPosToDb (v);
        return db <= -99.0f ? String ("-inf") : String (db, 1);
    });
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "gainA", 1 }, "Level A", NormalisableRange<float> (0.0f, 1.0f), faderDefaultPos, fader));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "gainB", 1 }, "Level B", NormalisableRange<float> (0.0f, 1.0f), faderDefaultPos, fader));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "muteA", 1 },  "Mute A", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "muteB", 1 },  "Mute B", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "phaseA", 1 }, "Phase invert A", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "phaseB", 1 }, "Phase invert B", false));

    NormalisableRange<float> lowR (20.0f, 400.0f); lowR.setSkewForCentre (80.0f);
    NormalisableRange<float> highR (2000.0f, 20000.0f); highR.setSkewForCentre (7000.0f);
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "lowcut", 1 }, "Low cut", lowR, 20.0f,
        AudioParameterFloatAttributes().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return v <= 20.5f ? String ("Off") : String ((int) std::lround (v)); })));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "highcut", 1 }, "High cut", highR, 20000.0f,
        AudioParameterFloatAttributes().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return v >= 19900.0f ? String ("Off") : String ((int) std::lround (v)); })));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "normalize", 1 }, "Normalize IRs", true));
    return { p.begin(), p.end() };
}

ImpulseProcessor::ImpulseProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (auto& c : conv)
        c = std::make_unique<juce::dsp::Convolution> (juce::dsp::Convolution::NonUniform { 4096 }, convQueue);
    for (auto& h : hasIR) h.store (false);

    pActive = apvts.getRawParameterValue ("active");     pMix = apvts.getRawParameterValue ("mix");
    pOverload = apvts.getRawParameterValue ("overload"); pVariphi = apvts.getRawParameterValue ("variphi");
    pGainA = apvts.getRawParameterValue ("gainA");       pGainB = apvts.getRawParameterValue ("gainB");
    pMuteA = apvts.getRawParameterValue ("muteA");       pMuteB = apvts.getRawParameterValue ("muteB");
    pPhaseA = apvts.getRawParameterValue ("phaseA");     pPhaseB = apvts.getRawParameterValue ("phaseB");
    pLowCut = apvts.getRawParameterValue ("lowcut");     pHighCut = apvts.getRawParameterValue ("highcut");
    pNormalize = apvts.getRawParameterValue ("normalize");
    for (int i = 0; i < numParamLearn; ++i)
    {
        queuedPtr[i] = apvts.getRawParameterValue (learnDef (i).id);
        pendingValue[i].store (0.0f); pendingSet[i].store (false);
    }
    resetMidiMap();
    lastNormalize = pNormalize->load() > 0.5f;
    lastFolder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    accentColour = ImpulseSettings::accent (JucePlugin_Name);   // derniere couleur choisie
    presetSnapshot = snapshotString();
    startTimerHz (60);   // anti-gel v2 : aucun appel a l'hote depuis le thread audio
}

ImpulseProcessor::~ImpulseProcessor()
{
    stopTimer();
}

bool ImpulseProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    const bool okIn  = in  == juce::AudioChannelSet::mono() || in  == juce::AudioChannelSet::stereo();
    const bool okOut = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
    return okIn && okOut && in.size() <= out.size();   // mono -> stereo autorise (IR stereo)
}

// ============================================================================ preparation
void ImpulseProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hostRate = sampleRate > 0 ? sampleRate : 44100.0;
    maxBlock = juce::jmax (32, samplesPerBlock);
    const juce::dsp::ProcessSpec spec { hostRate, (juce::uint32) maxBlock, 2 };

    for (auto& c : conv) { c->prepare (spec); c->reset(); }
    for (int s = 0; s < numSlots; ++s) pushKernel (s);     // IR re-echantillonnees a la frequence de l'hote

    for (int c = 0; c < 2; ++c) adaaX[c] = 0.0f;
    setLatencySamples ((int) kBaseDelay);

    const int maxDelay = (int) std::ceil (kBaseDelay + kVariphiMaxMs * 0.001 * hostRate) + 8;
    for (auto* d : { &delayA, &delayB })
    {
        d->setMaximumDelayInSamples (maxDelay);
        d->prepare (spec);
        d->reset();
    }

    dryBuf.setSize (2, maxBlock, false, true, true);
    passBuf.setSize (2, maxBlock, false, true, true);
    bufA.setSize (2, maxBlock, false, true, true);
    bufB.setSize (2, maxBlock, false, true, true);

    const float k = 1.0f - std::exp (-1.0f / (0.012f * (float) hostRate));   // 2 poles de 12 ms
    for (auto* s : { &smVar, &smGA, &smGB, &smDryW, &smActive }) s->k = k;
    osK = k;
    smVar.set (pVariphi->load() * 0.01f);
    smActive.set (pActive->load() > 0.5f ? 1.0f : 0.0f);
    smGA.set (0.0f); smGB.set (0.0f); smDryW.set (1.0f);
    overOs = overOs2 = pOverload->load() * 0.01f;

    hpf.reset(); lpf.reset(); lastLow = lastHigh = -1.0f;
    for (int c = 0; c < 2; ++c) dcX[c] = dcY[c] = 0.0f;
}

// ============================================================================ filtres de sortie
void ImpulseProcessor::setHighPass (Biquad& f, double fc, double sr)
{
    if (fc <= 20.5) { f.b0 = 1; f.b1 = f.b2 = f.a1 = f.a2 = 0; return; }
    const double w = juce::MathConstants<double>::twoPi * fc / sr, cw = std::cos (w), al = std::sin (w) / (2.0 * 0.7071);
    const double a0 = 1.0 + al;
    f.b0 = (float) ((1.0 + cw) * 0.5 / a0); f.b1 = (float) (-(1.0 + cw) / a0); f.b2 = f.b0;
    f.a1 = (float) (-2.0 * cw / a0);        f.a2 = (float) ((1.0 - al) / a0);
}

void ImpulseProcessor::setLowPass (Biquad& f, double fc, double sr)
{
    if (fc >= 19900.0 || fc >= sr * 0.45) { f.b0 = 1; f.b1 = f.b2 = f.a1 = f.a2 = 0; return; }
    const double w = juce::MathConstants<double>::twoPi * fc / sr, cw = std::cos (w), al = std::sin (w) / (2.0 * 0.7071);
    const double a0 = 1.0 + al;
    f.b0 = (float) ((1.0 - cw) * 0.5 / a0); f.b1 = (float) ((1.0 - cw) / a0); f.b2 = f.b0;
    f.a1 = (float) (-2.0 * cw / a0);        f.a2 = (float) ((1.0 - al) / a0);
}

// ============================================================================ audio
void ImpulseProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    handleMidi (midi);

    const int totalIn = getTotalNumInputChannels(), totalOut = getTotalNumOutputChannels();
    const int total = buffer.getNumSamples();
    if (totalIn == 1 && totalOut >= 2) buffer.copyFrom (1, 0, buffer, 0, 0, total);   // mono -> stereo
    for (int c = juce::jmax (totalIn, 2); c < totalOut; ++c) buffer.clear (c, 0, total);
    const int nCh = juce::jmin (2, buffer.getNumChannels(), juce::jmax (totalIn, totalOut));
    if (nCh <= 0 || total <= 0) return;

    // ---- cibles (lues une fois par bloc, sans chaine de caracteres)
    const bool loadedA = hasIR[0].load(), loadedB = hasIR[1].load();
    const bool actA = loadedA && pMuteA->load (std::memory_order_relaxed) < 0.5f;
    const bool actB = loadedB && pMuteB->load (std::memory_order_relaxed) < 0.5f;
    const float m = juce::jlimit (-1.0f, 1.0f, pMix->load (std::memory_order_relaxed) * 0.01f);
    float wA = 0.0f, wB = 0.0f, wDry = 0.0f;
    if (! loadedA && ! loadedB) wDry = 1.0f;                 // aucune IR : le son passe sans traitement
    else if (actA && actB) { wA = 0.5f * (1.0f - m); wB = 0.5f * (1.0f + m); }   // fondu lineaire (signaux correles)
    else if (actA) wA = 1.0f;
    else if (actB) wB = 1.0f;
    const float tGA = wA * faderPosToGain (pGainA->load (std::memory_order_relaxed)) * (pPhaseA->load (std::memory_order_relaxed) > 0.5f ? -1.0f : 1.0f);
    const float tGB = wB * faderPosToGain (pGainB->load (std::memory_order_relaxed)) * (pPhaseB->load (std::memory_order_relaxed) > 0.5f ? -1.0f : 1.0f);
    const float tOver = juce::jlimit (0.0f, 1.0f, pOverload->load (std::memory_order_relaxed) * 0.01f);
    const float tVar  = juce::jlimit (0.0f, 1.0f, pVariphi->load (std::memory_order_relaxed) * 0.01f);
    const float tAct  = pActive->load (std::memory_order_relaxed) > 0.5f ? 1.0f : 0.0f;
    const float varMaxSamples = kVariphiMaxMs * 0.001f * (float) hostRate;

    // filtres de sortie (lissage par bloc)
    const float lowT = pLowCut->load (std::memory_order_relaxed), highT = pHighCut->load (std::memory_order_relaxed);
    const float lowF  = lastLow  < 0 ? lowT  : lastLow  + 0.35f * (lowT - lastLow);
    const float highF = lastHigh < 0 ? highT : lastHigh + 0.35f * (highT - lastHigh);
    if (std::abs (lowF - lastLow) > 0.01f)   { setHighPass (hpf, lowF, hostRate);  lastLow = lowF; }
    if (std::abs (highF - lastHigh) > 0.5f)  { setLowPass (lpf, highF, hostRate);  lastHigh = highF; }
    const bool useHp = lastLow > 20.5f, useLp = lastHigh < 19900.0f && lastHigh < hostRate * 0.45;
    const float dcR = 1.0f - juce::MathConstants<float>::twoPi * 8.0f / (float) hostRate;

    for (int start = 0; start < total; start += maxBlock)
    {
        const int n = juce::jmin (maxBlock, total - start);
        float* io[2] = { buffer.getWritePointer (0, start), nCh > 1 ? buffer.getWritePointer (1, start) : nullptr };
        for (int c = 0; c < nCh; ++c) dryBuf.copyFrom (c, 0, io[c], n);

        // ---- OVERLOAD : saturation asymetrique facon haut-parleur en surcharge
        //      anti-repliement par antiderivee (ADAA 1er ordre) : aucune latence, et a 0 % le signal
        //      passe a l'identique (seule la difference saturee - propre est ajoutee, puis coupe-DC).
        // ---- puis retards A / B (VARIPHI = decalage fin de B) et copie de passage
        for (int i = 0; i < n; ++i)
        {
            overOs  += osK * (tOver - overOs);
            overOs2 += osK * (overOs - overOs2);
            const float o = overOs2;
            const float vp = kBaseDelay + smVar.next (tVar) * varMaxSamples;
            float d = 1.0f, b = 0.0f, tb = 0.0f, scale = 0.0f, w = 0.0f;
            if (o > 1.0e-5f)
            {
                d  = std::exp (o * 24.0f * 0.1151293f);      // 0 a +24 dB d'attaque
                b  = 0.25f * o;                               // asymetrie (harmoniques paires)
                tb = std::tanh (b);
                scale = std::sqrt (d) / (d * (1.0f - tb * tb));   // gain petit signal = sqrt(d)
                w  = juce::jmin (1.0f, o * 10.0f);
            }
            for (int c = 0; c < nCh; ++c)
            {
                const float x = io[c][i];
                float e = 0.0f;
                if (w > 0.0f)
                {
                    const float u1 = d * x + b, u0 = d * adaaX[c] + b;
                    float y;
                    if (std::abs (u1 - u0) > 1.0e-4f) y = (logCosh (u1) - logCosh (u0)) / (u1 - u0);
                    else y = std::tanh (0.5f * (u1 + u0));
                    e = w * ((y - tb) * scale - x);
                }
                adaaX[c] = x;
                // coupe-DC sur la seule partie ajoutee par la saturation
                const float ed = e - dcX[c] + dcR * dcY[c];
                dcX[c] = e; dcY[c] = ed;
                const float sx = x + ed;
                delayA.pushSample (c, sx);
                delayB.pushSample (c, sx);
                const float a = delayA.popSample (c, kBaseDelay);
                bufA.setSample (c, i, a);
                passBuf.setSample (c, i, a);
                bufB.setSample (c, i, delayB.popSample (c, vp));
            }
        }

        // ---- convolution (moteur JUCE partitionne, sans latence, fondu interne au changement d'IR)
        const bool runA = loadedA || std::abs (smGA.b) > 1.0e-5f;
        const bool runB = loadedB || std::abs (smGB.b) > 1.0e-5f;
        if (runA)
        {
            juce::dsp::AudioBlock<float> b (bufA.getArrayOfWritePointers(), (size_t) nCh, (size_t) n);
            conv[0]->process (juce::dsp::ProcessContextReplacing<float> (b));
        }
        if (runB)
        {
            juce::dsp::AudioBlock<float> b (bufB.getArrayOfWritePointers(), (size_t) nCh, (size_t) n);
            conv[1]->process (juce::dsp::ProcessContextReplacing<float> (b));
        }

        // ---- melange A / B, filtres, power
        for (int i = 0; i < n; ++i)
        {
            const float gA = smGA.next (tGA), gB = smGB.next (tGB), gD = smDryW.next (wDry), act = smActive.next (tAct);
            for (int c = 0; c < nCh; ++c)
            {
                float y = gD * passBuf.getSample (c, i);
                if (runA) y += gA * bufA.getSample (c, i);
                if (runB) y += gB * bufB.getSample (c, i);
                if (useHp) y = hpf.process (y, c);
                if (useLp) y = lpf.process (y, c);
                const float dry = dryBuf.getSample (c, i);
                io[c][i] = dry + act * (y - dry);
            }
        }
    }
}

// ============================================================================ MIDI (thread audio : aucun appel a l'hote)
void ImpulseProcessor::handleMidi (const juce::MidiBuffer& midiBuf)
{
    auto queueValue = [this] (int j, float norm)
    {
        pendingValue[j].store (juce::jlimit (0.0f, 1.0f, norm), std::memory_order_relaxed);
        pendingSet[j].store (true, std::memory_order_release);
    };
    auto effective = [this] (int j)
    {
        return pendingSet[j].load (std::memory_order_acquire) ? pendingValue[j].load (std::memory_order_relaxed)
                                                               : queuedPtr[j]->load (std::memory_order_relaxed);
    };
    auto fire = [this, &queueValue, &effective] (int j, bool on, bool edgeOnly)
    {
        const int type = learnDef (j).type;
        if (type == learnTrigger)
        {
            if (on && ! ccHeld[j]) pendingTrigger.fetch_or (1 << (j - lPrevA));
        }
        else if (type == learnToggle)
        {
            if (edgeOnly || toggleMomentary.load()) { if (on && ! ccHeld[j]) queueValue (j, effective (j) > 0.5f ? 0.0f : 1.0f); }
            else queueValue (j, on ? 1.0f : 0.0f);           // footswitch a accrochage : 127 = on, 0 = off
        }
        ccHeld[j] = on;
    };

    for (const auto meta : midiBuf)
    {
        const auto msg = meta.getMessage();

        const int lt = learnTarget.load();
        if (lt >= 0 && lt < numLearn)
        {
            if (msg.isController() && msg.getControllerNumber() < 120)
            {
                const int cc = msg.getControllerNumber();
                for (int j = 0; j < numLearn; ++j) if (j != lt && ccMap[j].load() == cc) ccMap[j].store (-1);
                ccMap[lt].store (cc); noteMap[lt].store (-1);
                ccMin[lt] = 127; ccMax[lt] = 0; ccHeld[lt] = msg.getControllerValue() >= 64;
                lastLearned.store (lt); learnTarget.store (-1);
                continue;
            }
            if (msg.isNoteOn() && learnDef (lt).type != learnContinuous)
            {
                const int note = msg.getNoteNumber();
                for (int j = 0; j < numLearn; ++j) if (j != lt && noteMap[j].load() == note) noteMap[j].store (-1);
                noteMap[lt].store (note); ccMap[lt].store (-1);
                lastLearned.store (lt); learnTarget.store (-1);
                continue;
            }
        }

        if (msg.isController() && msg.getControllerNumber() < 120)
        {
            const int c = msg.getControllerNumber(), v = msg.getControllerValue();
            if (c < 32) msb[c] = v;
            // CC 14 bits : LSB (32-63) qui suit le MSB d'une commande continue
            if (c >= 32 && c < 64)
            {
                bool used = false;
                for (int j = 0; j < numParamLearn; ++j)
                    if (learnDef (j).type == learnContinuous && ccMap[j].load (std::memory_order_relaxed) == c - 32)
                    {
                        queueValue (j, (float) (msb[c - 32] * 128 + v) / 16383.0f);
                        used = true;
                    }
                if (used) continue;
            }
            for (int j = 0; j < numLearn; ++j)
            {
                if (ccMap[j].load (std::memory_order_relaxed) != c) continue;
                if (learnDef (j).type == learnContinuous)
                {
                    // course apprise automatiquement (pedale qui n'envoie que 2-116 : 0 et 100 % exacts) + zones mortes
                    ccMin[j] = juce::jmin (ccMin[j], v); ccMax[j] = juce::jmax (ccMax[j], v);
                    const int span = ccMax[j] - ccMin[j];
                    float x = span >= 24 ? (float) (v - ccMin[j]) / (float) span : (float) v / 127.0f;
                    x = juce::jlimit (0.0f, 1.0f, (x - 0.015f) / 0.97f);
                    queueValue (j, x);
                }
                else fire (j, v >= 64, false);
            }
        }
        else if (msg.isNoteOn() || msg.isNoteOff())
        {
            const int note = msg.getNoteNumber();
            const bool on = msg.isNoteOn();
            for (int j = 0; j < numLearn; ++j)
                if (noteMap[j].load (std::memory_order_relaxed) == note) fire (j, on, true);
        }
    }
}

// ============================================================================ timer (thread de messages)
void ImpulseProcessor::timerCallback()
{
    for (int i = 0; i < numParamLearn; ++i)
        if (pendingSet[i].exchange (false, std::memory_order_acq_rel))
            if (auto* p = apvts.getParameter (learnDef (i).id))
                p->setValueNotifyingHost (pendingValue[i].load (std::memory_order_relaxed));

    const int trig = pendingTrigger.exchange (0);
    if (trig & 1)  stepIR (0, -1);
    if (trig & 2)  stepIR (0, +1);
    if (trig & 4)  stepIR (1, -1);
    if (trig & 8)  stepIR (1, +1);
    if (trig & 16) stepPreset (-1);
    if (trig & 32) stepPreset (+1);

    const bool norm = pNormalize->load() > 0.5f;
    if (norm != lastNormalize)
    {
        lastNormalize = norm;
        for (int s = 0; s < numSlots; ++s) pushKernel (s);
        infoVersion++;
    }
}

// ============================================================================ IR : re-echantillonnage sinc fenetre
juce::AudioBuffer<float> ImpulseProcessor::resampleSinc (const juce::AudioBuffer<float>& in, double srcRate, double dstRate)
{
    if (srcRate <= 0 || dstRate <= 0 || std::abs (srcRate - dstRate) < 0.5) return in;
    const double r = srcRate / dstRate;                    // echantillons d'entree par echantillon de sortie
    const double beta = juce::jmin (1.0, 1.0 / r) * 0.97;  // coupure anti-repliement
    constexpr int Z = 24;                                  // passages par zero
    const double halfW = Z / beta;
    const double kaiserB = 8.0, i0b = besselI0 (kaiserB);
    const int nIn = in.getNumSamples();
    const int nOut = juce::jmax (1, (int) std::ceil (nIn / r));
    juce::AudioBuffer<float> out (in.getNumChannels(), nOut);
    std::vector<double> w;
    for (int n = 0; n < nOut; ++n)
    {
        const double t = n * r;
        const int k0 = (int) std::ceil (t - halfW), k1 = (int) std::floor (t + halfW);
        w.clear();
        for (int k = k0; k <= k1; ++k)
        {
            const double x = t - k;
            const double u = x / halfW;
            if (std::abs (u) >= 1.0) { w.push_back (0.0); continue; }
            const double bx = beta * x;
            const double sinc = std::abs (bx) < 1e-9 ? 1.0 : std::sin (juce::MathConstants<double>::pi * bx) / (juce::MathConstants<double>::pi * bx);
            w.push_back (beta * sinc * besselI0 (kaiserB * std::sqrt (1.0 - u * u)) / i0b);
        }
        for (int c = 0; c < in.getNumChannels(); ++c)
        {
            const float* src = in.getReadPointer (c);
            double acc = 0.0;
            for (int k = k0, j = 0; k <= k1; ++k, ++j)
                if (k >= 0 && k < nIn) acc += w[(size_t) j] * src[k];
            out.setSample (c, n, (float) (acc * r));   // r : conserve la reponse en frequence (meme loi que JUCE)
        }
    }
    return out;
}

void ImpulseProcessor::pushKernel (int s)
{
    const juce::ScopedLock sl (slotLock);
    auto& slot = slots[s];
    if (slot.raw.getNumSamples() == 0 || slot.rawRate <= 0) return;
    auto kernel = resampleSinc (slot.raw, slot.rawRate, hostRate);
    const bool norm = pNormalize->load() > 0.5f;
    // normalise (energie unite, methode TONE3000) : -6 dB pour un baffle -> sortie ~ entree sur un signal d'ampli
    // brut (loi NAM) : IR telle quelle a -18 dB ; IR longue (reverb) : pas de pad
    const float pad = slot.info.isLong ? 1.0f : dbToGain (norm ? -6.0f : -18.0f);
    const float g = (norm ? slot.normGain : 1.0f) * pad;
    kernel.applyGain (g);
    slot.info.normDb = juce::Decibels::gainToDecibels (g, -120.0f);
    const auto stereo = kernel.getNumChannels() > 1 ? juce::dsp::Convolution::Stereo::yes : juce::dsp::Convolution::Stereo::no;
    conv[s]->loadImpulseResponse (std::move (kernel), hostRate, stereo, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
}

// ============================================================================ IR : chargement
bool ImpulseProcessor::isIRFile (const juce::File& f)
{
    return f.existsAsFile() && f.hasFileExtension ("wav;aif;aiff;flac");
}

bool ImpulseProcessor::loadIR (int s, const juce::File& file)
{
    if (s < 0 || s >= numSlots) return false;
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (file.existsAsFile() ? fm.createReaderFor (file) : nullptr);
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->numChannels <= 0 || reader->sampleRate <= 0)
    {
        {
            const juce::ScopedLock sl (slotLock);
            slots[s].info.error = file.existsAsFile() ? "Unsupported file: " + file.getFileName() : "File not found: " + file.getFileName();
        }
        infoVersion++;
        return false;
    }
    const int ch = (int) juce::jmin (2u, reader->numChannels);
    const auto len = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (kMaxIrSeconds * reader->sampleRate));
    juce::AudioBuffer<float> data (ch, len);
    reader->read (&data, 0, len, 0, true, ch > 1);
    lastFolder = file.getParentDirectory();
    return setSlotData (s, std::move (data), reader->sampleRate, file.getFileName(), file.getFullPathName(),
                        (int) reader->bitsPerSample, reader->usesFloatingPointData,
                        1000.0 * (double) reader->lengthInSamples / reader->sampleRate, false);
}

bool ImpulseProcessor::setSlotData (int s, juce::AudioBuffer<float>&& data, double rate, const juce::String& name,
                                    const juce::String& path, int bits, bool isFloat, double fileLengthMs, bool embedded)
{
    const int ch = data.getNumChannels();
    int len = data.getNumSamples();
    if (ch <= 0 || len <= 0) return false;

    // valeurs non finies -> 0 ; crete
    float peak = 0.0f;
    for (int c = 0; c < ch; ++c)
    {
        auto* d = data.getWritePointer (c);
        for (int i = 0; i < len; ++i) { if (! std::isfinite (d[i])) d[i] = 0.0f; peak = juce::jmax (peak, std::abs (d[i])); }
    }
    if (peak <= 1.0e-9f)
    {
        { const juce::ScopedLock sl (slotLock); slots[s].info.error = "Silent file: " + name; }
        infoVersion++;
        return false;
    }

    // fin silencieuse retiree (-90 dB sous la crete) ; le debut est garde (alignement de phase A / B)
    const float thr = peak * 3.16e-5f;
    int last = 0;
    for (int c = 0; c < ch; ++c)
    {
        const auto* d = data.getReadPointer (c);
        for (int i = len - 1; i > last; --i) if (std::abs (d[i]) > thr) { last = i; break; }
    }
    const int newLen = juce::jmax (16, juce::jmin (len, last + 1));
    const bool truncated = fileLengthMs > 1000.0 * len / rate + 0.5;
    if (newLen < len) { juce::AudioBuffer<float> t (ch, newLen); for (int c = 0; c < ch; ++c) t.copyFrom (c, 0, data, c, 0, newLen); data = std::move (t); }
    len = newLen;
    if (truncated) { const int f = juce::jmin (256, len / 8); for (int c = 0; c < ch; ++c) data.applyGainRamp (c, len - f, f, 1.0f, 0.0f); }

    // normalisation TONE3000 : energie unite (par canal), corrigee de la frequence d'echantillonnage (base 48 kHz)
    double sumSq = 0.0;
    for (int c = 0; c < ch; ++c) { const auto* d = data.getReadPointer (c); for (int i = 0; i < len; ++i) sumSq += (double) d[i] * d[i]; }
    const double l2 = std::sqrt (sumSq / ch * (rate / kNormBaseRate));
    const float normGain = l2 > 0.0 ? (float) juce::jlimit (juce::Decibels::decibelsToGain (-48.0), juce::Decibels::decibelsToGain (12.0), 1.0 / l2) : 1.0f;

    // forme d'onde : canal gauche (ou moyenne L/R), normalisee a la crete
    IRInfo info;
    info.loaded = true; info.fromEmbedded = embedded;
    info.name = name; info.path = path;
    info.fileRate = rate; info.lengthMs = fileLengthMs; info.bits = bits; info.isFloat = isFloat; info.channels = ch;
    info.isLong = (double) len / rate > kShortIrSeconds;
    {
        constexpr int P = 1024;
        info.peakMin.assign (P, 0.0f); info.peakMax.assign (P, 0.0f);
        float dp = 0.0f;
        std::vector<float> mono ((size_t) len);
        for (int i = 0; i < len; ++i)
        {
            float v = data.getSample (0, i);
            if (ch > 1) v = 0.5f * (v + data.getSample (1, i));
            mono[(size_t) i] = v; dp = juce::jmax (dp, std::abs (v));
        }
        const float inv = dp > 0 ? 1.0f / dp : 1.0f;
        for (int p = 0; p < P; ++p)
        {
            const int a = (int) ((juce::int64) p * len / P), b = juce::jmax (a + 1, (int) ((juce::int64) (p + 1) * len / P));
            float mn = 1e9f, mx = -1e9f;
            for (int i = a; i < juce::jmin (b, len); ++i) { mn = juce::jmin (mn, mono[(size_t) i]); mx = juce::jmax (mx, mono[(size_t) i]); }
            if (mn > mx) mn = mx = 0.0f;
            info.peakMin[(size_t) p] = mn * inv; info.peakMax[(size_t) p] = mx * inv;
        }
    }

    {
        const juce::ScopedLock sl (slotLock);
        slots[s].raw = std::move (data);
        slots[s].rawRate = rate;
        slots[s].normGain = normGain;
        slots[s].info = std::move (info);
    }
    pushKernel (s);
    hasIR[s].store (true);
    updateTail();
    infoVersion++;
    return true;
}

void ImpulseProcessor::clearIR (int s)
{
    if (s < 0 || s >= numSlots) return;
    hasIR[s].store (false);
    {
        const juce::ScopedLock sl (slotLock);
        slots[s].raw.setSize (0, 0);
        slots[s].rawRate = 0;
        slots[s].info = IRInfo();
    }
    updateTail();
    infoVersion++;
}

void ImpulseProcessor::stepIR (int s, int dir)
{
    if (s < 0 || s >= numSlots) return;
    juce::File current;
    { const juce::ScopedLock sl (slotLock); current = juce::File (slots[s].info.path.isNotEmpty() && juce::File::isAbsolutePath (slots[s].info.path) ? slots[s].info.path : juce::String()); }
    const juce::File folder = startFolder (s);
    if (! folder.isDirectory()) return;
    juce::Array<juce::File> files;
    for (const auto& f : folder.findChildFiles (juce::File::findFiles, false))
        if (isIRFile (f)) files.add (f);
    if (files.isEmpty()) return;
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileName().compareNatural (b.getFileName()) < 0; });
    int idx = files.indexOf (current);
    idx = idx < 0 ? (dir > 0 ? 0 : files.size() - 1) : (idx + dir + files.size()) % files.size();
    loadIR (s, files[idx]);
}

juce::File ImpulseProcessor::startFolder (int s) const
{
    juce::String path;
    { const juce::ScopedLock sl (slotLock); path = slots[juce::jlimit (0, numSlots - 1, s)].info.path; }
    if (path.isNotEmpty() && juce::File::isAbsolutePath (path) && juce::File (path).getParentDirectory().isDirectory())
        return juce::File (path).getParentDirectory();
    const auto def = ImpulseSettings::irFolder (JucePlugin_Name);
    if (def.isDirectory()) return def;
    return lastFolder;
}

IRInfo ImpulseProcessor::getInfo (int s) const
{
    const juce::ScopedLock sl (slotLock);
    return slots[juce::jlimit (0, numSlots - 1, s)].info;
}

void ImpulseProcessor::updateTail()
{
    double t = 0.0;
    const juce::ScopedLock sl (slotLock);
    for (auto& s : slots) if (s.rawRate > 0) t = juce::jmax (t, s.raw.getNumSamples() / s.rawRate);
    tailSeconds.store (t);
}

// ============================================================================ etat (projet) et presets
std::unique_ptr<juce::XmlElement> ImpulseProcessor::buildStateXml (bool embedIRs) const
{
    auto xml = std::make_unique<juce::XmlElement> ("IMPULSE");
    xml->setAttribute ("version", JucePlugin_VersionString);
    if (auto st = const_cast<juce::AudioProcessorValueTreeState&> (apvts).copyState().createXml()) xml->addChildElement (st.release());
    const juce::ScopedLock sl (slotLock);
    for (int s = 0; s < numSlots; ++s)
    {
        const auto& slot = slots[s];
        if (! slot.info.loaded) continue;
        auto* e = xml->createNewChildElement ("SLOT");
        e->setAttribute ("index", s);
        e->setAttribute ("name", slot.info.name);
        e->setAttribute ("path", slot.info.path);
        e->setAttribute ("bits", slot.info.bits);
        e->setAttribute ("float", slot.info.isFloat ? 1 : 0);
        e->setAttribute ("lengthMs", slot.info.lengthMs);
        if (embedIRs && slot.raw.getNumSamples() <= (int) (kMaxEmbedSeconds * slot.rawRate))
        {
            // copie de l'IR dans le projet : le son reste identique meme si le fichier est deplace
            juce::MemoryOutputStream mo;
            {
                juce::GZIPCompressorOutputStream gz (mo, 9);
                for (int c = 0; c < slot.raw.getNumChannels(); ++c)
                    gz.write (slot.raw.getReadPointer (c), sizeof (float) * (size_t) slot.raw.getNumSamples());
            }
            e->setAttribute ("rate", slot.rawRate);
            e->setAttribute ("channels", slot.raw.getNumChannels());
            e->setAttribute ("samples", slot.raw.getNumSamples());
            e->setAttribute ("data", mo.getMemoryBlock().toBase64Encoding());
        }
    }
    auto* midiXml = xml->createNewChildElement ("MIDI");
    midiXml->setAttribute ("momentary", toggleMomentary.load() ? 1 : 0);
    for (int i = 0; i < numLearn; ++i)
        if (ccMap[i].load() >= 0 || noteMap[i].load() >= 0)
        {
            auto* l = midiXml->createNewChildElement ("L");
            l->setAttribute ("id", learnDef (i).id);
            l->setAttribute ("cc", ccMap[i].load());
            l->setAttribute ("note", noteMap[i].load());
        }
    return xml;
}

void ImpulseProcessor::applyStateXml (const juce::XmlElement& xml)
{
    if (auto* st = xml.getChildByName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*st));

    for (int s = 0; s < numSlots; ++s)
    {
        const juce::XmlElement* e = nullptr;
        for (auto* c : xml.getChildWithTagNameIterator ("SLOT")) if (c->getIntAttribute ("index", -1) == s) e = c;
        if (e == nullptr) { clearIR (s); continue; }
        const juce::String path = e->getStringAttribute ("path");
        if (path.isNotEmpty() && juce::File::isAbsolutePath (path) && juce::File (path).existsAsFile() && loadIR (s, juce::File (path)))
            continue;
        // fichier introuvable : copie enregistree
        const int ch = e->getIntAttribute ("channels", 0), ns = e->getIntAttribute ("samples", 0);
        const double rate = e->getDoubleAttribute ("rate", 0.0);
        juce::MemoryBlock packed;
        if (ch > 0 && ch <= 2 && ns > 0 && rate > 0 && packed.fromBase64Encoding (e->getStringAttribute ("data")))
        {
            juce::MemoryInputStream mi (packed, false);
            juce::GZIPDecompressorInputStream gz (mi);
            juce::AudioBuffer<float> data (ch, ns);
            bool ok = true;
            for (int c = 0; c < ch && ok; ++c)
                ok = gz.read (data.getWritePointer (c), (int) sizeof (float) * ns) == (int) sizeof (float) * ns;
            if (ok && setSlotData (s, std::move (data), rate, e->getStringAttribute ("name"), path,
                                   e->getIntAttribute ("bits", 24), e->getIntAttribute ("float") != 0,
                                   e->getDoubleAttribute ("lengthMs", 1000.0 * ns / rate), true))
                continue;
        }
        clearIR (s);
        { const juce::ScopedLock sl (slotLock); slots[s].info.error = "IR not found: " + e->getStringAttribute ("name"); }
        infoVersion++;
    }

    if (auto* midiXml = xml.getChildByName ("MIDI"))
    {
        resetMidiMap();
        toggleMomentary = midiXml->getIntAttribute ("momentary") != 0;
        for (auto* l : midiXml->getChildWithTagNameIterator ("L"))
            for (int i = 0; i < numLearn; ++i)
                if (l->getStringAttribute ("id") == learnDef (i).id)
                {
                    ccMap[i].store (juce::jlimit (-1, 127, l->getIntAttribute ("cc", -1)));
                    noteMap[i].store (juce::jlimit (-1, 127, l->getIntAttribute ("note", -1)));
                }
    }
}

void ImpulseProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto xml = buildStateXml (true);
    xml->setAttribute ("preset", presetName);
    xml->setAttribute ("snap", presetSnapshot);
    xml->setAttribute ("uiScale", uiScaleIdx.load());
    xml->setAttribute ("accent", juce::String::toHexString ((juce::int64) accentColour.load()));
    xml->setAttribute ("lastFolder", lastFolder.getFullPathName());
    copyXmlToBinary (*xml, dest);
}

void ImpulseProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName ("IMPULSE")) return;
    applyStateXml (*xml);
    presetName = xml->getStringAttribute ("preset");
    presetSnapshot = xml->hasAttribute ("snap") ? xml->getStringAttribute ("snap") : snapshotString();
    uiScaleIdx = juce::jlimit (-1, 2, xml->getIntAttribute ("uiScale", -1));
    if (xml->hasAttribute ("accent")) accentColour = 0xff000000u | (juce::uint32) xml->getStringAttribute ("accent").getHexValue64();
    const juce::File lf (xml->getStringAttribute ("lastFolder"));
    if (lf.isDirectory()) lastFolder = lf;
    presetVersion++;
}

juce::String ImpulseProcessor::snapshotString() const
{
    juce::String s;
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            s << rp->getParameterID() << '=' << juce::String (rp->getValue(), 4) << ';';
    const juce::ScopedLock sl (slotLock);
    for (auto& slot : slots) s << '|' << slot.info.path;
    return s;
}

bool ImpulseProcessor::isPresetDirty() const
{
    return presetName.isNotEmpty() && snapshotString() != presetSnapshot;
}

juce::File ImpulseProcessor::presetFolder()
{
    return WigaWindowSize::folder ("Impulse").getChildFile ("Presets");   // NINE HANDS DSP/Impulse/Presets
}

juce::StringArray ImpulseProcessor::getPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : presetFolder().findChildFiles (juce::File::findFiles, false, "*.impulse"))
        names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

bool ImpulseProcessor::loadPreset (const juce::String& name)
{
    const auto f = presetFolder().getChildFile (name + ".impulse");
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr || ! xml->hasTagName ("IMPULSE")) return false;
    // le preset ne touche pas aux affectations MIDI de l'instance
    if (auto* m = xml->getChildByName ("MIDI")) xml->removeChildElement (m, true);
    applyStateXml (*xml);
    presetName = name;
    presetSnapshot = snapshotString();
    presetVersion++;
    return true;
}

bool ImpulseProcessor::savePreset (const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty()) return false;
    auto xml = buildStateXml (true);
    if (auto* m = xml->getChildByName ("MIDI")) xml->removeChildElement (m, true);
    presetFolder().createDirectory();
    if (! xml->writeTo (presetFolder().getChildFile (name + ".impulse"))) return false;
    presetName = name;
    presetSnapshot = snapshotString();
    presetVersion++;
    return true;
}

bool ImpulseProcessor::exportPreset (const juce::File& file)
{
    auto xml = buildStateXml (true);
    if (auto* m = xml->getChildByName ("MIDI")) xml->removeChildElement (m, true);
    xml->setAttribute ("presetName", presetName.isNotEmpty() ? presetName : file.getFileNameWithoutExtension());
    file.getParentDirectory().createDirectory();
    return xml->writeTo (file);
}

bool ImpulseProcessor::importPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("IMPULSE")) return false;
    // copie dans la liste des presets (un nom deja pris recoit un numero) puis chargement
    auto name = juce::File::createLegalFileName (file.getFileNameWithoutExtension().trim());
    if (name.isEmpty()) name = "Imported preset";
    auto dest = presetFolder().getChildFile (name + ".impulse");
    if (dest.existsAsFile() && dest != file)
    {
        int n = 2;
        while (presetFolder().getChildFile (name + " " + juce::String (n) + ".impulse").existsAsFile()) ++n;
        name = name + " " + juce::String (n);
        dest = presetFolder().getChildFile (name + ".impulse");
    }
    presetFolder().createDirectory();
    if (dest != file && ! file.copyFileTo (dest)) return false;
    return loadPreset (name);
}

bool ImpulseProcessor::deletePreset (const juce::String& name)
{
    const bool ok = presetFolder().getChildFile (name + ".impulse").deleteFile();
    if (ok && name == presetName) { presetName.clear(); presetVersion++; }
    return ok;
}

void ImpulseProcessor::stepPreset (int dir)
{
    const auto names = getPresetNames();
    if (names.isEmpty()) return;
    int idx = names.indexOf (presetName);
    idx = idx < 0 ? (dir > 0 ? 0 : names.size() - 1) : (idx + dir + names.size()) % names.size();
    loadPreset (names[idx]);
}

void ImpulseProcessor::initPreset()
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
    for (int s = 0; s < numSlots; ++s) clearIR (s);
    presetName.clear();
    presetSnapshot = snapshotString();
    presetVersion++;
}

juce::AudioProcessorEditor* ImpulseProcessor::createEditor() { return new ImpulseEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ImpulseProcessor(); }
