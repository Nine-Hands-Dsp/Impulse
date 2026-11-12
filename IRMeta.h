#pragma once
#include <juce_audio_formats/juce_audio_formats.h>

// ============================================================================
// Metadonnees d'une IR pour la bibliotheque (meme structure que les captures d'Ampire, pour reprendre
// sa fenetre de bibliotheque telle quelle) : lues dans l'en-tete du fichier, sans charger le son.
// ============================================================================
struct CaptureMeta
{
    juce::String path, fileName, title;
    juce::String architecture;
    juce::String family = "IR";                  // pastille de format (violette, comme les IR de TONE3000)
    juce::String variant;
    juce::String gearType, gearMake, gearModel, toneType, modeledBy, date;   // toneType : « Mono » / « Stereo »
    double sampleRate = 48000.0;
    bool   hasLoudness = false, hasInputLevel = false, hasOutputLevel = false;
    double loudness = 0.0, inputLevel = 0.0, outputLevel = 0.0, gain = -1.0, esr = -1.0;
    bool   isA2 = false, slimmable = false, phaseSafe = true;
    int    channels = 1, bits = 0;
    double lengthMs = 0.0;
    double fileKB = 0.0;

    juce::String gearLine() const
    {
        juce::StringArray parts;
        if (toneType.isNotEmpty()) parts.add (toneType);
        if (lengthMs > 0.0) parts.add (juce::String (juce::roundToInt (lengthMs)) + " ms");
        return parts.joinIntoString (juce::String (juce::CharPointer_UTF8 ("  \xc2\xb7  ")));
    }
    juce::String formatLine() const
    {
        juce::StringArray parts;
        juce::String k = juce::String (sampleRate / 1000.0, 1);
        if (k.endsWith (".0")) k = k.dropLastCharacters (2);
        parts.add ("IR");
        parts.add (k + " kHz");
        if (bits > 0) parts.add (juce::String (bits) + "-bit");
        return parts.joinIntoString ("  |  ");
    }
};

struct CaptureBuilder
{
    static bool isIRFile (const juce::File& f)
    {
        return f.hasFileExtension ("wav;aif;aiff;flac");
    }
    static bool readMeta (const juce::File& f, CaptureMeta& out)
    {
        if (! isIRFile (f) || ! f.existsAsFile()) return false;
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
        if (r == nullptr || r->sampleRate <= 0.0 || r->lengthInSamples <= 0) return false;
        CaptureMeta m;
        m.path = f.getFullPathName();
        m.fileName = f.getFileName();
        m.title = f.getFileNameWithoutExtension();
        m.sampleRate = r->sampleRate;
        m.channels = (int) r->numChannels;
        m.bits = (int) r->bitsPerSample;
        m.lengthMs = 1000.0 * (double) r->lengthInSamples / r->sampleRate;
        m.toneType = m.channels >= 2 ? "Stereo" : "Mono";
        m.fileKB = (double) f.getSize() / 1024.0;
        out = m;
        return true;
    }
};
