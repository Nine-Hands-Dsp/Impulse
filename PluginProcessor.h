#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>

// ============================================================================
// IMPULSE (Studio Wiga) : chargeur de reponses impulsionnelles (IR) double A / B
// pour amplis guitare et basse.
//
// Moteur repris des references open source :
//  - TONE3000 plugin (MIT) : juce::dsp::Convolution partitionnee sans latence,
//    normalisation a energie unite + pad de -18 dB pour les IR de baffle (< 1 s),
//    pas de pad pour les IR longues (reverb), IR mono ou stereo vraie.
//  - NAM / AudioDSPTools (MIT) : loi de gain -18 dB, IR conservee brute pour
//    etre re-echantillonnee a chaque changement de frequence de l'hote.
// Ameliorations : re-echantillonnage sinc fenetre (Kaiser) au lieu de l'interpolation
// cubique (NAM) / ResamplingAudioSource (JUCE), suppression du silence de fin
// seulement (le debut est garde pour ne pas casser l'alignement de phase A / B).
// ============================================================================

struct IRInfo
{
    bool loaded = false;           // une IR est chargee dans ce slot
    bool fromEmbedded = false;     // fichier introuvable : copie enregistree dans le projet / preset
    juce::String name, path, error;
    double fileRate = 0.0, lengthMs = 0.0;
    int bits = 0, channels = 0;
    bool isFloat = false, isLong = false;
    float normDb = 0.0f;           // gain de normalisation applique (dB)
    std::vector<float> peakMin, peakMax;   // forme d'onde pour l'affichage (-1..1, normalisee a la crete)
};

class ImpulseProcessor : public juce::AudioProcessor,
                         private juce::Timer
{
public:
    ImpulseProcessor();
    ~ImpulseProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Impulse"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return tailSeconds.load(); }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---------------------------------------------------------------- IR slots (jamais depuis le thread audio)
    static constexpr int numSlots = 2;
    bool loadIR (int slot, const juce::File& file);          // charge un fichier wav / aif / flac
    void clearIR (int slot);
    void stepIR (int slot, int dir);                         // IR precedente / suivante du meme dossier
    IRInfo getInfo (int slot) const;
    juce::uint32 getInfoVersion() const { return infoVersion.load(); }
    static bool isIRFile (const juce::File& f);
    juce::File lastFolder;                                   // dernier dossier ouvert (bouton dossier)
    juce::File startFolder (int slot) const;                 // dossier de l'IR du slot, sinon dossier des IR par defaut, sinon le dernier

    // ---------------------------------------------------------------- presets (fichiers .impulse)
    static juce::File presetFolder();
    juce::StringArray getPresetNames() const;
    juce::String getPresetName() const { return presetName; }
    bool isPresetDirty() const;
    bool loadPreset (const juce::String& name);
    bool savePreset (const juce::String& name);
    bool deletePreset (const juce::String& name);
    void stepPreset (int dir);
    void initPreset();
    juce::uint32 getPresetVersion() const { return presetVersion.load(); }

    // ---------------------------------------------------------------- fader (dB) <-> position 0..1
    static float faderPosToDb (float pos);      // -100 = -inf
    static float faderDbToPos (float db);
    static float faderPosToGain (float pos);
    static constexpr float faderDefaultPos = 7.0f / 9.0f;   // 0 dB

    // ---------------------------------------------------------------- MIDI Learn (clic droit)
    enum LearnType { learnContinuous, learnToggle, learnTrigger };
    struct LearnDef { const char* id; const char* name; int type; };
    enum { lMix, lOverload, lVariphi, lGainA, lGainB,               // continus
           lMuteA, lMuteB, lPhaseA, lPhaseB, lActive,               // bascules
           lPrevA, lNextA, lPrevB, lNextB, lPrevPreset, lNextPreset, // impulsions
           numLearn };
    static constexpr int numParamLearn = lActive + 1;   // commandes reliees a un parametre (file anti-gel)
    static const LearnDef& learnDef (int i);
    std::atomic<int> ccMap[numLearn];
    std::atomic<int> noteMap[numLearn];
    std::atomic<int> learnTarget { -1 };
    std::atomic<int> lastLearned { -1 };
    std::atomic<bool> toggleMomentary { false };   // footswitch momentane : chaque appui inverse
    void resetMidiMap();
    void clearLearn (int i);
    juce::String learnAssignText (int i) const;

    std::atomic<juce::uint32> accentColour { 0xff4fc3f7 };   // couleur d'accent de l'interface (nuancier)
    std::atomic<int> uiScaleIdx { -1 };    // -1 = taille par defaut choisie, 0..2 = 60 / 75 / 100 %

    juce::AudioProcessorValueTreeState apvts;

private:
    struct Slot
    {
        juce::AudioBuffer<float> raw;      // IR telle que lue (fin silencieuse retiree), sans gain
        double rawRate = 0.0;
        IRInfo info;
        float normGain = 1.0f;             // normalisation (energie unite), sans le pad
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;          // 60 Hz, thread de messages : tous les appels a l'hote
    void handleMidi (const juce::MidiBuffer&);
    void pushKernel (int slot);              // re-echantillonne + gain -> moteur de convolution
    bool setSlotData (int slot, juce::AudioBuffer<float>&& data, double rate, const juce::String& name,
                      const juce::String& path, int bits, bool isFloat, double fileLengthMs, bool embedded);
    std::unique_ptr<juce::XmlElement> buildStateXml (bool embedIRs) const;
    void applyStateXml (const juce::XmlElement&);
    juce::String snapshotString() const;     // pour savoir si le preset a ete modifie
    void updateTail();

    static juce::AudioBuffer<float> resampleSinc (const juce::AudioBuffer<float>& in, double srcRate, double dstRate);

    // ---- convolution
    juce::dsp::ConvolutionMessageQueue convQueue { 64 };
    std::unique_ptr<juce::dsp::Convolution> conv[numSlots];
    Slot slots[numSlots];
    juce::CriticalSection slotLock;          // thread de messages / chargement / prepareToPlay (jamais l'audio)
    std::atomic<bool> hasIR[numSlots];
    std::atomic<juce::uint32> infoVersion { 1 };
    std::atomic<double> tailSeconds { 0.0 };
    double hostRate = 44100.0;
    int maxBlock = 512;
    bool lastNormalize = true;

    // ---- DSP
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayA { 1024 }, delayB { 1024 };
    juce::AudioBuffer<float> dryBuf, passBuf, bufA, bufB;
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float z1[2] = {}, z2[2] = {};
        float process (float x, int c) { const float y = b0 * x + z1[c]; z1[c] = b1 * x - a1 * y + z2[c]; z2[c] = b2 * x - a2 * y; return y; }
        void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    };
    static void setHighPass (Biquad&, double fc, double sr);
    static void setLowPass (Biquad&, double fc, double sr);
    Biquad hpf, lpf;
    float lastLow = -1.0f, lastHigh = -1.0f;
    float dcX[2] = {}, dcY[2] = {}, adaaX[2] = {};
    // lissage 2 poles des commandes (pedale d'expression sans saccades)
    struct Smooth2 { float a = 0, b = 0; float k = 0.01f; void set (float v) { a = b = v; } float next (float t) { a += k * (t - a); b += k * (a - b); return b; } };
    Smooth2 smVar, smGA, smGB, smDryW, smActive;
    float osK = 0.01f;
    float overOs = 0.0f, overOs2 = 0.0f;

    std::atomic<float> *pActive, *pMix, *pOverload, *pVariphi, *pGainA, *pGainB, *pMuteA, *pMuteB,
                       *pPhaseA, *pPhaseB, *pLowCut, *pHighCut, *pNormalize;

    // ---- anti-gel : MIDI (thread audio) -> parametres (timer)
    std::atomic<float> pendingValue[numParamLearn];
    std::atomic<bool>  pendingSet[numParamLearn];
    std::atomic<float>* queuedPtr[numParamLearn] {};
    std::atomic<int> pendingTrigger { 0 };         // bits : 1 << (i - lPrevA)
    bool ccHeld[numLearn] {};
    int ccMin[numLearn], ccMax[numLearn];          // course apprise de la pedale d'expression
    int msb[32] {};                                // CC 14 bits (MSB 0-31 + LSB 32-63)

    // ---- presets
    juce::String presetName;
    juce::String presetSnapshot;
    std::atomic<juce::uint32> presetVersion { 1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImpulseProcessor)
};
